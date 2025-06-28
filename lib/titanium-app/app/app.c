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

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "math.h"

#include "app/error/error_num.h"

#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/interface/task_interface.h"

#include "app/application_external_types.h"
#include "app/driver/ads1115.h"
#include "app/driver/tca9548a.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st *_global_structures = NULL;  ///< Pointer to the global configuration structure.

sensor_response_st sensor_response = {0};  ///< Sensor response structure to hold sensor data.

static const char *TAG = "Application Task";  ///< Tag used for logging.

mqtt_topic_st mqtt_topic = {
    .topic               = "temperature",
    .data_size           = sizeof(uint32_t),
    .qos                 = QOS_1,
    .mqtt_data_direction = PUBLISH,
    .parse_store_json    = NULL,
    .encode_json         = NULL,
};

static esp_err_t app_task_initialize() {
    sensor_manager_initialize();

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
        for (int i = 0; i < NUM_OF_CHANNELS; i++) {
            float voltage = 0.0f;
            if (sensor_get_voltage(i, &voltage) != KERNEL_ERROR_NONE) {
                logger_print(ERR, TAG, "Failed to get voltage for sensor %d", i);
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            sensor_response.sensor_array[i].type      = sensor_get_type(i);  // Set the sensor type
            sensor_response.sensor_array[i].raw_value = voltage;             // Set the raw value from the sensor
            sensor_response.num_of_active_sensors     = (i + 1);             // Set the number of active sensors
            logger_print(INFO, TAG, "Sensor %d: Type: %d, Voltage: %.2f",
                         i,
                         sensor_response.sensor_array[i].type,
                         voltage);  // Print the sensor data
        }

        led_on = !led_on;
        gpio_set_level(GPIO_NUM_32, led_on);
        vTaskDelay(pdMS_TO_TICKS(10000));  // Wait for the specified time interval
    }
}