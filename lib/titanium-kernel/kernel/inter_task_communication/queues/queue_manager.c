#include "queue_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "kernel/error/error_num.h"
#include "kernel/logger/logger.h"

#define QUEUE_MANAGER_MAX_QUEUES 16 /**< Maximum number of queues that can be registered */

typedef struct queue_manager_entry_s {
    int index;            /**< User-assigned queue ID */
    QueueHandle_t handle; /**< FreeRTOS queue handle associated with this ID */
} queue_manager_entry_st;

static const char *TAG                                             = "Queue Manager";    /**< Logging tag used for Queue Manager messages */
static const TickType_t QUEUE_MANAGER_MUTEX_TIMEOUT                = pdMS_TO_TICKS(100); /**< Timeout in ticks when attempting to acquire the registry mutex (default: 100 ms) */
static queue_manager_entry_st s_registry[QUEUE_MANAGER_MAX_QUEUES] = {0};                /**< Static array holding all registered queue entries */
static bool is_initialized                                         = false;              /**< Flag indicating whether the Queue Manager has been initialized */
static SemaphoreHandle_t s_registry_lock;                                                /**< FreeRTOS mutex protecting access to the queue registry */

/**
 * @brief Initialize the Queue Manager.
 *
 * This function sets up the internal mutex used to protect the queue registry.
 * It must be called before any other Queue Manager functions (e.g., register or get).
 *
 * Behavior:
 *   - Creates a FreeRTOS mutex for registry protection.
 *   - If mutex creation fails, the function logs an error and returns a failure code.
 *   - On success, marks the manager as initialized.
 *
 * @return kernel_error_st:
 *   - KERNEL_SUCCESS if the manager is successfully initialized.
 *   - KERNEL_ERROR_FAILED_TO_ALLOCATE_MUTEX if mutex creation fails.
 */

kernel_error_st queue_manager_init(void) {
    s_registry_lock = xSemaphoreCreateMutex();

    if (s_registry_lock == NULL) {
        logger_print(ERR, TAG, "Failed to create registry mutex!");
        return KERNEL_ERROR_FAILED_TO_ALLOCATE_MUTEX;
    }

    is_initialized = true;
    return KERNEL_SUCCESS;
}

/**
 * @brief Create and register a FreeRTOS queue in the manager.
 *
 * Creates a new FreeRTOS queue with the specified length and item size,
 * and associates it with a user-defined ID. The queue handle is stored
 * internally and can later be retrieved using queue_manager_get().
 *
 * Behavior:
 *   - If the Queue Manager has not been initialized, the call fails.
 *   - If the queue parameters are invalid (zero length or item size), the call fails.
 *   - If the registry is full, the created queue is deleted and the call fails.
 *   - If a queue with the same ID already exists, it is not overwritten; a new
 *     queue is still created, but not stored (to prevent accidental override).
 *
 * @param[in] index         The ID to assign to this queue.
 * @param[in] queue_length  Maximum number of items the queue can hold.
 * @param[in] item_size     Size (in bytes) of each item in the queue.
 *
 * @return kernel_error_st:
 *   - KERNEL_SUCCESS on success.
 *   - KERNEL_ERROR_INVALID_ARG if parameters are invalid.
 *   - KERNEL_ERROR_MANAGER_NOT_INITIALIZED if init was not called.
 *   - KERNEL_ERROR_FAILED_TO_LOCK if the mutex could not be acquired.
 *   - KERNEL_ERROR_FAIL if queue creation failed or registry is full.
 */
kernel_error_st queue_manager_register(uint8_t index, UBaseType_t queue_length, UBaseType_t item_size) {
    if ((queue_length) == 0 || (item_size == 0)) {
        logger_print(ERR, TAG, "Invalid queue parameters: length=%d, item_size=%d", queue_length, item_size);
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (!is_initialized) {
        logger_print(ERR, TAG, "Queue Manager not initialized!");
        return KERNEL_ERROR_MANAGER_NOT_INITIALIZED;
    }

    if (xSemaphoreTake(s_registry_lock, QUEUE_MANAGER_MUTEX_TIMEOUT) != pdTRUE) {
        logger_print(ERR, TAG, "Failed to acquire registry lock");
        return KERNEL_ERROR_FAILED_TO_LOCK;
    }

    // Create the queue
    QueueHandle_t queue_handle = xQueueCreate(queue_length, item_size);
    if (queue_handle == NULL) {
        logger_print(ERR, TAG, "Failed to create queue ID=%d", index);
        xSemaphoreGive(s_registry_lock);
        return KERNEL_ERROR_FAIL;
    }

    for (int i = 0; i < QUEUE_MANAGER_MAX_QUEUES; i++) {
        if (s_registry[i].handle == NULL) {
            s_registry[i].index  = index;
            s_registry[i].handle = queue_handle;
            logger_print(DEBUG, TAG, "Registered queue ID=%d at slot %d", index, i);
            xSemaphoreGive(s_registry_lock);
            return KERNEL_SUCCESS;
        }
    }

    vQueueDelete(queue_handle);
    xSemaphoreGive(s_registry_lock);
    logger_print(ERR, TAG, "Registry full, cannot register ID=%d", index);
    return KERNEL_ERROR_FAIL;
}

/**
 * @brief Retrieve a queue handle by its registered ID.
 *
 * Searches the registry for a queue associated with the given ID.
 *
 * @param[in] index   The ID of the queue to look up.
 *
 * @return QueueHandle_t:
 *   - Queue handle if found.
 *   - NULL if not found.
 */
QueueHandle_t queue_manager_get(uint8_t index) {
    QueueHandle_t handle = NULL;

    if (!is_initialized || s_registry_lock == NULL) {
        logger_print(ERR, TAG, "Queue Manager not initialized!");
        return NULL;
    }

    if (xSemaphoreTake(s_registry_lock, QUEUE_MANAGER_MUTEX_TIMEOUT) != pdTRUE) {
        logger_print(ERR, TAG, "Failed to acquire registry lock");
        return NULL;
    }

    for (int i = 0; i < QUEUE_MANAGER_MAX_QUEUES; i++) {
        if (s_registry[i].handle && s_registry[i].index == index) {
            handle = s_registry[i].handle;
            logger_print(DEBUG, TAG, "Found queue ID=%d at slot %d", index, i);
            break;
        }
    }

    if (handle == NULL) {
        logger_print(WARN, TAG, "Queue ID=%d not found", index);
    }

    xSemaphoreGive(s_registry_lock);

    return handle;
}