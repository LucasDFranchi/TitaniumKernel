#ifndef NETWORK_EXTERNAL_TYPES_H
#define NETWORK_EXTERNAL_TYPES_H

#include <stdint.h>

#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "kernel/inter_task_communication/inter_task_communication.h"

#define NETWORK_MAXIMUM_SSID_SIZE 32      ///< Defines the maximum length of an SSID string.
#define NETWORK_MAXIMUM_PASSWORD_SIZE 64  ///< Defines the maximum length of an password string.

/**
 * @brief Structure to hold WiFi credentials.
 *
 * This structure is used to store the SSID and password for WiFi connections.
 * It is designed to be used with FreeRTOS queues for inter-task communication.
 */
typedef struct credentials_s {
    char ssid[NETWORK_MAXIMUM_SSID_SIZE];
    char password[NETWORK_MAXIMUM_PASSWORD_SIZE];
} credentials_st;

/**
 * @brief Function pointer type for initializing the Ethernet driver.
 *
 * @param[out] eth_handle Pointer to store the initialized Ethernet handle.
 * @return KERNEL_ERROR_NONE on success, or an error code on failure.
 */
typedef kernel_error_st (*initialize_driver_t)(esp_eth_handle_t *eth_handle);

/**
 * @brief Function pointer type for handling Ethernet events.
 *
 * @param[in] event_id    Ethernet event identifier.
 * @param[in] event_data  Pointer to event-specific data.
 */
typedef void (*handle_ethernet_events_t)(int32_t event_id, void *event_data);

/**
 * @brief Function pointer type for handling the event of obtaining an IP address.
 *
 * @param[in] event_data Pointer to IP event data.
 */
typedef void (*got_ip_t)(void *event_data);

/**
 * @brief Function pointer type to get the Ethernet connection status.
 *
 * @return true if Ethernet is connected and has a valid IP, false otherwise.
 */
typedef bool (*get_ethernet_status_t)(void);

/**
 * @brief Structure defining the network bridge interface with function callbacks.
 *
 * Contains function pointers to Ethernet driver initialization,
 * event handling, IP acquisition callback, and connection status query.
 */
typedef struct network_bridge_s {
    initialize_driver_t initialize_driver;           /**< Initialize Ethernet driver */
    handle_ethernet_events_t handle_ethernet_events; /**< Handle Ethernet events */
    got_ip_t got_ip;                                 /**< Handle IP acquisition event */
    get_ethernet_status_t get_ethernet_status;       /**< Query Ethernet connection status */
} network_bridge_st;

#endif /* NETWORK_EXTERNAL_TYPES_H */
