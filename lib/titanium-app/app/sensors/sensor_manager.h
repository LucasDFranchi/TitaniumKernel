#pragma once
/**
 * @file sensor_manager.h
 * @brief Sensor manager interface for multi-channel data acquisition.
 *
 * This module provides the public interface for managing and reading sensors
 * connected through a TCA9548A I2C multiplexer and an ADS1115 ADC.
 * It defines the supported sensor channels, report structures, and API functions
 * for initialization, measurement, calibration, and sensor type queries.
 *
 * Key responsibilities:
 * - Initialize and configure the ADC and multiplexer controllers.
 * - Manage up to NUM_OF_CHANNEL_SENSORS logical sensor channels.
 * - Collect raw voltage measurements and convert them into sensor reports.
 * - Apply per-sensor calibration (gain and offset).
 * - Send aggregated device reports to a FreeRTOS queue for higher-level processing.
 *
 * This module acts as the central abstraction layer between low-level hardware
 * drivers (ADC + MUX) and application-level tasks that consume sensor data.
 */


#include "stdbool.h"
#include "stdint.h"

// #include "app/sensors/sensor_interface/sensor_interface.h"
#include "app/sensors/sensor_types.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

typedef struct sensor_manager_config_s {
    QueueHandle_t sensor_manager_queue; /**< Queue for sensor manager tasks */
} sensor_manager_config_st;

/**
 * @brief Initialize the sensor manager and its dependencies.
 *
 * This function sets up the sensor manager using the provided configuration.
 * It validates the input parameters, stores the event queue reference,
 * initializes the ADC and multiplexer controllers, and assigns the appropriate
 * sensor interfaces (ADC + MUX + read function) for all available channels.
 *
 * The function must be called once before performing any sensor operations.
 *
 * @param[in] config Pointer to the sensor manager configuration structure.
 *                   Must not be NULL, and must contain a valid event queue.
 *
 * @return
 *     - KERNEL_ERROR_NONE on success
 *     - KERNEL_ERROR_INVALID_ARG if the configuration is invalid
 *     - KERNEL_ERROR_MUX_INIT_ERROR if the multiplexer controller fails to initialize
 *     - KERNEL_ERROR_ADC_INIT_ERROR if the ADC controller fails to initialize
 *
 * @note This function initializes global/static controllers and interfaces.
 *       It should be called only once during system startup.
 */
kernel_error_st sensor_manager_initialize(sensor_manager_config_st *config);

/**
 * @brief Gathers sensor readings and sends a device report to the provided queue.
 *
 * This function:
 * - Retrieves the current device timestamp.
 * - Iterates through all sensor channels, collecting voltage readings.
 * - Marks each successfully read sensor as active and stores the voltage.
 * - Populates a `device_report_st` structure with the results.
 * - Sends the report to the specified FreeRTOS queue.
 *
 * If a sensor reading fails, it logs an error and skips to the next channel.
 *
 * @param device_report_queue FreeRTOS queue to send the generated device report.
 */
void sensor_manager_loop();

/**
 * @brief Gets the sensor type.
 *
 * Returns the type (e.g., temperature, pressure) of the sensor at the given index.
 *
 * @param sensor_index Index into the sensor_info table.
 * @return Sensor type, or SENSOR_TYPE_UNDEFINED if index is out of bounds.
 */
sensor_type_et sensor_get_type(uint8_t sensor_index);

/**
 * @brief Calibrates a specific sensor by updating its gain and offset.
 *
 * This function updates the calibration parameters for a sensor identified by
 * its index. The gain is used to scale the final measured voltage, and the offset
 * is subtracted from the raw internal voltage before gain is applied.
 *
 * Example:
 * - Measured voltage = (raw_voltage - offset) * gain
 *
 * These values are stored in the 'sensor_info' table and applied in future readings.
 *
 * @param sensor_index Index of the sensor to calibrate (0 to NUM_OF_MUX_CHANNELS - 1).
 * @param offset       Offset value to subtract from the raw measured voltage.
 * @param gain         Gain factor to apply after offset adjustment.
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_INVALID_ARG if the sensor index is out of range
 */
kernel_error_st sensor_calibrate(uint8_t sensor_index, float offset, float gain);
