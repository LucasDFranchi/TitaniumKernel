#ifndef NVS_UTIL_H
#define NVS_UTIL_H

#include <stddef.h>

#include "kernel/error/error_num.h"

/**
 * @brief Initialize the Non-Volatile Storage (NVS) subsystem.
 *
 * This function initializes the flash-based key-value storage system. It must be called
 * before any other NVS operations. Handles page errors and version upgrades automatically.
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_NVS_INIT on failure.
 */
kernel_error_st nvs_util_init(void);

/**
 * @brief Save a null-terminated string into NVS.
 *
 * @param nvs_namespace  The NVS namespace.
 * @param key            The key under which to store the string.
 * @param value          The null-terminated string to store.
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_NULL if any input is NULL,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_SAVE on failure.
 */
kernel_error_st nvs_util_save_str(const char *nvs_namespace, const char *key, const char *value);

/**
 * @brief Load a string from NVS into a user-provided buffer.
 *
 * The function first checks the required size and ensures the buffer is large enough.
 *
 * @param nvs_namespace  The NVS namespace.
 * @param key            The key of the stored string.
 * @param out_value      Buffer to store the string.
 * @param max_len        Maximum length of the buffer (including null terminator).
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_NULL or KERNEL_ERROR_INVALID_SIZE for invalid arguments,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_LOAD on failure.
 */
kernel_error_st nvs_util_load_str(const char *nvs_namespace, const char *key, char *out_value, size_t max_len);

/**
 * @brief Erase a single key from the NVS.
 *
 * @param nvs_namespace  The NVS namespace.
 * @param key            The key to erase.
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_NULL for invalid arguments,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_ERASE_KEY on failure.
 */
kernel_error_st nvs_util_erase_key(const char *nvs_namespace, const char *key);

/**
 * @brief Erase all keys in a given NVS namespace.
 *
 * @param nvs_namespace  The NVS namespace to erase.
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_NULL for invalid arguments,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_ERASE_ALL on failure.
 */
kernel_error_st nvs_util_erase_all(const char *nvs_namespace);

#endif  // NVS_UTIL_H
