#include "serializer_handlers.h"

#include "app/app_extern_types.h"
#include "app/external/json_handler.h"

#include "app/iot/schemas/schema_validator.h"
#include "app/iot/schemas/commands_schema.h"

#define MAXIMUM_DOC_SIZE 1024

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
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_NULL if the output buffer is null or size is 0
 *         - KERNEL_ERROR_NULL_MQTT_QUEUE if the queue is null
 *         - KERNEL_ERROR_EMPTY_QUEUE if no report was available within timeout
 *         - KERNEL_ERROR_FORMATTING if the resulting JSON didn't fit in the buffer
 */
kernel_error_st serialize_data_report(QueueHandle_t queue, char *out_buffer, size_t buffer_size) {
    if (out_buffer == NULL || buffer_size == 0) {
        return KERNEL_ERROR_NULL;
    }

    if (queue == NULL) {
        return KERNEL_ERROR_QUEUE_NULL;
    }

    device_report_st device_report{};
    if (xQueueReceive(queue, &device_report, pdMS_TO_TICKS(100)) != pdTRUE) {
        return KERNEL_ERROR_EMPTY_QUEUE;
    }

    StaticJsonDocument<MAXIMUM_DOC_SIZE> doc;

    doc["timestamp"] = device_report.timestamp;

    JsonArray sensors = doc.createNestedArray("sensors");
    for (int i = 0; i < device_report.num_of_channels; i++) {
        JsonObject sensor = sensors.createNestedObject();
        sensor["value"]   = device_report.sensors[i].value;
        sensor["active"]  = device_report.sensors[i].active;
    }

    size_t json_size = serializeJson(doc, out_buffer, buffer_size);

    if (json_size == 0 || json_size >= buffer_size) {
        return KERNEL_ERROR_FORMATTING;
    }

    return KERNEL_ERROR_NONE;
}

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
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_MISSING_FIELD if a required key is missing
 *         - KERNEL_ERROR_INVALID_TYPE if any value is of the wrong type
 *         - KERNEL_ERROR_QUEUE_SEND if sending to the queue fails
 */
kernel_error_st deserialize_command_set_calibration(QueueHandle_t queue, JsonObject &json_object) {
    kernel_error_st validation_result = validate_json_schema(
        json_object, calibration_schema, sizeof(calibration_schema) / sizeof(json_field_t));

    if (validation_result != KERNEL_ERROR_NONE) {
        return validation_result;
    }

    command_st command{};

    command.command_index                          = CMD_SET_CALIBRATION;
    command.command_u.set_calibration.sensor_index = json_object["sensor_id"];
    command.command_u.set_calibration.gain         = json_object["gain"];
    command.command_u.set_calibration.offset       = json_object["offset"];

    if (xQueueSend(queue, &command, pdMS_TO_TICKS(100)) != pdPASS) {
        return KERNEL_ERROR_QUEUE_SEND;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Deserializes a command from a JSON string buffer and dispatches it.
 *
 * This function parses a JSON string from `buffer`, extracts the command identifier (`"command"`)
 * and its associated parameters (`"params"`), and delegates to the appropriate handler based
 * on the command type. The parsed command is then sent to the specified FreeRTOS queue.
 *
 * Expected JSON structure:
 * {
 *   "command": 1,            // integer corresponding to command_index_et
 *   "params": {
 *     "sensor_id": 1,
 *     "gain": 10.0,
 *     "offset": -0.5
 *   }
 * }
 *
 * @param queue         Queue handle to which the decoded command will be sent.
 * @param buffer        JSON string buffer to deserialize.
 * @param buffer_size   Size of the buffer (unused here but kept for API consistency).
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_DESERIALIZE_JSON if the JSON is malformed
 *         - KERNEL_ERROR_MISSING_FIELD if required fields are not found
 *         - KERNEL_ERROR_INVALID_COMMAND if the command index is unrecognized
 *         - Any other error code returned by a specific command deserializer
 */
kernel_error_st deserialize_command(QueueHandle_t queue, char *buffer, size_t buffer_size) {
    StaticJsonDocument<MAXIMUM_DOC_SIZE> doc{};
    kernel_error_st result = KERNEL_ERROR_NONE;

    DeserializationError error = deserializeJson(doc, buffer);

    if (error) {
        return KERNEL_ERROR_DESERIALIZE_JSON;
    }

    if (!doc.containsKey("command")) {
        return KERNEL_ERROR_MISSING_FIELD;
    }

    if (!doc["command"].is<int>()) {
        return KERNEL_ERROR_INVALID_TYPE;
    }

    command_index_et command = doc["command"];
    if (!doc.containsKey("params")) {
        return KERNEL_ERROR_MISSING_FIELD;
    }

    JsonObject params = doc["params"];

    switch (command) {
        case CMD_SET_CALIBRATION: {
            result = deserialize_command_set_calibration(queue, params);
            break;
        }
        default:
            result = KERNEL_ERROR_INVALID_COMMAND;
    }

    return result;
}
