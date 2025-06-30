/**
 * @file app.c
 * @brief Temperature monitoring and logging for temperature and humidity data.
 *
 * This module interfaces with the AHT10 temperature and humidity sensor,
 * reads the data, and logs the temperature and humidity values periodically.
 * It provides functions to initialize the sensor, execute continuous readings,
 * and log the results.
 */
#include "app.h"

#include "string.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "math.h"

#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/interface/task_interface.h"
#include "kernel/utils/utils.h"

#include "app/app_extern_types.h"
#include "app/error/error_num.h"
#include "app/iot/mqtt_topic_defs.h"
#include "app/sensor/sensor.h"
#include "app/translation/report_serializer.h"

/* Application Global Variables */
static mqtt_topic_st mqtt_topics[TOPIC_COUNT] = {0};  ///< Array of MQTT topics for sensor data.
static size_t mqtt_topics_count               = 0;    ///< Number of MQTT topics initialized.
static device_report_st device_report         = {0};

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st *_global_structures = NULL;                ///< Pointer to the global configuration structure.
static const char *TAG                          = "Application Task";  ///< Tag used for logging.

static esp_err_t app_task_initialize() {
    sensor_manager_initialize();

    mqtt_topics_init(mqtt_topics, &mqtt_topics_count, TOPIC_COUNT);
    mqtt_topics[0].queue          = _global_structures->global_queues.sensor_report_queue;
    mqtt_topics[0].serialize_data = serialize_device_report;

    for (size_t i = 0; i < mqtt_topics_count; i++) {
        if (xQueueSend(_global_structures->global_queues.mqtt_topic_queue,
                       &mqtt_topics[i],
                       pdMS_TO_TICKS(100)) != pdPASS) {
            logger_print(ERR, TAG, "Failed to send MQTT topic %s to queue", mqtt_topics[i].topic);
        }
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_32),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    return ESP_OK;
}

void app_task_execute(void *pvParameters) {
    _global_structures = (global_structures_st *)pvParameters;
    if ((app_task_initialize() != ESP_OK) ||
        (_global_structures == NULL) ||
        (_global_structures->global_events.firmware_event_group == NULL)) {
        logger_print(ERR, TAG, "Failed to initialize app task");
        vTaskDelete(NULL);
    }
    static bool led_on = false;

    while (1) {
        memset(&device_report, 0, sizeof(device_report_st));

        get_timestamp_in_iso_format(device_report.timestamp, sizeof(device_report.timestamp));

        for (int i = 0; i < NUM_OF_CHANNELS; i++) {
            float voltage = 0.0f;
            if (sensor_get_voltage(i, &voltage) != KERNEL_ERROR_NONE) {
                logger_print(ERR, TAG, "Failed to get voltage for sensor %d", i);
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            device_report.sensors[i].value  = voltage;
            device_report.sensors[i].active = true;
        }
        if (xQueueSend(_global_structures->global_queues.sensor_report_queue,
                       &device_report,
                       pdMS_TO_TICKS(100)) != pdPASS) {
            logger_print(ERR, TAG, "Failed to send sensor report to queue");
        }

        led_on = !led_on;
        gpio_set_level(GPIO_NUM_32, led_on);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}