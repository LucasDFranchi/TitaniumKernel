#include "wifi_manager.h"

#include "string.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"
#include "kernel/utils/nvs_util.h"

/**
 * @brief Struct representing the current wifi connection status.
 */
typedef struct wifi_status_s {
    bool is_connect_ap;   ///< Indicates if the device is connected to the Access Point.
    bool is_connect_sta;  ///< Indicates if the device is connected to a Station.
} wifi_status_st;

/* ---------------------------------------------------------------------------
 *                         Default Wi-Fi Configuration
 * ---------------------------------------------------------------------------
 * These constants define the default parameters for the Wi-Fi Access Point
 * mode and connection handling. They can be overridden at runtime if needed.
 * ---------------------------------------------------------------------------
 */

static const char AP_SSID[]               = "IoCloud\0";        ///< Access Point SSID.
static const char AP_PASSWORD[]           = "root1234\0";       ///< Access Point password.
static const uint8_t AP_CHANNEL           = 1;                  ///< Access Point channel (1-14 depending on region).
static const uint8_t AP_VISIBILITY        = 0;                  ///< Access Point visibility (0: hidden, 1: visible).
static const uint8_t AP_MAX_CONNECTIONS   = 1;                  ///< Maximum number of connections to the Access Point.
static const uint8_t AP_BEACON_INTERVAL   = 100;                ///< Beacon interval in milliseconds.
static const char *AP_IP                  = "192.168.0.1";      ///< Access Point IP address.
static const char *AP_GW                  = "192.168.0.1";      ///< Access Point gateway address.
static const char *AP_NETMASK             = "255.255.255.0";    ///< Access Point netmask.
static const wifi_bandwidth_t AP_BW       = WIFI_BW_HT20;       ///< Access Point bandwidth configuration.
static const wifi_ps_type_t AP_POWER_SAVE = WIFI_PS_MIN_MODEM;  ///< Access Point power save mode.

/* Global Constants Definition */
static const char *TAG                      = "WiFi Manager";  ///< Tag for logging.
static const uint8_t MAX_RECONNECT_ATTEMPTS = 3;               ///< Maximum number of reconnection attempts

/* Global Variables Definition */
static esp_netif_t *esp_netif_sta          = {0};    ///< Pointer to the Station network interface.
static esp_netif_t *esp_netif_ap           = {0};    ///< Pointer to the Access Point network interface.
static wifi_config_t ap_config             = {0};    ///< Configuration structure for the Access Point.
static wifi_config_t sta_config            = {0};    ///< Configuration structure for the Station.
static credentials_st cred                 = {0};    ///< Credentials structure for storing Wi-Fi credentials.
static uint8_t connection_retry_counter    = 0;      ///< Counter for tracking connection retries.
static uint8_t is_retry_limit_exceeded     = 0;      ///< Flag to indicate that the limit was exceeded.
static bool is_credential_set              = false;  ///< Flag to indicate if credentials are set.
static wifi_status_st wifi_status          = {0};    ///<
static SemaphoreHandle_t wifi_status_mutex = NULL;   ///<

/**
 * @brief Configure and initialize the ESP32 Access Point (AP) mode.
 *
 * This function sets up the Wi-Fi access point with predefined parameters,
 * including SSID, password, channel, IP configuration, bandwidth, and power saving mode.
 *
 * It performs the following operations:
 *  - Copies and validates SSID and password strings.
 *  - Sets AP-specific parameters like channel, visibility, authentication mode, max connections, and beacon interval.
 *  - Configures static IP address, gateway, and netmask, disabling DHCP server before and re-enabling it afterwards.
 *  - Applies the Wi-Fi AP configuration, bandwidth, and power save settings.
 *
 * @note
 *  - The function expects that Wi-Fi driver and network interface for AP are already initialized.
 *  - SSID and password lengths are checked against buffer sizes to avoid truncation.
 *  - In case of errors during any configuration step, a specific kernel error code is returned.
 *
 * @return kernel_error_st
 *  - KERNEL_ERROR_AP_SSID_TOO_LONG if the SSID length exceeds buffer size.
 *  - KERNEL_ERROR_AP_PASSWORD_TOO_LONG if the password length exceeds buffer size.
 *  - KERNEL_ERROR_AP_SET_IP_INFO if setting the static IP info fails.
 *  - KERNEL_ERROR_AP_DHCPS_START if starting the DHCP server fails.
 *  - KERNEL_ERROR_AP_SET_CONFIG if applying Wi-Fi configuration fails.
 *  - KERNEL_ERROR_AP_SET_BANDWIDTH if setting bandwidth fails.
 *  - KERNEL_ERROR_AP_SET_PS if setting power save mode fails.
 *  - KERNEL_OK (or ESP_OK) on success.
 */
static kernel_error_st set_access_point_mode(void) {
    esp_err_t result = ESP_OK;

    size_t ssid_len = snprintf((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), "%s", AP_SSID);
    if (ssid_len >= sizeof(ap_config.ap.ssid)) {
        logger_print(WARN, TAG, "SSID truncated: original length %zu", ssid_len);
        return KERNEL_ERROR_AP_SSID_TOO_LONG;
    }
    size_t password_len = snprintf((char *)ap_config.ap.password, sizeof(ap_config.ap.password), "%s", AP_PASSWORD);
    if (password_len >= sizeof(ap_config.ap.password)) {
        logger_print(WARN, TAG, "Password truncated: original length %zu", password_len);
        return KERNEL_ERROR_AP_PASSWORD_TOO_LONG;
    }

    ap_config.ap.ssid_len        = strlen(AP_SSID);
    ap_config.ap.channel         = AP_CHANNEL;
    ap_config.ap.ssid_hidden     = AP_VISIBILITY;
    ap_config.ap.authmode        = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection  = AP_MAX_CONNECTIONS;
    ap_config.ap.beacon_interval = AP_BEACON_INTERVAL;

    esp_netif_ip_info_t ap_ip_info = {0};
    esp_netif_dhcps_stop(esp_netif_ap);
    inet_pton(AF_INET, AP_IP, &ap_ip_info.ip);
    inet_pton(AF_INET, AP_GW, &ap_ip_info.gw);
    inet_pton(AF_INET, AP_NETMASK, &ap_ip_info.netmask);

    result = esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set AP IP info: %d", result);
        return KERNEL_ERROR_AP_SET_IP_INFO;
    };

    result = esp_netif_dhcps_start(esp_netif_ap);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to start DHCP server on AP: %d", result);
        return KERNEL_ERROR_AP_DHCPS_START;
    }

    result = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set AP Wi-Fi config: %d", result);
        return KERNEL_ERROR_AP_SET_CONFIG;
    }

    result = esp_wifi_set_bandwidth(WIFI_IF_AP, AP_BW);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set AP bandwidth: %d", result);
        return KERNEL_ERROR_AP_SET_BANDWIDTH;
    }

    result = esp_wifi_set_ps(AP_POWER_SAVE);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set AP power save mode: %d", result);
        return KERNEL_ERROR_AP_SET_PS;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Set the Access Point connection status.
 *
 * This function safely updates the `is_connect_ap` flag indicating whether
 * the device's AP interface currently has connected clients. It uses a FreeRTOS
 * mutex to ensure thread-safe access to the shared status variable.
 *
 * If the mutex cannot be acquired within 100 ms, an error is logged and the
 * update is skipped.
 *
 * @param connected Boolean value indicating AP connection status (true = connected).
 */
static void wifi_status_set_ap(bool connected) {
    if (xSemaphoreTake(wifi_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        wifi_status.is_connect_ap = connected;
        xSemaphoreGive(wifi_status_mutex);
    } else {
        logger_print(ERR, TAG, "Failed to take wifi_status mutex in set_ap");
    }
}

/**
 * @brief Set the Station connection status.
 *
 * This function safely updates the `is_connect_sta` flag indicating whether
 * the device's station interface is currently connected to an AP. It uses a FreeRTOS
 * mutex to ensure thread-safe access to the shared status variable.
 *
 * If the mutex cannot be acquired within 100 ms, an error is logged and the
 * update is skipped.
 *
 * @param connected Boolean value indicating station connection status (true = connected).
 */
static void wifi_status_set_sta(bool connected) {
    if (xSemaphoreTake(wifi_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        wifi_status.is_connect_sta = connected;
        xSemaphoreGive(wifi_status_mutex);
    } else {
        logger_print(ERR, TAG, "Failed to take wifi_status mutex in set_sta");
    }
}

/**
 * @brief Get the current Access Point connection status.
 *
 * This function safely reads the `is_connect_ap` flag indicating whether
 * the device's AP interface currently has connected clients. It uses a FreeRTOS
 * mutex to ensure thread-safe access to the shared status variable.
 *
 * If the mutex cannot be acquired within 100 ms, an error is logged and
 * the function returns false as a safe default.
 *
 * @return true if AP has connected clients, false otherwise.
 */
bool wifi_status_get_ap(void) {
    bool connected = false;
    if (xSemaphoreTake(wifi_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        connected = wifi_status.is_connect_ap;
        xSemaphoreGive(wifi_status_mutex);
    } else {
        logger_print(ERR, TAG, "Failed to take wifi_status mutex in get_ap");
    }
    return connected;
}

/**
 * @brief Get the current Station connection status.
 *
 * This function safely reads the `is_connect_sta` flag indicating whether
 * the device's station interface is currently connected to an AP. It uses a FreeRTOS
 * mutex to ensure thread-safe access to the shared status variable.
 *
 * If the mutex cannot be acquired within 100 ms, an error is logged and
 * the function returns false as a safe default.
 *
 * @return true if station is connected, false otherwise.
 */
bool wifi_status_get_sta(void) {
    bool connected = false;
    if (xSemaphoreTake(wifi_status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        connected = wifi_status.is_connect_sta;
        xSemaphoreGive(wifi_status_mutex);
    } else {
        logger_print(ERR, TAG, "Failed to take wifi_status mutex in get_sta");
    }
    return connected;
}

/**
 * @brief Configure the Wi-Fi station interface with security and PMF settings.
 *
 * This function sets the Wi-Fi station configuration parameters including the
 * authentication mode (WPA2-PSK) and Protected Management Frames (PMF) capability.
 * It applies the configuration to the Wi-Fi driver and returns a kernel error
 * code indicating success or failure.
 *
 * @return kernel_error_st
 *  - KERNEL_ERROR_NONE if configuration applied successfully.
 *  - KERNEL_ERROR_STA_CONFIG if `esp_wifi_set_config` fails.
 */
static kernel_error_st set_station_mode(void) {
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_config.sta.pmf_cfg.capable    = true;
    sta_config.sta.pmf_cfg.required   = false;

    esp_err_t result = esp_wifi_set_config(WIFI_IF_STA, &sta_config);

    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set station config: %d", result);
        return KERNEL_ERROR_STA_CONFIG;
    }

    return KERNEL_ERROR_NONE;
}

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
 *  - KERNEL_ERROR_NONE if credentials are successfully stored and station mode configured.
 *  - KERNEL_ERROR_NULL if `ssid` or `password` is NULL.
 *  - KERNEL_ERROR_STA_SSID_TOO_LONG if SSID string exceeds buffer size.
 *  - KERNEL_ERROR_STA_PASSWORD_TOO_LONG if password string exceeds buffer size.
 *  - KERNEL_ERROR_STA_CREDENTIALS if applying station mode configuration fails.
 */
kernel_error_st wifi_manager_set_credentials(const char *ssid, const char *password) {
    if (ssid == NULL || password == NULL) {
        logger_print(ERR, TAG, "Invalid credentials: SSID or password is NULL");
        return KERNEL_ERROR_NULL;
    }

    char ssid_buf[sizeof(sta_config.sta.ssid)]         = {0};
    char password_buf[sizeof(sta_config.sta.password)] = {0};

    int ssid_len = snprintf(ssid_buf, sizeof(ssid_buf), "%s", ssid);
    if (ssid_len < 0 || ssid_len >= sizeof(ssid_buf)) {
        logger_print(WARN, TAG, "SSID truncated or formatting error: length %d", ssid_len);
        return KERNEL_ERROR_STA_SSID_TOO_LONG;
    }

    int password_len = snprintf(password_buf, sizeof(password_buf), "%s", password);
    if (password_len < 0 || password_len >= sizeof(password_buf)) {
        logger_print(WARN, TAG, "Password truncated or formatting error: length %d", password_len);
        return KERNEL_ERROR_STA_PASSWORD_TOO_LONG;
    }

    memcpy(sta_config.sta.ssid, ssid_buf, sizeof(ssid_buf));
    memcpy(sta_config.sta.password, password_buf, sizeof(password_buf));

    is_credential_set        = true;
    is_retry_limit_exceeded  = false;
    connection_retry_counter = 0;

    if (set_station_mode() != KERNEL_ERROR_NONE) {
        return KERNEL_ERROR_STA_CREDENTIALS;
    }

    return KERNEL_ERROR_NONE;
}

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
 *  - KERNEL_ERROR_NONE on successful initialization.
 */
kernel_error_st wifi_manager_initialize() {
    esp_err_t result = ESP_OK;

    wifi_status_mutex = xSemaphoreCreateMutex();
    if (wifi_status_mutex == NULL) {
        logger_print(ERR, TAG, "Failed to create wifi_status mutex");
        return KERNEL_ERROR_MUTEX_INIT_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    result                 = esp_wifi_init(&cfg);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to initialize Wi-Fi driver: %d", result);
        return KERNEL_ERROR_WIFI_INIT;
    }

    result = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set Wi-Fi storage: %d", result);
        return KERNEL_ERROR_WIFI_SET_STORAGE;
    }

    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap  = esp_netif_create_default_wifi_ap();

    if (!esp_netif_sta || !esp_netif_ap) {
        logger_print(ERR, TAG, "Failed to create default network interfaces (STA/AP)");
        return KERNEL_ERROR_WIFI_NETIF_CREATE;
    }

    result = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to set Wi-Fi mode to APSTA: %d", result);
        return KERNEL_ERROR_WIFI_SET_MODE;
    }

    kernel_error_st ap_result = set_access_point_mode();
    if (ap_result != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to configure access point mode: %d", ap_result);
        return ap_result;
    }

    result = esp_wifi_start();
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to start Wi-Fi driver: %d", result);
        return KERNEL_ERROR_WIFI_START;
    }

    kernel_error_st err_ssid = KERNEL_ERROR_NONE;
    kernel_error_st err_pwd  = KERNEL_ERROR_NONE;

    err_ssid = nvs_util_load_str("wifi", "ssid", cred.ssid, sizeof(cred.ssid));
    if (err_ssid != KERNEL_ERROR_NONE) {
        logger_print(WARN, TAG, "Failed to load SSID from NVS: %d", err_ssid);
    }

    err_pwd = nvs_util_load_str("wifi", "pwd", cred.password, sizeof(cred.password));
    if (err_pwd != KERNEL_ERROR_NONE) {
        logger_print(WARN, TAG, "Failed to load password from NVS: %d", err_pwd);
    }

    if (err_ssid == KERNEL_ERROR_NONE && err_pwd == KERNEL_ERROR_NONE) {
        logger_print(INFO, TAG, "Loaded credentials from NVS: SSID='%s'", cred.ssid);
        wifi_manager_set_credentials(cred.ssid, cred.password);
    } else {
        logger_print(INFO, TAG, "Credentials not fully available in NVS. Skipping set.");
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Handle Wi-Fi-related system events.
 *
 * This function processes events received from the Wi-Fi driver and
 * performs actions such as updating connection status and logging.
 *
 * @param[in] event_id    Identifier for the Wi-Fi event.
 * @param[in] event_data  Pointer to event-specific data (must be cast according to event_id).
 */
void wifi_manager_wifi_event_handler(int32_t event_id, void *event_data) {
    if (event_data == NULL) {
        logger_print(WARN, TAG, "Received event %d with NULL data", event_id);
        return;
    }

    switch (event_id) {
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t *evt = (wifi_event_ap_staconnected_t *)event_data;
            logger_print(INFO, TAG, "AP Client Connected - MAC: %02x:%02x:%02x:%02x:%02x:%02x, AID=%d",
                         evt->mac[0], evt->mac[1], evt->mac[2], evt->mac[3], evt->mac[4], evt->mac[5], evt->aid);
            wifi_status_set_ap(true);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t *evt = (wifi_event_ap_stadisconnected_t *)event_data;
            logger_print(INFO, TAG, "AP Client Disconnected - MAC: %02x:%02x:%02x:%02x:%02x:%02x, AID=%d",
                         evt->mac[0], evt->mac[1], evt->mac[2], evt->mac[3], evt->mac[4], evt->mac[5], evt->aid);
            wifi_status_set_ap(false);
            break;
        }
        case WIFI_EVENT_STA_START:
            logger_print(INFO, TAG, "Station Mode Started");
            break;
        case WIFI_EVENT_STA_CONNECTED: {
            wifi_event_sta_connected_t *evt = (wifi_event_sta_connected_t *)event_data;
            logger_print(INFO, TAG, "Station Connected to SSID '%s', BSSID %02x:%02x:%02x:%02x:%02x:%02x, Channel %d",
                         (char *)evt->ssid,
                         evt->bssid[0], evt->bssid[1], evt->bssid[2], evt->bssid[3], evt->bssid[4], evt->bssid[5],
                         evt->channel);
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *evt = (wifi_event_sta_disconnected_t *)event_data;
            logger_print(WARN, TAG, "Station Disconnected from SSID '%s', Reason: %d",
                         (char *)evt->ssid, evt->reason);
            wifi_status_set_sta(false);
            break;
        }
        default:
            logger_print(DEBUG, TAG, "Unhandled Wi-Fi event: %d", event_id);
            break;
    }
}

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
void wifi_manager_sta_got_ip(int32_t event_id, void *event_data) {
    if (event_id != IP_EVENT_STA_GOT_IP) {
        return;
    }

    if (event_data == NULL) {
        logger_print(ERR, TAG, "event_data is NULL in IP_EVENT_STA_GOT_IP");
        return;
    }

    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    logger_print(INFO, TAG, "Station acquired IP address: " IPSTR, IP2STR(&event->ip_info.ip));

    char stored_ssid[32]     = {0};
    char stored_password[64] = {0};

    bool ssid_changed = true;
    bool pwd_changed  = true;

    if (nvs_util_load_str("wifi", "ssid", stored_ssid, sizeof(stored_ssid)) == KERNEL_ERROR_NONE &&
        strcmp(stored_ssid, cred.ssid) == 0) {
        ssid_changed = false;
    }

    if (nvs_util_load_str("wifi", "pwd", stored_password, sizeof(stored_password)) == KERNEL_ERROR_NONE &&
        strcmp(stored_password, cred.password) == 0) {
        pwd_changed = false;
    }

    kernel_error_st result = KERNEL_ERROR_NONE;
    if (ssid_changed) {
        result = nvs_util_save_str("wifi", "ssid", cred.ssid);
        if (result != KERNEL_ERROR_NONE) {
            logger_print(WARN, TAG, "Failed to save SSID to NVS: %d", result);
        } else {
            logger_print(INFO, TAG, "SSID updated in NVS");
        }
    }

    if (pwd_changed) {
        result = nvs_util_save_str("wifi", "pwd", cred.password);
        if (result != KERNEL_ERROR_NONE) {
            logger_print(WARN, TAG, "Failed to save password to NVS: %d", result);
        } else {
            logger_print(INFO, TAG, "Password updated in NVS");
        }
    }

    if (!ssid_changed && !pwd_changed) {
        logger_print(DEBUG, TAG, "Credentials unchanged, NVS update skipped");
    }

    wifi_status_set_sta(true);
}

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
 */
void wifi_manager_manage_connection() {
    if (wifi_status_get_sta()) {
        connection_retry_counter = 0;
        return;
    }

    if (is_retry_limit_exceeded) {
        return;
    }

    if (connection_retry_counter < MAX_RECONNECT_ATTEMPTS) {
        logger_print(DEBUG, TAG, "Reconnecting to the STA (Attempt %d of %d)...",
                     connection_retry_counter + 1,
                     MAX_RECONNECT_ATTEMPTS);

        esp_err_t err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
        if (err == ESP_OK) {
            logger_print(DEBUG, TAG, "Connection attempt initiated.");
            connection_retry_counter++;
        } else {
            logger_print(ERR, TAG, "Reconnect attempt failed: %s", esp_err_to_name(err));
        }
    } else {
        logger_print(ERR, TAG, "Max reconnect attempts reached. Stopping further attempts.");
        is_credential_set       = false;
        is_retry_limit_exceeded = true;
    }
}

/**
 * @brief Get the current connection status of the specified Wi-Fi interface.
 *
 * This function returns the connection status for either the Access Point (AP)
 * or Station (STA) interface, based on the specified enum.
 *
 * @param[in] wifi_manager_interface The interface to query: WIFI_MANAGER_IF_AP or WIFI_MANAGER_IF_STA.
 *
 * @return true if the interface is connected, false otherwise.
 */
bool wifi_manager_get_connection_status(wifi_manager_interface_et wifi_manager_interface) {
    switch (wifi_manager_interface) {
        case WIFI_MANAGER_IF_AP:
            return wifi_status_get_ap();
        case WIFI_MANAGER_IF_STA:
            return wifi_status_get_sta();
        default:
            logger_print(ERR, TAG, "Invalid interface (expected AP or STA)");
    }

    return false;
}
