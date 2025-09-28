// #include "queues_definition.h"

// #include "kernel/error/error_num.h"
// #include "kernel/inter_task_communication/inter_task_communication.h"

// #include "app/app_extern_types.h"

// #define MQTT_QUEUE_SIZE 10        ///< Size of the MQTT queue for handling messages.
// #define NETWORK_QUEUE_SIZE 5      ///< Size of the queue for network bridge devices.
// #define CREDENTIALS_QUEUE_SIZE 1  ///< Size of the credentials queue for handling WiFi or other service credentials.

// /**
//  * @brief Initialize FreeRTOS queues used for inter-task communication.
//  *
//  * This function creates and configures queues for various system components,
//  * allowing safe message passing between tasks. It initializes queues for
//  * network bridge events, MQTT bridge messages, and credential updates.
//  *
//  * @param[out] config Pointer to the global_queues_st structure to initialize.
//  * @return KERNEL_SUCCESS on success, or an error code if queue creation fails.
//  */
// kernel_error_st global_queues_initialize(global_queues_st *config) {
//     if (config == NULL) {
//         return KERNEL_ERROR_INVALID_ARG;
//     }

//     config->network_bridge_queue = xQueueCreate(NETWORK_QUEUE_SIZE, sizeof(network_bridge_st));
//     if (config->network_bridge_queue == NULL) {
//         return KERNEL_ERROR_NO_MEM;
//     }

//     config->mqtt_bridge_queue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(mqtt_bridge_st));
//     if (config->mqtt_bridge_queue == NULL) {
//         return KERNEL_ERROR_NO_MEM;
//     }

//     config->credentials_queue = xQueueCreate(CREDENTIALS_QUEUE_SIZE, sizeof(credentials_st));
//     if (config->credentials_queue == NULL) {
//         return KERNEL_ERROR_NO_MEM;
//     }

//     return KERNEL_SUCCESS;
// }