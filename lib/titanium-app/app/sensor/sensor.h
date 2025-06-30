#pragma once

#include "stdint.h"
#include "stdbool.h"

#include "kernel/error/error_num.h"

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
#define NUM_OF_CHANNELS (22)

/**
 * @enum sensor_type_et
 * @brief Enumerates the types of sensors supported.
 */
typedef enum sensor_type_e {
    SENSOR_TYPE_TEMPERATURE = 0, /**< Temperature sensor */
    SENSOR_TYPE_PRESSURE,        /**< Pressure sensor */
    SENSOR_TYPE_VOLTAGE,         /**< Voltage sensor */
    SENSOR_TYPE_CURRENT,         /**< Current sensor */
    SENSOR_TYPE_UNDEFINED,       /**< Unknown or invalid sensor type */
} sensor_type_et;

/**
 * @brief Structure representing a single sensor's report.
 */
typedef struct sensor_report_s {
    float value; /**< Measured value from the sensor */
    bool active; /**< Indicates whether the sensor is currently active */
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
 * @brief Reads the voltage for a given sensor channel.
 *
 * This function waits for the ADC conversion to complete, retrieves the raw ADC value,
 * and converts it to a voltage using the sensor's configuration.
 *
 * @param sensor_index Index into the sensor_info table.
 * @param voltage Pointer to a float where the resulting voltage will be stored.
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success,
 *         - KERNEL_ERROR_INVALID_ARG if index is invalid,
 *         - KERNEL_ERROR_ADC_CONVERSION_ERROR if the ADC times out.
 */
kernel_error_st sensor_get_voltage(uint8_t sensor_index, float *voltage);

/**
 * @brief Gets the sensor type.
 *
 * Returns the type (e.g., temperature, pressure) of the sensor at the given index.
 *
 * @param sensor_index Index into the sensor_info table.
 * @return Sensor type, or SENSOR_TYPE_UNDEFINED if index is out of bounds.
 */
sensor_type_et sensor_get_type(uint8_t sensor_index);
