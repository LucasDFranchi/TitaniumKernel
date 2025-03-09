#ifndef LOGGER_H
#define LOGGER_H

#include "error/error_num.h"
#include "inter_task_communication/events/events_definition.h"

/**
 * @file logger.h
 * @brief Interface for logging functionality.
 *
 * This header defines the interface for initializing the logger and
 * sending log messages with different log levels (INFO, WARN, ERR, DEBUG).
 * The logger supports both serial and UDP logging, and the implementation
 * ensures that the appropriate logging method is used based on network status.
 */


/**
 * @enum log_output_e
 * @brief Enumeration of log output channels.
 *
 * Defines the available output channels for log messages:
 * - SERIAL: Logs sent through the serial port.
 * - UDP: Logs transmitted over the network to a PaperTrail server.
 */
typedef enum log_output_e {
    SERIAL = 0,  /**< Output log messages to the serial console. */
    UDP,         /**< Output log messages to a UDP server. */
} log_output_et;

/**
 * @enum log_level_e
 * @brief Enumeration of log levels.
 *
 * Defines the different log levels that can be used to categorize log messages.
 * - INFO: General informational messages.
 * - WARN: Warning messages indicating potential issues.
 * - ERR: Error messages indicating failure or critical issues.
 * - DEBUG: Detailed debug messages for troubleshooting.
 */
typedef enum log_level_e {
    INFO = 0,   /**< Informational messages */
    WARN,       /**< Warning messages */
    ERR,        /**< Error messages */
    DEBUG,      /**< Debug messages */
} log_level_et;


kernel_error_st logger_initialize(log_output_et log_output, global_events_st* global_events);

/**
 * @brief Prints a log message with a specified log level.
 *
 * This function formats the log message and sends it to the appropriate
 * logging mechanism based on the log level (INFO, WARN, ERR, DEBUG).
 * The message will be routed through serial or UDP, depending on the network
 * initialization state and connectivity.
 *
 * @param log_level The severity level of the log message (INFO, WARN, ERR, DEBUG).
 * @param tag A tag identifying the source of the log message.
 * @param format A format string for the log message, similar to printf.
 * @param ... Additional arguments to be inserted into the format string.
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if invalid arguments are provided,
 *         ESP_ERR_INVALID_SIZE if the log message exceeds the size limit, or
 *         other error codes based on the underlying send function.
 */
esp_err_t logger_print(log_level_et log_level, const char* tag, const char* format, ...);

#endif  // LOGGER_H
