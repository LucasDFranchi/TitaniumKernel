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
    KERNEL_ERROR_NONE                = 0,   /**< No error, operation successful */
    KERNEL_ERROR_NULL                = -1,  /**< Null pointer error */
    KERNEL_ERROR_INVALID_ARG         = -2,  /**< Invalid argument passed */
    KERNEL_ERROR_INVALID_SIZE        = -3,  /**< Invalid size parameter */
    KERNEL_ERROR_NOT_FOUND           = -4,  /**< Requested item not found */
    KERNEL_ERROR_FAIL                = -5,  /**< General failure */
    KERNEL_ERROR_NO_MEM              = -6,  /**< Memory allocation failure */
    KERNEL_ERROR_TASK_FULL           = -7,  /**< Task queue is full */
    KERNEL_ERROR_TASK_INIT           = -8,  /**< Task queue is not initialized */
    KERNEL_ERROR_QUEUE_NULL          = -9,  /**< Queue is NULL */
    KERNEL_ERROR_QUEUE_FULL          = -10, /**< Queue is full */
    KERNEL_ERROR_INVALID_INTERFACE   = -11, /**< Task queue is not initialized */
    KERNEL_ERROR_SNPRINTF            = -12, /**< Error in snprintf function */
    KERNEL_ERROR_MQTT_PUBLISH        = -13, /**< Error in MQTT publish */
    KERNEL_ERROR_MQTT_SUBSCRIBE      = -14, /**< Error in MQTT subscribe */
    KERNEL_ERROR_MQTT_REGISTER_FAIL  = -15, /**< Error in MQTT registration */
    KERNEL_ERROR_MQTT_URI_FAIL       = -16, /**< Error in MQTT URI */
    KERNEL_ERROR_MQTT_CONFIG_FAIL    = -17, /**< Error in MQTT configuration */
    KERNEL_ERROR_TASK_CREATE         = -18, /**< Error in task creation */
    KERNEL_ERROR_NVS_INIT            = -19, /**< Error in NVS initialization */
    KERNEL_ERROR_INITIALIZATION_FAIL = -20, /**< Initialization failure */
    KERNEL_ERROR_MUTEX_INIT_FAIL     = -21, /**< Mutex initialization failure */
    KERNEL_ERROR_SOCK_CREATE_FAIL    = -22, /**< Socket creation failure */
    KERNEL_ERROR_GLOBAL_EVENTS_INIT  = -23, /**< Global events initialization failure */
} kernel_error_st;

#endif /* ERROR_ENUM_H */
