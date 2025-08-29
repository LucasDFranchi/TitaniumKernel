#pragma once

/**
 * @file command_manager.h
 * @brief Interface for dispatching and handling application commands.
 *
 * Provides the declarations for command processing functions used to interpret
 * incoming commands and produce structured responses.
 */
#include "kernel/inter_task_communication/inter_task_communication.h"

#include "app/app_extern_types.h"

/**
 * @brief Main loop of the command manager task.
 *
 * Continuously processes incoming commands from both target and broadcast queues
 * and sends responses. Runs indefinitely with a delay between iterations.
 *
 * @param args Pointer to a command_manager_init_st structure containing queue handles.
 */
void command_manager_loop(void* args);
