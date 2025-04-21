/**
 * @file network.c
 * @brief Implementation of Wi-Fi management and network connectivity for ESP32.
 *
 * This file contains functions and configurations to manage Wi-Fi in both
 * Access Point (AP) and Station (STA) modes. It supports setting up an AP,
 * connecting to an external network as a STA, and handling network events.
 */
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "tasks/system/network/network_task.h"
#include "inter_task_communication/events/events_definition.h"
#include "logger/logger.h"

static const char *TAG                      = "Network Task";     ///< Tag for logging.
static const char AP_SSID[]                 = "Titanium\0";       ///< Access Point SSID.
static const char AP_PASSWORD[]             = "root1234\0";       ///< Access Point password.
static const uint8_t AP_CHANNEL             = 1;                  ///< Access Point channel (1-14 depending on region).
static const uint8_t AP_VISIBILITY          = 0;                  ///< Access Point visibility (0: hidden, 1: visible).
static const uint8_t AP_MAX_CONNECTIONS     = 1;                  ///< Maximum number of connections to the Access Point.
static const uint8_t AP_BEACON_INTERVAL     = 100;                ///< Beacon interval in milliseconds.
static const char *AP_IP                    = "192.168.0.1";      ///< Access Point IP address.
static const char *AP_GW                    = "192.168.0.1";      ///< Access Point gateway address.
static const char *AP_NETMASK               = "255.255.255.0";    ///< Access Point netmask.
static const wifi_bandwidth_t AP_BW         = WIFI_BW_HT20;       ///< Access Point bandwidth configuration.
static const wifi_ps_type_t AP_POWER_SAVE   = WIFI_PS_MIN_MODEM;  ///< Access Point power save mode.
static const uint8_t MAX_RECONNECT_ATTEMPTS = 3;                  ///< Maximum number of reconnection attempts
static const uint16_t RECONNECTION_DELAY_MS = 5000;               ///< Delay between reconnection attempts, in milliseconds

static network_status_st network_status = {
    .is_connect_ap  = false,  ///< Initial state: not connected to the Access Point.
    .is_connect_sta = false,  ///< Initial state: not connected to the Station.
};

// Global variables for connection handling
static uint8_t connection_retry_counter = 0;      ///< Counter for tracking connection retries.
static uint8_t is_retry_limit_exceeded  = 0;      ///< Flag to indicate that the limit was exceeded.
static bool is_credential_set           = false;  ///< Flag to indicate if credentials are set.
static esp_netif_t *esp_netif_sta       = {0};    ///< Pointer to the Station network interface.
static esp_netif_t *esp_netif_ap        = {0};    ///< Pointer to the Access Point network interface.
static wifi_config_t ap_config          = {0};    ///< Configuration structure for the Access Point.
static wifi_config_t sta_config         = {0};    ///< Configuration structure for the Station.

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_events_st *global_events = NULL;

/**
 * @brief Event handler for Wi-Fi-related events.
 *
 * Handles events for AP/STA connection and disconnection, as well as IP acquisition.
 *
 * @param[in] arg Pointer to user data (optional).
 * @param[in] event_base Event base.
 * @param[in] event_id Event ID.
 * @param[in] event_data Pointer to event data (event-specific).
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_STACONNECTED:
                logger_print(INFO, TAG, "WIFI_EVENT_AP_STACONNECTED");
                xEventGroupSetBits(global_events->firmware_event_group, WIFI_CONNECTED_AP);
                network_status.is_connect_ap = true;
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                logger_print(INFO, TAG, "WIFI_EVENT_AP_STADISCONNECTED");
                xEventGroupClearBits(global_events->firmware_event_group, WIFI_CONNECTED_AP);
                network_status.is_connect_ap = false;
                break;
            case WIFI_EVENT_STA_START:
                logger_print(INFO, TAG, "WIFI_EVENT_STA_START");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                logger_print(INFO, TAG, "WIFI_EVENT_STA_DISCONNECTED");
                xEventGroupClearBits(global_events->firmware_event_group, WIFI_CONNECTED_STA);
                network_status.is_connect_sta = false;
                break;
            case WIFI_EVENT_STA_CONNECTED:
                logger_print(INFO, TAG, "WIFI_EVENT_STA_CONNECTED");
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                logger_print(INFO, TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));

                xEventGroupSetBits(global_events->firmware_event_group, WIFI_CONNECTED_STA);
                network_status.is_connect_sta = true;
                break;
        }
    }
}

/**
 * @brief Configure and initialize Station (STA) mode.
 *
 * Sets up the Station mode with predefined configurations, such as security settings.
 */
static void set_station_mode(void) {
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_config.sta.pmf_cfg.capable    = true;
    sta_config.sta.pmf_cfg.required   = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
}

/**
 * @brief Configure and initialize Access Point (AP) mode.
 *
 * Configures AP settings like SSID, password, IP, and starts the AP.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t set_access_point_mode(void) {
    esp_err_t result = ESP_OK;

    size_t ssid_len = snprintf((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), "%s", AP_SSID);
    if (ssid_len >= sizeof(sta_config.ap.ssid)) {
        logger_print(WARN, TAG, "SSID truncated: original length %zu", ssid_len);
        result = ESP_FAIL;
    }
    size_t password_len = snprintf((char *)ap_config.ap.password, sizeof(ap_config.ap.password), "%s", AP_PASSWORD);
    if (password_len >= sizeof(sta_config.sta.password)) {
        logger_print(WARN, TAG, "Password truncated: original length %zu", password_len);
        result = ESP_FAIL;
    }

    ap_config.ap.ssid_len        = sizeof(AP_SSID);
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

    result += esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += esp_netif_dhcps_start(esp_netif_ap);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += esp_wifi_set_bandwidth(WIFI_IF_AP, AP_BW);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += esp_wifi_set_ps(AP_POWER_SAVE);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    return result;
}

/**
 * @brief Initialize Wi-Fi system and network interfaces.
 *
 * Sets up Wi-Fi in AP and STA modes, configures event handlers, and starts the Wi-Fi driver.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t network_task_initialize(void) {
    esp_err_t result = ESP_OK;

    result += esp_netif_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    result += esp_wifi_init(&cfg);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result += esp_event_loop_create_default();
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    result += esp_event_handler_instance_register(WIFI_EVENT,
                                                  ESP_EVENT_ANY_ID,
                                                  &wifi_event_handler,
                                                  NULL,
                                                  &instance_any_id);
    result += esp_event_handler_instance_register(IP_EVENT,
                                                  IP_EVENT_STA_GOT_IP,
                                                  &wifi_event_handler,
                                                  NULL,
                                                  &instance_got_ip);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap  = esp_netif_create_default_wifi_ap();

    result += esp_wifi_set_mode(WIFI_MODE_APSTA);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    set_access_point_mode();
    result += esp_wifi_start();
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    return result;
}

/**
 * @brief Set Wi-Fi credentials for connecting to a station.
 *
 * This function stores the provided SSID and password for connecting the ESP32
 * to a Wi-Fi network in station mode.
 *
 * @param[in] ssid     Pointer to the SSID string.
 * @param[in] password Pointer to the password string.
 *
 * @return ESP_OK on success, ESP_FAIL if input validation fails or other errors occur.
 */
esp_err_t network_set_credentials(const char *ssid, const char *password) {
    esp_err_t result = ESP_OK;

    if (ssid == NULL || password == NULL) {
        logger_print(ERR, TAG, "Invalid credentials: SSID or password is NULL");
        result = ESP_FAIL;
    } else {
        size_t ssid_len = snprintf((char *)sta_config.sta.ssid, sizeof(sta_config.sta.ssid), "%s", ssid);
        if (ssid_len >= sizeof(sta_config.sta.ssid)) {
            logger_print(WARN, TAG, "SSID truncated: original length %zu", ssid_len);
            result = ESP_FAIL;
        }

        size_t password_len = snprintf((char *)sta_config.sta.password, sizeof(sta_config.sta.password), "%s", password);
        if (password_len >= sizeof(sta_config.sta.password)) {
            logger_print(WARN, TAG, "Password truncated: original length %zu", password_len);
            result = ESP_FAIL;
        }
    }

    if (result == ESP_OK) {
        is_credential_set        = true;
        is_retry_limit_exceeded  = false;
        connection_retry_counter = 0;
        set_station_mode();
    }

    return result;
}

/**
 * @brief Main execution function for network management.
 *
 * This function initializes the network, starts the Wi-Fi subsystem, and
 * continuously monitors the connection status, attempting to reconnect
 * if credentials are set but the device is disconnected.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void network_task_execute(void *pvParameters) {
    global_events = (global_events_st *)pvParameters;
    if ((network_task_initialize() != ESP_OK) ||
        (global_events == NULL) ||
        (global_events->firmware_event_group == NULL)) {
        logger_print(ERR, TAG, "Failed to initialize network task");
        vTaskDelete(NULL);
    }

    network_set_credentials("NETPARQUE_PAOLA", "NPQ196253");

    while (1) {
        do {
            if (network_status.is_connect_sta) {
                // Network Already Connect
                connection_retry_counter = 0;
                break;
            }
            if (is_retry_limit_exceeded) {
                // The retry limit was exceeded
                break;
            }
            if (!is_credential_set) {
                // No credential was passed trough the webserver
                break;
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

                vTaskDelay(pdMS_TO_TICKS(RECONNECTION_DELAY_MS));
            } else {
                logger_print(ERR, TAG, "Max reconnect attempts reached. Stopping further attempts.");
                is_retry_limit_exceeded = true;
            }
        } while (0);

        vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_DELAY));
    }
}
