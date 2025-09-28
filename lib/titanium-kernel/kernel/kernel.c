#include "kernel.h"

#include "kernel/device/device_info.h"
#include "kernel/inter_task_communication/queues/queue_manager.h"
#include "kernel/utils/nvs_util.h"

task_interface_st sntp_task = {
    .arg          = NULL,
    .name         = SNTP_TASK_NAME,
    .priority     = SNTP_TASK_PRIORITY,
    .stack_size   = SNTP_TASK_STACK_SIZE,
    .task_execute = sntp_task_execute,
    .handle       = NULL,
};

task_interface_st watchdog_task = {
    .arg          = NULL,
    .name         = WATCHDOG_TASK_NAME,
    .priority     = WATCHDOG_TASK_PRIORITY,
    .stack_size   = WATCHDOG_TASK_STACK_SIZE,
    .task_execute = watchdog_task_execute,
    .handle       = NULL,
};

task_interface_st network_task = {
    .arg          = NULL,
    .name         = NETWORK_TASK_NAME,
    .priority     = NETWORK_TASK_PRIORITY,
    .stack_size   = NETWORK_TASK_STACK_SIZE,
    .task_execute = network_task_execute,
    .handle       = NULL,
};

task_interface_st http_server_task = {
    .arg          = NULL,
    .name         = HTTP_SERVER_TASK_NAME,
    .priority     = HTTP_SERVER_TASK_PRIORITY,
    .stack_size   = HTTP_SERVER_TASK_STACK_SIZE,
    .task_execute = http_server_task_execute,
    .handle       = NULL,
};

task_interface_st mqtt_task = {
    .arg          = NULL,
    .name         = MQTT_CLIENT_TASK_NAME,
    .priority     = MQTT_CLIENT_TASK_PRIORITY,
    .stack_size   = MQTT_CLIENT_TASK_STACK_SIZE,
    .task_execute = mqtt_client_task_execute,
    .handle       = NULL,
};

static const char *TAG = "KERNEL";  ///< Tag for logging

/**
 * @brief Perform a system restart on the ESP32.
 *
 * This function triggers a software reset using the ESP-IDF `esp_restart()` API.
 * It should be used in critical error handling paths where recovery from the
 * current state is not possible or safe.
 *
 * @note This function does not return. The ESP32 will reboot immediately.
 *
 * Example usage:
 * ```c
 * if (critical_fault_detected) {
 *     kernel_restart(); // Recover system by restarting
 * }
 * ```
 */
void kernel_restart(void) {
#ifndef DEBUG
    logger_print(INFO, TAG, "Restarting system due to critical error");
    esp_restart();
#endif
}

/*
 * @brief Initialize the Non-Volatile Storage (NVS) for the device.
 *
 * This function initializes the NVS (Non-Volatile Storage) module, which is used to store
 * configuration data and other persistent information. If the NVS initialization fails due
 * to no free pages or a new version of NVS being found, it will erase the existing NVS data
 * and reinitialize it.
 *
 * @return
 * - ESP_OK on successful initialization.
 * - Other error codes from `nvs_flash_init()` in case of failure.
 */
static kernel_error_st kernel_initialize_nvs(void) {
    return nvs_util_init();
}

kernel_error_st kernel_global_events_initialize(global_events_st *global_events) {
    if (global_events_initialize(global_events) != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize global events");
        return KERNEL_ERROR_GLOBAL_EVENTS_INIT;
    }

    return KERNEL_SUCCESS;
}

kernel_error_st kernel_global_queues_initialize() {
    kernel_error_st err = queue_manager_init();
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize global queues - %d", err);
        return KERNEL_ERROR_GLOBAL_QUEUES_INIT;
    }

    err = queue_manager_register(NETWORK_BRIDGE_QUEUE_ID, 2, sizeof(network_bridge_st));
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to register network bridge queue - %d", err);
        return err;
    }
    err = queue_manager_register(MQTT_BRIDGE_QUEUE_ID, 10, sizeof(mqtt_bridge_st));
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to register network bridge queue - %d", err);
        return err;
    }
    err = queue_manager_register(CREDENTIALS_QUEUE_ID, 1, sizeof(credentials_st));
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to register network bridge queue - %d", err);
        return err;
    };

    return KERNEL_SUCCESS;
}

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
 * @return KERNEL_SUCCESS on success, or an appropriate error code if initialization fails.
 */
kernel_error_st kernel_initialize(release_mode_et release_mode, log_output_et log_output, global_structures_st *global_structures) {
    kernel_error_st ret = KERNEL_SUCCESS;

    if ((global_structures == NULL)) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (kernel_initialize_nvs() != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize NVS");
        kernel_restart();
        return KERNEL_ERROR_NVS_INIT;
    }
    logger_initialize(release_mode, log_output, global_structures);

    device_info_init();

    if (kernel_global_events_initialize(&global_structures->global_events) != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize global events");
        kernel_restart();
        return KERNEL_ERROR_GLOBAL_EVENTS_INIT;
    }
    if (kernel_global_queues_initialize() != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize global queues");
        kernel_restart();
        return KERNEL_ERROR_GLOBAL_QUEUES_INIT;
    }

    sntp_task.arg = (void *)global_structures;
    ret           = task_handler_enqueue_task(&sntp_task);

    if (ret != KERNEL_SUCCESS) {
        return ret;
    }

    watchdog_task.arg = (void *)global_structures;
    ret               = task_handler_enqueue_task(&watchdog_task);

    if (ret != KERNEL_SUCCESS) {
        return ret;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Enables network connectivity by starting the network task.
 *
 * This function spawns a task to handle network operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_SUCCESS on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_network(global_structures_st *global_structures) {
    if (global_structures == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    network_task.arg = (void *)global_structures;
    return task_handler_enqueue_task(&network_task);
}

/**
 * @brief Starts the HTTP server by creating its task.
 *
 * This function spawns a task to handle HTTP server operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_SUCCESS on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_http_server(global_structures_st *global_structures) {
    if (global_structures == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    http_server_task.arg = (void *)global_structures;
    return task_handler_enqueue_task(&http_server_task);
}

/**
 * @brief Enables the MQTT client by creating its task.
 *
 * This function spawns a task to handle MQTT client operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_SUCCESS on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_mqtt(global_structures_st *global_structures) {
    if (global_structures == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    mqtt_task.arg = (void *)global_structures;
    return task_handler_enqueue_task(&mqtt_task);
}

/**
 * @brief Starts all tasks that have been enqueued in the kernel's task manager.
 *
 * This function triggers the execution of all previously enqueued tasks.
 * It should be called after all necessary tasks have been enqueued and
 * the system is ready to start multitasking.
 *
 * @return KERNEL_SUCCESS on success, or an error code if starting tasks fails.
 */
kernel_error_st kernel_start_tasks(void) {
    return task_handler_start_queued_tasks();
}

/**
 * @brief Enqueues a task to the kernel's task manager for later execution.
 *
 * Adds the specified task to the internal queue managed by the task manager.
 * The task will be started when `kernel_start_tasks()` is called.
 *
 * @param task Pointer to the task interface structure to enqueue. Must not be NULL.
 *
 * @return KERNEL_SUCCESS on success,
 *         KERNEL_ERROR_NULL if the task pointer is NULL,
 *         or other task manager specific error codes.
 */
kernel_error_st kernel_enqueue_task(task_interface_st *task) {
    if (task == NULL) {
        return KERNEL_ERROR_NULL;
    }

    return task_handler_enqueue_task(task);
}
