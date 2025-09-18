/**
 * @file uart_hal.c
 * @brief UART Hardware Abstraction Layer (HAL) for ESP32.
 *
 * This module provides a thread-safe UART driver wrapper for ESP32,
 * exposing read and write functions via an interface structure.
 *
 * - UART0 is reserved for system use (not available for this HAL).
 * - UART1 is not connected and disabled.
 * - UART2 is configured on GPIO17 (TX) and GPIO16 (RX) with a 1 KB buffer.
 *
 * @note This implementation uses FreeRTOS semaphores for thread safety.
 */
#include "kernel/hal/uart/uart.h"

#include "driver/gpio.h"

#define UART_DEFAULT_BAUDRATE 115200 /**< Default UART baudrate in bps. */

/**
 * @struct uart_hw_config_t
 * @brief Pin configuration for a UART instance.
 *
 * This structure maps the UART to its TX, RX, and optional DE (Driver Enable) pin.
 * The DE pin is used in half-duplex RS-485 mode to control the bus transceiver.
 */
typedef struct uart_hw_config_t {
    gpio_num_t tx_pin;  /**< GPIO number for TX pin. */
    gpio_num_t rx_pin;  /**< GPIO number for RX pin. */
    gpio_num_t dir_pin; /**< Optional GPIO number for DE (Driver Enable). Set to GPIO_NUM_NC if unused. */
} uart_hw_config_st;

/**
 * @struct uart_instance_t
 * @brief Runtime state and configuration for a UART instance.
 */
typedef struct uart_instance_t {
    bool is_initialized_;         /**< True if the UART is already initialized. */
    size_t buffer_size;          /**< RX buffer size in bytes. */
    uart_hw_config_st hw_config; /**< Hardware pin configuration. */
    SemaphoreHandle_t mutex;     /**< Mutex for thread-safe read/write. */
} uart_instance_st;

/**
 * @brief Global UART instances table.
 *
 * - UART0: Reserved for system, disabled.
 * - UART1: Not connected, disabled.
 * - UART2: Configured for external use (GPIO17 TX / GPIO16 RX).
 */
static uart_instance_st uart_instance[UART_NUM_MAX] = {
    [UART_NUM_0] = {
        .is_initialized_ = false,
        .buffer_size    = 0,
        .hw_config      = {
                 .tx_pin  = GPIO_NUM_NC,
                 .rx_pin  = GPIO_NUM_NC,
                 .dir_pin = GPIO_NUM_NC,
        },
    },
    [UART_NUM_1] = {
        .is_initialized_ = false,
        .buffer_size    = 0,
        .hw_config      = {
                 .tx_pin  = GPIO_NUM_NC,
                 .rx_pin  = GPIO_NUM_NC,
                 .dir_pin = GPIO_NUM_NC,
        },
    },
    [UART_NUM_2] = {
        .is_initialized_ = false,
        .buffer_size    = 1024,
        .hw_config      = {
                 .tx_pin  = GPIO_NUM_17,
                 .rx_pin  = GPIO_NUM_16,
                 .dir_pin = GPIO_NUM_4,
        },
    },
};

/**
 * @brief Set the UART transceiver direction (DE pin) for RS-485.
 *
 * This function controls the Driver Enable (DE) pin if configured for the UART instance.
 * It switches the transceiver between transmit and receive mode:
 * - `enable = true` → transmit mode (DE = HIGH)
 * - `enable = false` → receive mode (DE = LOW)
 *
 * @param port UART port number to control.
 * @param enable Boolean flag to set transmit (true) or receive (false) mode.
 *
 * @return
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if the UART port number is invalid
 * - ESP_ERR_INVALID_STATE if the UART is not initialized or the DE pin is not configured
 */
static esp_err_t uart_set_transmit_mode(uart_port_t port, bool enable) {
    if (port < 0 || port >= UART_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!uart_instance[port].is_initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    if (uart_instance[port].hw_config.dir_pin == GPIO_NUM_NC) {
        return ESP_ERR_INVALID_STATE;
    }

    return gpio_set_level(uart_instance[port].hw_config.dir_pin, enable ? 1 : 0);
}

/**
 * @brief Set the UART transceiver direction (DE pin).
 *
 * This function controls the Driver Enable (DE) pin if configured.
 * Used to switch between transmit and receive mode for RS-485.
 *
 * @param port UART port number.
 * @param enable true to enable transmit mode (DE=1), false for receive (DE=0).
 *
 * @return
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if port is invalid
 * - ESP_ERR_INVALID_STATE if UART is not initialized or DE pin not configured
 */
static esp_err_t uart_handle_write(uart_port_t port, uint8_t* buffer, size_t buffer_size, TickType_t ticks_to_wait) {
    if (buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (port < 0 || port >= UART_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!uart_instance[port].is_initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    if (uart_set_transmit_mode(port, true) != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(uart_instance[port].mutex, ticks_to_wait) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    int bytes_written = uart_write_bytes(port, (const char*)buffer, buffer_size);
    uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(ticks_to_wait * 10));
    xSemaphoreGive(uart_instance[port].mutex);

    if (uart_set_transmit_mode(port, false) != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }

    return (bytes_written < 0) ? ESP_FAIL : ESP_OK;
}

/**
 * @brief Read data from the specified UART port.
 *
 * @param port UART port number.
 * @param buffer Pointer to buffer where received data will be stored.
 * @param buffer_size Maximum number of bytes to read.
 * @param ticks_to_wait Timeout in FreeRTOS ticks for receiving data.
 *
 * @return
 * - Number of bytes read on success
 * - Negative error code:
 *   - ESP_ERR_INVALID_ARG if arguments are invalid
 *   - ESP_ERR_INVALID_STATE if UART is not initialized
 *   - ESP_ERR_TIMEOUT if mutex could not be acquired
 */
static int32_t uart_handle_read(uart_port_t port, uint8_t* buffer, size_t buffer_size, TickType_t ticks_to_wait) {
    if (buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (port < 0 || port >= UART_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!uart_instance[port].is_initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(uart_instance[port].mutex, ticks_to_wait) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    int32_t bytes_read = uart_read_bytes(port, buffer, buffer_size, pdMS_TO_TICKS(ticks_to_wait));
    xSemaphoreGive(uart_instance[port].mutex);

    return bytes_read;
}

/**
 * @brief Initialize a UART instance if not already initialized.
 *
 * @param port UART port number.
 *
 * @return
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if invalid port
 * - ESP_ERR_INVALID_STATE if pins are not configured
 * - ESP_ERR_NO_MEM if mutex allocation fails
 * - Other ESP-IDF UART driver error codes
 */
static esp_err_t uart_init_once(uart_port_t port) {
    if (uart_instance[port].is_initialized_) {
        return ESP_OK;
    }

    if (port < 0 || port >= UART_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((uart_instance[port].hw_config.tx_pin == GPIO_NUM_NC) || (uart_instance[port].hw_config.rx_pin == GPIO_NUM_NC)) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!uart_instance[port].mutex) {
        uart_instance[port].mutex = xSemaphoreCreateMutex();
        if (!uart_instance[port].mutex)
            return ESP_ERR_NO_MEM;
    }

    const uart_config_t uart_config = {
        .baud_rate  = UART_DEFAULT_BAUDRATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_2,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    esp_err_t err = uart_driver_install(port, uart_instance[port].buffer_size, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        return err;
    }

    err = uart_param_config(port, &uart_config);
    if (err != ESP_OK) {
        return err;
    }

    err = uart_set_pin(port, uart_instance[port].hw_config.tx_pin, uart_instance[port].hw_config.rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        return err;
    }

    if (uart_instance[port].hw_config.dir_pin != GPIO_NUM_NC) {
        gpio_reset_pin(uart_instance[port].hw_config.dir_pin);
        gpio_set_direction(uart_instance[port].hw_config.dir_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(uart_instance[port].hw_config.dir_pin, 0);
    }

    uart_instance[port].is_initialized_ = true;

    return ESP_OK;
}

/**
 * @brief Get a UART interface for reading and writing.
 *
 * This function ensures the UART is initialized and returns
 * function pointers to read and write handlers.
 *
 * @param port UART port number.
 * @param iface_out Pointer to interface struct to be filled.
 *
 * @return
 * - ESP_OK on success
 * - ESP_ERR_INVALID_ARG if iface_out is NULL or port invalid
 * - ESP_ERR_INVALID_STATE if UART is disabled
 * - Other errors from uart_init_once()
 */
esp_err_t uart_get_interface(uart_port_t port, uart_interface_st* iface_out) {
    if (!iface_out)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = uart_init_once(port);
    if (err != ESP_OK)
        return err;

    iface_out->uart_read_fn  = uart_handle_read;
    iface_out->uart_write_fn = uart_handle_write;

    return ESP_OK;
}
