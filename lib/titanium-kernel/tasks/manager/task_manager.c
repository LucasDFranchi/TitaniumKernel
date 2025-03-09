#include "task_manager.h"

#include "logger/logger.h"

#define TASK_QUEUE_SIZE 10

static const char *TAG                                = "TASK_MANAGER";
static task_interface_st *task_queue[TASK_QUEUE_SIZE] = {0};
static int task_queue_index                           = 0;

/**
 * @brief Adds a task to the task queue.
 *
 * This function attempts to enqueue a task into the task queue.
 * It verifies that the task pointer is not NULL and that there is available space in the queue.
 *
 * @param task Pointer to the task_interface_st structure representing the task.
 * @return
 *   - `KERNEL_ERROR_NONE` if the task was successfully added.
 *   - `KERNEL_ERROR_NULL` if the task pointer is NULL.
 *   - `KERNEL_ERROR_TASK_FULL` if the task queue is full.
 *
 * @note This function is designed to run before tasks are initialized,
 *       so it does not require a mutex for thread safety.
 */
kernel_error_st task_manager_enqueue_task(task_interface_st *task) {
    if (task == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (task_queue_index >= TASK_QUEUE_SIZE) {
        return KERNEL_ERROR_TASK_FULL;
    }

    task_queue[task_queue_index] = task;
    task_queue_index++;

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Starts all tasks in the task queue.
 *
 * This function iterates through the queued tasks and starts each one using FreeRTOS's `xTaskCreate()`.
 * If a task in the queue is NULL, it is skipped.
 *
 * @return
 *   - `KERNEL_ERROR_NONE` if all tasks were successfully started.
 *   - `KERNEL_ERROR_FAIL` if any task creation fails.
 *
 * @note This function should only be called after all tasks have been queued.
 */
kernel_error_st task_manager_start_queued_tasks(void) {
    for (uint8_t i = 0; i < task_queue_index; i++) {
        if (task_queue[i] == NULL) {
            continue;
        }

        BaseType_t result = xTaskCreate(
            task_queue[i]->task_execute,
            task_queue[i]->name,
            task_queue[i]->stack_size,
            task_queue[i]->arg,
            task_queue[i]->priority,
            task_queue[i]->handle);

        if (result != pdPASS) {
            logger_print(ERR, TAG, "Failed to create task: %s", task_queue[i]->name);
            return KERNEL_ERROR_TASK_INIT;
        }
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Retrieves the current count of tasks in the task queue.
 *
 * This function returns the number of tasks currently in the queue by checking
 * the `task_queue_index`. It does not remove or modify any tasks.
 *
 * @return The number of tasks currently queued.
 *
 * @note This function simply returns the value of `task_queue_index` and does
 *       not perform any synchronization, so the value might change if other
 *       operations modify the queue concurrently.
 */
int task_manager_get_task_count(void) {
    return task_queue_index;
}
