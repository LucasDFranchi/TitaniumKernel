#include "task_handler.h"

#include "kernel/logger/logger.h"

#define TASK_LIST_SIZE 20

static const char *TAG                                     = "Task Manager";
static task_interface_st *enqued_task_list[TASK_LIST_SIZE] = {0};
static int enqueued_task_index                               = 0;

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
kernel_error_st task_handler_enqueue_task(task_interface_st *task) {
    if (task == NULL) {
        return KERNEL_ERROR_INVALID_ARG;
    }
    if (enqueued_task_index >= TASK_LIST_SIZE) {
        return KERNEL_ERROR_TASK_FULL;
    }

    enqued_task_list[enqueued_task_index] = task;
    enqueued_task_index++;

    return KERNEL_SUCCESS;
}

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
kernel_error_st task_handler_start_queued_tasks(void) {
    for (uint8_t i = 0; i < enqueued_task_index; i++) {
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

    return KERNEL_SUCCESS;
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
 * @return KERNEL_SUCCESS       Task successfully enqueued and created.
 * @return KERNEL_ERROR_INVALID_ARG Null pointer passed as task.
 * @return KERNEL_ERROR_TASK_FULL  Task queue is full; cannot enqueue task.
 * @return KERNEL_ERROR_TASK_INIT  FreeRTOS task creation failed.
 *
 * @note The task handle in the structure will be NULL if task creation fails.
 *       Make sure the task structure remains valid in memory for the lifetime
 *       of the created FreeRTOS task.
 */
kernel_error_st task_handler_attach_task(task_interface_st *task) {
    if (task == NULL) {
        return KERNEL_ERROR_NULL;
    }

    BaseType_t result = xTaskCreate(
        task->task_execute,
        task->name,
        task->stack_size,
        task->arg,
        task->priority,
        &task->handle);

    if (result != pdPASS) {
        logger_print(ERR, TAG, "Failed to create task: %s", task->name);
        return KERNEL_ERROR_TASK_INIT;
    }

    kernel_error_st err = task_handler_enqueue_task(task);

    if (err != KERNEL_SUCCESS) {
        return err;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Retrieves the current count of tasks in the task queue.
 *
 * This function returns the number of tasks currently in the queue by checking
 * the `enqueued_task_index`. It does not remove or modify any tasks.
 *
 * @return The number of tasks currently queued.
 *
 * @note This function simply returns the value of `enqueued_task_index` and does
 *       not perform any synchronization, so the value might change if other
 *       operations modify the queue concurrently.
 */
int task_handler_get_task_count(void) {
    return enqueued_task_index;
}

/**
 * @brief Get the stack high-water mark for a specific task.
 *
 * Retrieves the minimum amount of stack space that has remained
 * since the specified task started execution.
 *
 * @param index Index of the task in the enqueued task list.
 * @return The high-water mark value, in words.
 */
UBaseType_t task_handler_get_highwater(size_t index) {
    if (enqued_task_list[index]->handle == NULL) {
        return 0;
    }
    return uxTaskGetStackHighWaterMark(enqued_task_list[index]->handle);
}

/**
 * @brief Get the name of a specific task.
 *
 * Returns a pointer to the null-terminated string containing
 * the name of the specified task.
 *
 * @param index Index of the task in the enqueued task list.
 * @return Pointer to the task name string (read-only).
 */
const char *task_handler_get_task_name(size_t index) {
    return enqued_task_list[index]->name;
}
