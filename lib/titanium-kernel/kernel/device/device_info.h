#pragma once

#include <stddef.h>

#include "kernel/error/error_num.h"

#define DEVICE_ID_LENGTH 13  // 12 hex digits + '\0'

/**
 * @brief Initialize the device info module.
 *
 * Retrieves the ESP32 MAC address, formats it as a unique device ID string,
 * and stores it internally. Must be called once before any calls to
 * device_info_get_id().
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_UNKNOWN_MAC if MAC retrieval fails,
 *         KERNEL_ERROR_FORMATTING if formatting fails.
 */
kernel_error_st device_info_init(void);

/**
 * @brief Get the unique device ID string.
 *
 * Returns a pointer to the internal unique ID buffer.
 * The string is guaranteed to be null-terminated.
 *
 * @return Pointer to the unique ID string (length DEVICE_ID_LENGTH).
 */
const char* device_info_get_id(void);

/**
 * @brief Get the current timestamp in ISO 8601 format.
 *
 * This function retrieves the current system time and formats it
 * as an ISO 8601 string (e.g., "2024-12-24T15:30:45").
 *
 * @param[out] buffer      Pointer to the buffer where the formatted timestamp will be stored.
 * @param[in]  buffer_size Size of the buffer.
 *
 * @return ESP_OK on success, or an appropriate error code on failure:
 *         - ESP_ERR_INVALID_ARG if the buffer is NULL or the size is zero.
 *         - ESP_ERR_INVALID_STATE if the system time is not set.
 *         - ESP_FAIL if time formatting fails.
 */
kernel_error_st device_info_get_current_time(char* buffer, size_t buffer_size);
