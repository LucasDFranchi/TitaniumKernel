/**
 * @file pressure_sensor.h
 * @brief Public interface for the pressure sensor driver.
 *
 * Provides the API to read pressure values from an analog sensor using
 * multiplexer and ADC controllers. Converts raw ADC readings into
 * calibrated pressure values and fills a sensor report entry.
 */

#pragma once

#include "app/sensor_manager/sensor_interface/sensor_interface.h"

/**
 * @brief Read pressure sensor data and populate the sensor report.
 *
 * This function performs the full read sequence for a pressure sensor:
 * - Validates input parameters.
 * - Selects the correct multiplexer channel for the sensor.
 * - Configures the ADC channel for measurement.
 * - Reads the raw ADC value from the sensor branch.
 * - Converts the raw ADC reading into a voltage, then into pressure (Pa).
 * - Applies calibration (gain and offset) from the sensor context.
 * - Updates the corresponding entry in the @p sensor_report array.
 *
 * @param[in]  ctx            Pointer to the sensor interface context.
 *                            Must provide valid MUX and ADC controller handles.
 * @param[out] sensor_report  Array of sensor reports to update. The entry at
 *                            @p ctx->index will be updated with the pressure data.
 *
 * @return kernel_error_st
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_NULL if @p ctx or @p sensor_report is NULL
 *         - KERNEL_ERROR_xxx if MUX selection, ADC configuration, or ADC read fails
 *
 * @note The measured pressure value is scaled by @p ctx->conversion_gain and shifted
 *       by @p ctx->offset to apply calibration.
 */
kernel_error_st pressure_sensor_read(sensor_interface_st *ctx, sensor_report_st *out_value);