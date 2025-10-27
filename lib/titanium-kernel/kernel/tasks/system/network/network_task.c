/**
 * @file network.c
 * @brief Implementation of Wi-Fi management and network connectivity for ESP32.
 *
 * This file contains functions and configurations to manage Wi-Fi in both
 * Access Point (AP) and Station (STA) modes. It supports setting up an AP,
 * connecting to an external network as a STA, and handling network events.
 */
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "kernel/device/device_info.h"
#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/system/network/ethernet/ethernet_manager.h"
#include "kernel/tasks/system/network/network_task.h"
#include "kernel/tasks/system/network/wifi/wifi_manager.h"
#include "kernel/utils/utils.h"

/* Global Constants Definition */
static const char* TAG = "Network Task";  ///< Tag for logging.

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st* _global_structures = NULL;  ///< Pointer to the global configuration structure.

/**
 * @brief Central event handler for Wi-Fi, IP, and Ethernet events.
 *
 * This function dispatches incoming network-related events to their respective
 * subsystem handlers:
 * - Wi-Fi events are handled by wifi_manager_wifi_event_handler()
 * - Station IP events (Wi-Fi) are handled by wifi_manager_sta_got_ip()
 * - Ethernet IP events are handled by ethernet_manager_sta_got_ip()
 * - Ethernet IP loss events trigger ethernet_manager_lost_ip()
 * - Ethernet link events are handled by ethernet_manager_handle_events()
 *
 * After processing an event, the function updates the system event group
 * (firmware_event_group) to reflect the current network status. The event bits
 * WIFI_CONNECTED_AP and STA_GOT_IP allow other FreeRTOS tasks to synchronize
 * network-dependent behavior.
 *
 * @param[in] arg         Optional user data pointer (unused).
 * @param[in] event_base  Event base (e.g., WIFI_EVENT, IP_EVENT, ETH_EVENT).
 * @param[in] event_id    Event identifier.
 * @param[in] event_data  Pointer to event-specific data.
 */
static void network_task_event_handler(void* arg, esp_event_base_t event_base,
                                       int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        wifi_manager_wifi_event_handler(event_id, event_data);
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            wifi_manager_sta_got_ip(event_id, event_data);
        } else if (event_id == IP_EVENT_ETH_GOT_IP) {
            ethernet_manager_sta_got_ip(event_data);
        } else if (event_id == IP_EVENT_ETH_LOST_IP) {
            ethernet_manager_lost_ip();
        }

        if (event_id == IP_EVENT_STA_GOT_IP || event_id == IP_EVENT_ETH_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            device_info_set_ip_address(event->ip_info.ip);
        }
    } else if (event_base == ETH_EVENT) {
        ethernet_manager_handle_events(event_id, event_data);
    }

    EventGroupHandle_t firmware_event_group = _global_structures->global_events.firmware_event_group;

    bool wifi_ap_connected  = wifi_manager_get_connection_status(WIFI_MANAGER_IF_AP);
    bool wifi_sta_connected = wifi_manager_get_connection_status(WIFI_MANAGER_IF_STA);
    bool ethernet_connected = ethernet_manager_get_connection_status();

    if (wifi_ap_connected) {
        xEventGroupSetBits(firmware_event_group, WIFI_CONNECTED_AP);
    } else {
        xEventGroupClearBits(firmware_event_group, WIFI_CONNECTED_AP);
    }

    if (ethernet_connected || wifi_sta_connected) {
        xEventGroupSetBits(firmware_event_group, STA_GOT_IP);
    } else {
        xEventGroupClearBits(firmware_event_group, STA_GOT_IP);
    }
}

/**
 * @brief Initialize Wi-Fi system and network interfaces.
 *
 * Sets up Wi-Fi in AP and STA modes, configures event handlers, and starts the Wi-Fi driver.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
static kernel_error_st network_task_initialize(void) {
    esp_err_t result = ESP_OK;

    result = esp_netif_init();
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to initialize TCP/IP stack (esp_netif_init): %s", esp_err_to_name(result));
        return KERNEL_ERROR_WIFI_EVENT_REGISTER;
    }

    result = esp_event_loop_create_default();
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to create default event loop (esp_event_loop_create_default): %s", esp_err_to_name(result));
        return KERNEL_ERROR_WIFI_EVENT_REGISTER;
    }

    result = esp_event_handler_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &network_task_event_handler,
                                        NULL);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to register WIFI event handler: %s", esp_err_to_name(result));
        return KERNEL_ERROR_WIFI_EVENT_REGISTER;
    }

    result = esp_event_handler_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &network_task_event_handler,
                                        NULL);

    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to register STA IP event handler: %s", esp_err_to_name(result));
        return KERNEL_ERROR_IP_EVENT_REGISTER;
    }

    result = esp_event_handler_register(IP_EVENT,
                                        IP_EVENT_ETH_GOT_IP,
                                        &network_task_event_handler,
                                        NULL);

    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to register Ethernet GOT IP handler: %s", esp_err_to_name(result));
        return KERNEL_ERROR_IP_EVENT_REGISTER;
    }

    result = esp_event_handler_register(IP_EVENT,
                                        IP_EVENT_ETH_LOST_IP,
                                        &network_task_event_handler,
                                        NULL);

    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to register Ethernet LOST IP handler: %s", esp_err_to_name(result));
        return KERNEL_ERROR_IP_EVENT_REGISTER;
    }

    result = esp_event_handler_register(ETH_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &network_task_event_handler,
                                        NULL);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to register Ethernet event handler: %s", esp_err_to_name(result));
        return KERNEL_ERROR_ETH_EVENT_REGISTER;
    }

    kernel_error_st err = wifi_manager_initialize();
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initalized the WiFi Manager");
        return err;
    }

    err = ethernet_manager_initialize();
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initalized the Ethernet Manager");
        return err;
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
void network_task_execute(void* pvParameters) {
    _global_structures   = (global_structures_st*)pvParameters;
    kernel_error_st kerr = validate_global_structure(_global_structures);
    if ((network_task_initialize() != KERNEL_SUCCESS) || (kerr != KERNEL_SUCCESS)) {
        logger_print(ERR, TAG, "Failed to initialize network task");
        vTaskDelete(NULL);
    }

    credentials_st cred      = {0};
    QueueHandle_t cred_queue = queue_manager_get(CREDENTIALS_QUEUE_ID);
    if (cred_queue == NULL) {
        logger_print(ERR, TAG, "Credentials queue is NULL");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        if (xQueueReceive(cred_queue, &cred, pdMS_TO_TICKS(100)) == pdPASS) {
            logger_print(DEBUG, TAG, "SSID: %s, Password: %s", cred.ssid, cred.password);
            wifi_manager_set_credentials(cred.ssid, cred.password);
        }

        wifi_manager_manage_connection();

        vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_DELAY));
    }
}
