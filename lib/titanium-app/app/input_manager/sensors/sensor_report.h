#pragma once

#include "app/input_manager/sensor_types.h"

// /**
//  * @enum sensor_type_et
//  * @brief Enumerates the types of supported sensors.
//  *
//  * Used to categorize sensors by their measurement domain. This type is also
//  * embedded in sensor reports to indicate how the raw value should be interpreted.
//  */
// typedef enum sensor_type_e {
//     SENSOR_TYPE_TEMPERATURE = 0, /**< Temperature sensor (e.g., NTC thermistor) */
//     SENSOR_TYPE_PRESSURE,        /**< Pressure sensor */
//     SENSOR_TYPE_VOLTAGE,         /**< Direct voltage measurement */
//     SENSOR_TYPE_CURRENT,         /**< Current measurement */
//     SENSOR_TYPE_POWER,           /**< Power measurement */
//     SENSOR_TYPE_POWER_FACTOR,    /**< Power factor measurement */
//     SENSOR_TYPE_UNDEFINED,       /**< Unknown or unsupported sensor */
// } sensor_type_et;

// /**
//  * @brief Structure representing a single sensor's report.
//  */
// typedef struct sensor_report_s {
//     float value;                /**< Measured value from the sensor */
//     bool active;                /**< Indicates whether the sensor is currently active */
//     sensor_type_et sensor_type; /**< Indicates the sensor type */
// } sensor_report_st;