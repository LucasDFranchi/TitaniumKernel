#ifndef APPLICATION_EXTERNAL_TYPES_H
#define APPLICATION_EXTERNAL_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "app/sensor/sensor.h"

/**
 * @file application_external_types.h
 * @brief Definitions for commonly used data structures.
 *
 * This header file provides type definitions for data structures
 * used across the application. These structures are designed to
 * standardize the representation of various data elements, ensuring
 * consistency and facilitating communication between different
 * modules.
 */

// #define NUM_OF_CHANNELS (24) /*!< Number of channels for the TCA9548A multiplexer */

typedef enum data_struct_types_e {
    DATA_STRUCT_SENSOR_READ = 0,
    END_OF_DATA_STRUCT_TYPES,
} data_struct_types_et;

typedef struct sensor_response_s {
    struct sensor_array_s {
        sensor_type_et type;         /*!< Type of sensor */
        float raw_value;               /*!< Raw value from the sensor */
    } sensor_array[NUM_OF_CHANNELS]; /*!< Array of sensor values */
    int num_of_active_sensors;       /*!< Number of sensors in the array */
} sensor_response_st;

typedef struct data_info_s {
    data_struct_types_et type;
    uint32_t size;
} data_info_st;

#endif /* APPLICATION_EXTERNAL_TYPES_H */
