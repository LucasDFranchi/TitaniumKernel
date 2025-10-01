#pragma once

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/events/events_definition.h"
#include "kernel/tasks/interface/task_interface.h"

/**
 * @brief Adds a task to the task queue.
 *
 * This function attempts to enqueue a task into the task queue.
 * It verifies that the task pointer is not NULL and that there is available space in the queue.
 *
 * @param task Pointer to the task_interface_st structure representing the task.
 * @return
 *   - `KERNEL_SUCCESS` if the task was successfully added.
 *   - `KERNEL_ERROR_NULL` if the task pointer is NULL.
 *   - `KERNEL_ERROR_TASK_FULL` if the task queue is full.
 *
 * @note This function is designed to run before tasks are initialized,
 *       so it does not require a mutex for thread safety.
 */
kernel_error_st task_handler_enqueue_task(task_interface_st *task);

/**
 * @brief Starts all tasks in the task queue.
 *
 * This function iterates through the queued tasks and starts each one using FreeRTOS's `xTaskCreate()`.
 * If a task in the queue is NULL, it is skipped.
 *
 * @return
 *   - `KERNEL_SUCCESS` if all tasks were successfully started.
 *   - `KERNEL_ERROR_FAIL` if any task creation fails.
 *
 * @note This function should only be called after all tasks have been queued.
 */
kernel_error_st task_handler_start_queued_tasks(void);

/**
 * @brief Enqueue and start a task in the task manager.
 *
 * This function performs two steps:
 *   1. Adds the provided task to the global task queue.
 *   2. Immediately creates the corresponding FreeRTOS task and updates
 *      the task's handle in the task structure.
 *
 * @param task Pointer to the task descriptor structure to attach.
 *
 * @return KERNEL_SUCCESS       Task successfully enqueued and created.
 * @return KERNEL_ERROR_INVALID_ARG Null pointer passed as task.
 * @return KERNEL_ERROR_TASK_FULL  Task queue is full; cannot enqueue task.
 * @return KERNEL_ERROR_TASK_INIT  FreeRTOS task creation failed.
 *
 * @note The task handle in the structure will be NULL if task creation fails.
 *       Make sure the task structure remains valid in memory for the lifetime
 *       of the created FreeRTOS task.
 */
kernel_error_st task_handler_attach_task(task_interface_st *task);

/**
 * @brief Retrieves the current count of tasks in the task queue.
 *
 * This function returns the number of tasks currently in the queue by checking
 * the `enqued_task_index`. It does not remove or modify any tasks.
 *
 * @return The number of tasks currently queued.
 *
 * @note This function simply returns the value of `enqued_task_index` and does
 *       not perform any synchronization, so the value might change if other
 *       operations modify the queue concurrently.
 */
int task_handler_get_task_count(void);

/**
 * @brief Get the stack high-water mark for a specific task.
 *
 * Retrieves the minimum amount of stack space that has remained
 * since the specified task started execution.
 *
 * @param index Index of the task in the enqueued task list.
 * @return The high-water mark value, in words.
 */
UBaseType_t task_handler_get_highwater(size_t index);

/**
 * @brief Get the name of a specific task.
 *
 * Returns a pointer to the null-terminated string containing
 * the name of the specified task.
 *
 * @param index Index of the task in the enqueued task list.
 * @return Pointer to the task name string (read-only).
 */
const char *task_handler_get_task_name(size_t index);
