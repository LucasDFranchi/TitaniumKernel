#ifndef APPLICATION_EXTERNAL_TYPES_H
#define APPLICATION_EXTERNAL_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "app/sensors/sensor_manager.h"

//
// NOTE: This file should be auto-generated from a JSON schema defined in the API repository.
//

/**
 * @file app_external_types.h
 * @brief Application-wide external type definitions.
 *
 * This header defines the common data structures and enumerations used across
 * different modules of the application. It standardizes representations for device
 * reports, commands, and command responses to ensure consistent communication
 * and processing.
 */

/**
 * @enum app_data_type_et
 * @brief Enumerates the types of application-level data.
 */
typedef enum app_data_type_e {
    DATA_TYPE_REPORT = 0,       /**< Sensor/device report */
    DATA_TYPE_COMMAND,          /**< Command sent to the device */
    DATA_TYPE_COMMAND_RESPONSE, /**< Response to a previously issued command */
    END_OF_DATA_TYPES           /**< End marker for enumeration */
} app_data_type_et;

/**
 * @struct device_report_st
 * @brief Represents a device report containing sensor readings.
 *
 * Includes a timestamp and an array of sensor data for each active channel.
 */
typedef struct device_report_s {
    char timestamp[21];                               /**< Timestamp in ISO 8601 format (e.g., "2025-06-29T15:20:00") */
    sensor_report_st sensors[NUM_OF_CHANNEL_SENSORS]; /**< Sensor readings per channel */
    uint8_t num_of_channels;                          /**< Number of active/valid channels in the report */
} device_report_st;

/* === Command Definitions === */

/**
 * @enum command_index_et
 * @brief Enumerates all supported command types.
 */
typedef enum command_index_e {
    CMD_GET_TIME = 0,   /**< Request device time */
    CMD_SET_CALIBRATION /**< Set calibration parameters for a sensor */
    // Future commands can be added here
} command_index_et;

/**
 * @enum command_status_et
 * @brief Enumerates possible results of command execution.
 */
typedef enum command_status_e {
    COMMAND_SUCCESS          = 0,  /**< Command executed successfully */
    COMMAND_FAIL             = -1, /**< General command failure */
    COMMAND_CALIBRATION_FAIL = -2  /**< Calibration-specific failure */
} command_status_et;

/**
 * @struct cmd_set_calibration_st
 * @brief Payload for CMD_SET_CALIBRATION.
 *
 * Contains the sensor index to calibrate, and the gain and offset parameters
 * used in the calibration process.
 */
typedef struct cmd_set_calibration_s {
    int32_t sensor_index; /**< Index of targeted sensor, or SENSOR_INDEX_BROADCAST (-1) for broadcast */
    float gain;           /**< Calibration gain factor */
    float offset;         /**< Calibration offset value */
} cmd_set_calibration_st;

/**
 * @struct command_st
 * @brief Represents a command issued to the device.
 *
 * Contains the command type and its associated payload.
 */
typedef struct command_s {
    command_index_et command_index; /**< Type of command */
    union {
        cmd_set_calibration_st set_calibration; /**< Payload for CMD_SET_CALIBRATION */
        // Additional payloads for future commands can be added here
    } command_u;
} command_st;

/**
 * @struct cmd_device_response_st
 * @brief Response payload for device-level commands (not sensor-specific).
 *
 * Used for general command results, such as CMD_GET_TIME.
 */
typedef struct cmd_device_response_s {
    command_status_et command_status; /**< Execution result of the command */
} cmd_device_response_st;

/**
 * @struct cmd_sensor_response_st
 * @brief Response payload for sensor-specific commands.
 *
 * Contains the result of the command and the index of the affected sensor.
 */
typedef struct cmd_sensor_response_s {
    command_status_et command_status; /**< Execution result of the command */
    uint8_t sensor_index;             /**< Index of the sensor that responded */
    sensor_type_et sensor_type;       /**< Type of the sensor that responded */
    float gain;                       /**< Gain factor used in calibration */
    float offset;                     /**< Offset value used in calibration */
} cmd_sensor_response_st;

/**
 * @struct command_response_st
 * @brief Response returned after executing a command.
 *
 * Echoes the original command identifier and contains a union with the
 * corresponding payload based on the type of command executed.
 */
typedef struct command_response_s {
    command_index_et command_index; /**< Original command identifier */
    union {
        cmd_device_response_st cmd_device_response; /**< Payload for device-level command responses */
        cmd_sensor_response_st cmd_sensor_response; /**< Payload for sensor-level command responses */
        // Additional response payloads for future commands can be added here
    } command_u;
} command_response_st;

#endif /* APPLICATION_EXTERNAL_TYPES_H */
