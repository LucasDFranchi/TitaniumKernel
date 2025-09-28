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
#include "kernel/tasks/system/network/network_task.h"
#include "kernel/tasks/system/network/wifi/wifi_manager.h"
#include "kernel/utils/utils.h"

/* Global Constants Definition */
static const char *TAG = "Network Task";        ///< Tag for logging.
static esp_eth_handle_t eth_handle;             ///< Handle for the Ethernet driver instance; currently supports only one external Ethernet device.
static network_bridge_st network_bridge = {0};  ///< Network bridge interface instance used for Ethernet event handling and driver operations.

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st *_global_structures = NULL;  ///< Pointer to the global configuration structure.

/**
 * @brief Central event handler for Wi-Fi, IP, and Ethernet events.
 *
 * This function dispatches incoming events from the Wi-Fi, IP, and Ethernet
 * subsystems to their respective handlers. It routes:
 * - Wi-Fi events to `wifi_manager_wifi_event_handler`
 * - IP events to `wifi_manager_sta_got_ip` or `network_bridge.got_ip`
 * - Ethernet events to `network_bridge.handle_ethernet_events`
 *
 * After processing the event, it updates the system's global event group
 * (`firmware_event_group`) to reflect the current connection statuses of
 * the Wi-Fi AP interface, the Wi-Fi STA interface, and the Ethernet interface.
 *
 * These event group bits (`WIFI_CONNECTED_AP`, `STA_GOT_IP`) allow other
 * FreeRTOS tasks to synchronize with network state changes.
 *
 * @param[in] arg         Optional user data pointer (unused).
 * @param[in] event_base  Base of the event (e.g., WIFI_EVENT, IP_EVENT, ETH_EVENT).
 * @param[in] event_id    ID of the specific event.
 * @param[in] event_data  Pointer to event-specific data structure.
 */
static void network_task_event_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        wifi_manager_wifi_event_handler(event_id, event_data);
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            wifi_manager_sta_got_ip(event_id, event_data);
        } else if ((event_id == IP_EVENT_ETH_GOT_IP) && network_bridge.got_ip) {
            network_bridge.got_ip(event_data);
        }
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        device_info_set_ip_address(event->ip_info.ip);
    } else if ((event_base == ETH_EVENT) && network_bridge.handle_ethernet_events) {
        network_bridge.handle_ethernet_events(event_id, event_data);
    }

    EventGroupHandle_t firmware_event_group = _global_structures->global_events.firmware_event_group;

    bool wifi_ap_connected  = wifi_manager_get_connection_status(WIFI_MANAGER_IF_AP);
    bool wifi_sta_connected = wifi_manager_get_connection_status(WIFI_MANAGER_IF_STA);
    bool ethernet_connected = network_bridge.get_ethernet_status ? network_bridge.get_ethernet_status() : false;

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
 * @brief Registers and starts the Ethernet network interface.
 *
 * This function creates a new Ethernet interface, installs the Ethernet driver
 * using the configured bridge initializer, starts the DHCP client, and starts
 * the Ethernet interface.
 *
 * @return KERNEL_SUCCESS on success, or a specific kernel error on failure.
 */
static kernel_error_st register_network_device(void) {
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);

    if (network_bridge.initialize_driver == NULL) {
        return KERNEL_ERROR_FUNC_POINTER_NULL;
    }

    kernel_error_st err = network_bridge.initialize_driver(&eth_handle);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to install ethernet driver: %d", err);
    }

    esp_err_t result = esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to attach Ethernet netif: %s", esp_err_to_name(result));
        return KERNEL_ERROR_ETH_NET_INTERFACE_ATTACH;
    }

    result = esp_netif_dhcpc_start(eth_netif);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to start DHCP client: %s", esp_err_to_name(result));
        return KERNEL_ERROR_DHCP_START;
    }

    logger_print(INFO, TAG, "DHCP client started successfully");

    result = esp_eth_start(eth_handle);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to start Ethernet: %s", esp_err_to_name(result));
        return KERNEL_ERROR_ETHERNET_START;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Installs the external network bridge if not already installed.
 *
 * Attempts to receive a bridge configuration from the global queue and,
 * if successful, registers the Ethernet device. Ensures the installation
 * is performed only once.
 *
 * @return KERNEL_SUCCESS on success or if already installed, or a specific error code on failure.
 */
static kernel_error_st network_install_bridge(QueueHandle_t eth_queue) {
    static bool is_external_device_installed = false;

    if (is_external_device_installed) {
        return KERNEL_SUCCESS;
    }

    if (xQueueReceive(eth_queue, &network_bridge, pdMS_TO_TICKS(100)) == pdTRUE) {
        kernel_error_st err = register_network_device();
        if (err != KERNEL_SUCCESS) {
            logger_print(INFO, TAG, "Failed to install external ethernet device: %d", err);
            return err;
        }
        logger_print(INFO, TAG, "Network bridge successfully installed.");
        is_external_device_installed = true;
    } else {
        logger_print(DEBUG, TAG, "No network bridge configuration available yet. Will retry later.");
    }

    return KERNEL_SUCCESS;
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
        logger_print(ERR, TAG, "Failed to register WiFi IP event handler: %s", esp_err_to_name(result));
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
    result = esp_event_handler_register(IP_EVENT,
                                        IP_EVENT_ETH_GOT_IP,
                                        &network_task_event_handler,
                                        NULL);
    if (result != ESP_OK) {
        logger_print(ERR, TAG, "Failed to register Ethernet IP event handler: %s", esp_err_to_name(result));
        return KERNEL_ERROR_IP_EVENT_REGISTER;
    }

    kernel_error_st err = wifi_manager_initialize();
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initalized the WiFi Manager");
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
void network_task_execute(void *pvParameters) {
    _global_structures = (global_structures_st *)pvParameters;
    if ((network_task_initialize() != ESP_OK) || validate_global_structure(_global_structures)) {
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

    QueueHandle_t eth_queue = queue_manager_get(NETWORK_BRIDGE_QUEUE_ID);
    if (eth_queue == NULL) {
        logger_print(ERR, TAG, "Ethernet bridge queue is NULL");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        network_install_bridge(eth_queue);

        if (xQueueReceive(cred_queue, &cred, pdMS_TO_TICKS(100)) == pdPASS) {
            logger_print(DEBUG, TAG, "SSID: %s, Password: %s", cred.ssid, cred.password);
            wifi_manager_set_credentials(cred.ssid, cred.password);
        }

        wifi_manager_manage_connection();

        vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_DELAY));
    }
}
