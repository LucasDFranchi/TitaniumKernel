#ifndef INTER_TASK_COMMUNICATION_H
#define INTER_TASK_COMMUNICATION_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/events/events_definition.h"
#include "kernel/inter_task_communication/iot/mqtt/mqtt_client_external_types.h"
#include "kernel/inter_task_communication/queues/queues_definition.h"

typedef struct global_structures_s {
    global_queues_st global_queues;  ///< Pointer to the global queues structure.
    global_events_st global_events;  ///< Pointer to the global events structure.
} global_structures_st;

/**
 * @brief Structure defining the inter-task communication interface.
 *
 * This structure provides a standardized way to define inter-task communication parameters,
 * including the event group, queue, and task handle.
 */

#endif /* INTER_TASK_COMMUNICATION_H */
