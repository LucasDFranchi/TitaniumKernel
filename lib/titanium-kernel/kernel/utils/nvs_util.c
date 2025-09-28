#include "nvs_util.h"
#include "nvs.h"
#include "nvs_flash.h"

static bool is_nvs_initialized = false;

/**
 * @brief Initialize the Non-Volatile Storage (NVS) subsystem.
 *
 * This function initializes the flash-based key-value storage system. It must be called
 * before any other NVS operations. Handles page errors and version upgrades automatically.
 *
 * @return KERNEL_SUCCESS on success,
 *         KERNEL_ERROR_NVS_INIT on failure.
 */
kernel_error_st nvs_util_init(void) {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        return KERNEL_ERROR_NVS_INIT;
    }

    is_nvs_initialized = true;

    return KERNEL_SUCCESS;
}

/**
 * @brief Save a null-terminated string into NVS.
 *
 * @param nvs_namespace  The NVS namespace.
 * @param key            The key under which to store the string.
 * @param value          The null-terminated string to store.
 *
 * @return KERNEL_SUCCESS on success,
 *         KERNEL_ERROR_NULL if any input is NULL,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_SAVE on failure.
 */
kernel_error_st nvs_util_save_str(const char *nvs_namespace, const char *key, const char *value) {
    nvs_handle_t handle;

    if (nvs_namespace == NULL || key == NULL || value == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (!is_nvs_initialized) {
        return KERNEL_ERROR_NVS_NOT_INITIALIZED;
    }

    esp_err_t result = nvs_open(nvs_namespace, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_OPEN;
    }

    result = nvs_set_str(handle, key, value);
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);

    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_SAVE;
    }

    return KERNEL_SUCCESS;
}

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
 * @return KERNEL_SUCCESS on success,
 *         KERNEL_ERROR_NULL or KERNEL_ERROR_INVALID_SIZE for invalid arguments,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_LOAD on failure.
 */
kernel_error_st nvs_util_load_str(const char *nvs_namespace, const char *key, char *out_value, size_t max_len) {
    nvs_handle_t handle;

    if (nvs_namespace == NULL || key == NULL || out_value == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (!is_nvs_initialized) {
        return KERNEL_ERROR_NVS_NOT_INITIALIZED;
    }

    if (max_len == 0) {
        return KERNEL_ERROR_INVALID_SIZE;
    }

    esp_err_t result = nvs_open(nvs_namespace, NVS_READONLY, &handle);
    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_OPEN;
    }

    size_t required_size = max_len;
    result               = nvs_get_str(handle, key, out_value, &required_size);

    if (required_size > max_len) {
        nvs_close(handle);
        return KERNEL_ERROR_INVALID_SIZE;
    }

    nvs_close(handle);

    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_LOAD;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Erase a single key from the NVS.
 *
 * @param nvs_namespace  The NVS namespace.
 * @param key            The key to erase.
 *
 * @return KERNEL_SUCCESS on success,
 *         KERNEL_ERROR_NULL for invalid arguments,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_ERASE_KEY on failure.
 */
kernel_error_st nvs_util_erase_key(const char *nvs_namespace, const char *key) {
    nvs_handle_t handle;

    if (nvs_namespace == NULL || key == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (!is_nvs_initialized) {
        return KERNEL_ERROR_NVS_NOT_INITIALIZED;
    }

    esp_err_t result = nvs_open(nvs_namespace, NVS_READWRITE, &handle);

    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_OPEN;
    }

    result = nvs_erase_key(handle, key);
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);

    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_ERASE_KEY;
    }

    return KERNEL_SUCCESS;
}


/**
 * @brief Erase all keys in a given NVS namespace.
 *
 * @param nvs_namespace  The NVS namespace to erase.
 *
 * @return KERNEL_SUCCESS on success,
 *         KERNEL_ERROR_NULL for invalid arguments,
 *         KERNEL_ERROR_NVS_NOT_INITIALIZED if NVS is not initialized,
 *         KERNEL_ERROR_NVS_OPEN or KERNEL_ERROR_NVS_ERASE_ALL on failure.
 */
kernel_error_st nvs_util_erase_all(const char *nvs_namespace) {
    nvs_handle_t handle;

    if (nvs_namespace == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (!is_nvs_initialized) {
        return KERNEL_ERROR_NVS_NOT_INITIALIZED;
    }

    esp_err_t result = nvs_open(nvs_namespace, NVS_READWRITE, &handle);

    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_OPEN;
    }

    result = nvs_erase_all(handle);
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);

    if (result != ESP_OK) {
        return KERNEL_ERROR_NVS_ERASE_ALL;
    }

    return KERNEL_SUCCESS;
}
