#pragma once
/**
 * @file converter.h
 * @brief Header file for all conversion functions related to sensors.
 *
 * This includes functions to convert raw sensor data such as voltage into
 * physical units like temperature, as well as lookup table structures for
 * interpolation-based conversions.
 */

/**
 * @brief Convert voltage output from thermistor divider to temperature.
 *
 * @param vout Voltage output from the sensor divider.
 * @param sensor_index Index of the sensor (for logging).
 * @return Interpolated temperature in Celsius.
 */
float voltage_to_temperature(float vout, int sensor_index);