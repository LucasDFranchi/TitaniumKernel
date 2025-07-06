#include "mqtt_serializer.h"

#include "kernel/logger/logger.h"

#include "app/app_extern_types.h"

/* MQTT Serializer Global Variables */
static const char *TAG = "MQTT_Serializer";

const char *json_payload =
    "{\n"
    "  \"timestamp\": \"%s\",\n"
    "  \"sensors\": [\n"
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 00
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 01
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 02
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 03
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 04
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 05
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 06
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 07
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 08
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 09
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 10
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 11
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 12
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 13
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 14
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 15
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 16
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 17
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 18
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 19
    "    {\"value\": %.2f, \"active\": %d},\n"  // Sensor 20
    "    {\"value\": %.2f, \"active\": %d}\n"   // Sensor 21
    "  ]\n"
    "}";

kernel_error_st serialize_data_report(QueueHandle_t queue, char* out_buffer, const char buffer_size) {
    if (queue == NULL || out_buffer == NULL || buffer_size == 0) {
        return KERNEL_ERROR_NULL;
    }
    device_report_st device_report = {0};
    if (xQueueReceive(queue, &device_report, pdMS_TO_TICKS(100)) != pdTRUE) {
        return KERNEL_ERROR_EMPTY_QUEUE;
    }
    
    int out_json_size = snprintf(out_buffer, buffer_size, json_payload,
                                 device_report.timestamp,
                                 device_report.sensors[0].value, device_report.sensors[0].active,
                                 device_report.sensors[1].value, device_report.sensors[1].active,
                                 device_report.sensors[2].value, device_report.sensors[2].active,
                                 device_report.sensors[3].value, device_report.sensors[3].active,
                                 device_report.sensors[4].value, device_report.sensors[4].active,
                                 device_report.sensors[5].value, device_report.sensors[5].active,
                                 device_report.sensors[6].value, device_report.sensors[6].active,
                                 device_report.sensors[7].value, device_report.sensors[7].active,
                                 device_report.sensors[8].value, device_report.sensors[8].active,
                                 device_report.sensors[9].value, device_report.sensors[9].active,
                                 device_report.sensors[10].value, device_report.sensors[10].active,
                                 device_report.sensors[11].value, device_report.sensors[11].active,
                                 device_report.sensors[12].value, device_report.sensors[12].active,
                                 device_report.sensors[13].value, device_report.sensors[13].active,
                                 device_report.sensors[14].value, device_report.sensors[14].active,
                                 device_report.sensors[15].value, device_report.sensors[15].active,
                                 device_report.sensors[16].value, device_report.sensors[16].active,
                                 device_report.sensors[17].value, device_report.sensors[17].active,
                                 device_report.sensors[18].value, device_report.sensors[18].active,
                                 device_report.sensors[19].value, device_report.sensors[19].active,
                                 device_report.sensors[20].value, device_report.sensors[20].active,
                                 device_report.sensors[21].value, device_report.sensors[21].active);

    if ((out_json_size < 0) || (size_t)out_json_size >= buffer_size) {
        return KERNEL_ERROR_FORMATTING;
    }

    return KERNEL_ERROR_NONE;
}
kernel_error_st serialize_command(QueueHandle_t queue, char* buffer, const char buffer_size) {
    return KERNEL_ERROR_SERIALIZE_JSON;
}

/**
 * @brief Serializes data from a topic's queue into a buffer for MQTT transmission.
 *
 * This function handles serialization based on the topic's data type.
 * It reads data from the queue and formats it into the given buffer.
 *
 * @param[in] topic        Pointer to the MQTT topic containing the queue and metadata.
 * @param[out] buffer      Output buffer where serialized data will be stored.
 * @param[in] buffer_size  Size of the output buffer in bytes.
 *
 * @return KERNEL_ERROR_NONE on success.
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

        case DATA_TYPE_COMMAND:
            err = serialize_command(topic->queue, buffer, buffer_size);
            break;

        default:
            logger_print(ERR, TAG, "Unsupported data type: %d", topic->info->data_type);
            return KERNEL_ERROR_UNSUPPORTED_TYPE;
    }

    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Serialization failed for topic %s", topic->info->topic);
    }

    return err;
}