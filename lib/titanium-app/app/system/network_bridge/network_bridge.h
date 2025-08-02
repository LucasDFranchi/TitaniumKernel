#pragma once

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

#include "app/hardware/drivers/w5500.h"

/**
 * @brief Initialization parameters for the network bridge module.
 *
 * Contains the Ethernet device configuration and a pointer to the network
 * bridge interface structure where function pointers and callbacks are set.
 */
typedef struct network_bridge_init_s {
    ethernet_device_st ethernet_device; /**< Ethernet device configuration */
    network_bridge_st *network_bridge;  /**< Pointer to network bridge interface */
} network_bridge_init_st;

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
kernel_error_st network_bridge_initialize(network_bridge_init_st *network_bridge_init);
