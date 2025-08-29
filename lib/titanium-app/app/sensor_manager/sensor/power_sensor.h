/**
 * @file power_sensor.h
 * @brief Public interface for the PZEM power sensor driver.
 *
 * Provides a high-level API to query voltage, current, power,
 * and power factor measurements from a PZEM energy meter over Modbus RTU.
 *
 * @note This driver uses UART2 by default and assumes a single Modbus slave (address 0x01).
 */
#pragma once

#include "app/sensor_manager/sensor_interface/sensor_interface.h"
/**
 * @brief Reads voltage, current, power, and power factor from the PZEM power sensor.
 *
 * This is the main public entry point for the power sensor driver. It initializes
 * the UART interface if needed, sends a Modbus request, waits for a response, and
 * fills the provided sensor report array.
 *
 * Example usage:
 * @code
 * sensor_report_st report[4];
 * sensor_interface_st ctx = {.index = 0};
 * if (power_sensor_read(&ctx, report) == KERNEL_ERROR_NONE) {
 *     printf("Voltage: %.2f V\n", report[0].value);
 *     printf("Current: %.3f A\n", report[1].value);
 * }
 * @endcode
 *
 * @param[in]  ctx           Pointer to the sensor interface context (defines index offset).
 * @param[out] sensor_report Array where measurement values will be stored.
 *
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_NULL if ctx or sensor_report is NULL
 *         - KERNEL_ERROR_UART_NOT_INITIALIZED if UART interface is unavailable
 *         - KERNEL_ERROR_FAILED_TO_ENCODE_PACKET if Modbus request failed
 *         - KERNEL_ERROR_FAIL if UART transmission failed
 *         - KERNEL_ERROR_TIMEOUT if no response received
 *         - KERNEL_ERROR_FAILED_TO_DECODE_PACKET if Modbus response parsing failed
 */
kernel_error_st power_sensor_read(sensor_interface_st *ctx, sensor_report_st *sensor_report);