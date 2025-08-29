#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "app/sensor_manager/sensor_manager.h"

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

/* === Constants === */

/**
 * @def SYSTEM_ROOT_USER_SIZE
 * @brief Maximum length of the root user string (excluding null terminator).
 */
#define SYSTEM_ROOT_USER_SIZE 32

/**
 * @def SYSTEM_ROOT_PASSWORD_SIZE
 * @brief Maximum length of the root password string (excluding null terminator).
 */
#define SYSTEM_ROOT_PASSWORD_SIZE 32

/**
 * @def TIME_UTC_STRING_SIZE
 * @brief Size of the UTC time string in ISO 8601 format (including null terminator).
 */
#define TIME_UTC_STRING_SIZE 21

/**
 * @def DEVICE_ID_SIZE
 * @brief Size device id (including null terminator).
 */
#define DEVICE_ID_SIZE 13

/**
 * @def IP_ADDRESS_SIZE
 * @brief Size of the ip address string (including null terminator).
 */
#define IP_ADDRESS_SIZE 16

/* === Data Types === */

/* MQTT Topics Definition */
/**
 * @brief Enum listing the MQTT topic indexes used in the system.
 *
 * Each value corresponds to a specific type of MQTT topic
 * that the device publishes to or subscribes from. The
 * enumeration provides an easy way to index into arrays
 * of topic strings or handler functions.
 */
typedef enum mqtt_topic_index_e {
    SENSOR_REPORT = 0, /**< Topic for publishing sensor data reports. */
    BROADCAST_COMMAND, /**< Topic for receiving broadcast commands (affects multiple devices). */
    TARGET_COMMAND,    /**< Topic for receiving direct commands targeted at this device. */
    RESPONSE_COMMAND,  /**< Topic for publishing responses/acknowledgments to commands. */
    TOPIC_COUNT,       /**< Total number of defined topics (used for bounds checking). */
} mqtt_topic_index_et;

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
 * @enum command_index_et
 * @brief Enumerates all supported command types.
 */
typedef enum command_index_e {
    CMD_GET_TIME = 0,    /**< Request device time */
    CMD_SET_CALIBRATION, /**< Set calibration parameters for a sensor */
    CMD_GET_SYSTEM_INFO  /**< Request system information (user/password protected) */
    // Future commands can be added here
} command_index_et;

/**
 * @enum command_status_et
 * @brief Enumerates possible results of command execution.
 */
typedef enum command_status_e {
    COMMAND_SUCCESS             = 0,  /**< Command executed successfully */
    COMMAND_FAIL                = -1, /**< General command failure */
    COMMAND_CALIBRATION_FAIL    = -2, /**< Calibration-specific failure */
    COMMAND_AUTHENTICATION_FAIL = -3, /**< Calibration-specific failure */
} command_status_et;

typedef struct app_queues_s {
    QueueHandle_t sensor_report_queue;
    QueueHandle_t target_command_queue;
    QueueHandle_t broadcast_command_queue;
    QueueHandle_t command_response_queue;
} app_queues_st;

/**
 * @struct device_report_st
 * @brief Represents a device report containing sensor readings.
 *
 * Includes a timestamp and an array of sensor data for each active channel.
 */
typedef struct device_report_s {
    char timestamp[TIME_UTC_STRING_SIZE];     /**< Timestamp in ISO 8601 format (e.g., "2025-06-29T15:20:00") */
    sensor_report_st sensors[NUM_OF_SENSORS]; /**< Sensor readings per channel */
    uint8_t num_of_channels;                  /**< Number of active/valid channels in the report */
} device_report_st;

/* === Command Definitions === */

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
 * @struct cmd_get_system_info_st
 * @brief Payload for CMD_GET_SYSTEM_INFO.
 *
 * Used to authenticate system information requests with credentials.
 */
typedef struct cmd_get_system_info_s {
    char user[SYSTEM_ROOT_USER_SIZE];         /**< Root user string (null-terminated) */
    char password[SYSTEM_ROOT_PASSWORD_SIZE]; /**< Root password string (null-terminated) */
} cmd_get_system_info_st;

/**
 * @struct command_st
 * @brief Represents a targeted command issued to a device.
 *
 * Contains the command type and its associated payload. Used for
 * commands that affect a specific device or sensor.
 */
typedef struct target_command_s {
    command_index_et command_index; /**< Type of command */
    union {
        cmd_set_calibration_st set_calibration;     /**< Payload for CMD_SET_CALIBRATION */
        cmd_get_system_info_st cmd_get_system_info; /**< Payload for CMD_GET_SYSTEM_INFO */
        // Additional payloads for future targeted commands can be added here
    } command_u;
} command_st;

/* === Command Responses === */

/**
 * @struct cmd_sensor_response_st
 * @brief Response payload for sensor-specific commands.
 *
 * Contains the result of the command and the index/type of the affected sensor.
 */
typedef struct cmd_sensor_response_s {
    uint8_t sensor_index;       /**< Index of the sensor that responded */
    sensor_type_et sensor_type; /**< Type of the sensor that responded */
    float gain;                 /**< Gain factor used in calibration */
    float offset;               /**< Offset value used in calibration */
} cmd_sensor_response_st;

typedef struct sensor_calibration_status_s {
    uint8_t sensor_index;       /**< Index of the sensor that responded */
    sensor_type_et sensor_type; /**< Type of the sensor that responded */
    float gain;                 /**< Gain factor used in calibration */
    float offset;               /**< Offset value used in calibration */
    sensor_state_et state;      /**< Whether the sensor is active */
} sensor_calibration_status_st;

typedef struct cmd_system_info_response_s {
    char device_id[DEVICE_ID_SIZE];                                         /**< Execution result of the command */
    char ip_address[IP_ADDRESS_SIZE];                                       /**< Index of the sensor that responded */
    uint64_t uptime;                                                        /**< Type of the sensor that responded */
    sensor_calibration_status_st sensor_calibration_status[NUM_OF_SENSORS]; /**< Offset value used in calibration */
} cmd_system_info_response_st;

/**
 * @struct command_response_st
 * @brief Response returned after executing a command.
 *
 * Echoes the original command identifier and contains a union with the
 * corresponding payload based on the type of command executed.
 */
typedef struct command_response_s {
    command_index_et command_index;   /**< Original command identifier */
    command_status_et command_status; /**< Execution result of the command */
    union {
        cmd_sensor_response_st cmd_sensor_response; /**< Payload for sensor-level command responses */
        cmd_system_info_response_st cmd_system_info_response;
        // Additional response payloads for future commands can be added here
    } command_u;
} command_response_st;
