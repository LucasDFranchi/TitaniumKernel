/**
 * @file uart_hal.h
 * @brief UART Hardware Abstraction Layer (HAL) interface for ESP32.
 *
 * This header exposes the UART HAL interface used by applications
 * and higher-level drivers. It provides function pointers for
 * thread-safe read/write operations on UART ports.
 *
 * - UART0 is reserved for system use (not available for this HAL).
 * - UART1 is not connected and disabled.
 * - UART2 is available for external use (GPIO17 TX / GPIO16 RX).
 */

#pragma once

#include "esp_err.h"
#include "driver/uart.h"

/**
 * @struct uart_interface_s
 * @brief Function table for accessing UART HAL operations.
 *
 * This structure provides function pointers to UART read and
 * write operations. Applications should retrieve a valid instance
 * via ::uart_get_interface().
 */
typedef struct uart_interface_s {
    /**
     * @brief Read data from a UART port.
     *
     * @param port UART port number.
     * @param buffer Pointer to buffer where received data will be stored.
     * @param buffer_size Maximum number of bytes to read.
     * @param ticks_to_wait Timeout in FreeRTOS ticks to wait for data.
     *
     * @return
     * - >0: number of bytes read
     * -  0: no data received before timeout
     * - <0: negative error code (::ESP_ERR_INVALID_ARG, ::ESP_ERR_INVALID_STATE, ::ESP_ERR_TIMEOUT)
     */
    int32_t (*uart_read_fn)(uart_port_t port, uint8_t* buffer, size_t buffer_size, TickType_t ticks_to_wait);

    /**
     * @brief Write data to a UART port.
     *
     * @param port UART port number.
     * @param buffer Pointer to data buffer.
     * @param buffer_size Number of bytes to send.
     * @param ticks_to_wait Timeout in FreeRTOS ticks to wait for mutex acquisition.
     *
     * @return
     * - ::ESP_OK on success
     * - ::ESP_ERR_INVALID_ARG if arguments are invalid
     * - ::ESP_ERR_INVALID_STATE if UART is not initialized
     * - ::ESP_ERR_TIMEOUT if mutex could not be acquired
     * - ::ESP_FAIL if write failed
     */
    esp_err_t (*uart_write_fn)(uart_port_t port, uint8_t* buffer, size_t buffer_size, TickType_t ticks_to_wait);
} uart_interface_st;

/**
 * @brief Retrieve the UART interface for a given port.
 *
 * This function ensures the UART instance is initialized
 * (if supported/enabled) and fills the provided interface
 * structure with function pointers for read and write.
 *
 * @param port UART port number.
 * @param iface_out Pointer to an ::uart_interface_st structure to be filled.
 *
 * @return
 * - ::ESP_OK on success
 * - ::ESP_ERR_INVALID_ARG if @p iface_out is NULL or port invalid
 * - ::ESP_ERR_INVALID_STATE if UART is disabled (e.g., UART0 or UART1)
 * - Other error codes from the initialization process
 */
esp_err_t uart_get_interface(uart_port_t port, uart_interface_st* iface_out);
