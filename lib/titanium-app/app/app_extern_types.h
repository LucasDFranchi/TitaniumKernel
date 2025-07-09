#ifndef APPLICATION_EXTERNAL_TYPES_H
#define APPLICATION_EXTERNAL_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "app/sensor/sensor.h"

//
// This file needs to be auto-generated based on some external JSON schema, present in the API repo
//

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
 * @enum app_data_type_e
 * @brief Enumerates the types of application data.
 *
 * Used to differentiate between different data payloads such as reports or commands.
 */
typedef enum app_data_type_e {
    DATA_TYPE_REPORT = 0, /**< Device report data type */
    DATA_TYPE_COMMAND,    /**< Command data type */
    END_OF_DATA_TYPES     /**< Marker for the end of data types */
} app_data_type_e;

/**
 * @struct device_report_s
 * @brief Represents a device report containing sensor readings.
 *
 * Contains a timestamp and an array of sensor reports for each channel.
 */
typedef struct device_report_s {
    char timestamp[20];                        /**< Timestamp of the report in ISO 8601 format (e.g., "2025-06-29T15:20:00") */
    sensor_report_st sensors[NUM_OF_CHANNELS]; /**< Array of sensor reports for each channel */
    uint8_t num_of_channels;                   /**< Number of valid sensor channels in the report */
} device_report_st;

/* Command List */

/**
 * @enum command_index_e
 * @brief Enumerates supported command types.
 *
 * Used to identify the command payload contained in the `command_st` union.
 */
typedef enum command_index_e {
    CMD_GET_TIME = 0,   /**< Command to request the current time */
    CMD_SET_CALIBRATION /**< Command to set sensor calibration parameters */
} command_index_et;

/**
 * @struct cmd_set_calibration_s
 * @brief Payload structure for the calibration command.
 *
 * Contains sensor index and calibration parameters gain and offset.
 */
typedef struct cmd_set_calibration_s {
    uint32_t sensor_index; /**< Index of the sensor to calibrate */
    float gain;            /**< Gain calibration factor */
    float offset;          /**< Offset calibration factor */
} cmd_set_calibration_st;

/**
 * @struct command_s
 * @brief Generic command structure with type-discriminated payload.
 *
 * Contains a command index indicating the type of command and a union
 * holding the associated payload for that command.
 */
typedef struct command_s {
    command_index_et command_index; /**< Command identifier */
    union {
        cmd_set_calibration_st set_calibration; /**< Calibration command payload */
        // Future commands can be added here as union members
    } command_u;
} command_st;

typedef enum command_status_e {
    COMMAND_SUCCESS = 0,            /**< Command executed successfully */
    COMMAND_FAIL = -1,              /**< General failure of the command */
    COMMAND_CALIBRATION_FAIL = -2,  /**< Failure specific to calibration commands */
} command_status_et;


/**
 * @struct command_response
 * @brief Represents a response to a command sent to the device.
 *
 * Contains the original command identifier, the status/result of the command,
 * and the sensor index related to the command. If the command applies to all sensors
 * (a broadcast), the sensor_index is set to 0.
 */
typedef struct command_response {
    command_index_et command_index;   /**< Command identifier */
    command_status_et command_status; /**< Status/result of the command execution */
    uint32_t sensor_index;            /**< Index of the sensor related to the command; 0 for broadcast commands */
} command_response_st;

#endif /* APPLICATION_EXTERNAL_TYPES_H */
