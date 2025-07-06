#include "kernel.h"

#include "kernel/utils/nvs_util.h"
#include "kernel/device/device_info.h"

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
    if (global_events_initialize(global_events) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize global events");
        return KERNEL_ERROR_GLOBAL_EVENTS_INIT;
    }

    return KERNEL_ERROR_NONE;
}

kernel_error_st kernel_global_queues_initialize(global_queues_st *global_queue) {
    if (global_queues_initialize(global_queue) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize global queues");
        return KERNEL_ERROR_GLOBAL_QUEUES_INIT;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Initializes the kernel by creating essential tasks.
 *
 * This function creates the SNTP and watchdog tasks required for system operation.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails.
 */
kernel_error_st kernel_initialize(log_output_et log_output, global_structures_st *global_structures) {
    kernel_error_st ret = KERNEL_ERROR_NONE;

    if ((global_structures == NULL)) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    device_info_init();

    if (kernel_initialize_nvs() != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize NVS");
        kernel_restart();
        return KERNEL_ERROR_NVS_INIT;
    }
    logger_initialize(log_output, global_structures);

    if (kernel_global_events_initialize(&global_structures->global_events) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize global events");
        kernel_restart();
        return KERNEL_ERROR_GLOBAL_EVENTS_INIT;
    }
    if (kernel_global_queues_initialize(&global_structures->global_queues) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize global queues");
        kernel_restart();
        return KERNEL_ERROR_GLOBAL_QUEUES_INIT;
    }

    sntp_task.arg = (void *)global_structures;
    ret           = task_manager_enqueue_task(&sntp_task);

    if (ret != KERNEL_ERROR_NONE) {
        return ret;
    }

    watchdog_task.arg = (void *)&global_structures;
    ret               = task_manager_enqueue_task(&watchdog_task);

    if (ret != KERNEL_ERROR_NONE) {
        return ret;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Enables network connectivity by starting the network task.
 *
 * This function spawns a task to handle network operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_network(global_structures_st *global_structures) {
    if (global_structures == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    network_task.arg = (void *)global_structures;
    return task_manager_enqueue_task(&network_task);
}

/**
 * @brief Starts the HTTP server by creating its task.
 *
 * This function spawns a task to handle HTTP server operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_http_server(global_structures_st *global_structures) {
    if (global_structures == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    http_server_task.arg = (void *)global_structures;
    return task_manager_enqueue_task(&http_server_task);
}

/**
 * @brief Enables the MQTT client by creating its task.
 *
 * This function spawns a task to handle MQTT client operations.
 *
 * @param global_events Pointer to the global configuration structure.
 * @return KERNEL_ERROR_NONE on success, KERNEL_ERROR_TASK_CREATE if task creation fails,
 *         or KERNEL_ERROR_NULL if global_events is NULL.
 */
kernel_error_st kernel_enable_mqtt(global_structures_st *global_structures) {
    if (global_structures == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    mqtt_task.arg = (void *)global_structures;
    return task_manager_enqueue_task(&mqtt_task);
}

kernel_error_st kernel_start_tasks(void) {
    return task_manager_start_queued_tasks();
}

kernel_error_st kernel_enqueue_task(task_interface_st *task) {
    if (task == NULL) {
        return KERNEL_ERROR_NULL;
    }

    return task_manager_enqueue_task(task);
}
