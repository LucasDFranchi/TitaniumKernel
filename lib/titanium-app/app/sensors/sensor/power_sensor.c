#include "power_sensor.h"

#include "kernel/logger/logger.h"
#include "kernel/hal/uart/uart.h"

#include "app/protocols/modbus/master/modbus_master.h"

static const char *TAG = "Power Sensor";
static uart_interface_st uart_interface = {0};

static kernel_error_st initialize_uart_driver(void) {
    if (uart_get_interface(UART_NUM_2, &uart_interface) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to get UART interface");
        return KERNEL_ERROR_FAIL;
    }

    return KERNEL_ERROR_NONE;
}

kernel_error_st power_sensor_read(sensor_interface_st *ctx, uint8_t sensor_index, float *out_value) {
    uint8_t buffer[256] = {0};
    uint8_t response[256] = {0};

    if ((!out_value) || (!ctx)) {
        return KERNEL_ERROR_NULL;
    }

    if (uart_interface.uart_read_fn == NULL || uart_interface.uart_write_fn == NULL) {
        kernel_error_st err = initialize_uart_driver();
        if (err != KERNEL_ERROR_NONE) {
            return err;
        }
    }

    size_t message_size = encode_read_request(1, 0x0000, 1, buffer, sizeof(buffer));
    if (message_size == 0) {
        logger_print(ERR, TAG, "Failed to encode Modbus request");
        return KERNEL_ERROR_FAIL;
    }

    uart_interface.uart_write_fn(UART_NUM_2, (const char *)buffer, message_size, 20);

    int len = uart_interface.uart_read_fn(UART_NUM_2, response, sizeof(response), 200);
    if (len <= 0) {
        logger_print(ERR, TAG, "No response from slave");
        return KERNEL_ERROR_TIMEOUT;
    }

    logger_print(DEBUG, TAG, "Received %d bytes from slave", len);
    for (int i = 0; i < len; i++) {
        logger_print(DEBUG, TAG, "0x%02X ", response[i]);
    }
    uint16_t reg = 0;
    int err = decode_read_response(response, sizeof(response), &reg, 2);
    
    if (err < 0) {
        logger_print(ERR, TAG, "Failed to decode Modbus response: %d", err);
        return KERNEL_ERROR_FAIL;
    }

    logger_print(INFO, TAG, "Power sensor reading: %d V", reg);
    return KERNEL_ERROR_NONE;
}