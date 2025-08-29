/**
 * @file app.h
 * @brief Main application initialization and task management.
 *
 * This module is responsible for:
 * - Initializing the network bridge (Ethernet) and MQTT bridge.
 * - Setting up Sensor, Command, and Health Manager tasks.
 * - Attaching tasks to the FreeRTOS scheduler.
 * - Providing centralized configuration via global structures.
 *
 * It defines static configuration structures for bridges, tasks, and
 * manager modules, which are initialized at runtime. Logging is
 * integrated for initialization steps and error handling.
 */
#pragma once

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

/**
 * @brief Initialize the application and attach core tasks.
 *
 * This function sets up the main application components:
 * 1. Validates the global structure.
 * 2. Initializes the network bridge and sends it to its queue.
 * 3. Initializes the MQTT bridge and sends it to its queue.
 * 4. Configures and attaches the Sensor Manager, Command Manager,
 *    and Health Manager tasks to the task manager.
 *
 * @param[in] global_structures Pointer to the global configuration structure.
 *                              Must contain valid queues for network and MQTT bridges.
 *
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_INVALID_ARG if the global structures are invalid
 *         - KERNEL_ERROR_xxx if initialization of network bridge, MQTT bridge,
 *           or any manager task fails
 *
 * @note The function must be called once during system startup before any tasks run.
 *       It assumes FreeRTOS is initialized and queues are created.
 */
kernel_error_st app_initialize(global_structures_st *global_structures);
