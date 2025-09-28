#pragma once

#include "stddef.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Serializes a device report into JSON format.
 *
 * This function receives a `device_report_st` structure from the provided FreeRTOS queue
 * and serializes it into a JSON object using ArduinoJson. The JSON format includes a
 * timestamp and an array of sensor readings with their `value` and `active` status.
 *
 * Example output:
 * {
 *   "timestamp": "2025-07-07T14:23:00",
 *   "sensors": [
 *     {"value": 23.4, "active": 1},
 *     {"value": 18.9, "active": 0}
 *   ]
 * }
 *
 * @param queue         The FreeRTOS queue from which the device report will be read.
 * @param out_buffer    A pointer to the buffer where the serialized JSON will be written.
 * @param buffer_size   The size of the output buffer in bytes.
 * @return kernel_error_st
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_NULL if the output buffer is null or size is 0
 *         - KERNEL_ERROR_NULL_MQTT_QUEUE if the queue is null
 *         - KERNEL_ERROR_EMPTY_QUEUE if no report was available within timeout
 *         - KERNEL_ERROR_FORMATTING if the resulting JSON didn't fit in the buffer
 */
kernel_error_st serialize_data_report(QueueHandle_t queue, char *out_buffer, size_t buffer_size);

/**
 * @brief Serializes a CMD_SET_CALIBRATION command response into JSON format.
 *
 * Outputs a JSON object with:
 * {
 *   "command_status": <status>,
 *   "sensor_id": <sensor index>
 * }
 *
 * @param cmd_sensor_response Pointer to the sensor response payload.
 * @param out_buffer Buffer where the serialized JSON will be written.
 * @param buffer_size Size of the output buffer.
 * @return kernel_error_st Serialization result.
 */
kernel_error_st serialize_command_response(QueueHandle_t queue, char *out_buffer, size_t buffer_size);

/**
 * @brief Deserializes a `set_calibration` command from a JSON object and pushes it to a queue.
 *
 * This function expects a JSON object with the following keys:
 * - `"sensor_id"` (int)
 * - `"gain"` (float)
 * - `"offset"` (float)
 *
 * It validates the presence and types of all required fields. If valid,
 * the function constructs a `command_st` structure with the `CMD_SET_CALIBRATION`
 * index and populates its payload, then sends it to the provided FreeRTOS queue.
 *
 * Example expected JSON input:
 * {
 *   "sensor_id": 1,
 *   "gain": 10.5,
 *   "offset": -2.1
 * }
 *
 * @param queue         FreeRTOS queue where the parsed command will be sent.
 * @param json_object   Reference to a JsonObject containing command parameters.
 * @return kernel_error_st
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_MISSING_FIELD if a required key is missing
 *         - KERNEL_ERROR_INVALID_TYPE if any value is of the wrong type
 *         - KERNEL_ERROR_QUEUE_SEND if sending to the queue fails
 */
kernel_error_st deserialize_command(QueueHandle_t queue, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif