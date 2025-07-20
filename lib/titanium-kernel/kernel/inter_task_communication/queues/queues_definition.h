#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "kernel/error/error_num.h"

typedef struct global_queues_s {
    QueueHandle_t network_bridge_queue;  ///< Queue to receive network bridge configuration and events.
    QueueHandle_t mqtt_bridge_queue;     ///< Queue to handle MQTT bridge messages and events.
    QueueHandle_t credentials_queue;     ///< Queue for handling credentials (e.g., Wi-Fi SSID and password) updates.
} global_queues_st;

/**
 * @brief Initialize FreeRTOS queues used for inter-task communication.
 *
 * This function creates and configures queues for various system components,
 * allowing safe message passing between tasks. It initializes queues for
 * network bridge events, MQTT bridge messages, and credential updates.
 *
 * @param[out] config Pointer to the global_queues_st structure to initialize.
 * @return KERNEL_ERROR_NONE on success, or an error code if queue creation fails.
 */
kernel_error_st global_queues_initialize(global_queues_st *config);