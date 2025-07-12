#ifndef KERNEL_H
#define KERNEL_H

#include "kernel/inter_task_communication/events/events_definition.h"
#include "kernel/inter_task_communication/queues/queues_definition.h"

#include "kernel/tasks/interface/task_interface.h"
#include "kernel/tasks/iot/http_server/http_server_task.h"
#include "kernel/tasks/iot/mqtt/mqtt_client_task.h"
#include "kernel/tasks/system/network/network_task.h"
#include "kernel/tasks/system/sntp/sntp_task.h"
#include "kernel/tasks/system/watchdog/watchdog_task.h"

#include "error/error_num.h"
#include "kernel/logger/logger.h"

/**
 * @brief Initializes the kernel subsystems and global resources.
 *
 * This function sets up core components of the kernel, including:
 * - Device information
 * - Non-volatile storage (NVS)
 * - Logging system
 * - Global event and queue structures
 * - System tasks (e.g., SNTP and watchdog)
 *
 * It must be called once during system startup before using any kernel services.
 * If initialization fails at any stage, the function logs the error and attempts a system restart.
 *
 * @param release_mode        The build mode (RELEASE or DEBUG) used to configure logging behavior.
 * @param log_output          The log output backend (e.g., SERIAL or UDP).
 * @param global_structures   Pointer to the global system state structure. Must not be NULL.
 *
 * @return KERNEL_ERROR_NONE on success, or an appropriate error code if initialization fails.
 */
kernel_error_st kernel_initialize(release_mode_et release_mode, log_output_et log_output, global_structures_st *global_structures);

/**
 * @brief Enables network connectivity by starting the network task.
 *
 * This function spawns a task to handle network operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_network(global_structures_st *global_structures);

/**
 * @brief Starts the HTTP server by creating its task.
 *
 * This function spawns a task to handle HTTP server operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_http_server(global_structures_st *global_structures);

/**
 * @brief Enables the MQTT client by creating its task.
 *
 * This function spawns a task to handle MQTT client operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_mqtt(global_structures_st *global_structures);

/**
 * @brief Starts all tasks that have been enqueued in the kernel's task manager.
 *
 * This function triggers the execution of all previously enqueued tasks.
 * It should be called after all necessary tasks have been enqueued and
 * the system is ready to start multitasking.
 *
 * @return KERNEL_ERROR_NONE on success, or an error code if starting tasks fails.
 */
kernel_error_st kernel_start_tasks(void);

/**
 * @brief Enables the MQTT client by creating its task.
 *
 * This function spawns a task to handle MQTT client operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enqueue_task(task_interface_st *task);

#endif /* KERNEL_H */