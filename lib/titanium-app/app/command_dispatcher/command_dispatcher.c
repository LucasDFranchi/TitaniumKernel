/**
 * @file command_dispatcher.c
 * @brief Dispatches and handles incoming device commands.
 *
 * This module processes commands received by the device, invoking
 * the appropriate logic and generating structured responses.
 */

#include "command_dispatcher.h"

#include "stddef.h"

#include "kernel/logger/logger.h"

#include "app/sensors/sensor_manager.h"

/* Application Global Variables */
static const char* TAG = "Command Dispatcher";  ///< Tag used for logging.

/**
 * @brief Processes the CMD_SET_CALIBRATION command.
 *
 * This command adjusts the calibration parameters (gain and offset)
 * for a specific sensor. If the sensor index is invalid, an error is returned.
 *
 * @param command Pointer to the parsed command structure.
 * @param command_response Pointer to the response structure to populate.
 * @return kernel_error_st Result of calibration (success or failure reason).
 */
kernel_error_st process_set_calibration_command(command_st* command, command_response_st* command_response) {
    kernel_error_st result = KERNEL_ERROR_NONE;

    if ((command == NULL) || (command_response == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    cmd_set_calibration_st cmd = command->command_u.set_calibration;
    result                     = sensor_calibrate(cmd.sensor_index, cmd.offset, cmd.gain);

    command_response->command_index                                = command->command_index;
    command_response->command_u.cmd_sensor_response.sensor_index   = cmd.sensor_index;
    command_status_et command_status                               = result == KERNEL_ERROR_NONE ? COMMAND_SUCCESS : COMMAND_CALIBRATION_FAIL;
    command_response->command_u.cmd_sensor_response.command_status = command_status;
    command_response->command_u.cmd_sensor_response.sensor_type    = sensor_get_type(cmd.sensor_index);
    command_response->command_u.cmd_sensor_response.gain           = cmd.gain;
    command_response->command_u.cmd_sensor_response.offset         = cmd.offset;

    return result;
}

/**
 * @brief Dispatches a command to the appropriate handler.
 *
 * This function determines the command type and invokes the
 * corresponding handler to process the command and populate a response.
 *
 * @param command Pointer to the parsed command structure.
 * @param command_response Pointer to the structure where the response will be written.
 * @return kernel_error_st Result of command processing.
 */
kernel_error_st process_command(command_st* command, command_response_st* command_response) {
    kernel_error_st result = KERNEL_ERROR_NONE;

    if ((command == NULL) || (command_response == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    switch (command->command_index) {
        case CMD_SET_CALIBRATION: {
            result = process_set_calibration_command(command, command_response);
            break;
        }
        default:
            result = KERNEL_ERROR_INVALID_COMMAND;
    }

    return result;
}

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
void handle_incoming_command(QueueHandle_t command_queue, QueueHandle_t command_response_queue) {
    command_st command                   = {0};
    command_response_st command_response = {0};

    if (xQueueReceive(command_queue, &command, pdMS_TO_TICKS(100)) == pdPASS) {
        if (process_command(&command, &command_response) != KERNEL_ERROR_NONE) {
            logger_print(ERR, TAG, "Failed to process incoming command!");
        }

        if (xQueueSend(command_response_queue, &command_response, pdMS_TO_TICKS(100)) != pdPASS) {
            logger_print(ERR, TAG, "Failed to send command response to queue");
        }
    }
}