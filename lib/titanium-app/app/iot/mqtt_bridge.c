#include "stddef.h"
#include "string.h"

#include "mqtt_bridge.h"

#include "kernel/device/device_info.h"
#include "kernel/logger/logger.h"

#include "app/iot/mqtt_serializer.h"

/* Module Global Defines */
static const char *TAG = "MQTT Bridge";  ///< Tag used for logging MQTT bridge operations.

/* Module Global Variables */
static size_t mqtt_bridge_num_topics              = 0;    ///< Number of MQTT topics registered in the bridge.
static mqtt_topic_st mqtt_topics[MAX_MQTT_TOPICS] = {0};  ///< Array of MQTT topics for sensor data.

/**
 * @brief Validates the Quality of Service (QoS) level.
 *
 * Checks if the provided QoS level is one of the valid MQTT QoS levels (0, 1, or 2).
 *
 * @param[in] qos QoS level to validate.
 * @return KERNEL_ERROR_NONE if valid; otherwise, KERNEL_ERROR_MQTT_INVALID_QOS.
 */
kernel_error_st validate_qos(const qos_et qos) {
    switch (qos) {
        case QOS_0:
        case QOS_1:
        case QOS_2:
            break;
        default:
            return KERNEL_ERROR_MQTT_INVALID_QOS;
    }
    return KERNEL_ERROR_NONE;
}

/**
 * @brief Validates the MQTT data direction (PUBLISH or SUBSCRIBE).
 *
 * Ensures that the direction parameter is either PUBLISH or SUBSCRIBE.
 *
 * @param direction Data direction to validate.
 * @return KERNEL_ERROR_NONE if valid; otherwise, KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION.
 */
kernel_error_st validate_data_direction(const mqtt_data_direction_et direction) {
    switch (direction) {
        case PUBLISH:
        case SUBSCRIBE:
            break;
        default:
            return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION;
    }
    return KERNEL_ERROR_NONE;
}


/**
 * @brief Validates the length and existence of the topic string.
 *
 * Checks that the topic string is non-null, non-empty, and shorter than the
 * maximum allowed MQTT topic length.
 *
 * @param[in] topic Pointer to the topic string.
 * @return KERNEL_ERROR_NONE if valid; otherwise, KERNEL_ERROR_MQTT_INVALID_TOPIC.
 */
kernel_error_st validate_mqtt_topic_length(const char *topic) {
    if (topic == NULL || strlen(topic) == 0 || strlen(topic) >= MQTT_MAXIMUM_TOPIC_LENGTH) {
        return KERNEL_ERROR_MQTT_INVALID_TOPIC;
    }
    return KERNEL_ERROR_NONE;
}

/**
 * @brief Validates the structure of an MQTT topic registration.
 *
 * Validates all important fields in the MQTT topic, including QoS, direction,
 * topic string, queue handle availability, and maximum number of topics.
 *
 * @param[in] topic Pointer to the topic structure to validate.
 * @return KERNEL_ERROR_NONE if valid; otherwise, appropriate error code.
 */
kernel_error_st validate_mqtt_topic(const mqtt_topic_st *topic) {
    if (topic == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (validate_qos(topic->info->qos) != KERNEL_ERROR_NONE) {
        return KERNEL_ERROR_MQTT_INVALID_QOS;
    }

    if (validate_data_direction(topic->info->mqtt_data_direction) != KERNEL_ERROR_NONE) {
        return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION;
    }

    if (validate_mqtt_topic_length(topic->info->topic) != KERNEL_ERROR_NONE) {
        return KERNEL_ERROR_MQTT_INVALID_TOPIC;
    }

    if (mqtt_bridge_num_topics >= MAX_MQTT_TOPICS) {
        return KERNEL_ERROR_MQTT_TOO_MANY_TOPICS;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Registers a new MQTT topic in the bridge.
 *
 * Validates the topic and, on success, creates a FreeRTOS queue and adds
 * the topic to the internal MQTT topics list.
 *
 * @param[in] topic Pointer to the topic structure to register.
 * @return KERNEL_ERROR_NONE on success;
 *         KERNEL_ERROR_NULL if topic is NULL;
 *         KERNEL_ERROR_MQTT_REGISTER_FAIL if validation fails or queue creation fails.
 */
kernel_error_st register_topic(mqtt_topic_st *topic) {
    if (topic == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (validate_mqtt_topic(topic) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to validate for topic %s", topic->info->topic);
        return KERNEL_ERROR_MQTT_REGISTER_FAIL;
    }

    topic->queue = xQueueCreate(topic->info->queue_length, topic->info->queue_item_size);

    if (topic->queue == NULL) {
        logger_print(ERR, TAG, "Failed to create queue for topic %s", topic->info->topic);
        return KERNEL_ERROR_QUEUE_NULL;
    }

    mqtt_topics[mqtt_bridge_num_topics++] = *topic;

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Checks if a given MQTT topic has data available to publish.
 *
 * Queries the FreeRTOS queue associated with the topic to check if any
 * messages are waiting to be sent.
 *
 * @param topic Pointer to the mqtt_topic_st instance to check.
 * @return true if there is data waiting in the topic's queue.
 * @return false if the topic pointer or its queue is NULL, or if no messages are waiting.
 */
bool has_data_to_publish(const mqtt_topic_st *topic) {
    if (topic == NULL || topic->queue == NULL) {
        return false;
    }
    return (uxQueueMessagesWaiting(topic->queue) > 0);
}

/**
 * @brief Fetches the next payload and topic to publish for a given MQTT topic index.
 *
 * This function retrieves data from the queue associated with the specified MQTT topic,
 * serializes the payload, and formats the full MQTT topic string including the device unique ID.
 * It also returns the QoS level for the message.
 *
 * @param mqtt_index Index of the MQTT topic in the bridge's topic list.
 * @param topic Pointer to an mqtt_buffer_st struct where the formatted MQTT topic string will be stored.
 * @param payload Pointer to an mqtt_buffer_st struct where the serialized payload will be stored.
 * @param qos Pointer to a qos_et variable where the QoS level will be stored.
 *
 * @return KERNEL_ERROR_NONE if data was successfully fetched and serialized.
 * @return KERNEL_ERROR_NULL if any pointer arguments are NULL.
 * @return KERNEL_ERROR_INVALID_INDEX if mqtt_index is out of bounds.
 * @return KERNEL_ERROR_MQTT_QUEUE_NULL if the topic's queue is NULL.
 * @return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION if the topic is not for publishing.
 * @return KERNEL_ERROR_EMPTY_QUEUE if no data is available to publish.
 * @return KERNEL_ERROR_FORMATTING if the topic buffer is too small to hold the full topic string.
 * @return Other error codes returned by mqtt_serialize_data on serialization failure.
 */
kernel_error_st fetch_publish_data(uint8_t mqtt_index, mqtt_buffer_st *topic, mqtt_buffer_st *payload, qos_et *qos) {
    if (topic == NULL || payload == NULL || qos == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (mqtt_index >= mqtt_bridge_num_topics) {
        return KERNEL_ERROR_INVALID_INDEX;
    }

    mqtt_topic_st *current = &mqtt_topics[mqtt_index];

    if (current->queue == NULL) {
        logger_print(ERR, TAG, "Queue for topic %s is NULL", current->info->topic);
        return KERNEL_ERROR_MQTT_QUEUE_NULL;
    }

    if (current->info->mqtt_data_direction != PUBLISH) {
        return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION;
    }

    if (!has_data_to_publish(current)) {
        return KERNEL_ERROR_EMPTY_QUEUE;
    }

    kernel_error_st err = mqtt_serialize_data(
        current,
        payload->buffer,
        payload->size);

    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to serialize message for topic %s", current->info->topic);
        return err;
    }

    size_t channel_size = snprintf(
        topic->buffer,
        topic->size,
        "iocloud/response/%s/%s",
        device_info_get_id(),
        current->info->topic);

    if (channel_size >= topic->size) {
        logger_print(WARN, TAG, "Channel buffer too small");
        return KERNEL_ERROR_FORMATTING;
    }

    *qos = current->info->qos;

    return KERNEL_ERROR_NONE;
}


/**
 * @brief Retrieves the number of MQTT topics currently registered in the bridge.
 *
 * Provides the count of topics that have been successfully registered.
 *
 * @return Number of registered MQTT topics.
 */
static inline size_t get_topics_count(void) {
    return mqtt_bridge_num_topics;
}

/**
 * @brief Prepares subscription details for an MQTT topic.
 *
 * Constructs the full MQTT subscription topic string including the device unique ID,
 * and returns the QoS level for the subscription.
 *
 * @param mqtt_index Index of the MQTT topic in the bridge's topic list.
 * @param topic Pointer to an mqtt_buffer_st struct where the formatted subscription topic will be stored.
 * @param qos Pointer to a qos_et variable where the QoS level will be stored.
 * @return KERNEL_ERROR_NONE if successful;
 *         KERNEL_ERROR_INVALID_INDEX if mqtt_index is invalid;
 *         KERNEL_ERROR_MQTT_QUEUE_NULL if topic's queue is NULL;
 *         KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION if topic is not for subscription;
 *         KERNEL_ERROR_FORMATTING if topic buffer is too small.
 */
static kernel_error_st subscribe(uint8_t mqtt_index, mqtt_buffer_st *topic, qos_et *qos) {
    if (mqtt_index >= mqtt_bridge_num_topics) {
        return KERNEL_ERROR_INVALID_INDEX;
    }

    mqtt_topic_st *current = &mqtt_topics[mqtt_index];

    if (current->queue == NULL) {
        logger_print(ERR, TAG, "Queue for topic %s is NULL", current->info->topic);
        return KERNEL_ERROR_MQTT_QUEUE_NULL;
    }

    if (current->info->mqtt_data_direction != SUBSCRIBE) {
        return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION;
    }

    size_t channel_size = snprintf(
        topic->buffer,
        topic->size,
        "iocloud/request/%s/%s",
        device_info_get_id(),
        current->info->topic);

    if (channel_size >= topic->size) {
        logger_print(WARN, TAG, "Channel buffer too small");
        return KERNEL_ERROR_FORMATTING;
    }

    *qos = current->info->qos;

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
static kernel_error_st handle_event_data(char *topic, mqtt_buffer_st *payload) {
    if ((topic == NULL) || (payload->buffer == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    if (payload->size == 0) {
        return KERNEL_ERROR_INVALID_SIZE;
    }
    kernel_error_st err = KERNEL_ERROR_NONE;

    for (uint8_t i = 0; i < mqtt_bridge_num_topics; i++) {
        mqtt_topic_st *current = &mqtt_topics[i];
        if (strstr(topic, mqtt_topics[i].info->topic) != NULL) {
            err = mqtt_deserialize_data(
                current,
                payload->buffer,
                payload->size);

            break;
        }
    }

    return err;
}

/**
 * @brief Initializes the MQTT bridge and sets the device unique ID.
 *
 * This function initializes the provided mqtt_bridge_st instance by setting
 * the fetch function pointer and copying the device unique ID into an internal buffer.
 * It also registers all MQTT topics provided in the init struct.
 *
 * @param mqtt_bridge_init_struct Pointer to the MQTT bridge init struct containing bridge instance, topics, and topic count.
 * @param device_unique_id Null-terminated string containing the device unique identifier (max 12 chars).
 *
 * @return KERNEL_ERROR_NONE on successful initialization.
 * @return KERNEL_ERROR_NULL if any pointer arguments are NULL.
 * @return KERNEL_ERROR_FORMATTING if the unique ID string is too long to fit in the internal buffer.
 * @return ESP_FAIL if any topic registration fails.
 */
kernel_error_st mqtt_bridge_initialize(mqtt_bridge_init_struct_st *mqtt_bridge_init_struct) {
    if (mqtt_bridge_init_struct == NULL) {
        return KERNEL_ERROR_NULL;
    }

    mqtt_bridge_st *mqtt_bridge = mqtt_bridge_init_struct->mqtt_bridge;

    if (mqtt_bridge == NULL) {
        return KERNEL_ERROR_NULL;
    }

    mqtt_bridge->fetch_publish_data = fetch_publish_data;
    mqtt_bridge->get_topics_count   = get_topics_count;
    mqtt_bridge->subscribe          = subscribe;
    mqtt_bridge->handle_event_data  = handle_event_data;

    for (size_t i = 0; i < mqtt_bridge_init_struct->topic_count; i++) {
        mqtt_topic_st *current = &mqtt_bridge_init_struct->topics[i];
        kernel_error_st err    = register_topic(current);

        if (err != KERNEL_ERROR_NONE) {
            logger_print(ERR, TAG, "Failed to register topic %s", current->info->topic);
            return KERNEL_ERROR_MQTT_REGISTER_FAIL;
        }
    }

    return KERNEL_ERROR_NONE;
}
