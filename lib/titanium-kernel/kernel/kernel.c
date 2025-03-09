#include "kernel.h"

#include "nvs_flash.h"

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
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result == ESP_OK ? KERNEL_ERROR_NONE : KERNEL_ERROR_NVS_INIT;
}

kernel_error_st kernel_global_events_initialize(global_events_st *global_events) {
    if (global_events == NULL) {
        logger_print(ERR, "KERNEL", "Invalid global events pointer");
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (global_events_initialize(global_events) != KERNEL_ERROR_NONE) {
        logger_print(ERR, "KERNEL", "Failed to initialize global events");
        return KERNEL_ERROR_GLOBAL_EVENTS_INIT;
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
kernel_error_st kernel_initialize(log_output_et log_output, global_events_st *global_events) {
    kernel_error_st ret = KERNEL_ERROR_NONE;

    if (global_events == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (kernel_initialize_nvs() != KERNEL_ERROR_NONE) {
        logger_print(ERR, "KERNEL", "Failed to initialize NVS");
        return KERNEL_ERROR_NVS_INIT;
    }

    kernel_global_events_initialize(global_events);

    logger_initialize(log_output, global_events);

    sntp_task.arg = (void *)global_events;
    ret           = task_manager_enqueue_task(&sntp_task);

    if (ret != KERNEL_ERROR_NONE) {
        return ret;
    }

    watchdog_task.arg = (void *)global_events;
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
kernel_error_st kernel_enable_network(global_events_st *global_events) {
    if (global_events == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    network_task.arg = (void *)global_events;
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
kernel_error_st kernel_enable_http_server(global_events_st *global_events) {
    if (global_events == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    http_server_task.arg = (void *)global_events;
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
kernel_error_st kernel_enable_mqtt(global_events_st *global_events) {
    if (global_events == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    mqtt_task.arg = (void *)global_events;
    return task_manager_enqueue_task(&mqtt_task);
}

kernel_error_st kernel_start_tasks(void) {
    return task_manager_start_queued_tasks();
}
