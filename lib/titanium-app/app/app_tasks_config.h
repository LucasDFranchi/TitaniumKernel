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
