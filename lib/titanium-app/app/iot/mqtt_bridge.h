#pragma once

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

/**
 * @brief Structure used to initialize the MQTT bridge.
 *
 * This struct groups all input data required for initializing the MQTT bridge,
 * including a pointer to the bridge instance, the list of MQTT topics, and the number of topics.
 * 
 * It allows the bridge to own its internal state without exposing topic details
 * to other components like the MQTT task.
 */
typedef struct mqtt_bridge_init_struct_s {
    mqtt_bridge_st *mqtt_bridge;      ///< Pointer to the MQTT bridge instance to initialize.
    mqtt_topic_st *topics;            ///< Pointer to the array of runtime MQTT topics.
    size_t topic_count;               ///< Number of topics in the array.
} mqtt_bridge_init_struct_st;

/**
 * @brief Initializes the MQTT bridge and sets the device unique ID.
 *
 * This function initializes the provided mqtt_bridge_st instance by setting
 * the fetch function pointer and copying the device unique ID into an internal buffer.
 * It also registers all MQTT topics provided in the init struct.
 *
 * @param mqtt_bridge_init_struct Pointer to the MQTT bridge init struct containing bridge instance, topics, and topic count.
 * @param device_unique_id Null-terminated string containing the device unique identifier (max 12 chars).
 *
 * @return KERNEL_SUCCESS on successful initialization.
 * @return KERNEL_ERROR_NULL if any pointer arguments are NULL.
 * @return KERNEL_ERROR_FORMATTING if the unique ID string is too long to fit in the internal buffer.
 * @return ESP_FAIL if any topic registration fails.
 */
kernel_error_st mqtt_bridge_initialize(mqtt_bridge_init_struct_st *mqtt_bridge_init_struct);
