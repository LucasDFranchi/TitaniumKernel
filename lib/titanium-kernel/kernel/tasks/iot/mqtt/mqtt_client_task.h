#ifndef MQTT_CLIENT_TASK_H
#define MQTT_CLIENT_TASK_H

/**
 * @file mqtt_client_task.h
 * @brief MQTT Client interface for handling web requests on the ESP32.
 */
#include "kernel/inter_task_communication/events/events_definition.h"
#include "kernel/inter_task_communication/iot/mqtt/mqtt_client_external_types.h"
#include "kernel/tasks/manager/task_manager.h"
#include "kernel/error/error_num.h"

/**
 * @brief Main execution function for the MQTT Client.
 *
 * This function initializes and starts the MQTT Client, enabling the ESP32 to
 * handle incoming requests, from the broker. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void mqtt_client_task_execute(void* pvParameters);

#endif /* MQTT_CLIENT_TASK_H */