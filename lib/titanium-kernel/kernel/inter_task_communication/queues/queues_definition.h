#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "kernel/error/error_num.h"

typedef struct global_queues_s {
    QueueHandle_t mqtt_topic_queue;   ///< Queue for handling MQTT topics and messages.
    QueueHandle_t credentials_queue;  ///< Queue for handling Credentials for WiFi or other services.
} global_queues_st;

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
kernel_error_st global_queues_initialize(global_queues_st *config);