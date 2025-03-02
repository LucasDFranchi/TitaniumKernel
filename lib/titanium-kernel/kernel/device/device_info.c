#include "device_info.h"

#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"

static const char* TAG = "device_info";

static char device_id[DEVICE_ID_LENGTH] = {0};

/**
 * @brief Set the device ID string to "UNKNOWN".
 *
 * Writes "UNKNOWN" to the internal device_id buffer, ensuring it is null-terminated.
 */
static void device_info_set_unknown_id(void) {
    int ret = snprintf(device_id, DEVICE_ID_LENGTH, "UNKNOWN");
    if (ret < 0 || ret >= DEVICE_ID_LENGTH) {
        device_id[0] = '\0';
    }
}

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
kernel_error_st device_info_init(void) {
    uint8_t mac[6];
    esp_err_t err = esp_efuse_mac_get_default(mac);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get MAC address: %d", err);
        device_info_set_unknown_id();
        return KERNEL_ERROR_UNKNOWN_MAC;
    }

    int ret = snprintf(device_id, DEVICE_ID_LENGTH, "%02X%02X%02X%02X%02X%02X",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (ret < 0 || ret >= DEVICE_ID_LENGTH) {
        device_info_set_unknown_id();
        return KERNEL_ERROR_FORMATTING;
    }

    ESP_LOGI(TAG, "Device unique ID: %s", device_id);
    return KERNEL_ERROR_NONE;
}

/**
 * @brief Get the unique device ID string.
 *
 * Returns a pointer to a statically stored device ID string. The string
 * is guaranteed to be valid after successful initialization.
 *
 * @return Pointer to a null-terminated unique device ID string.
 */
const char* device_info_get_id(void) {
    return device_id;
}

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
kernel_error_st device_info_get_current_time(char* buffer, size_t buffer_size) {
    if (buffer == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (buffer_size == 0) {
        return KERNEL_ERROR_INVALID_SIZE;
    }

    time_t now = time(NULL);
    if (now == (time_t)(-1)) {
        return KERNEL_ERROR_INVALID_INTERFACE;
    }

    struct tm timeinfo;
    if (localtime_r(&now, &timeinfo) == NULL) {
        return KERNEL_ERROR_FAIL;
    }

    if (strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S", &timeinfo) == 0) {
        return KERNEL_ERROR_FORMATTING;
    }

    return KERNEL_ERROR_NONE;
}
