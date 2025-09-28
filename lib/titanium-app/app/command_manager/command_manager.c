/**
 * @file command_manager.c
 * @brief Dispatches and handles incoming device commands.
 *
 * This module processes commands received by the device, invoking
 * the appropriate logic and generating structured responses.
 */
#include "command_manager.h"

#include "stddef.h"
#include "string.h"

#include "kernel/device/device_info.h"
#include "kernel/error/error_num.h"
#include "kernel/logger/logger.h"

#include "app/app_tasks_config.h"

/* Application Global Variables */

/**
 * @brief Tag used for logging purposes.
 */
static const char* TAG = "Command Manager";

/**
 * @brief Global initialization structure for the command manager.
 */
command_manager_init_st command_manager_init = {0};

/**
 * @brief Processes the CMD_SET_CALIBRATION command.
 *
 * Updates the calibration parameters (gain and offset) for a specific sensor.
 * If the sensor index is invalid or the command pointers are NULL, an error is returned.
 *
 * @param command Pointer to the parsed command structure containing calibration data.
 * @param command_response Pointer to the response structure to populate with status and updated values.
 * @return kernel_error_st Result of the calibration operation:
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_NULL if input pointers are NULL
 *         - KERNEL_ERROR_INVALID_SENSOR if sensor index is invalid
 *         - KERNEL_ERROR_CALIBRATION_FAIL if calibration fails
 */
kernel_error_st process_set_calibration_command(command_st* command, command_response_st* command_response) {
    kernel_error_st result = KERNEL_SUCCESS;

    if ((command == NULL) || (command_response == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    cmd_set_calibration_st cmd = command->command_u.set_calibration;
    result                     = sensor_calibrate(cmd.sensor_index, cmd.offset, cmd.gain);

    command_response->command_index  = CMD_SET_CALIBRATION;
    command_response->command_status = result == KERNEL_SUCCESS ? COMMAND_SUCCESS : COMMAND_CALIBRATION_FAIL;

    if (command_response->command_status == COMMAND_SUCCESS) {
        command_response->command_u.cmd_sensor_response.sensor_index = cmd.sensor_index;
        command_response->command_u.cmd_sensor_response.sensor_type  = sensor_get_type(cmd.sensor_index);
        command_response->command_u.cmd_sensor_response.gain         = cmd.gain;
        command_response->command_u.cmd_sensor_response.offset       = cmd.offset;
    }

    return result;
}

/**
 * @brief Processes the CMD_GET_SYSTEM_INFO command.
 *
 * Retrieves system information including device ID, IP address, uptime, and sensor calibration statuses.
 * Authentication is verified against hardcoded user credentials ("root"/"root").
 *
 * @param command Pointer to the parsed command structure containing authentication information.
 * @param command_response Pointer to the response structure to populate with system information.
 * @return kernel_error_st Result of system info retrieval:
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_NULL if input pointers are NULL
 *         - KERNEL_ERROR_INVALID_USER if username is incorrect
 *         - KERNEL_ERROR_INVALID_PASSWORD if password is incorrect
 *         - KERNEL_ERROR_INVALID_SIZE if device ID or IP address exceeds buffer size
 */
kernel_error_st process_get_system_info_command(command_st* command, command_response_st* command_response) {
    kernel_error_st result                = KERNEL_SUCCESS;
    static const char expected_user[]     = "root";
    static const char expected_password[] = "root";

    if ((command == NULL) || (command_response == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    cmd_get_system_info_st cmd = command->command_u.cmd_get_system_info;

    if (memcmp(cmd.user, expected_user, sizeof(expected_user)) != 0) {
        result = KERNEL_ERROR_INVALID_USER;
    }

    if (memcmp(cmd.password, expected_password, sizeof(expected_password)) != 0) {
        result = KERNEL_ERROR_INVALID_PASSWORD;
    }

    command_response->command_index  = CMD_GET_SYSTEM_INFO;
    command_response->command_status = result == KERNEL_SUCCESS ? COMMAND_SUCCESS : COMMAND_AUTHENTICATION_FAIL;

    if (command_response->command_status == COMMAND_SUCCESS) {
        size_t device_id_size = snprintf(command_response->command_u.cmd_system_info_response.device_id,
                                         sizeof(command_response->command_u.cmd_system_info_response.device_id),
                                         "%s",
                                         device_info_get_id());

        if (device_id_size >= sizeof(command_response->command_u.cmd_system_info_response.device_id)) {
            return KERNEL_ERROR_INVALID_SIZE;
        }

        size_t ip_address_size = snprintf(command_response->command_u.cmd_system_info_response.ip_address,
                                          sizeof(command_response->command_u.cmd_system_info_response.ip_address),
                                          "%s",
                                          device_info_get_ip_address());

        if (ip_address_size >= sizeof(command_response->command_u.cmd_system_info_response.ip_address)) {
            return KERNEL_ERROR_INVALID_SIZE;
        }

        command_response->command_u.cmd_system_info_response.uptime = device_info_get_uptime();

        for (int i = 0; i < NUM_OF_SENSORS; i++) {
            command_response->command_u.cmd_system_info_response.sensor_calibration_status[i].sensor_index = i;
            command_response->command_u.cmd_system_info_response.sensor_calibration_status[i].sensor_type  = sensor_get_type(i);
            command_response->command_u.cmd_system_info_response.sensor_calibration_status[i].gain         = sensor_get_gain(i);
            command_response->command_u.cmd_system_info_response.sensor_calibration_status[i].offset       = sensor_get_offset(i);
            command_response->command_u.cmd_system_info_response.sensor_calibration_status[i].state        = sensor_get_state(i);
        }
    }
    return result;
}

/**
 * @brief Dispatches a command to the appropriate handler.
 *
 * Determines the command type from command_index and calls the corresponding handler.
 *
 * @param command Pointer to the parsed command structure.
 * @param command_response Pointer to the response structure to populate.
 * @return kernel_error_st Result of command processing:
 *         - KERNEL_SUCCESS if processed successfully
 *         - KERNEL_ERROR_NULL if input pointers are NULL
 *         - KERNEL_ERROR_INVALID_COMMAND if command_index is not recognized
 */
kernel_error_st process_command(command_st* command, command_response_st* command_response) {
    kernel_error_st result = KERNEL_SUCCESS;

    if ((command == NULL) || (command_response == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    switch (command->command_index) {
        case CMD_SET_CALIBRATION: {
            result = process_set_calibration_command(command, command_response);
            break;
        }
        case CMD_GET_SYSTEM_INFO: {
            result = process_get_system_info_command(command, command_response);
            break;
        }
        default:
            result = KERNEL_ERROR_INVALID_COMMAND;
    }

    return result;
}

/**
 * @brief Handles incoming commands from a queue and sends responses to another queue.
 *
 * This function retrieves commands from the command_queue, processes them, and
 * sends responses to the response_command_queue. If an error occurs, it is logged.
 *
 * @param command_queue Queue handle from which to receive incoming commands.
 * @param response_command_queue Queue handle to send command responses.
 * @return kernel_error_st Result of processing:
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_NULL if input pointers are invalid
 *         - KERNEL_ERROR_QUEUE_FULL if sending response fails
 */
kernel_error_st handle_incoming_command(QueueHandle_t command_queue, QueueHandle_t response_command_queue) {
    command_st command                   = {0};
    command_response_st command_response = {0};

    if (xQueueReceive(command_queue, &command, pdMS_TO_TICKS(100)) == pdPASS) {
        kernel_error_st err = process_command(&command, &command_response);
        if (err != KERNEL_SUCCESS) {
            logger_print(WARN, TAG, "Failed to process incoming command! - %d", err);
            // return err;
        }

        if (xQueueSend(response_command_queue, &command_response, pdMS_TO_TICKS(100)) != pdPASS) {
            logger_print(ERR, TAG, "Failed to send command response to queue");
            return KERNEL_ERROR_QUEUE_FULL;
        }
    }
    return KERNEL_SUCCESS;
}

/**
 * @brief Main loop of the command manager task.
 *
 * Continuously processes incoming commands from both target and broadcast queues
 * and sends responses. Runs indefinitely with a delay between iterations.
 *
 * @param args Pointer to a command_manager_init_st structure containing queue handles.
 */
void command_manager_loop(void* args) {
    if (args == NULL) {
        logger_print(ERR, TAG, "Command Manager Loop received NULL args");
        return;
    }
    command_manager_init.target_command_queue    = ((command_manager_init_st*)args)->target_command_queue;
    command_manager_init.broadcast_command_queue = ((command_manager_init_st*)args)->broadcast_command_queue;
    command_manager_init.response_command_queue  = ((command_manager_init_st*)args)->response_command_queue;

    while (1) {
        handle_incoming_command(command_manager_init.target_command_queue, command_manager_init.response_command_queue);
        handle_incoming_command(command_manager_init.broadcast_command_queue, command_manager_init.response_command_queue);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}