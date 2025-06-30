/**
 * @file error_enum.h
 * @brief Defines standard error codes used throughout the Application.
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

#ifndef APPLICATION_ERROR_ENUM_H
#define APPLICATION_ERROR_ENUM_H

/**
 * @enum kernel_error_st
 * @brief Standard error codes for the kernel.
 */
typedef enum app_error_s {
    APP_ERROR_NONE                    = 0,     /**< No error, operation successful */
    APP_ERROR_NULL                    = 0x101, /**< Null pointer error */
    APP_ERROR_TAKS_CREATION           = 0x102, /**< Null pointer error */
    APP_ERROR_ADC_CONVERSION_ERROR    = 0x103, /**< ADC conversion error */
    APP_ERROR_ADC_UPDATE_ERROR        = 0x104, /**< ADC update error */
    APP_ERROR_ADC_CONFIGURE_ERROR     = 0x105, /**< ADC update error */
    APP_ERROR_MUX_CHANNEL_ERROR       = 0x106, /**< MUX channel error */
    APP_ERROR_INVALID_MQTT_TOPIC_SIZE = 0x107, /**< Invalid MQTT topic size */
    APP_ERROR_SNPRINTF                = 0x108, /**< Error in snprintf function */
} app_error_st;

#endif /* APPLICATION_ERROR_ENUM_H */
