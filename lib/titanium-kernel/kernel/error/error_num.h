/**
 * @file error_enum.h
 * @brief Defines standard error codes used throughout the kernel.
 *
 * This header file contains a set of error codes represented as an enum.
 * These codes provide a standardized way to handle and report errors
 * across different kernel modules.
 *
 * Usage Example:
 * @code
 * kernel_error_t err = some_function();
 * if (err != KERNEL_ERROR_NONE) {
 *     printf("Error: %d\n", err);
 * }
 * @endcode
 *
 * @note Ensure that functions returning these error codes follow
 *       the convention where `KERNEL_ERROR_NONE` (0) indicates success.
 */

#ifndef ERROR_ENUM_H
#define ERROR_ENUM_H

/**
 * @enum kernel_error_st
 * @brief Standard error codes for the kernel.
 */
typedef enum kernel_error_s {
    KERNEL_ERROR_NONE                        = 0,   /**< No error, operation successful */
    KERNEL_ERROR_NULL                        = -1,  /**< Null pointer error */
    KERNEL_ERROR_INVALID_ARG                 = -2,  /**< Invalid argument passed */
    KERNEL_ERROR_INVALID_SIZE                = -3,  /**< Invalid size parameter */
    KERNEL_ERROR_NOT_FOUND                   = -4,  /**< Requested item not found */
    KERNEL_ERROR_FAIL                        = -5,  /**< General failure */
    KERNEL_ERROR_NO_MEM                      = -6,  /**< Memory allocation failure */
    KERNEL_ERROR_TASK_FULL                   = -7,  /**< Task queue is full */
    KERNEL_ERROR_TASK_INIT                   = -8,  /**< Task queue is not initialized */
    KERNEL_ERROR_QUEUE_NULL                  = -9,  /**< Queue is NULL */
    KERNEL_ERROR_QUEUE_FULL                  = -10, /**< Queue is full */
    KERNEL_ERROR_INVALID_INTERFACE           = -11, /**< Task queue is not initialized */
    KERNEL_ERROR_FORMATTING                  = -12, /**< Error in snprintf function */
    KERNEL_ERROR_MQTT_PUBLISH                = -13, /**< Error in MQTT publish */
    KERNEL_ERROR_MQTT_SUBSCRIBE              = -14, /**< Error in MQTT subscribe */
    KERNEL_ERROR_MQTT_REGISTER_FAIL          = -15, /**< Error in MQTT registration */
    KERNEL_ERROR_MQTT_URI_FAIL               = -16, /**< Error in MQTT URI */
    KERNEL_ERROR_MQTT_CONFIG_FAIL            = -17, /**< Error in MQTT configuration */
    KERNEL_ERROR_TASK_CREATE                 = -18, /**< Error in task creation */
    KERNEL_ERROR_NVS_INIT                    = -19, /**< Error in NVS initialization */
    KERNEL_ERROR_INITIALIZATION_FAIL         = -20, /**< Initialization failure */
    KERNEL_ERROR_MUTEX_INIT_FAIL             = -21, /**< Mutex initialization failure */
    KERNEL_ERROR_SOCK_CREATE_FAIL            = -22, /**< Socket creation failure */
    KERNEL_ERROR_GLOBAL_EVENTS_INIT          = -23, /**< Global events initialization failure */
    KERNEL_ERROR_GLOBAL_QUEUES_INIT          = -24, /**< Global queues initialization failure */
    KERNEL_ERROR_EMPTY_SSID                  = -25, /**< Empty SSID error */
    KERNEL_ERROR_SSID_TOO_LONG               = -26, /**< SSID too long error */
    KERNEL_ERROR_EMPTY_PASSWORD              = -27, /**< Empty password error */
    KERNEL_ERROR_PASSWORD_TOO_LONG           = -28, /**< Password too long error */
    KERNEL_ERROR_READING_SSID                = -29, /**< Error reading SSID */
    KERNEL_ERROR_READING_PASSWORD            = -30, /**< Error reading password */
    KERNEL_ERROR_RECEIVING_FORM_DATA         = -31, /**< Error receiving form data */
    KERNEL_ERROR_NO_OTA_PARTITION_FOUND      = -32, /**< No OTA partition found */
    KERNEL_ERROR_OTA_BEGIN_FAILED            = -33, /**< OTA begin failed */
    KERNEL_ERROR_NVS_OPEN                    = -34, /**< NVS open failed */
    KERNEL_ERROR_NVS_SAVE                    = -35, /**< NVS save failed */
    KERNEL_ERROR_NVS_LOAD                    = -36, /**< NVS load failed */
    KERNEL_ERROR_NVS_ERASE_KEY               = -37, /**< NVS erase key failed */
    KERNEL_ERROR_NVS_ERASE_ALL               = -38, /**< NVS erase all failed */
    KERNEL_ERROR_NVS_NOT_INITIALIZED         = -39, /**< NVS not initialized */
    KERNEL_ERROR_TIMESTAMP_FORMAT            = -40, /**< Timestamp format error */
    KERNEL_ERROR_EMPTY_MQTT_TOPIC            = -41, /**< Empty MQTT topic error */
    KERNEL_ERROR_NULL_MQTT_QUEUE             = -42, /**< Null MQTT queue error */
    KERNEL_ERROR_MQTT_ENQUEUE_FAIL           = -43, /**< MQTT enqueue failure */
    KERNEL_ERROR_EMPTY_QUEUE                 = -44, /**< Queue is empty */
    KERNEL_ERROR_MQTT_INVALID_QOS            = -45, /**< Invalid MQTT Quality of Service (QoS) level */
    KERNEL_ERROR_MQTT_INVALID_DATA_DIRECTION = -46, /**< Invalid MQTT data direction */
    KERNEL_ERROR_MQTT_INVALID_TOPIC          = -47, /**< Invalid MQTT topic */
    KERNEL_ERROR_MQTT_QUEUE_NULL             = -48, /**< MQTT queue is NULL */
    KERNEL_ERROR_MQTT_TOO_MANY_TOPICS        = -49, /**< Too many MQTT topics registered */
    KERNEL_ERROR_SERIALIZE_JSON              = -50, /**< Error serializing the struct */
    KERNEL_ERROR_DESERIALIZE_JSON            = -51, /**< Error deserializing the struct */
    KERNEL_ERROR_INVALID_INDEX               = -52, /**< */
    KERNEL_ERROR_UNKNOWN_MAC                 = -53, /**< */
    KERNEL_ERROR_UNSUPPORTED_TYPE            = -54, /**< */
} kernel_error_st;

#endif /* ERROR_ENUM_H */
