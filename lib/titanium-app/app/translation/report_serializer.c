/**
 * @file serialize_device_report.c
 * @brief Serializes a 'device_report_st' struct into a human-readable JSON string.
 *
 * This module provides a serialization function that converts device sensor data into
 * a JSON-formatted string using 'snprintf'. It avoids dynamic memory allocation or external
 * libraries like 'cJSON' to ensure compatibility with embedded systems or memory-constrained environments.
 *
 * @note This approach prioritizes readability and simplicity over compactness. It is not
 *       the most memory-efficient or scalable method for JSON serialization but is very
 *       useful when working without heap allocation.
 */
#include "app/translation/report_serializer.h"

#include "app/app_extern_types.h"

#include "kernel/logger/logger.h" //deletar

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

/**
 * @brief Serializes a 'device_report_st' structure into a JSON string.
 *
 * This function formats the contents of the device report into a human-readable JSON format
 * using 'snprintf'. It avoids dynamic memory allocation, making it suitable for embedded use.
 *
 * @param data Pointer to a 'device_report_st' structure.
 * @param out_buffer Pre-allocated buffer to write the resulting JSON string into.
 * @param buffer_size Size of the output buffer.
 * @return 'APP_ERROR_NONE' on success, or an appropriate error code if serialization fails.
 */
kernel_error_st serialize_device_report(QueueHandle_t queue, char *out_buffer, size_t buffer_size) {
    if (queue == NULL || out_buffer == NULL || buffer_size == 0) {
        return KERNEL_ERROR_NULL;
    }
    device_report_st device_report = {0};
    if (xQueueReceive(queue, &device_report, pdMS_TO_TICKS(100)) != pdTRUE) {
        return KERNEL_ERROR_QUEUE_EMPTY;
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
        return KERNEL_ERROR_SNPRINTF;
    }

    return KERNEL_ERROR_NONE;
}