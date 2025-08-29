/**
 * @file power_sensor.c
 * @brief Interface for reading electrical parameters from a PZEM power sensor using Modbus RTU.
 *
 * This module communicates with a PZEM-004T (or compatible) energy meter over UART
 * via Modbus RTU protocol. It sends "Read Input Registers" requests, decodes responses,
 * and populates `sensor_report_st` structures with voltage, current, power, and
 * power factor measurements.
 *
 * @note Currently only supports a single Modbus slave (address 0x01).
 * @note Uses UART2 hardware interface by default.
 */

#include "power_sensor.h"

#include "kernel/hal/uart/uart.h"
#include "kernel/logger/logger.h"

#include "app/protocols/modbus/master/modbus_master.h"

static const char *TAG                    = "Power Sensor";
static const uint8_t SLAVE_ADDRESS        = 0x01;  // Modbus slave address
static const uint16_t TRANSMIT_TIMEOUT_MS = 100;
static const uint16_t RECEIVE_TIMEOUT_MS  = 2000;

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
static const uint8_t VOLTAGE_REGISTER_ADDRESS      = 0x00;
static const uint8_t CURRENT_LOW_REGISTER_ADDRESS  = 0x01;
static const uint8_t CURRENT_HIGH_REGISTER_ADDRESS = 0x02;
static const uint8_t POWER_LOW_REGISTER_ADDRESS    = 0x03;
static const uint8_t POWER_HIGH_REGISTER_ADDRESS   = 0x04;
static const uint8_t POWER_FACTOR_REGISTER_ADDRESS = 0x08;
static const float VOLTAGE_SCALE_FACTOR            = 10.0f;
static const float CURRENT_SCALE_FACTOR            = 1000.0f;
static const float POWER_SCALE_FACTOR              = 10.0f;
static const float POWER_FACTOR_SCALE_FACTOR       = 100.0f;
static const uint8_t VOLTAGE_INDEX                 = 0;
static const uint8_t CURRENT_INDEX                 = 1;
static const uint8_t POWER_INDEX                   = 2;
static const uint8_t POWER_FACTOR_INDEX            = 3;

/**
 * @brief Send a Modbus request to the PZEM sensor to read power data registers.
 *
 * This function encodes a Modbus RTU "Read Input Registers" request frame,
 * writes it to the configured UART interface, and handles basic validation
 * before transmission.
 *
 * @param[in] ctx                    Pointer to the sensor interface context.
 * @param[out] transmit_buffer       Buffer where the encoded Modbus frame is stored.
 * @param[in] transmit_buffer_size   Size of the transmit buffer in bytes.
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_INVALID_ARG if input arguments are invalid,
 *         KERNEL_ERROR_FAILED_TO_ENCODE_PACKET if the request could not be encoded,
 *         KERNEL_ERROR_FAIL if UART transmission fails.
 */
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

/**
 * @brief Receive and parse Modbus response from the PZEM sensor.
 *
 * This function reads raw bytes from the UART interface, decodes the Modbus RTU
 * response, and fills the sensor report structure with voltage, current, power,
 * and power factor values using the PZEM scaling factors.
 *
 * @param[in]  ctx                  Pointer to the sensor interface context.
 * @param[out] response_buffer      Buffer to store the raw response frame.
 * @param[in]  response_buffer_size Size of the response buffer in bytes.
 * @param[out] sensor_report        Array of sensor report entries to update.
 *
 * @return KERNEL_ERROR_NONE on success,
 *         KERNEL_ERROR_INVALID_ARG if inputs are invalid,
 *         KERNEL_ERROR_TIMEOUT if no response is received,
 *         KERNEL_ERROR_FAILED_TO_DECODE_PACKET if response decoding fails.
 */
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

    sensor_report[ctx->index + VOLTAGE_INDEX].value = registers[VOLTAGE_REGISTER_ADDRESS] /
                                                      VOLTAGE_SCALE_FACTOR;
    sensor_report[ctx->index + VOLTAGE_INDEX].sensor_type = SENSOR_TYPE_VOLTAGE;
    sensor_report[ctx->index + VOLTAGE_INDEX].active      = true;

    sensor_report[ctx->index + CURRENT_INDEX].value = (registers[CURRENT_HIGH_REGISTER_ADDRESS] << 16 |
                                                       registers[CURRENT_LOW_REGISTER_ADDRESS]) /
                                                      CURRENT_SCALE_FACTOR;
    sensor_report[ctx->index + CURRENT_INDEX].sensor_type = SENSOR_TYPE_CURRENT;
    sensor_report[ctx->index + CURRENT_INDEX].active      = true;

    sensor_report[ctx->index + POWER_INDEX].value = (registers[POWER_HIGH_REGISTER_ADDRESS] << 16 |
                                                     registers[POWER_LOW_REGISTER_ADDRESS]) /
                                                    POWER_SCALE_FACTOR;
    sensor_report[ctx->index + POWER_INDEX].sensor_type = SENSOR_TYPE_POWER;
    sensor_report[ctx->index + POWER_INDEX].active      = true;

    sensor_report[ctx->index + POWER_FACTOR_INDEX].value = registers[POWER_FACTOR_REGISTER_ADDRESS] /
                                                           POWER_FACTOR_SCALE_FACTOR;
    sensor_report[ctx->index + POWER_FACTOR_INDEX].sensor_type = SENSOR_TYPE_POWER_FACTOR;
    sensor_report[ctx->index + POWER_FACTOR_INDEX].active      = true;

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Reads voltage, current, power, and power factor from the PZEM power sensor.
 *
 * This is the main public entry point for the power sensor driver. It initializes
 * the UART interface if needed, sends a Modbus request, waits for a response, and
 * fills the provided sensor report array.
 *
 * Example usage:
 * @code
 * sensor_report_st report[4];
 * sensor_interface_st ctx = {.index = 0};
 * if (power_sensor_read(&ctx, report) == KERNEL_ERROR_NONE) {
 *     printf("Voltage: %.2f V\n", report[0].value);
 *     printf("Current: %.3f A\n", report[1].value);
 * }
 * @endcode
 *
 * @param[in]  ctx           Pointer to the sensor interface context (defines index offset).
 * @param[out] sensor_report Array where measurement values will be stored.
 *
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_NULL if ctx or sensor_report is NULL
 *         - KERNEL_ERROR_UART_NOT_INITIALIZED if UART interface is unavailable
 *         - KERNEL_ERROR_FAILED_TO_ENCODE_PACKET if Modbus request failed
 *         - KERNEL_ERROR_FAIL if UART transmission failed
 *         - KERNEL_ERROR_TIMEOUT if no response received
 *         - KERNEL_ERROR_FAILED_TO_DECODE_PACKET if Modbus response parsing failed
 */
kernel_error_st power_sensor_read(sensor_interface_st *ctx, sensor_report_st *sensor_report) {
    uint8_t buffer[256]   = {0};
    uint8_t response[256] = {0};

    if ((!sensor_report) || (!ctx)) {
        return KERNEL_ERROR_NULL;
    }

    uint8_t sensor_index = ctx->index;

    sensor_report[sensor_index + VOLTAGE_INDEX].value       = 0;
    sensor_report[sensor_index + VOLTAGE_INDEX].sensor_type = 0;
    sensor_report[sensor_index + VOLTAGE_INDEX].active      = false;

    sensor_report[sensor_index + CURRENT_INDEX].value       = 0;
    sensor_report[sensor_index + CURRENT_INDEX].sensor_type = 0;
    sensor_report[sensor_index + CURRENT_INDEX].active      = false;

    sensor_report[sensor_index + POWER_INDEX].value       = 0;
    sensor_report[sensor_index + POWER_INDEX].sensor_type = 0;
    sensor_report[sensor_index + POWER_INDEX].active      = false;

    sensor_report[sensor_index + POWER_FACTOR_INDEX].value       = 0;
    sensor_report[sensor_index + POWER_FACTOR_INDEX].sensor_type = 0;
    sensor_report[sensor_index + POWER_FACTOR_INDEX].active      = false;

    if (uart_interface.uart_read_fn == NULL || uart_interface.uart_write_fn == NULL) {
        if (uart_get_interface(UART_NUM_2, &uart_interface) != ESP_OK) {
            return KERNEL_ERROR_UART_NOT_INITIALIZED;
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
