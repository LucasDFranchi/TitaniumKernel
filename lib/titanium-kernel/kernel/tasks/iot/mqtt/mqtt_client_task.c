/**
 * @file
 * @brief MQTT client task implementation for managing MQTT connection and publishing sensor data.
 */
#include "mqtt_client.h"

#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/iot/mqtt/mqtt_client_task.h"
#include "kernel/tasks/system/network/network_task.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st* _global_structures = NULL;         ///< Pointer to the global configuration structure.
static esp_mqtt_client_handle_t mqtt_client     = {0};          ///< MQTT client handle.
static const char* TAG                          = "MQTT Task";  ///< Log tag for MQTT task.
static bool is_mqtt_connected                   = false;        ///< MQTT connection status.
static bool is_waiting_for_connection           = false;        ///<
static mqtt_bridge_st mqtt_bridge               = {0};          ///< Pointer to the MQTT bridge structure.

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
static kernel_error_st subscribe(void);

/**
 * @brief Handles incoming MQTT event data for subscribed topics.
 *
 * Matches the incoming topic string with registered topics and deserializes
 * the payload accordingly.
 *
 * @param topic Pointer to the incoming MQTT topic string.
 * @param payload Pointer to the mqtt_buffer_st containing the incoming payload data.
 * @return KERNEL_ERROR_NONE on success;
 *         KERNEL_ERROR_NULL if pointers are NULL;
 *         KERNEL_ERROR_INVALID_SIZE if payload size is zero;
 *         Other error codes as returned by mqtt_deserialize_data.
 */
static kernel_error_st handle_event_data(char* topic, char* data, size_t data_length);

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
        case MQTT_EVENT_BEFORE_CONNECT:
            logger_print(INFO, TAG, "MQTT_EVENT_BEFORE_CONNECT");
            is_waiting_for_connection = true;
            break;

        case MQTT_EVENT_CONNECTED:
            logger_print(INFO, TAG, "MQTT_EVENT_CONNECTED");
            is_mqtt_connected         = true;
            is_waiting_for_connection = false;
            subscribe();
            break;

        case MQTT_EVENT_DISCONNECTED:
            logger_print(INFO, TAG, "MQTT_EVENT_DISCONNECTED");
            is_mqtt_connected         = false;
            is_waiting_for_connection = false;
            break;

        case MQTT_EVENT_DATA:
            logger_print(DEBUG, TAG, "MQTT_EVENT_DATA: Topic=%.*s, Data=%.*s",
                         event->topic_len, event->topic,
                         event->data_len, event->data);
            handle_event_data(event->topic, event->data, event->data_len);
            break;

        case MQTT_EVENT_ERROR:
            logger_print(ERR, TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            logger_print(ERR, TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            break;

        default:
            logger_print(DEBUG, TAG, "MQTT event: %d", event->event_id);
            break;
    }
}

/**
 * @brief Starts the MQTT client if it is initialized.
 *
 * This function starts the MQTT client and logs the status of the operation.
 */
static void start_mqtt_client(void) {
    if (mqtt_client) {
        esp_err_t err = esp_mqtt_client_start(mqtt_client);
        if (err != ESP_OK) {
            logger_print(ERR, TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        } else {
            logger_print(INFO, TAG, "MQTT client started");
        }
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
 * @brief Publishes all available MQTT messages for registered topics.
 *
 * This function iterates through all topics registered in the MQTT bridge,
 * and attempts to fetch and publish any available messages marked for publishing.
 *
 * For each topic:
 * - If the queue is empty or direction is not `PUBLISH`, it is skipped.
 * - If serialization or publishing fails, an error is logged, and the loop continues.
 * - On success, the message is sent using `esp_mqtt_client_publish()`.
 *
 * The function does **not return early** on errors — it continues through all topics,
 * ensuring that a failure on one topic does not block others.
 *
 * @note This function assumes that the payload and topic buffers are properly written
 * and null-terminated by the bridge's fetch function and serializer.
 *
 * @warning No internal delays are used — if calling this rapidly, consider rate-limiting externally.
 */
static void publish(void) {
    char payload[MQTT_MAXIMUM_PAYLOAD_LENGTH] = {0};
    char topic[MQTT_MAXIMUM_TOPIC_LENGTH]     = {0};
    qos_et qos                                = QOS_0;

    mqtt_buffer_st mqtt_buffer_payload = {
        .buffer = payload,
        .size   = sizeof(payload)};
    mqtt_buffer_st mqtt_buffer_topic = {
        .buffer = topic,
        .size   = sizeof(topic)};

    for (size_t i = 0; i < mqtt_bridge.get_topics_count(); i++) {
        kernel_error_st err = mqtt_bridge.fetch_publish_data(i, &mqtt_buffer_topic, &mqtt_buffer_payload, &qos);

        if ((err == KERNEL_ERROR_EMPTY_QUEUE) || (err == KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION)) {
            continue;
        }

        if (err != KERNEL_ERROR_NONE) {
            logger_print(ERR, TAG, "Failed to publish to topic %s - %d", topic, err);
            continue;
        }

        int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, qos, 0);
        if (msg_id < 0) {
            logger_print(ERR, TAG, "Failed to publish MQTT message (topic=%s, qos=%d)", topic, qos);
        } else {
            logger_print(DEBUG, TAG, "Published to topic %s, msg_id=%d", topic, msg_id);
        }
    }
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
static kernel_error_st subscribe(void) {
    char topic[MQTT_MAXIMUM_TOPIC_LENGTH] = {0};
    qos_et qos     = QOS_0;

    mqtt_buffer_st mqtt_buffer_topic = {
        .buffer = topic,
        .size   = sizeof(topic)};

    for (size_t i = 0; i < mqtt_bridge.get_topics_count(); i++) {
        kernel_error_st err = mqtt_bridge.subscribe(i, &mqtt_buffer_topic, &qos);

        if ((err != KERNEL_ERROR_NONE) && (err != KERNEL_ERROR_EMPTY_QUEUE)) {
            logger_print(ERR, TAG, "Failed to subscribe to topic %s - %d", topic, err);
            continue;
        }

        int msg_id = esp_mqtt_client_subscribe(mqtt_client, mqtt_buffer_topic.buffer, qos);
        if (msg_id < 0) {
            logger_print(ERR, TAG, "Failed to subscribe to topic %s", mqtt_buffer_topic.buffer);
            return KERNEL_ERROR_MQTT_SUBSCRIBE;
        }

        logger_print(DEBUG, TAG, "Subscribed to topic %s, msg_id=%d", mqtt_buffer_topic.buffer, msg_id);
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Handles incoming MQTT event data for subscribed topics.
 *
 * Matches the incoming topic string with registered topics and deserializes
 * the payload accordingly.
 *
 * @param topic Pointer to the incoming MQTT topic string.
 * @param payload Pointer to the mqtt_buffer_st containing the incoming payload data.
 * @return KERNEL_ERROR_NONE on success;
 *         KERNEL_ERROR_NULL if pointers are NULL;
 *         KERNEL_ERROR_INVALID_SIZE if payload size is zero;
 *         Other error codes as returned by mqtt_deserialize_data.
 */
static kernel_error_st handle_event_data(char* topic, char* data, size_t data_length) {
    if ((topic == NULL) || (data == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    if (data_length == 0 || data_length >= MQTT_MAXIMUM_PAYLOAD_LENGTH) {
        logger_print(ERR, TAG, "Invalid data length %zu, max allowed is 1024", data_length);
        return KERNEL_ERROR_INVALID_SIZE;
    }

    char payload_buffer[256] = {0};
    memcpy(payload_buffer, data, data_length);
    payload_buffer[data_length] = '\0';

    mqtt_buffer_st payload = {
        .buffer = payload_buffer,
        .size   = data_length + 1};

    return mqtt_bridge.handle_event_data(topic, &payload);
}

/**
 * @brief Install the MQTT bridge instance from the global queue.
 *
 * This function retrieves a pointer to the MQTT bridge from the global queue and stores it
 * in the `mqtt_bridge` variable. It must be called before any interaction with the bridge.
 *
 * @note This function expects `_global_structures->global_queues.mqtt_bridge_queue` to be
 *       initialized and populated elsewhere in the system.
 *
 * @retval KERNEL_ERROR_NONE         Bridge installed successfully.
 * @retval KERNEL_ERROR_NULL         Global structures or queue is NULL.
 * @retval KERNEL_ERROR_EMPTY_QUEUE  Queue did not return a valid bridge within timeout.
 */
kernel_error_st install_bridge(void) {
    if (xQueueReceive(_global_structures->global_queues.mqtt_bridge_queue, &mqtt_bridge, pdMS_TO_TICKS(100)) == pdTRUE) {
        logger_print(INFO, TAG, "MQTT bridge successfully installed.");
    } else {
        return KERNEL_ERROR_EMPTY_QUEUE;
    }

    return KERNEL_ERROR_NONE;
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

    if (esp_mqtt_client_set_uri(mqtt_client, "mqtt://broker.hivemq.com") != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set MQTT URI");
        return KERNEL_ERROR_MQTT_URI_FAIL;
    }

    if (esp_mqtt_set_config(mqtt_client, &mqtt_cfg) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set MQTT config");
        return KERNEL_ERROR_MQTT_CONFIG_FAIL;
    }

    return KERNEL_ERROR_NONE;
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
    _global_structures = (global_structures_st*)pvParameters;

    if ((mqtt_client_task_initialize() != KERNEL_ERROR_NONE) ||
        (_global_structures == NULL) ||
        (_global_structures->global_queues.mqtt_bridge_queue == NULL) ||
        (_global_structures->global_events.firmware_event_group == NULL)) {
        logger_print(ERR, TAG, "Failed to initialize MQTT task");
        vTaskDelete(NULL);
    }

    while (install_bridge() != KERNEL_ERROR_NONE) {
        logger_print(WARN, TAG, "Waiting for MQTT bridge to be available...");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    TickType_t last_connect_attempt = 0;
    TickType_t waiting_since        = 0;

    while (1) {
        EventBits_t firmware_event_bits = xEventGroupGetBits(
            _global_structures->global_events.firmware_event_group);

        bool is_wifi_connected = firmware_event_bits & WIFI_CONNECTED_STA;
        bool is_time_synced    = firmware_event_bits & TIME_SYNCED;
        TickType_t now         = xTaskGetTickCount();

        if (!is_mqtt_connected && !is_waiting_for_connection) {
            if (is_wifi_connected && (now - last_connect_attempt > pdMS_TO_TICKS(5000))) {
                logger_print(DEBUG, TAG, "Trying to start MQTT client...");
                start_mqtt_client();
                last_connect_attempt      = now;
                waiting_since             = now;
                is_waiting_for_connection = true;
            }
        }

        if (is_waiting_for_connection && (now - waiting_since > pdMS_TO_TICKS(15000))) {
            logger_print(WARN, TAG, "MQTT connect timeout, restarting client...");
            stop_mqtt_client();
            is_waiting_for_connection = false;
        }

        if (is_mqtt_connected && !is_wifi_connected) {
            logger_print(DEBUG, TAG, "Stopping MQTT client due to Wi-Fi disconnection...");
            stop_mqtt_client();
        }

        if (is_mqtt_connected && is_wifi_connected && is_time_synced) {
            publish();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}