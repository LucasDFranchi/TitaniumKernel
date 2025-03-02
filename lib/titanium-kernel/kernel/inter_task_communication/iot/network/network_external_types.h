#ifndef NETWORK_EXTERNAL_TYPES_H
#define NETWORK_EXTERNAL_TYPES_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "kernel/inter_task_communication/inter_task_communication.h"
#include <stdint.h>

#define NETWORK_MAXIMUM_SSID_SIZE 32  ///< Defines the maximum length of an SSID string.
#define NETWORK_MAXIMUM_PASSWORD_SIZE 64  ///< Defines the maximum length of an password string.

/**
 * @brief Structure to hold WiFi credentials.
 *
 * This structure is used to store the SSID and password for WiFi connections.
 * It is designed to be used with FreeRTOS queues for inter-task communication.
 */
typedef struct credentials_s {
    char ssid[NETWORK_MAXIMUM_SSID_SIZE];
    char password[NETWORK_MAXIMUM_PASSWORD_SIZE];
} credentials_st;

#endif /* NETWORK_EXTERNAL_TYPES_H */
