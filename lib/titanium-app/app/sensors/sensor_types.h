#pragma once

/**
 * @enum sensor_index_et
 * @brief Unique identifiers for all supported sensors.
 *
 * Each entry in this enumeration corresponds to a distinct sensor in the system.
 * Unlike channels, which represent physical or logical input lines, sensor IDs
 * provide a global index for addressing sensors individually.
 *
 * The mapping between sensors and channels is not necessarily 1:1. Multiple
 * sensors may share the same channel (e.g., multiple measurements on the same
 * ADC input), and conversely, some sensors may not be directly tied to a
 * channel at all (e.g., virtual or derived sensors).
 *
 * Use ::NUM_OF_SENSORS to determine the total number of sensors defined.
 */
typedef enum sensor_index_e {
    SENSOR_ID_00 = 0,
    SENSOR_ID_01,
    SENSOR_ID_02,
    SENSOR_ID_03,
    SENSOR_ID_04,
    SENSOR_ID_05,
    SENSOR_ID_06,
    SENSOR_ID_07,
    SENSOR_ID_08,
    SENSOR_ID_09,
    SENSOR_ID_10,
    SENSOR_ID_11,
    SENSOR_ID_12,
    SENSOR_ID_13,
    SENSOR_ID_14,
    SENSOR_ID_15,
    SENSOR_ID_16,
    SENSOR_ID_17,
    SENSOR_ID_18,
    SENSOR_ID_19,
    SENSOR_ID_20,
    SENSOR_ID_21,
    SENSOR_ID_22,
    SENSOR_ID_23,
    SENSOR_ID_24,
    SENSOR_ID_25,
    NUM_OF_SENSORS
} sensor_index_et;

/**
 * @enum sensor_channel_et
 * @brief Enumeration for logical sensor channels.
 *
 * A sensor channel represents a physical or logical input line that can be
 * connected to one or more sensors. Each channel may host multiple sensors,
 * meaning the relationship between sensors and channels is no longer 1:1.
 *
 * For example, SENSOR_CH_00 may correspond to a physical ADC channel,
 * while SENSOR_ID_00 and SENSOR_ID_01 could both be attached to it.
 *
 * Use ::NUM_OF_CHANNEL_SENSORS to determine the total number of defined channels.
 */
typedef enum sensor_channel_e {
    SENSOR_CH_00 = 0,
    SENSOR_CH_01,
    SENSOR_CH_02,
    SENSOR_CH_03,
    SENSOR_CH_04,
    SENSOR_CH_05,
    SENSOR_CH_06,
    SENSOR_CH_07,
    SENSOR_CH_08,
    SENSOR_CH_09,
    SENSOR_CH_10,
    SENSOR_CH_11,
    SENSOR_CH_12,
    SENSOR_CH_13,
    SENSOR_CH_14,
    SENSOR_CH_15,
    SENSOR_CH_16,
    SENSOR_CH_17,
    SENSOR_CH_18,
    SENSOR_CH_19,
    SENSOR_CH_20,
    SENSOR_CH_21,
    SENSOR_CH_22,
    NUM_OF_CHANNEL_SENSORS
} sensor_channel_et;

/**
 * @enum sensor_type_et
 * @brief Enumerates the types of supported sensors.
 *
 * Used to categorize sensors by their measurement domain. This type is also
 * embedded in sensor reports to indicate how the raw value should be interpreted.
 */
typedef enum sensor_type_e {
    SENSOR_TYPE_TEMPERATURE = 0, /**< Temperature sensor (e.g., NTC thermistor) */
    SENSOR_TYPE_PRESSURE,        /**< Pressure sensor */
    SENSOR_TYPE_VOLTAGE,         /**< Direct voltage measurement */
    SENSOR_TYPE_CURRENT,         /**< Current measurement */
    SENSOR_TYPE_POWER,           /**< Power measurement */
    SENSOR_TYPE_POWER_FACTOR,    /**< Power factor measurement */
    SENSOR_TYPE_UNDEFINED,       /**< Unknown or unsupported sensor */
} sensor_type_et;

/**
 * @brief Structure representing a single sensor's report.
 */
typedef struct sensor_report_s {
    float value;                /**< Measured value from the sensor */
    bool active;                /**< Indicates whether the sensor is currently active */
    sensor_type_et sensor_type; /**< Indicates the sensor type */
} sensor_report_st;