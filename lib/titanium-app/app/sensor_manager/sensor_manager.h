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

#include "app/sensor_manager/sensor_types.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

/**
 * @brief Main loop for the Sensor Manager task.
 *
 * Periodically reads data from all available sensors, builds a device report,
 * and sends it to the sensor manager queue.
 *
 * @param args Pointer to a `sensor_manager_init_st` structure containing
 *             the queue handle for sending reports.
 *
 * @note Runs indefinitely as an RTOS task. This function should be registered
 *       with the RTOS task scheduler at startup.
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
 * @brief Get the calibration gain of a sensor.
 *
 * Safely returns the conversion gain applied to the sensor readings.
 *
 * @param sensor_index Index of the sensor (0..NUM_OF_SENSORS-1).
 * @return float The conversion gain, or 1.0f if the index is invalid
 *               or the mutex could not be taken.
 */
float sensor_get_gain(uint8_t sensor_index);

/**
 * @brief Get the calibration offset of a sensor.
 *
 * Safely returns the offset applied to the raw sensor readings.
 *
 * @param sensor_index Index of the sensor (0..NUM_OF_SENSORS-1).
 * @return float The offset, or 0.0f if the index is invalid or
 *               the mutex could not be taken.
 */
float sensor_get_offset(uint8_t sensor_index);

/**
 * @brief Get the current state of a sensor.
 *
 * Safely returns the runtime state of the sensor (enabled, disabled, etc.).
 *
 * @param sensor_index Index of the sensor (0..NUM_OF_SENSORS-1).
 * @return sensor_state_et The sensor state, or SENSOR_DISABLED if the
 *                         index is invalid or the mutex could not be taken.
 */
sensor_state_et sensor_get_state(uint8_t sensor_index);

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
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_INVALID_ARG if the sensor index is out of range
 */
kernel_error_st sensor_calibrate(uint8_t sensor_index, float offset, float gain);
