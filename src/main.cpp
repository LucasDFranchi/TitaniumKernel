extern "C" {
#include "kernel/kernel.h"

#include "app/app.h"
}
/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st global_structures{};

int main() {
    kernel_initialize(RELEASE_MODE_DEBUG, SERIAL, &global_structures);
    kernel_enable_network(&global_structures);
    kernel_enable_http_server(&global_structures);
    kernel_enable_mqtt(&global_structures);
    kernel_start_tasks();

    Application app;
    kernel_error_st err = app.initialize(&global_structures);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, "main", "Failed to initalized the application! - %d", err);
    }

    logger_print(INFO, "main", "Application started successfully");

    return 0;
}

extern "C" void app_main(void) {
    main();
}