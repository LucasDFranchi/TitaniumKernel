/**
 * @file
 * @brief MQTT client task implementation for managing MQTT connection and publishing sensor data.
 */
#include "mqtt_client.h"

#include "inter_task_communication/events/events_definition.h"
#include "logger/logger.h"
#include "tasks/system/network/network_task.h"
#include "tasks/iot/mqtt/mqtt_client_task.h"
#include "utils/utils.h"

#define MQTT_CLIEN_MAX_TOPIC_COUNT 10  ///< Maximum number of MQTT topics that can be subscribed to.

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_events_st* global_events = NULL;  ///< Pointer to the global configuration structure.

static esp_mqtt_client_handle_t mqtt_client = {0};  ///< MQTT client handle.

static const char* TAG                                        = "MQTT Task";  ///< Log tag for MQTT task.
static bool is_mqtt_connected                                 = false;        ///< MQTT connection status.
static mqtt_topic_st* mqtt_topics[MQTT_CLIEN_MAX_TOPIC_COUNT] = {0};          ///< Array of MQTT topics.
static int initalized_mqtt_topics_count                       = 0;            ///< Number of initialized MQTT topics.
static char unique_id[13]                                     = {0};          ///< Device unique ID (12 chars + null terminator).

/**
 * @brief Subscribes to all configured MQTT topics based on their direction.
 *
 * This function iterates through all the configured MQTT topics and subscribes
 * to those whose direction is set to `SUBSCRIBE`. It also checks if the topic
 * and the associated queue are valid. Any errors encountered are logged for
 * debugging purposes.
 *
 * @return KERNEL_ERROR_NONE if successful, or an appropriate error code if not.
 */
static kernel_error_st mqtt_subscribe(void);

/**
 * @brief Callback function for handling incoming MQTT messages.
 *
 * This function is called when an MQTT message is received on a subscribed topic.
 * It processes the incoming message and triggers the corresponding action.
 *
 * @param[in] topic       The topic on which the message was received.
 * @param[in] event_data  The message data received.
 * @param[in] event_data_len The length of the message data.
 */
static void mqtt_subscribe_topic_callback(const char* topic, const char* event_data, size_t event_data_len);

/**
 * @brief Handles MQTT events triggered by the client.
 *
 * This callback is responsible for managing MQTT events such as connection,
 * disconnection, incoming data, and errors.
 *
 * @param[in] arg        User-defined argument (not used).
 * @param[in] base       Event base, typically MQTT events.
 * @param[in] event_id   Specific MQTT event ID.
 * @param[in] event_data Event data containing details about the event.
 */
static void mqtt_event_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            logger_print(INFO, TAG, "MQTT_EVENT_CONNECTED");
            is_mqtt_connected = true;
            mqtt_subscribe();
            break;

        case MQTT_EVENT_DISCONNECTED:
            logger_print(INFO, TAG, "MQTT_EVENT_DISCONNECTED");
            is_mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA:
            logger_print(DEBUG, TAG, "MQTT_EVENT_DATA: Topic=%.*s, Data=%.*s",
                         event->topic_len, event->topic,
                         event->data_len, event->data);
            mqtt_subscribe_topic_callback(event->topic, event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            logger_print(ERR, TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            logger_print(DEBUG, TAG, "MQTT event: %d", event->event_id);
            break;
    }
}

/**
 * @brief Initializes the MQTT client and its configuration.
 *
 * This function sets up the MQTT client, configures its parameters, and
 * registers the event handler callback. It returns an error code if any step fails.
 *
 * @return esp_err_t ESP_OK if the initialization is successful, otherwise an error code.
 */
static kernel_error_st mqtt_client_task_initialize(void) {
    esp_mqtt_client_config_t mqtt_cfg = {0};

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        logger_print(ERR, TAG, "Failed to initialize MQTT client");
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to register event handler");
        return KERNEL_ERROR_MQTT_REGISTER_FAIL;
    }

    if (esp_mqtt_client_set_uri(mqtt_client, "mqtt://mqtt.eclipseprojects.io") != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set MQTT URI");
        return KERNEL_ERROR_MQTT_URI_FAIL;
    }

    if (esp_mqtt_set_config(mqtt_client, &mqtt_cfg) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set MQTT config");
        return KERNEL_ERROR_MQTT_CONFIG_FAIL;
    }

    get_unique_id(unique_id, sizeof(unique_id));

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Starts the MQTT client if it is initialized.
 *
 * This function starts the MQTT client and logs the status of the operation.
 */
static void start_mqtt_client(void) {
    if (mqtt_client) {
        esp_mqtt_client_start(mqtt_client);
        logger_print(INFO, TAG, "MQTT client started");
    } else {
        logger_print(ERR, TAG, "MQTT client not initialized");
    }
}

/**
 * @brief Stops the MQTT client if it is running.
 *
 * This function stops the MQTT client and logs the status of the operation.
 */
static void stop_mqtt_client(void) {
    if (mqtt_client) {
        esp_mqtt_client_stop(mqtt_client);
        logger_print(INFO, TAG, "MQTT client stopped");
    }
}

/**
 * @brief Publish the humidity data to the MQTT broker.
 *
 * This function formats the humidity value as a JSON string with a timestamp
 * and publishes it to the MQTT topic "/titanium/1/humidity". It logs the result
 * of the publish operation.
 *
 * @param[in] humidity The humidity value to be published (in percentage).
 */
static kernel_error_st mqtt_publish_topic(const mqtt_topic_st* mqtt_topic) {
    char message_buffer[512] = {0};
    char channel[64]         = {0};

    if (mqtt_topic == NULL) {
        logger_print(ERR, TAG, "MQTT topic is NULL");
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (xSemaphoreTake(mqtt_topic->semaphore, portMAX_DELAY)) {
        kernel_error_st err = mqtt_topic->encode_json(message_buffer);
        xSemaphoreGive(mqtt_topic->semaphore);

        if (err != KERNEL_ERROR_NONE) {
            logger_print(ERR, TAG, "Failed to publish to topic %s", mqtt_topic->topic);
            return err;
        }
    }

    size_t channel_size = snprintf(channel, sizeof(channel), "/titanium/%s/%s", unique_id, mqtt_topic->topic);
    if (channel_size >= sizeof(channel)) {
        logger_print(WARN, TAG, "Channel buffer too small");
        return KERNEL_ERROR_SNPRINTF;
    }

    int msg_id = esp_mqtt_client_publish(mqtt_client, channel, message_buffer, 0, mqtt_topic->qos, 0);
    if (msg_id < 0) {
        logger_print(ERR, TAG, "Message published successfully, msg_id=%d", msg_id);
        return KERNEL_ERROR_MQTT_PUBLISH;
    }

    logger_print(DEBUG, TAG, "Published to topic %s, msg_id=%d", channel, msg_id);

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Publishes sensor data to the MQTT topic.
 *
 * This function iterates through all the configured MQTT topics and publishes
 * data to those whose direction is set to `PUBLISH`. It checks if each topic
 * and its associated queue are valid. If any errors are encountered, they are
 * logged for debugging purposes. The function returns the error encountered
 * during the publishing process, or `KERNEL_ERROR_NONE` if all topics were
 * successfully published to.
 *
 * @return kernel_error_st - Returns the error status or `KERNEL_ERROR_NONE` on success.
 */
static kernel_error_st mqtt_publish(void) {
    for (uint8_t i = 0; i < initalized_mqtt_topics_count; i++) {
        if (mqtt_topics[i] == NULL) {
            logger_print(ERR, TAG, "MQTT topic %s is NULL", mqtt_topics[i]->topic);
            return KERNEL_ERROR_INVALID_ARG;
        }
        if (mqtt_topics[i]->queue == NULL) {
            logger_print(ERR, TAG, "Queue for topic %s is NULL", mqtt_topics[i]->topic);
            return KERNEL_ERROR_QUEUE_NULL;
        }
        if (mqtt_topics[i]->mqtt_data_direction == PUBLISH) {
            kernel_error_st err = mqtt_publish_topic(mqtt_topics[i]);
            if (err != KERNEL_ERROR_NONE) {
                logger_print(ERR, TAG, "Failed to publish to topic %s", mqtt_topics[i]->topic);
                return err;
            }
        }
    }
    return KERNEL_ERROR_NONE;
}

/**
 * @brief Subscribes to an MQTT topic and processes the subscription.
 *
 * This function constructs the full MQTT topic channel string and attempts to
 * subscribe to the topic using the `esp_mqtt_client_subscribe` API. It handles
 * errors such as invalid input parameters, buffer size issues, and subscription failures.
 * Upon a successful subscription, it logs the result.
 *
 * @param mqtt_topic A pointer to the mqtt_topic_st structure representing the topic to subscribe to.
 * @return kernel_error_st Returns KERNEL_ERROR_NONE on success, or an appropriate error code:
 *         - KERNEL_ERROR_NULL if the mqtt_topic pointer is NULL.
 *         - KERNEL_ERROR_INVALID_ARG if the topic is empty.
 *         - KERNEL_ERROR_SNPRINTF if the channel buffer size is insufficient.
 *         - KERNEL_ERROR_MQTT_SUBSCRIBE if subscribing to the topic fails.
 */
static kernel_error_st mqtt_subscribe_topic(mqtt_topic_st* mqtt_topic) {
    char channel[64] = {0};

    if (mqtt_topic == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    size_t channel_size = snprintf(channel, sizeof(channel), "/titanium/%s/%s", unique_id, mqtt_topic->topic);

    if (channel_size >= sizeof(channel)) {
        logger_print(ERR, TAG, "Channel buffer too small");
        return KERNEL_ERROR_SNPRINTF;
    }

    int msg_id = esp_mqtt_client_subscribe(mqtt_client, channel, 1);
    if (msg_id < 0) {
        logger_print(ERR, TAG, "Failed to subscribe to topic %s", channel);
        return KERNEL_ERROR_MQTT_SUBSCRIBE;
    }

    logger_print(DEBUG, TAG, "Subscribed to topic %s, msg_id=%d", channel, msg_id);

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Subscribes to all configured MQTT topics based on their direction.
 *
 * This function iterates through all the configured MQTT topics and subscribes
 * to those whose direction is set to `SUBSCRIBE`. It also checks if the topic
 * and the associated queue are valid. Any errors encountered are logged for
 * debugging purposes.
 *
 * @return KERNEL_ERROR_NONE if successful, or an appropriate error code if not.
 */
static kernel_error_st mqtt_subscribe(void) {
    if (!mqtt_client) {
        return KERNEL_ERROR_NULL;
    }

    for (uint8_t i = 0; i < initalized_mqtt_topics_count; i++) {
        if (mqtt_topics[i] == NULL) {
            logger_print(ERR, TAG, "MQTT topic %s is NULL", mqtt_topics[i]->topic);
            return KERNEL_ERROR_NULL;
        }
        if (mqtt_topics[i]->queue == NULL) {
            logger_print(ERR, TAG, "Queue for topic %s is NULL", mqtt_topics[i]->topic);
            return KERNEL_ERROR_QUEUE_NULL;
        }
        if (mqtt_topics[i]->mqtt_data_direction == SUBSCRIBE) {
            kernel_error_st err = mqtt_subscribe_topic(mqtt_topics[i]);
            if (err != KERNEL_ERROR_NONE) {
                logger_print(ERR, TAG, "Failed to subscribe to topic %s", mqtt_topics[i]->topic);
                return err;
            }
        }
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Callback function for handling incoming MQTT messages.
 *
 * This function is called when an MQTT message is received on a subscribed topic.
 * It processes the incoming message and triggers the corresponding action.
 *
 * @param[in] topic       The topic on which the message was received.
 * @param[in] event_data  The message data received.
 * @param[in] event_data_len The length of the message data.
 */
static void mqtt_subscribe_topic_callback(const char* topic, const char* event_data, size_t event_data_len) {
    if ((topic == NULL) || (event_data == NULL) || (event_data_len == 0)) {
        logger_print(ERR, TAG, "Invalid arguments");
        return;
    }

    for (int i = 0; i < initalized_mqtt_topics_count; i++) {
        if (mqtt_topics[i]->queue == NULL) {
            logger_print(ERR, TAG, "Queue for topic %s is NULL", mqtt_topics[i]->topic);
            continue;
        }
        if (mqtt_topics[i]->mqtt_data_direction == PUBLISH) {
            continue;
        }

        if (strstr(topic, mqtt_topics[i]->topic) == NULL) {
            continue;
        }
        if (xSemaphoreTake(mqtt_topics[i]->semaphore, portMAX_DELAY)) {
            kernel_error_st err = mqtt_topics[i]->parse_store_json(event_data);
            xSemaphoreGive(mqtt_topics[i]->semaphore);

            if (err != KERNEL_ERROR_NONE) {
                logger_print(ERR, TAG, "Failed to parse the received json from topic %s",  mqtt_topics[i]->topic);
            }
        }
    }
}

/**
 * @brief Main MQTT execution task.
 *
 * Manages the MQTT connection and periodically checks for new sensor data
 * to publish. It ensures the client is started and stopped based on Wi-Fi
 * connection status.
 *
 * @param[in] pvParameters User-defined parameters (not used).
 */
void mqtt_client_task_execute(void* pvParameters) {
    global_events = (global_events_st*)pvParameters;
    if ((mqtt_client_task_initialize() != ESP_OK) ||
        (global_events->firmware_event_group == NULL) ||
        (global_events == NULL)) {
        logger_print(ERR, TAG, "Failed to initialize MQTT task");
        vTaskDelete(NULL);
    }

    while (1) {
        EventBits_t firmware_event_bits = xEventGroupGetBits(global_events->firmware_event_group);

        if (is_mqtt_connected) {
            if ((firmware_event_bits & WIFI_CONNECTED_STA) == 0) {
                stop_mqtt_client();
            } else if (firmware_event_bits & TIME_SYNCED) {
                mqtt_publish();
            }
        } else {
            if (firmware_event_bits & WIFI_CONNECTED_STA) {
                start_mqtt_client();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Enqueues an MQTT topic for processing.
 *
 * This function attempts to enqueue the provided MQTT topic into the internal topic queue.
 * It performs several checks before enqueuing, including verifying that the topic is not null,
 * ensuring that the queue handle is valid, and checking that the necessary JSON encoding/decoding
 * functions are available based on the data direction (PUBLISH or SUBSCRIBE).
 * It also checks the size of the data to ensure it does not exceed the maximum allowed payload size.
 *
 * @param mqtt_topic A pointer to the mqtt_topic_st structure representing the MQTT topic to be enqueued.
 * @return kernel_error_st Returns KERNEL_ERROR_NONE on success, or an error code indicating the failure reason:
 *         - KERNEL_ERROR_NULL if the mqtt_topic pointer is NULL.
 *         - KERNEL_ERROR_QUEUE_NULL if the queue handle is NULL.
 *         - KERNEL_ERROR_INVALID_INTERFACE if the necessary encoding/decoding functions are not provided.
 *         - KERNEL_ERROR_INVALID_SIZE if the data size exceeds the allowed maximum.
 */
kernel_error_st mqtt_client_topic_enqueue(mqtt_topic_st* mqtt_topic) {
    if (mqtt_topic == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (mqtt_topic->queue == NULL) {
        return KERNEL_ERROR_QUEUE_NULL;
    }

    if ((mqtt_topic->mqtt_data_direction == PUBLISH && mqtt_topic->encode_json == NULL) ||
        (mqtt_topic->mqtt_data_direction == SUBSCRIBE && mqtt_topic->parse_store_json == NULL)) {
        return KERNEL_ERROR_INVALID_INTERFACE;
    }

    mqtt_topic->semaphore = xSemaphoreCreateMutex();

    if (mqtt_topic->semaphore == NULL) {
        return KERNEL_ERROR_NO_MEM;
    }

    mqtt_topics[initalized_mqtt_topics_count] = mqtt_topic;

    return KERNEL_ERROR_NONE;
}
