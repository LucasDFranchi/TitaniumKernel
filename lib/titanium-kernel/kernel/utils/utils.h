/**
 * @file utils.h
 * @brief Utility functions for the ESP32 platform.
 *
 * This file contains declarations of helper functions to simplify common tasks
 * on the ESP32, such as time management and formatting.
 *
 * The utility functions provided are designed to be modular and reusable,
 * enhancing code maintainability and reducing redundancy across projects.
 */
#ifndef UTILS_H
#define UTILS_H


#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

/**
 * @brief Validate the integrity of the global structures.
 *
 * This function checks if the provided `global_structures` pointer is valid
 * and verifies that its embedded sub-structures (`global_events` and `global_queues`)
 * have their critical members properly initialized (i.e., non-NULL).
 *
 * @param global_structures Pointer to the global_structures_st instance to validate.
 * @return KERNEL_SUCCESS if all validations pass.
 * @return KERNEL_ERROR_NULL if `global_structures` is NULL or any required member inside the
 *         embedded structures is NULL.
 */
kernel_error_st validate_global_structure(global_structures_st* global_structures);

#endif  // UTILS_H
