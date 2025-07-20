#include "network_bridge.h"

#include "esp_event.h"
#include "esp_netif.h"

#include "kernel/logger/logger.h"

/* Network Bridge Global Variables */
static const char *TAG              = "Network Bridge";  // Log tag for network bridge
static bool ethernet_got_ip         = false;             // Ethernet connection status flag
ethernet_device_st *ethernet_device = NULL;              // Pointer to Ethernet device instance

/**
 * @brief Handle Ethernet events and update connection status accordingly.
 *
 * Logs Ethernet events such as connection, disconnection, start, and stop.
 * Updates the internal connection status flag on disconnection.
 *
 * @param[in] event_id    The Ethernet event ID.
 * @param[in] event_data  Pointer to event-specific data (unused here).
 */
static void handle_ethernet_events(int32_t event_id, void *event_data) {
    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            logger_print(INFO, TAG, "Ethernet cable connected.");
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            logger_print(WARN, TAG, "Ethernet cable disconnected.");
            ethernet_got_ip = false;
            break;
        case ETHERNET_EVENT_START:
            logger_print(INFO, TAG, "Ethernet interface started.");
            break;
        case ETHERNET_EVENT_STOP:
            logger_print(INFO, TAG, "Ethernet interface stopped.");
            break;
        default:
            logger_print(DEBUG, TAG, "Received unknown Ethernet event ID: %d", event_id);
            break;
    }
}

/**
 * @brief Handle the event when the Ethernet interface gets an IP address.
 *
 * Logs the assigned IP address, netmask, and gateway information.
 * Sets the internal Ethernet connection status flag to true.
 *
 * @param[in] event_data  Pointer to the IP event data (cast to ip_event_got_ip_t).
 */
static void got_ip(void *event_data) {
    ip_event_got_ip_t *event           = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    logger_print(DEBUG, TAG, "Ethernet Got IP Address");
    logger_print(DEBUG, TAG, "-----------------------");
    logger_print(DEBUG, TAG, "IP: " IPSTR, IP2STR(&ip_info->ip));
    logger_print(DEBUG, TAG, "MASK: " IPSTR, IP2STR(&ip_info->netmask));
    logger_print(DEBUG, TAG, "GW: " IPSTR, IP2STR(&ip_info->gw));
    logger_print(DEBUG, TAG, "-----------------------");

    ethernet_got_ip = true;
}

/**
 * @brief Get the current Ethernet connection status.
 *
 * @return true if Ethernet is connected and has a valid IP, false otherwise.
 */
static bool get_ethernet_status(void) {
    return ethernet_got_ip;
}

/**
 * @brief Initialize the Ethernet driver for the specified Ethernet device.
 *
 * Calls the W5500 driver initialization function and logs any errors.
 *
 * @param[out] eth_handle  Pointer to store the initialized Ethernet handle.
 * @return KERNEL_ERROR_NONE on success, or an error code on failure.
 */
static kernel_error_st initialize_driver(esp_eth_handle_t *eth_handle) {
    if (eth_handle == NULL) {
        return KERNEL_ERROR_NULL;
    }

    kernel_error_st err = w5500_initialize(ethernet_device, eth_handle);
    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "W5500 driver initialization failed: %d", err);
        return err;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Initialize the network bridge interface with the given parameters.
 *
 * Sets up function pointers for Ethernet driver initialization, event handling,
 * IP acquisition callback, and status checking within the network bridge struct.
 *
 * @param[in,out] network_bridge_init Pointer to initialization structure containing
 *                                    Ethernet device and network bridge references.
 * @return KERNEL_ERROR_NONE on success, or KERNEL_ERROR_NULL if input is NULL.
 */
kernel_error_st network_bridge_initialize(network_bridge_init_st *network_bridge_init) {
    if (network_bridge_init == NULL) {
        return KERNEL_ERROR_NULL;
    }

    ethernet_device                                             = &network_bridge_init->ethernet_device;
    network_bridge_init->network_bridge->initialize_driver      = initialize_driver;
    network_bridge_init->network_bridge->handle_ethernet_events = handle_ethernet_events;
    network_bridge_init->network_bridge->got_ip                 = got_ip;
    network_bridge_init->network_bridge->get_ethernet_status    = get_ethernet_status;

    return KERNEL_ERROR_NONE;
}
