#pragma once

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

#include "app/hardware/drivers/w5500.h"

/**
 * @brief Handle Ethernet events and update connection status accordingly.
 *
 * Logs Ethernet events such as connection, disconnection, start, and stop.
 * Updates the internal connection status flag on disconnection.
 *
 * @param[in] event_id    The Ethernet event ID.
 * @param[in] event_data  Pointer to event-specific data (unused here).
 */
void ethernet_manager_handle_events(int32_t event_id, void* event_data);


void ethernet_manager_lost_ip(void);

/**
 * @brief Handle the event when the Ethernet interface receives an IP address.
 *
 * This function is called when the Ethernet interface successfully obtains an
 * IP address via DHCP. It logs the assigned IP, netmask, and gateway information,
 * and sets the internal connection status flag to indicate that Ethernet is online.
 *
 * @param[in] event_data  Pointer to the IP event data, which should be cast
 *                        to `ip_event_got_ip_t*`.
 */
void ethernet_manager_sta_got_ip(void* event_data);

/**
 * @brief Retrieve the current Ethernet connection status.
 *
 * This function returns whether the Ethernet interface is currently connected
 * and has obtained a valid IP address.
 *
 * @return true if Ethernet is connected and has a valid IP; false otherwise.
 */
bool ethernet_manager_get_connection_status(void);

/**
 * @brief Initialize and start the Ethernet interface and driver.
 *
 * This function:
 * - Creates a new Ethernet network interface.
 * - Installs and initializes the Ethernet driver (e.g., W5500).
 * - Attaches the driver to the ESP-IDF network interface glue layer.
 * - Starts the DHCP client to acquire an IP address.
 * - Starts the Ethernet interface.
 *
 * After this function returns successfully, the Ethernet interface should
 * be ready to connect to the network and obtain an IP.
 *
 * @return KERNEL_SUCCESS if the Ethernet interface was successfully initialized
 *         and started; otherwise, returns a specific kernel error code.
 */
kernel_error_st ethernet_manager_initialize();
