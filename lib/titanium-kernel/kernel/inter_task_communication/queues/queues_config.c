#include "queues_definition.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/iot/mqtt/mqtt_client_external_types.h"
#include "kernel/inter_task_communication/iot/network/network_external_types.h"

#include "app/app_extern_types.h"

#define MQTT_QUEUE_SIZE 10        ///< Size of the MQTT queue for handling messages.
#define CREDENTIALS_QUEUE_SIZE 1  ///< Size of the credentials queue for handling WiFi or other service credentials.

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
kernel_error_st global_queues_initialize(global_queues_st *config) {
    if (config == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    config->mqtt_bridge_queue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(mqtt_bridge_st));
    if (config->mqtt_bridge_queue == NULL) {
        return KERNEL_ERROR_NO_MEM;
    }

    config->credentials_queue = xQueueCreate(CREDENTIALS_QUEUE_SIZE, sizeof(credentials_st));
    if (config->credentials_queue == NULL) {
        return KERNEL_ERROR_NO_MEM;
    }

    return KERNEL_ERROR_NONE;
}