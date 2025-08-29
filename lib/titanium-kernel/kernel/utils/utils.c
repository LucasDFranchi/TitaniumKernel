#include "utils.h"

#include "esp_err.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/**
 * @brief Validate the integrity of the global structures.
 *
 * This function checks if the provided `global_structures` pointer is valid
 * and verifies that its embedded sub-structures (`global_events` and `global_queues`)
 * have their critical members properly initialized (i.e., non-NULL).
 *
 * @param global_structures Pointer to the global_structures_st instance to validate.
 * @return KERNEL_ERROR_NONE if all validations pass.
 * @return KERNEL_ERROR_NULL if `global_structures` is NULL or any required member inside the
 *         embedded structures is NULL.
 */
kernel_error_st validate_global_structure(global_structures_st* global_structures) {
    if (global_structures == NULL) {
        return KERNEL_ERROR_NULL;
    }
    global_queues_st* global_queues = &global_structures->global_queues;
    global_events_st* global_events = &global_structures->global_events;
    
    if (global_events->firmware_event_group == NULL) {
        return KERNEL_ERROR_NULL;
    }
    
    if ((global_queues->credentials_queue == NULL) || (global_queues->mqtt_bridge_queue == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    return KERNEL_ERROR_NONE;
}

