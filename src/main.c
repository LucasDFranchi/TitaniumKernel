#include "kernel/kernel.h"


/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_events_st global_events = {0};

void app_main() {
    kernel_initialize(SERIAL, &global_events);
    kernel_enable_network(&global_events);
    kernel_enable_http_server(&global_events);
    kernel_enable_mqtt(&global_events);

    kernel_start_tasks();
}