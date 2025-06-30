#ifndef APPLICATION_EXTERNAL_TYPES_H
#define APPLICATION_EXTERNAL_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "app/sensor/sensor.h"

/**
 * @file app_external_types.h
 * @brief Definitions for commonly used data structures.
 *
 * This header file provides type definitions for data structures
 * used across the application. These structures are designed to
 * standardize the representation of various data elements, ensuring
 * consistency and facilitating communication between different
 * modules.
 */

/**
 * @brief Structure representing a device report containing multiple sensor readings.
 */
typedef struct device_report_s {
    char timestamp[20];                        /**< Timestamp of the report in ISO 8601 format (e.g., "2025-06-29T15:20:00") */
    sensor_report_st sensors[NUM_OF_CHANNELS]; /**< Array of sensor reports for each channel */
} device_report_st;

#endif /* APPLICATION_EXTERNAL_TYPES_H */
