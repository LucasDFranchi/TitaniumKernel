#include "device_info.h"

#include <string.h>

#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "kernel/logger/logger.h"

static const char* TAG = "device_info";

static char device_id[DEVICE_ID_LENGTH]   = {0};
static char ip_address[IP_ADDRESS_LENGTH] = {0};

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
 * @return KERNEL_SUCCESS on success,
 *         KERNEL_ERROR_UNKNOWN_MAC if MAC retrieval fails,
 *         KERNEL_ERROR_FORMATTING if formatting fails.
 */
kernel_error_st device_info_init(void) {
    uint8_t mac[6];
    esp_err_t err = esp_efuse_mac_get_default(mac);

    if (err != ESP_OK) {
        logger_print(ERR, TAG, "Failed to get MAC address: %d", err);
        device_info_set_unknown_id();
        return KERNEL_ERROR_UNKNOWN_MAC;
    }

    int ret = snprintf(device_id, DEVICE_ID_LENGTH, "%02X%02X%02X%02X%02X%02X",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (ret < 0 || ret >= DEVICE_ID_LENGTH) {
        device_info_set_unknown_id();
        return KERNEL_ERROR_FORMATTING;
    }

    logger_print(INFO, TAG, "Device unique ID: %s", device_id);
    printf("Device unique ID: %s\n", device_id);
    return KERNEL_SUCCESS;
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
kernel_error_st device_info_get_current_time(time_t* timestamp) {
    if (timestamp == NULL) {
        return KERNEL_ERROR_NULL;
    }

    time_t now = time(NULL);
    if (now == (time_t)(-1)) {
        return KERNEL_ERROR_INVALID_INTERFACE;
    }

    *timestamp = now;
    return KERNEL_SUCCESS;
}

/**
 * @brief Get the device uptime in milliseconds.
 *
 * This function uses the ESP-IDF high-resolution timer to calculate
 * the system uptime since boot.
 *
 * @return int64_t Uptime in milliseconds.
 */
int64_t device_info_get_uptime(void) {
    return esp_timer_get_time() / 1000;
}

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
kernel_error_st device_info_set_ip_address(const esp_ip4_addr_t ip) {
    if (esp_ip4addr_ntoa(&ip, ip_address, sizeof(ip_address)) == NULL) {
        return KERNEL_ERROR_INVALID_SIZE;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Get the stored device identifier.
 *
 * Currently, the identifier is represented as the stored IP address string.
 *
 * @return const char* Pointer to the stored IP string (null-terminated).
 *         The pointer remains valid until the next call to device_info_set_ip_address().
 */
const char* device_info_get_ip_address(void) {
    return ip_address;
}

const region_fit_t* device_info_get_region_cal(void) {
    /* Device-specific calibration C0CDD6CD7850*/
    static const region_fit_t regions_device_C0CDD6CD7850[REGION_COUNT] = {
        /* Region 1: 2671.655 -> 533.244 kΩ */
        {2671.655f, 533.244f, 1.182307e-04f, 9.069289e-01f, 3.307210e+01f},

        /* Region 2: 325.154 -> 99.456 kΩ */
        {325.154f, 99.456f, 1.688087e-04f, 9.494021e-01f, 3.437045e+00f},

        /* Region 3: 84.261 -> 22.239 kΩ */
        {84.261f, 22.239f, 1.410034e-03f, 8.948450e-01f, 1.363572e+00f},

        /* Region 4: 17.938 -> 6.912 kΩ */
        {17.938f, 6.912f, 4.981085e-04f, 9.819512e-01f, -1.735783e-02f},

        /* Region 5: 4.736 -> 2.266 kΩ */
        {4.736f, 2.266f, 1.379979e-02f, 8.946382e-01f, 9.321247e-02f},
    };

    /* Device-specific calibration C0CDD6CD7814 */
    static const region_fit_t regions_device_C0CDD6CD7814[REGION_COUNT] = {
        /* Region 1: 2820.351 -> 540.543 kΩ */
        {2820.351f, 540.543f, 6.638856e-05f, 9.846284e-01f, -4.825885e+00f},

        /* Region 2: 327.598 -> 99.560 kΩ */
        {327.598f, 99.560f, 1.390894e-04f, 9.510418e-01f, 3.496762e+00f},

        /* Region 3: 84.190 -> 22.072 kΩ */
        {84.190f, 22.072f, 1.468559e-03f, 8.867961e-01f, 1.696872e+00f},

        /* Region 4: 17.719 -> 6.727 kΩ */
        {17.719f, 6.727f, 2.470669e-03f, 9.362600e-01f, 3.908942e-01f},

        /* Region 5: 4.533 -> 2.086 kΩ (lowest region: include lower bound) */
        {4.533f, 2.086f, 2.783823e-03f, 9.787391e-01f, 1.446947e-01f},
    };

    /* Device-specific calibration C0CDD6CD7838*/
    static const region_fit_t regions_device_C0CDD6CD7838[REGION_COUNT] = {
        /* Region 1: 2671.655 -> 533.244 kΩ */
        {2671.655f, 533.244f, 1.182307e-04f, 9.069289e-01f, 3.307210e+01f},

        /* Region 2: 325.154 -> 99.456 kΩ */
        {325.154f, 99.456f, 1.688087e-04f, 9.494021e-01f, 3.437045e+00f},

        /* Region 3: 84.261 -> 22.239 kΩ */
        {84.261f, 22.239f, 1.410034e-03f, 8.948450e-01f, 1.363572e+00f},

        /* Region 4: 17.938 -> 6.912 kΩ */
        {17.938f, 6.912f, 4.981085e-04f, 9.819512e-01f, -1.735783e-02f},

        /* Region 5: 4.736 -> 2.266 kΩ */
        {4.736f, 2.266f, 1.379979e-02f, 8.946382e-01f, 9.321247e-02f},
    };

    /* Device-specific calibration C0CDD6CD7828*/
    static const region_fit_t regions_device_C0CDD6CD7828[REGION_COUNT] = {
        /* Region 1: 2671.655 -> 533.244 kΩ */
        {2671.655f, 533.244f, 1.182307e-04f, 9.069289e-01f, 3.307210e+01f},

        /* Region 2: 325.154 -> 99.456 kΩ */
        {325.154f, 99.456f, 1.688087e-04f, 9.494021e-01f, 3.437045e+00f},

        /* Region 3: 84.261 -> 22.239 kΩ */
        {84.261f, 22.239f, 1.410034e-03f, 8.948450e-01f, 1.363572e+00f},

        /* Region 4: 17.938 -> 6.912 kΩ */
        {17.938f, 6.912f, 4.981085e-04f, 9.819512e-01f, -1.735783e-02f},

        /* Region 5: 4.736 -> 2.266 kΩ */
        {4.736f, 2.266f, 1.379979e-02f, 8.946382e-01f, 9.321247e-02f},
    };

    /* Device-specific calibration 1C69209DB778*/
    static const region_fit_t regions_device_1C69209DB778[REGION_COUNT] = {
        /* Region 1: 2671.655 -> 533.244 kΩ */
        {2671.655f, 533.244f, 1.182307e-04f, 9.069289e-01f, 3.307210e+01f},

        /* Region 2: 325.154 -> 99.456 kΩ */
        {325.154f, 99.456f, 1.688087e-04f, 9.494021e-01f, 3.437045e+00f},

        /* Region 3: 84.261 -> 22.239 kΩ */
        {84.261f, 22.239f, 1.410034e-03f, 8.948450e-01f, 1.363572e+00f},

        /* Region 4: 17.938 -> 6.912 kΩ */
        {17.938f, 6.912f, 4.981085e-04f, 9.819512e-01f, -1.735783e-02f},

        /* Region 5: 4.736 -> 2.266 kΩ */
        {4.736f, 2.266f, 1.379979e-02f, 8.946382e-01f, 9.321247e-02f},
    };

    /* Default calibration */
    static const region_fit_t regions_default[REGION_COUNT] = {
        /* Region 1: 3361.887 -> 329.300 kΩ */
        {3361.887f, 329.300f, 3.050603e-06f, 9.680608e-01f, 1.101766e+01f},

        /* Region 2: 329.300 -> 87.474 kΩ */
        {329.300f, 87.474f, 3.750742e-04f, 8.410913e-01f, 1.230265e+01f},

        /* Region 3: 87.474 -> 22.259 kΩ */
        {87.474f, 22.259f, -4.009059e-05f, 9.984124e-01f, -2.474721e-01f},

        /* Region 4: 22.259 -> 6.731 kΩ */
        {22.259f, 6.731f, -3.474550e-04f, 1.032403e+00f, -1.619189e-01f},

        /* Region 5: 6.731 -> 2.232 kΩ */
        {6.731f, 2.232f, -2.576672e-03f, 1.038778e+00f, -1.142167e-01f},
    };

    if (strcmp(device_id, "C0CDD6CD7850") == 0) {
        return regions_device_C0CDD6CD7850;
    } else if (strcmp(device_id, "C0CDD6CD7814") == 0) {
        return regions_device_C0CDD6CD7814;
    } else if (strcmp(device_id, "C0CDD6CD7838") == 0) {
        return regions_device_C0CDD6CD7838;
    } else if (strcmp(device_id, "C0CDD6CD7828") == 0) {
        return regions_device_C0CDD6CD7828;
    } else if (strcmp(device_id, "1C69209DB778") == 0) {
        return regions_device_1C69209DB778;
    }

    return regions_default;
}