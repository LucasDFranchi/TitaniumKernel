#include "stddef.h"
#include "string.h"

#include "mqtt_bridge.h"

#include "kernel/device/device_info.h"
#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"

#include "app/iot/mqtt_serializer.h"

/* Module Global Defines */
/**
 * @brief Tag used for logging MQTT bridge operations.
 */
static const char *TAG = "MQTT Bridge";

/* Module Global Variables */
/**
 * @brief Number of MQTT topics registered in the bridge.
 */
static size_t mqtt_bridge_num_topics = 0;

/**
 * @brief Array of registered MQTT topics for publish/subscribe operations.
 */
static mqtt_topic_st mqtt_topics[MAX_MQTT_TOPICS] = {0};

/**
 * @brief Validates the Quality of Service (QoS) level.
 *
 * Ensures that the provided QoS level is one of the valid MQTT QoS values:
 * - QOS_0
 * - QOS_1
 * - QOS_2
 *
 * @param[in] qos QoS level to validate.
 * @return KERNEL_SUCCESS if valid; otherwise, KERNEL_ERROR_MQTT_INVALID_QOS.
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
    return KERNEL_SUCCESS;
}

/**
 * @brief Validates the MQTT data direction.
 *
 * Ensures that the direction is either:
 * - PUBLISH
 * - SUBSCRIBE
 *
 * @param[in] direction Data direction to validate.
 * @return KERNEL_SUCCESS if valid; otherwise, KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION.
 */
kernel_error_st validate_data_direction(const mqtt_data_direction_et direction) {
    switch (direction) {
        case PUBLISH:
        case SUBSCRIBE:
            break;
        default:
            return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION;
    }
    return KERNEL_SUCCESS;
}

/**
 * @brief Validates the MQTT topic string.
 *
 * Checks that the topic string:
 * - Is not NULL
 * - Is not empty
 * - Does not exceed MQTT_MAXIMUM_TOPIC_LENGTH
 *
 * @param[in] topic Pointer to the topic string.
 * @return KERNEL_SUCCESS if valid; otherwise, KERNEL_ERROR_MQTT_INVALID_TOPIC.
 */
kernel_error_st validate_mqtt_topic_length(const char *topic) {
    if (topic == NULL || strlen(topic) == 0 || strlen(topic) >= MQTT_MAXIMUM_TOPIC_LENGTH) {
        return KERNEL_ERROR_MQTT_INVALID_TOPIC;
    }
    return KERNEL_SUCCESS;
}

/**
 * @brief Validates an MQTT topic registration structure.
 *
 * Verifies that the provided MQTT topic structure is valid, including:
 * - QoS
 * - Data direction
 * - Topic string
 * - Number of registered topics (does not exceed MAX_MQTT_TOPICS)
 *
 * @param[in] topic Pointer to the topic structure to validate.
 * @return KERNEL_SUCCESS if valid; otherwise, an appropriate error code.
 */
kernel_error_st validate_mqtt_topic(const mqtt_topic_st *topic) {
    if (topic == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (validate_qos(topic->info->qos) != KERNEL_SUCCESS) {
        return KERNEL_ERROR_MQTT_INVALID_QOS;
    }

    if (validate_data_direction(topic->info->mqtt_data_direction) != KERNEL_SUCCESS) {
        return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION;
    }

    if (validate_mqtt_topic_length(topic->info->topic) != KERNEL_SUCCESS) {
        return KERNEL_ERROR_MQTT_INVALID_TOPIC;
    }

    if (mqtt_bridge_num_topics >= MAX_MQTT_TOPICS) {
        return KERNEL_ERROR_MQTT_TOO_MANY_TOPICS;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Registers a new MQTT topic in the bridge.
 *
 * Validates the topic and, if successful:
 * - Creates a FreeRTOS queue for the topic
 * - Stores the topic in the bridge’s internal topic list
 *
 * @param[in,out] topic Pointer to the topic structure to register. Queue handle will be assigned.
 * @return KERNEL_SUCCESS on success;
 * @return KERNEL_ERROR_NULL if topic is NULL;
 * @return KERNEL_ERROR_MQTT_REGISTER_FAIL if validation fails;
 * @return KERNEL_ERROR_QUEUE_NULL if queue creation fails.
 */
kernel_error_st register_topic(mqtt_topic_st *topic) {
    if (topic == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (validate_mqtt_topic(topic) != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to validate for topic %s", topic->info->topic);
        return KERNEL_ERROR_MQTT_REGISTER_FAIL;
    }

    kernel_error_st err = queue_manager_register(topic->queue_index,
                                                 topic->info->queue_length,
                                                 topic->info->queue_item_size);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to create queue for topic %s", topic->info->topic);
        return KERNEL_ERROR_QUEUE_NULL;
    }

    mqtt_topics[mqtt_bridge_num_topics++] = *topic;

    return KERNEL_SUCCESS;
}

/**
 * @brief Checks if a topic has pending data to publish.
 *
 * Inspects the FreeRTOS queue associated with the topic to determine
 * if there are messages waiting to be sent.
 *
 * @param[in] topic Pointer to the MQTT topic structure.
 * @retval true  if data is available in the queue.
 * @retval false if topic or queue is NULL, or no messages are waiting.
 */
bool has_data_to_publish(const mqtt_topic_st *topic) {
    if (topic == NULL) {
        return false;
    }

    QueueHandle_t queue = queue_manager_get(topic->queue_index);
    if (queue == NULL) {
        return false;
    }
    
    return (uxQueueMessagesWaiting(queue) > 0);
}

/**
 * @brief Fetches the next publishable message for a topic.
 *
 * Retrieves data from the topic queue, serializes it, and builds
 * the complete MQTT topic string including the device ID.
 *
 * @param[in]  mqtt_index Index of the topic in the internal topic list.
 * @param[out] topic      Pointer to buffer structure for the formatted MQTT topic string.
 * @param[out] payload    Pointer to buffer structure for the serialized payload.
 * @param[out] qos        Pointer to store the message QoS level.
 *
 * @return KERNEL_SUCCESS on success.
 * @return KERNEL_ERROR_NULL if any pointer is NULL.
 * @return KERNEL_ERROR_INVALID_INDEX if mqtt_index is out of bounds.
 * @return KERNEL_ERROR_MQTT_QUEUE_NULL if topic queue is NULL.
 * @return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION if topic is not PUBLISH type.
 * @return KERNEL_ERROR_EMPTY_QUEUE if no data available.
 * @return KERNEL_ERROR_FORMATTING if topic buffer is too small.
 * @return Other serialization errors from mqtt_serialize_data().
 */
kernel_error_st fetch_publish_data(uint8_t mqtt_index, mqtt_buffer_st *topic, mqtt_buffer_st *payload, qos_et *qos) {
    if (topic == NULL || payload == NULL || qos == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (mqtt_index >= mqtt_bridge_num_topics) {
        return KERNEL_ERROR_INVALID_INDEX;
    }

    mqtt_topic_st *current = &mqtt_topics[mqtt_index];

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

    if (err != KERNEL_SUCCESS) {
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

    return KERNEL_SUCCESS;
}

/**
 * @brief Gets the number of topics registered in the bridge.
 *
 * @return Count of registered MQTT topics.
 */
static inline size_t get_topics_count(void) {
    return mqtt_bridge_num_topics;
}

/**
 * @brief Builds subscription details for a topic.
 *
 * Constructs the full MQTT subscription string including device ID (if required),
 * and retrieves the associated QoS.
 *
 * @param[in]  mqtt_index Index of the topic in the internal list.
 * @param[out] topic      Pointer to buffer for the subscription topic string.
 * @param[out] qos        Pointer to store QoS value.
 *
 * @return KERNEL_SUCCESS on success;
 * @return KERNEL_ERROR_INVALID_INDEX if index is invalid;
 * @return KERNEL_ERROR_MQTT_QUEUE_NULL if queue is NULL;
 * @return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION if topic is not SUBSCRIBE type;
 * @return KERNEL_ERROR_MQTT_INVALID_MESSAGE_TYPE if message type is not supported;
 * @return KERNEL_ERROR_FORMATTING if buffer is too small.
 */
static kernel_error_st get_topic(uint8_t mqtt_index, mqtt_buffer_st *topic, qos_et *qos) {
    if (mqtt_index >= mqtt_bridge_num_topics) {
        return KERNEL_ERROR_INVALID_INDEX;
    }

    mqtt_topic_st *current = &mqtt_topics[mqtt_index];

    if (current->info->mqtt_data_direction != SUBSCRIBE) {
        return KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION;
    }

    size_t channel_size = 0;
    if (current->info->message_type == MESSAGE_TYPE_TARGET) {
        channel_size = snprintf(
            topic->buffer,
            topic->size,
            "iocloud/request/%s/%s",
            device_info_get_id(),
            current->info->topic);

    } else if (current->info->message_type == MESSAGE_TYPE_BROADCAST) {
        channel_size = snprintf(
            topic->buffer,
            topic->size,
            "iocloud/request/%s",
            current->info->topic);

    } else {
        return KERNEL_ERROR_MQTT_INVALID_MESSAGE_TYPE;
    }

    if (channel_size >= topic->size) {
        logger_print(WARN, TAG, "Channel buffer too small");
        return KERNEL_ERROR_FORMATTING;
    }

    *qos = current->info->qos;

    return KERNEL_SUCCESS;
}

/**
 * @brief Initializes the MQTT bridge and registers topics.
 *
 * Sets function pointers in the bridge instance, assigns the device unique ID,
 * and registers all topics specified in the initialization structure.
 *
 * @param[in] mqtt_bridge_init_struct Pointer to initialization structure containing:
 *            - Bridge instance
 *            - List of topics
 *            - Number of topics
 *
 * @return KERNEL_SUCCESS on success;
 * @return KERNEL_ERROR_NULL if pointers are invalid;
 * @return KERNEL_ERROR_FORMATTING if device ID is too long;
 * @return KERNEL_ERROR_MQTT_REGISTER_FAIL if topic registration fails.
 */
static kernel_error_st handle_event_data(char *topic, mqtt_buffer_st *payload) {
    if ((topic == NULL) || (payload->buffer == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    if (payload->size == 0) {
        return KERNEL_ERROR_INVALID_SIZE;
    }
    kernel_error_st err = KERNEL_SUCCESS;

    for (uint8_t i = 0; i < mqtt_bridge_num_topics; i++) {
        mqtt_topic_st *current = &mqtt_topics[i];

        // TODO: Currently, strstr works well for specific target commands, but it causes an issue
        // when processing broadcast commands. All target commands may be mistakenly identified
        // as broadcast commands. To improve this, a more robust check is needed.
        //
        // As a temporary fix, we process all broadcast commands before target commands.
        // This ensures that target commands are not mistakenly handled as broadcast commands.
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
 * @return KERNEL_SUCCESS on successful initialization.
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
    mqtt_bridge->get_topic          = get_topic;
    mqtt_bridge->handle_event_data  = handle_event_data;

    for (size_t i = 0; i < mqtt_bridge_init_struct->topic_count; i++) {
        mqtt_topic_st *current = &mqtt_bridge_init_struct->topics[i];
        kernel_error_st err    = register_topic(current);

        if (err != KERNEL_SUCCESS) {
            logger_print(ERR, TAG, "Failed to register topic %s", current->info->topic);
            return KERNEL_ERROR_MQTT_REGISTER_FAIL;
        }
    }

    return KERNEL_SUCCESS;
}
