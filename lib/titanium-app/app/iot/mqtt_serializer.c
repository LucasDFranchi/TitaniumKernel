#include "mqtt_serializer.h"

#include "kernel/logger/logger.h"

#include "app/app_extern_types.h"
#include "app/iot/serializer_handlers.h"

/* MQTT Serializer Global Variables */
static const char *TAG = "MQTT_Serializer";

/**
 * @brief Serializes data from a topic's queue into a buffer for MQTT transmission.
 *
 * This function handles serialization based on the topic's data type.
 * It reads data from the associated FreeRTOS queue and converts it into a JSON string
 * or other appropriate format, storing it in the provided buffer.
 *
 * Currently supports:
 * - DATA_TYPE_REPORT: Uses `serialize_data_report()` to serialize sensor data.
 *
 * @param[in] topic        Pointer to the MQTT topic containing the queue and metadata.
 * @param[out] buffer      Output buffer where serialized data will be stored.
 * @param[in] buffer_size  Size of the output buffer in bytes.
 *
 * @return KERNEL_SUCCESS on success.
 * @return KERNEL_ERROR_NULL if any input pointer is NULL.
 * @return KERNEL_ERROR_INVALID_SIZE if the buffer size is zero.
 * @return KERNEL_ERROR_UNSUPPORTED_TYPE if the topic data type is not recognized.
 * @return Other kernel_error_st values returned by specific serializer functions.
 */
kernel_error_st mqtt_serialize_data(mqtt_topic_st *topic, char *buffer, size_t buffer_size) {
    if ((topic == NULL) || (buffer == NULL)) {
        logger_print(ERR, TAG, "%s - Null pointer argument", __func__);
        return KERNEL_ERROR_NULL;
    }

    if (buffer_size == 0) {
        logger_print(ERR, TAG, "%s - Buffer size is zero", __func__);
        return KERNEL_ERROR_INVALID_SIZE;
    }

    kernel_error_st err;

    switch (topic->info->data_type) {
        case DATA_TYPE_REPORT:
            err = serialize_data_report(topic->queue, buffer, buffer_size);
            break;
        case DATA_TYPE_COMMAND_RESPONSE:
            err = serialize_command_response(topic->queue, buffer, buffer_size);
            break;
        default:
            logger_print(ERR, TAG, "Unsupported data type: %d", topic->info->data_type);
            return KERNEL_ERROR_UNSUPPORTED_TYPE;
    }

    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Serialization failed for topic %s - %d", topic->info->topic, err);
    }

    return err;
}

/**
 * @brief Deserializes MQTT payload data and pushes the result into the appropriate queue.
 *
 * This function interprets incoming MQTT message content according to the topic’s expected
 * data type. It extracts structured data (e.g., commands) and dispatches it to the application
 * via a queue.
 *
 * Currently supports:
 * - DATA_TYPE_COMMAND: Uses `deserialize_command()` to parse the incoming JSON and enqueue a command.
 *
 * @param[in] topic        Pointer to the MQTT topic associated with the incoming data.
 * @param[in] buffer       Buffer containing the raw MQTT payload data (typically a JSON string).
 * @param[in] buffer_size  Size of the buffer in bytes.
 *
 * @return KERNEL_SUCCESS on success.
 * @return KERNEL_ERROR_NULL if any pointer argument is NULL.
 * @return KERNEL_ERROR_INVALID_SIZE if buffer size is zero.
 * @return KERNEL_ERROR_UNSUPPORTED_TYPE if the topic data type is not supported.
 * @return Other kernel_error_st values returned by specific deserializer functions.
 */
kernel_error_st mqtt_deserialize_data(mqtt_topic_st *topic, char *buffer, size_t buffer_size) {
    if ((topic == NULL) || (buffer == NULL)) {
        logger_print(ERR, TAG, "%s - Null pointer argument", __func__);
        return KERNEL_ERROR_NULL;
    }

    if (buffer_size == 0) {
        logger_print(ERR, TAG, "%s - Buffer size is zero", __func__);
        return KERNEL_ERROR_INVALID_SIZE;
    }

    kernel_error_st err = KERNEL_SUCCESS;
    switch (topic->info->data_type) {
        case DATA_TYPE_COMMAND:
            err = deserialize_command(topic->queue, buffer, buffer_size);
            break;

        default:
            logger_print(ERR, TAG, "Unsupported data type: %d", topic->info->data_type);
            return KERNEL_ERROR_UNSUPPORTED_TYPE;
    }

    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Deserialization failed for topic %s - %d", topic->info->topic, err);
    }

    return err;
}
