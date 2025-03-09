#ifndef MQTT_CLIENT_TASK_H
#define MQTT_CLIENT_TASK_H

/**
 * @file mqtt_client_task.h
 * @brief MQTT Client interface for handling web requests on the ESP32.
 */
#include "inter_task_communication/events/events_definition.h"
#include "inter_task_communication/iot/mqtt/mqtt_client_external_types.h"
#include "tasks/manager/task_manager.h"
#include "error/error_num.h"

/**
 * @brief Main execution function for the MQTT Client.
 *
 * This function initializes and starts the MQTT Client, enabling the ESP32 to
 * handle incoming requests, from the broker. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void mqtt_client_task_execute(void* pvParameters);

/**
 * @brief Enqueues an MQTT topic for processing.
 *
 * This function attempts to enqueue the provided MQTT topic into the internal topic queue.
 * It performs several checks before enqueuing, including verifying that the topic is not null,
 * ensuring that the queue handle is valid, and checking that the necessary JSON encoding/decoding
 * functions are available based on the data direction (PUBLISH or SUBSCRIBE).
 * It also checks the size of the data to ensure it does not exceed the maximum allowed payload size.
 *
 * @param mqtt_topic A pointer to the mqtt_topic_st structure representing the MQTT topic to be enqueued.
 * @return kernel_error_st Returns KERNEL_ERROR_NONE on success, or an error code indicating the failure reason:
 *         - KERNEL_ERROR_NULL if the mqtt_topic pointer is NULL.
 *         - KERNEL_ERROR_QUEUE_NULL if the queue handle is NULL.
 *         - KERNEL_ERROR_INVALID_INTERFACE if the necessary encoding/decoding functions are not provided.
 *         - KERNEL_ERROR_INVALID_SIZE if the data size exceeds the allowed maximum.
 */
kernel_error_st mqtt_client_topic_enqueue(mqtt_topic_st* mqtt_topic);

#endif /* MQTT_CLIENT_TASK_H */