#ifndef TASK_INTERFACE_H
#define TASK_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Structure defining a task interface for FreeRTOS tasks.
 *
 * This structure provides a standardized way to define task parameters,
 * including name, stack size, priority, and the function to execute.
 */
typedef struct task_interface_s {
    const char *name;             /**< Name of the task */
    const uint32_t stack_size;    /**< Stack size allocated for the task */
    const uint32_t priority;      /**< Task priority in FreeRTOS */
    void (*task_execute)(void *); /**< Pointer to the task execution function */
    void *arg;                    /**< Pointer to the task argument */
    TaskHandle_t handle;          /**< Task handle for the task */
} task_interface_st;

#endif /* TASK_INTERFACE_H */
