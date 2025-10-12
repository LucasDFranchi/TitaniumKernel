#pragma once

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "esp_netif.h"

#include "kernel/error/error_num.h"

#define DEVICE_ID_LENGTH 13   // 12 hex digits + '\0'
#define IP_ADDRESS_LENGTH 16  // enough for "255.255.255.255\0"

/**
 * @brief Initialize the device info module.
 *
 * Retrieves the ESP32 MAC address, formats it as a unique device ID string,
 * and stores it internally. Must be called once before any calls to
 * device_info_get_id().
 *
 * @return KERNEL_SUCCESS on success,
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
 * @brief Get the current Unix timestamp.
 *
 * This function retrieves the current system time as a Unix timestamp
 * (seconds since 1970-01-01 00:00:00 UTC).
 *
 * @param[out] timestamp Pointer to store the current timestamp.
 *
 * @return KERNEL_SUCCESS on success, or an appropriate error code on failure:
 *         - KERNEL_ERROR_NULL if the timestamp pointer is NULL.
 *         - KERNEL_ERROR_INVALID_INTERFACE if the system time is not set.
 */
kernel_error_st device_info_get_current_time(time_t* timestamp);

/**
 * @brief Get the device uptime in milliseconds.
 *
 * This function uses the ESP-IDF high-resolution timer to calculate
 * the system uptime since boot.
 *
 * @return int64_t Uptime in milliseconds.
 */
int64_t device_info_get_uptime(void);

/**
 * @brief Set the device IP address.
 *
 * Converts the given IP address into a human-readable string and stores it
 * internally for later retrieval. The stored value persists until the next call.
 *
 * @param[in] ip The IPv4 address to store (type: esp_ip4_addr_t).
 *
 * @return kernel_error_st
 *         - KERNEL_SUCCESS if the IP was successfully stored.
 *         - KERNEL_ERROR_INVALID_ARG if conversion failed (buffer too small).
 */
kernel_error_st device_info_set_ip_address(const esp_ip4_addr_t ip);

/**
 * @brief Get the stored device identifier.
 *
 * Currently, the identifier is represented as the stored IP address string.
 *
 * @return const char* Pointer to the stored IP string (null-terminated).
 *         The pointer remains valid until the next call to device_info_set_ip_address().
 */
const char* device_info_get_ip_address(void);
