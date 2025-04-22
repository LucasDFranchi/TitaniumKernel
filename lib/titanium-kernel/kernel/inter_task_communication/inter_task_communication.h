#ifndef INTER_TASK_COMMUNICATION_H
#define INTER_TASK_COMMUNICATION_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/iot/mqtt/mqtt_client_external_types.h"

/**
 * @brief Structure defining the inter-task communication interface.
 *
 * This structure provides a standardized way to define inter-task communication parameters,
 * including the event group, queue, and task handle.
 */


#endif /* INTER_TASK_COMMUNICATION_H */
