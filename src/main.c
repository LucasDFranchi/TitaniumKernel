#include "kernel/kernel.h"

#include "app/app.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st global_structures = {0};

task_interface_st app_task = {
    .arg          = (void *)&global_structures,
    .name         = "Application Task",
    .priority     = 1,
    .stack_size   = 1024 * 8,
    .task_execute = app_task_execute,
    .handle       = NULL,
};

void app_main() {
    kernel_initialize(UDP, &global_structures);
    kernel_enable_network(&global_structures);
    kernel_enable_http_server(&global_structures);
    // kernel_enable_mqtt(&global_structures);

    kernel_enqueue_task(&app_task);

    kernel_start_tasks();
    printf("Application started successfully");
}