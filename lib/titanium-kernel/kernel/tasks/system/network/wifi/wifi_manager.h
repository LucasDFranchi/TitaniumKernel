#pragma once

#include "stdbool.h"
#include "stdint.h"

#include "kernel/error/error_num.h"

/**
 * @brief Enumeration for selecting the Wi-Fi interface type.
 *
 * This enum is used to specify whether a function should operate on the
 * Access Point (AP) or Station (STA) interface within the Wi-Fi manager.
 *
 * It is useful for abstracting logic that applies to either interface, such
 * as checking connection status or toggling settings, without directly using
 * the ESP-IDF's `wifi_mode_t`, which includes broader combinations like `WIFI_MODE_APSTA`.
 */
typedef enum wifi_manager_interface_e {
    WIFI_MANAGER_IF_AP,
    WIFI_MANAGER_IF_STA
} wifi_manager_interface_et;

/**
 * @brief Initialize the Wi-Fi manager and configure the ESP32 Wi-Fi interfaces.
 *
 * This function initializes the Wi-Fi driver with default configurations, sets
 * Wi-Fi storage to RAM, creates the default network interfaces for station (STA)
 * and access point (AP) modes, sets the Wi-Fi mode to dual (AP + STA),
 * configures the access point settings, and finally starts the Wi-Fi driver.
 *
 * @return kernel_error_st
 *  - KERNEL_ERROR_WIFI_INIT if Wi-Fi driver initialization fails.
 *  - KERNEL_ERROR_WIFI_SET_STORAGE if setting Wi-Fi storage mode fails.
 *  - KERNEL_ERROR_WIFI_NETIF_CREATE if creation of default STA or AP network interface fails.
 *  - KERNEL_ERROR_WIFI_SET_MODE if setting Wi-Fi mode fails.
 *  - Errors returned by `set_access_point_mode()` if AP configuration fails.
 *  - KERNEL_ERROR_WIFI_START if starting the Wi-Fi driver fails.
 *  - KERNEL_SUCCESS on successful initialization.
 */
kernel_error_st wifi_manager_initialize();

/**
 * @brief Set Wi-Fi credentials for connecting in station mode.
 *
 * This function stores the provided SSID and password strings in the internal
 * station configuration buffers to be used for connecting the ESP32 to a Wi-Fi
 * network. It validates input pointers and ensures strings fit within buffer limits.
 *
 * To safely copy the strings, it uses intermediate buffers rather than casting away
 * the `const` qualifier of the input pointers. This avoids undefined behavior and
 * preserves const-correctness.
 *
 * @param[in] ssid     Null-terminated string representing the Wi-Fi SSID.
 * @param[in] password Null-terminated string representing the Wi-Fi password.
 *
 * @note This function does not attempt to connect to the network. It only sets credentials
 *       and configures the station mode. A connection should be triggered separately.
 *
 * @return kernel_error_st
 *  - KERNEL_SUCCESS if credentials are successfully stored and station mode configured.
 *  - KERNEL_ERROR_NULL if `ssid` or `password` is NULL.
 *  - KERNEL_ERROR_STA_SSID_TOO_LONG if SSID string exceeds buffer size.
 *  - KERNEL_ERROR_STA_PASSWORD_TOO_LONG if password string exceeds buffer size.
 *  - KERNEL_ERROR_STA_CREDENTIALS if applying station mode configuration fails.
 */
kernel_error_st wifi_manager_set_credentials(const char *ssid, const char *password);

/**
 * @brief Handle Wi-Fi-related system events.
 *
 * This function processes events received from the Wi-Fi driver and
 * performs actions such as updating connection status and logging.
 *
 * @param[in] event_id    Identifier for the Wi-Fi event.
 * @param[in] event_data  Pointer to event-specific data (must be cast according to event_id).
 */
void wifi_manager_wifi_event_handler(int32_t event_id, void *event_data);

/**
 * @brief Handle the IP_EVENT_STA_GOT_IP event.
 *
 * This function is triggered when the ESP32 station interface successfully
 * obtains an IP address from the connected access point. It logs the IP,
 * saves the credentials to NVS, and updates the internal Wi-Fi status.
 *
 * @param[in] event_id    Expected to be IP_EVENT_STA_GOT_IP.
 * @param[in] event_data  Pointer to ip_event_got_ip_t containing IP info.
 */
void wifi_manager_sta_got_ip(int32_t event_id, void *event_data);

/**
 * @brief Maintain and supervise Wi-Fi Station connection.
 *
 * This function is intended to be called periodically (e.g., in a loop).
 * It checks the current Wi-Fi connection status and attempts to reconnect
 * to the access point if disconnected and within the allowed retry limit.
 *
 * If already connected, it resets the retry counter. If the retry limit
 * has been exceeded, further attempts are skipped until credentials are
 * updated/reset externally.
 *
 */
void wifi_manager_manage_connection(void);

bool wifi_manager_get_connection_status(wifi_manager_interface_et wifi_manager_interface);
