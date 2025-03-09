#include "error/error_num.h"
#include "events_definition.h"

/**
 * @brief Initializes the global configuration structure.
 *
 * This function initializes the global configuration by setting up the firmware event
 * group and MQTT topics for "temperature" and "humidity". It ensures that memory
 * resources are available and initializes each MQTT topic with a unique queue for
 * sensor data. It also handles error cases for invalid arguments, memory allocation
 * failures, and topic initialization failures.
 *
 * @param config Pointer to the global configuration structure to be initialized.
 *
 * @return ESP_OK if the initialization is successful. Otherwise, returns one of the following error codes:
 *         - ESP_ERR_INVALID_ARG if the config pointer is NULL.
 *         - ESP_ERR_NO_MEM if memory allocation fails at any step.
 */
kernel_error_st global_events_initialize(global_events_st *config) {
    if (config == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    config->firmware_event_group = xEventGroupCreate();

    if (config->firmware_event_group == NULL) {
        return KERNEL_ERROR_NULL;
    }

    return KERNEL_ERROR_NONE;
}