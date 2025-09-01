#include "power_sensor.h"

#include "kernel/hal/uart/uart.h"
#include "kernel/logger/logger.h"

#include "app/protocols/modbus/master/modbus_master.h"

static const char *TAG                    = "Power Sensor";
static const uint8_t SLAVE_ADDRESS        = 0x01;  // Modbus slave address
static const uint16_t TRANSMIT_TIMEOUT_MS = 100;
static const uint16_t RECEIVE_TIMEOUT_MS  = 100;

static uart_interface_st uart_interface = {0};

/**
 * @file pzem_registers.h
 * @brief PZEM sensor Modbus register definitions.
 *
 * These constants define the number of 16-bit registers and their corresponding
 * Modbus register addresses for the PZEM energy meter. Each subsequent register
 * block's address is calculated by adding the previous block's size.
 *
 * @note This data is currently defined statically in this file. For better
 * decoupling, consider moving it to a dedicated sensor configuration module.
 */
// static const uint8_t VOLTAGE_REGISTERS             = 1;
// static const uint8_t VOLTAGE_REGISTER_ADDRESS      = 0x0000;
// static const uint8_t CURRENT_REGISTERS             = 2;
// static const uint8_t CURRENT_REGISTER_ADDRESS      = VOLTAGE_REGISTER_ADDRESS + CURRENT_REGISTERS;
// static const uint8_t POWER_REGISTERS               = 2;
// static const uint8_t POWER_REGISTER_ADDRESS        = CURRENT_REGISTER_ADDRESS + POWER_REGISTERS;
// static const uint8_t ENERGY_REGISTERS              = 2;
// static const uint8_t ENERGY_REGISTER_ADDRESS       = POWER_REGISTER_ADDRESS + ENERGY_REGISTERS;
// static const uint8_t FREQUENCY_REGISTERS           = 1;
// static const uint8_t FREQUENCY_REGISTER_ADDRESS    = ENERGY_REGISTER_ADDRESS + FREQUENCY_REGISTERS;
// static const uint8_t POWER_FACTOR_REGISTERS        = 1;
// static const uint8_t POWER_FACTOR_REGISTER_ADDRESS = FREQUENCY_REGISTER_ADDRESS + POWER_FACTOR_REGISTERS;

static kernel_error_st initialize_uart_driver(void) {
    if (uart_get_interface(UART_NUM_2, &uart_interface) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to get UART interface");
        return KERNEL_ERROR_FAIL;
    }

    return KERNEL_ERROR_NONE;
}

static kernel_error_st request_power_data(sensor_interface_st *ctx, uint8_t *transmit_buffer, size_t transmit_buffer_size) {
    if (!ctx || !transmit_buffer || (transmit_buffer_size == 0)) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    uint16_t register_address = 0;
    uint8_t register_qty      = 0x0A;

    size_t message_size = encode_read_request(SLAVE_ADDRESS, register_address, register_qty, transmit_buffer, transmit_buffer_size);
    if (message_size == 0) {
        logger_print(ERR, TAG, "Failed to encode Modbus request");
        return KERNEL_ERROR_FAILED_TO_ENCODE_PACKET;
    }

    esp_err_t err = uart_interface.uart_write_fn(UART_NUM_2, transmit_buffer, message_size, TRANSMIT_TIMEOUT_MS);

    return err == ESP_OK ? KERNEL_ERROR_NONE : KERNEL_ERROR_FAIL;
}

static kernel_error_st receive_power_data(sensor_interface_st *ctx, uint8_t *response_buffer, size_t response_buffer_size, sensor_report_st *sensor_report) {
    if (ctx == NULL || !sensor_report || !response_buffer || (response_buffer_size == 0)) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    size_t len = uart_interface.uart_read_fn(UART_NUM_2, response_buffer, response_buffer_size, RECEIVE_TIMEOUT_MS);
    if (len <= 0) {
        logger_print(ERR, TAG, "No response from slave: %d", SLAVE_ADDRESS);
        return KERNEL_ERROR_TIMEOUT;
    }

    uint16_t registers[10] = {0};

    int decode_result = decode_read_response(response_buffer, len, registers, sizeof(registers));
    if (decode_result < 0) {
        logger_print(ERR, TAG, "Failed to decode Modbus response: %d", decode_result);
        return KERNEL_ERROR_FAILED_TO_DECODE_PACKET;
    }

    sensor_report[ctx->index].value       = registers[0] / 10.0;
    sensor_report[ctx->index].sensor_type = SENSOR_TYPE_VOLTAGE;
    sensor_report[ctx->index].active      = true;

    sensor_report[ctx->index + 1].value       = (registers[1] << 16 | registers[2]) / 1000.0f;
    sensor_report[ctx->index + 1].sensor_type = SENSOR_TYPE_CURRENT;
    sensor_report[ctx->index + 1].active      = true;

    sensor_report[ctx->index + 2].value       = (registers[3] << 16 | registers[4]) / 10.0f;
    sensor_report[ctx->index + 2].sensor_type = SENSOR_TYPE_POWER;
    sensor_report[ctx->index + 2].active      = true;

    sensor_report[ctx->index + 3].value       = registers[8] / 100.0f;
    sensor_report[ctx->index + 3].sensor_type = SENSOR_TYPE_POWER_FACTOR;
    sensor_report[ctx->index + 3].active      = true;

    return KERNEL_ERROR_NONE;
}

kernel_error_st power_sensor_read(sensor_interface_st *ctx, sensor_report_st *sensor_report) {
    uint8_t buffer[256]   = {0};
    uint8_t response[256] = {0};

    if ((!sensor_report) || (!ctx)) {
        return KERNEL_ERROR_NULL;
    }

    uint8_t sensor_index = ctx->index;

    sensor_report[sensor_index].value       = 0;
    sensor_report[sensor_index].sensor_type = 0;
    sensor_report[sensor_index].active      = false;

    if (uart_interface.uart_read_fn == NULL || uart_interface.uart_write_fn == NULL) {
        kernel_error_st err = initialize_uart_driver();
        if (err != KERNEL_ERROR_NONE) {
            return err;
        }
    }

    kernel_error_st err = request_power_data(ctx, buffer, sizeof(buffer));
    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to send Modbus request - %d", sensor_index);
        return err;
    }

    err = receive_power_data(ctx, response, sizeof(response), sensor_report);

    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to receive Modbus response - %d", sensor_index);
        return err;
    }

    return KERNEL_ERROR_NONE;
}