#include "kernel/kernel.h"

#include "app/app.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_events_st global_events = {0};

task_interface_st app_task = {
    .arg          = NULL,
    .name         = "Application Task",
    .priority     = 1,
    .stack_size   = 1024 * 8,
    .task_execute = app_task_execute,
    .handle       = NULL,
};

void app_main() {
    kernel_initialize(SERIAL, &global_events);
    kernel_enable_network(&global_events);
    kernel_enable_http_server(&global_events);
    kernel_enable_mqtt(&global_events);

    kernel_enqueue_task(&app_task);

    kernel_start_tasks();
}