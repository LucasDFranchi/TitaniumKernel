#ifndef EVENTS_DEFINITION_H
#define EVENTS_DEFINITION_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/**
 * @brief FreeRTOS event group for signaling system events.
 *
 * This event group uses individual bits to signal different system events:
 * - WIFI_CONNECTED_STA: Indicates that the device has successfully connected to a network in station (STA) mode.
 * - WIFI_CONNECTED_AP: Indicates that the device has successfully established a network in access point (AP) mode.
 * - TIME_SYNCED: Indicates that the system time has been successfully synchronized with an external time source.
 *
 */
#define WIFI_CONNECTED_STA BIT0
#define WIFI_CONNECTED_AP BIT1
#define TIME_SYNCED BIT2

typedef struct global_events_s {
    EventGroupHandle_t firmware_event_group;  ///< Event group for signaling system status and events.
    QueueHandle_t mqtt_topic_queue;         ///< Queue for handling MQTT topics and messages.
} global_events_st;

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
esp_err_t global_events_initialize(global_events_st *config);

#endif /* EVENTS_DEFINITION_H */