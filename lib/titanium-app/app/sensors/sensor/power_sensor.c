#include "power_sensor.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "kernel/logger/logger.h"

#include "app/protocols/modbus/master/modbus_master.h"

#define TXD_PIN (GPIO_NUM_17)   // Change to your TX pin
#define RXD_PIN (GPIO_NUM_16)   // Change to your RX pin
#define BUF_SIZE (1024)

static const char *TAG = "Power Sensor";

static bool is_uart_initialized = false;

static kernel_error_st initialize_uart_driver(void){
     const uart_config_t uart_config = {
        .baud_rate = 115200,                        // Set baudrate
        .data_bits = UART_DATA_8_BITS,              // 8 data bits
        .parity    = UART_PARITY_DISABLE,           // No parity
        .stop_bits = UART_STOP_BITS_1,              // 1 stop bit
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,      // No flow control
        .source_clk = UART_SCLK_APB,                // Use APB clock
    };

    // Configure UART2 parameters
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    logger_print(INFO, TAG, "UART2 initialized: TX=%d, RX=%d", TXD_PIN, RXD_PIN);

    gpio_reset_pin(GPIO_NUM_26);
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT);
    is_uart_initialized = true;

    return KERNEL_ERROR_NONE;
}


kernel_error_st power_sensor_read(sensor_interface_st *ctx, uint8_t sensor_index, float *out_value) {
    uint8_t buffer[256] = {0};

    if ((!out_value) || (!ctx)) {
        return KERNEL_ERROR_NULL;
    }

    if (!is_uart_initialized) {
        // This is a workaround and will only be keep temporarily, we need a more decoupled way
        // to handle different hardware sensors;
        initialize_uart_driver();
    }

    size_t message_size = encode_read_request(1, 0x0000, 4, buffer, sizeof(buffer));
    if (message_size == 0) {
        logger_print(ERR, TAG, "Failed to encode Modbus request");
        return KERNEL_ERROR_FAIL;
    }
    // A serial manager will be need to not blocking the app task
    gpio_set_level(GPIO_NUM_26, 1); // Set DE high to enable transmission
    uart_write_bytes(UART_NUM_1, buffer, message_size);
    gpio_set_level(GPIO_NUM_26, 0); // Set DE high to enable transmission

    return KERNEL_ERROR_NONE;
}