// device_info.c
#include "device_info.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include <string.h>

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
