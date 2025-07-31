#pragma once

#include "stdbool.h"
#include "stdint.h"

#include "kernel/error/error_num.h"

#include "kernel/inter_task_communication/inter_task_communication.h"

/**
 * @file sensor.h
 * @brief Sensor configuration and readout interface.
 *
 * This module defines the types, constants, and functions required to configure
 * and retrieve measurements from various sensors connected via MUX and ADS1115 ADC.
 */

/**
 * @def NUM_OF_CHANNELS
 * @brief Total number of sensor channels supported.
 */
#define NUM_OF_CHANNELS 2

/**
 * @enum sensor_type_et
 * @brief Enumerates the types of sensors supported.
 */
typedef enum sensor_type_e {
    SENSOR_TYPE_TEMPERATURE = 0, /**< Temperature sensor */
    SENSOR_TYPE_PRESSURE,        /**< Pressure sensor */
    SENSOR_TYPE_VOLTAGE,         /**< Voltage sensor */
    SENSOR_TYPE_CURRENT,         /**< Current sensor */
    SENSOR_TYPE_POWER_FACTOR,    /**< Power Factor sensor */
    SENSOR_TYPE_UNDEFINED,       /**< Unknown or invalid sensor type */
} sensor_type_et;

/**
 * @brief Structure representing a single sensor's report.
 */
typedef struct sensor_report_s {
    float value;                /**< Measured value from the sensor */
    bool active;                /**< Indicates whether the sensor is currently active */
    sensor_type_et sensor_type; /**< Indicates the sensor type */
} sensor_report_st;

/**
 * @brief Initializes the I2C interface, GPIOs, and ADS1115 ADC.
 *
 * This function sets up GPIO used to reset the TCA9548A multiplexers,
 * configures and installs the I2C driver, and initializes the ADS1115 with
 * default comparator and operation settings.
 *
 * This function must be called before any sensor operations.
 */
void sensor_manager_initialize(void);

/**
 * @brief Configures the ADC and multiplexer for a specific sensor.
 *
 * Sets the MUX address and channel, ADC configuration, and triggers conversion.
 *
 * @param sensor_index Index into the sensor_info table.
 * @return KERNEL_ERROR_NONE on success, or KERNEL_ERROR_INVALID_ARG on invalid index.
 */
kernel_error_st sensor_configure(uint8_t sensor_index);

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
 * These values are stored in the `sensor_info` table and applied in future readings.
 *
 * @param sensor_index Index of the sensor to calibrate (0 to NUM_OF_CHANNELS - 1).
 * @param offset       Offset value to subtract from the raw measured voltage.
 * @param gain         Gain factor to apply after offset adjustment.
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_INVALID_ARG if the sensor index is out of range
 */
kernel_error_st sensor_calibrate(uint8_t sensor_index, float offset, float gain);

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
void handle_device_report(QueueHandle_t device_report_queue);

/**
 * @brief Gets the sensor type.
 *
 * Returns the type (e.g., temperature, pressure) of the sensor at the given index.
 *
 * @param sensor_index Index into the sensor_info table.
 * @return Sensor type, or SENSOR_TYPE_UNDEFINED if index is out of bounds.
 */
sensor_type_et sensor_get_type(uint8_t sensor_index);
