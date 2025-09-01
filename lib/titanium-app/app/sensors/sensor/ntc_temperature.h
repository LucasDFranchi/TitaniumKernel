#pragma once

#include "app/sensors/sensor_interface/sensor_interface.h"

/**
 * @brief Acquire and convert temperature reading from an NTC sensor.
 *
 * This is the main entry point for sensor reading. The function:
 *  - Selects the appropriate MUX channel for the sensor.
 *  - Configures and samples both reference and sensor ADC branches.
 *  - Dynamically adjusts PGA gain if required to improve accuracy.
 *  - Converts raw ADC readings into voltages, then into resistance and finally temperature.
 *
 * @param ctx Sensor interface context containing hardware configuration and driver callbacks.
 * @param[out] sensor_report Pointer to store the resulting temperature in Celsius.
 * @return kernel_error_st Error code indicating success or failure.
 */
kernel_error_st temperature_sensor_read(sensor_interface_st *ctx, sensor_report_st *sensor_report);