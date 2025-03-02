#pragma once

#include "stddef.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

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
 * @return KERNEL_ERROR_NONE on success.
 * @return KERNEL_ERROR_NULL if any pointer argument is NULL.
 * @return KERNEL_ERROR_INVALID_SIZE if buffer size is zero.
 * @return KERNEL_ERROR_UNSUPPORTED_TYPE if the topic data type is not supported.
 * @return Other kernel_error_st values returned by specific deserializer functions.
 */
kernel_error_st mqtt_serialize_data(mqtt_topic_st *topic, char *buffer, size_t buffer_size);

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
 * @return KERNEL_ERROR_NONE on success.
 * @return KERNEL_ERROR_NULL if any pointer argument is NULL.
 * @return KERNEL_ERROR_INVALID_SIZE if buffer size is zero.
 * @return KERNEL_ERROR_UNSUPPORTED_TYPE if the topic data type is not supported.
 * @return Other kernel_error_st values returned by specific deserializer functions.
 */
kernel_error_st mqtt_deserialize_data(mqtt_topic_st *topic, char *buffer, size_t buffer_size);