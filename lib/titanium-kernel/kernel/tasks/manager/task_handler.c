#include "task_handler.h"

#include "kernel/logger/logger.h"

#define TASK_LIST_SIZE 20
//TODO: this is not a manager, this is a handler!

static const char *TAG                                     = "Task Manager";
static task_interface_st *enqued_task_list[TASK_LIST_SIZE] = {0};
static int enqued_task_index                               = 0;

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
kernel_error_st task_handler_enqueue_task(task_interface_st *task) {
    if (task == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }
    if (enqued_task_index >= TASK_LIST_SIZE) {
        return KERNEL_ERROR_TASK_FULL;
    }

    enqued_task_list[enqued_task_index] = task;
    enqued_task_index++;

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
kernel_error_st task_handler_start_queued_tasks(void) {
    for (uint8_t i = 0; i < enqued_task_index; i++) {
        if (enqued_task_list[i] == NULL) {
            continue;
        }

        BaseType_t result = xTaskCreate(
            enqued_task_list[i]->task_execute,
            enqued_task_list[i]->name,
            enqued_task_list[i]->stack_size,
            enqued_task_list[i]->arg,
            enqued_task_list[i]->priority,
            &enqued_task_list[i]->handle);

        if (result != pdPASS) {
            logger_print(ERR, TAG, "Failed to create task: %s", enqued_task_list[i]->name);
            return KERNEL_ERROR_TASK_INIT;
        }
    }

    return KERNEL_ERROR_NONE;
}

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
 * @return KERNEL_ERROR_NONE       Task successfully enqueued and created.
 * @return KERNEL_ERROR_INVALID_ARG Null pointer passed as task.
 * @return KERNEL_ERROR_TASK_FULL  Task queue is full; cannot enqueue task.
 * @return KERNEL_ERROR_TASK_INIT  FreeRTOS task creation failed.
 *
 * @note The task handle in the structure will be NULL if task creation fails.
 *       Make sure the task structure remains valid in memory for the lifetime
 *       of the created FreeRTOS task.
 */
kernel_error_st task_handler_attach_task(task_interface_st *task) {
    kernel_error_st err = task_handler_enqueue_task(task);;
    if (err != KERNEL_ERROR_NONE) {
        return err;
    }

    int current_index = enqued_task_index - 1;

    BaseType_t result = xTaskCreate(
        enqued_task_list[current_index]->task_execute,
        enqued_task_list[current_index]->name,
        enqued_task_list[current_index]->stack_size,
        enqued_task_list[current_index]->arg,
        enqued_task_list[current_index]->priority,
        &enqued_task_list[current_index]->handle);

    if (result != pdPASS) {
        logger_print(ERR, TAG, "Failed to create task: %s", enqued_task_list[current_index]->name);
        return KERNEL_ERROR_TASK_INIT;
    }

    return KERNEL_ERROR_NONE;
}

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
int task_handler_get_task_count(void) {
    return enqued_task_index;
}
