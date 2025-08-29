/**
 * @file task_handler_config.h
 * @brief Task manager initialization structures and configuration macros.
 *
 * This header defines the initialization structures for the Sensor Manager,
 * Command Manager, and Health Manager tasks in the system. It also provides
 * the priority, stack size, and task name macros for each task.
 */

#pragma once

#include "kernel/inter_task_communication/inter_task_communication.h"

/**
 * @brief Initialization structure for the Sensor Manager task.
 *
 * Contains the queue handle used for inter-task communication with
 * the Sensor Manager.
 */
typedef struct sensor_manager_init_s {
    QueueHandle_t sensor_manager_queue; /**< Queue handle for sensor manager messages */
} sensor_manager_init_st;

/**
 * @brief Initialization structure for the Command Manager task.
 *
 * Contains the queue handles used for inter-task communication with
 * the Command Manager for different command types.
 */
typedef struct command_manager_init_s {
    QueueHandle_t target_command_queue;    /**< Queue for targeted commands */
    QueueHandle_t broadcast_command_queue; /**< Queue for broadcast commands */
    QueueHandle_t response_command_queue;  /**< Queue for command responses */
} command_manager_init_st;

/**
 * @brief Initialization structure for the Health Manager task.
 *
 * Contains the queue handle used for inter-task communication with
 * the Health Manager.
 */
typedef struct health_manager_init_s {
    QueueHandle_t health_manager_queue; /**< Queue handle for health manager messages */
} health_manager_init_st;

/** @name Sensor Manager Task Configuration */
/** @{ */
#define SENSOR_MANAGER_TASK_PRIORITY 6
#define SENSOR_MANAGER_TASK_STACK_SIZE (2048 * 2)
#define SENSOR_MANAGER_TASK_NAME "Sensor Manager"
/** @} */

/** @name Command Manager Task Configuration */
/** @{ */
#define COMMAND_MANAGER_TASK_PRIORITY 6
#define COMMAND_MANAGER_TASK_STACK_SIZE (2048 * 2)
#define COMMAND_MANAGER_TASK_NAME "Command Manager"
/** @} */

/** @name Health Manager Task Configuration */
/** @{ */
#define HEALTH_MANAGER_TASK_PRIORITY 2
#define HEALTH_MANAGER_TASK_STACK_SIZE (2048)
#define HEALTH_MANAGER_TASK_NAME "Health Manager"
/** @} */
