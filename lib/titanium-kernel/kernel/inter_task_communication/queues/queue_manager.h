#pragma once

/*
 * queue_manager.h
 *
 * Simple Queue Manager for FreeRTOS (ESP-IDF compatible)
 *
 * This module allows creating, registering, and retrieving FreeRTOS queues
 * using user-defined IDs. Queues are managed internally in a static registry
 * protected by a FreeRTOS mutex.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "kernel/error/error_num.h"

/**
 * @enum
 * @brief Kernel and bridge queue identifiers.
 *
 * @var NETWORK_BRIDGE_QUEUE_ID Queue for the network bridge.
 * @var MQTT_BRIDGE_QUEUE_ID Queue for the MQTT bridge.
 * @var CREDENTIALS_QUEUE_ID Queue for credentials handling.
 * @var LAST_KERNEL_QUEUE_ID Last reserved kernel queue ID (app queues start after this).
 */
enum {
    NETWORK_BRIDGE_QUEUE_ID = 0,
    MQTT_BRIDGE_QUEUE_ID    = 1,
    CREDENTIALS_QUEUE_ID    = 2,
    LAST_KERNEL_QUEUE_ID    = 10
};

/**
 * @brief Initialize the Queue Manager.
 *
 * This function must be called before using any other Queue Manager functions.
 *
 * @return kernel_error_st
 *   - KERNEL_SUCCESS on success.
 *   - KERNEL_ERROR_FAILED_TO_ALLOCATE_MUTEX if mutex creation fails.
 */
kernel_error_st queue_manager_init(void);

/**
 * @brief Create and register a FreeRTOS queue with the Queue Manager.
 *
 * Creates a new queue with the specified length and item size and associates
 * it with the given user-defined ID. If the registry is full, the queue is
 * deleted and the function fails.
 *
 * @param[in] index         User-defined ID for the queue.
 * @param[in] queue_length  Maximum number of items the queue can hold.
 * @param[in] item_size     Size (in bytes) of each queue item.
 *
 * @return kernel_error_st
 *   - KERNEL_SUCCESS on success.
 *   - KERNEL_ERROR_INVALID_ARG if queue_length or item_size is zero.
 *   - KERNEL_ERROR_MANAGER_NOT_INITIALIZED if queue_manager_init() was not called.
 *   - KERNEL_ERROR_FAILED_TO_LOCK if mutex could not be acquired.
 *   - KERNEL_ERROR_FAIL if queue creation failed or registry is full.
 */
kernel_error_st queue_manager_register(uint8_t index, UBaseType_t queue_length, UBaseType_t item_size);

/**
 * @brief Retrieve a registered queue by its user-defined ID.
 *
 * @param[in] index  The ID of the queue to retrieve.
 *
 * @return QueueHandle_t
 *   - Queue handle if found.
 *   - NULL if no queue with the given ID is registered.
 */
QueueHandle_t queue_manager_get(uint8_t index);
