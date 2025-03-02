#pragma once

/**
 * @file command_dispatcher.h
 * @brief Interface for dispatching and handling application commands.
 *
 * Provides the declarations for command processing functions used to interpret
 * incoming commands and produce structured responses.
 */

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

#include "app/app_extern_types.h"

/**
 * @brief Dispatches a command to the appropriate handler.
 *
 * Determines the command type from the input and invokes the corresponding handler
 * to process the command and populate a structured response.
 *
 * @param command Pointer to the parsed command structure.
 * @param command_response Pointer to the structure where the response will be written.
 * @return kernel_error_st Result of command processing (e.g., success, invalid command).
 */
kernel_error_st process_command(command_st* command, command_response_st* command_response);

/**
 * @brief Handles an incoming command from a queue and dispatches a response.
 *
 * This function attempts to read a command from the specified command queue.
 * If a command is received, it invokes the appropriate handler via
 * `process_command()` and sends the resulting response to the response queue.
 *
 * It logs an error if command processing fails or if the response cannot be enqueued.
 *
 * @param command_queue           FreeRTOS queue containing incoming commands.
 * @param command_response_queue  FreeRTOS queue to send command responses.
 */
void handle_incoming_command(QueueHandle_t command_queue, QueueHandle_t command_response_queue);
