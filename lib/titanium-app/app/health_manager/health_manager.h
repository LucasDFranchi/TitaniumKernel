#pragma once

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"
#include "app/app_extern_types.h"

/**
 * @file health_manager.h
 * @brief Health Manager task interface and related definitions.
 *
 * This header declares the main Health Manager task loop and
 * provides definitions for initializing and managing the health LED.
 * The Health Manager monitors system health and provides a visual
 * heartbeat via the LED.
 */

/**
 * @brief Entry point for the Health Manager task.
 *
 * This function should be started as a FreeRTOS task. It initializes
 * the health LED GPIO and toggles it at a fixed interval. Additional
 * health monitoring features can be added in future versions.
 *
 * @param args Pointer to initialization parameters (currently unused, reserved for future use).
 */
void health_manager_loop(void* args);
