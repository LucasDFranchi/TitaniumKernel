/**
 * @file network.c
 * @brief Implementation of Wi-Fi management and network connectivity for ESP32.
 *
 * This file contains functions and configurations to manage Wi-Fi in both
 * Access Point (AP) and Station (STA) modes. It supports setting up an AP,
 * connecting to an external network as a STA, and handling network events.
 */
#include "esp_event.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "kernel/inter_task_communication/events/events_definition.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/system/network/network_task.h"
#include "kernel/tasks/system/network/wifi/wifi_manager.h"
#include "kernel/utils/utils.h"

/* Global Constants Definition */
static const char *TAG = "Network Task";  ///< Tag for logging.

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st *_global_structures = NULL;  ///< Pointer to the global configuration structure.

/**
 * @brief Central event handler for Wi-Fi and IP events.
 *
 * This function dispatches incoming events from the Wi-Fi and IP subsystems
 * to their respective handlers (`wifi_manager_wifi_event_handler` and
 * `wifi_manager_sta_got_ip`). It also updates the system's global event group
 * with the current Wi-Fi interface connection statuses.
 *
 * This mechanism enables other tasks to synchronize with the AP or STA
 * connection state using FreeRTOS event groups.
 *
 * @param[in] arg         Optional user data pointer (unused).
 * @param[in] event_base  Base of the event (e.g., WIFI_EVENT, IP_EVENT).
 * @param[in] event_id    ID of the specific event.
 * @param[in] event_data  Pointer to event-specific data structure.
 */
static void network_task_event_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        wifi_manager_wifi_event_handler(event_id, event_data);
    } else if (event_base == IP_EVENT) {
        wifi_manager_sta_got_ip(event_id, event_data);
    }

    EventGroupHandle_t firmware_event_group = _global_structures->global_events.firmware_event_group;
    if (wifi_manager_get_connection_status(WIFI_MANAGER_IF_AP)) {
        xEventGroupSetBits(firmware_event_group, WIFI_CONNECTED_AP);
    } else {
        xEventGroupClearBits(firmware_event_group, WIFI_CONNECTED_AP);
    }

    if (wifi_manager_get_connection_status(WIFI_MANAGER_IF_STA)) {
        xEventGroupSetBits(firmware_event_group, WIFI_CONNECTED_STA);
    } else {
        xEventGroupClearBits(firmware_event_group, WIFI_CONNECTED_STA);
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

    /* Start Common Block */
    result = esp_netif_init();
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result = esp_event_loop_create_default();
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);

    result = esp_event_handler_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &network_task_event_handler,
                                        NULL);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result = esp_event_handler_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &network_task_event_handler,
                                        NULL);

    kernel_error_st err = wifi_manager_initialize();
    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initalized the WiFi Manager");
        return err;
    }
    // ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    // result = esp_event_handler_register(ETH_EVENT,
    //                                     ESP_EVENT_ANY_ID,
    //                                     &network_task_event_handler,
    //                                     NULL);
    /* End Common Block */

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

    while (1) {
        credentials_st cred = {0};
        if (xQueueReceive(_global_structures->global_queues.credentials_queue, &cred, pdMS_TO_TICKS(100)) == pdPASS) {
            logger_print(DEBUG, TAG, "SSID: %s, Password: %s", cred.ssid, cred.password);
            wifi_manager_set_credentials(cred.ssid, cred.password);
        }

        wifi_manager_manage_connection();

        vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_DELAY));
    }
}
