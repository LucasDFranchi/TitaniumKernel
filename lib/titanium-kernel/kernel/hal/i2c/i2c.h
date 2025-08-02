#pragma once

#include "esp_err.h"

#include <driver/i2c.h>

/**
 * @brief I2C interface abstraction for communication with devices.
 *
 * Encapsulates the I2C port and function pointers for read/write operations.
 * This allows higher-level modules to interact with I2C devices through a common interface,
 * enabling loose coupling and easier testing or mocking.
 */
typedef struct i2c_interface_s {
    /**
     * @brief Read a sequence of bytes from a specific register over I2C.
     *
     * Sends a register address to the device, then reads the specified number of bytes.
     * The received data is stored in the provided buffer.
     *
     * @param port    I2C port number to use (e.g., I2C_NUM_0 or I2C_NUM_1).
     * @param dev_adr 7-bit I2C address of the target device.
     * @param r_adr   Register address within the device to read from.
     * @param r_len   Number of bytes to read into the buffer.
     * @param buff    Pointer to the buffer where read data will be stored.
     *
     * @return
     *     - ESP_OK: Read successful
     *     - ESP_ERR_INVALID_ARG, ESP_FAIL, or other esp_err_t codes on error
     */
    esp_err_t (*i2c_read_fn)(i2c_port_t port, uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff);

    /**
     * @brief Write a sequence of bytes to a specific register over I2C.
     *
     * Constructs and sends an I2C write transaction to a given device and register address.
     *
     * @param port    I2C port number to use (e.g., I2C_NUM_0 or I2C_NUM_1).
     * @param dev_adr 7-bit I2C address of the target device.
     * @param w_adr   Register address within the device to write to.
     * @param w_len   Number of bytes to write from the buffer.
     * @param buff    Pointer to the buffer containing data to be written.
     *
     * @return
     *     - ESP_OK: Write successful
     *     - ESP_ERR_INVALID_ARG, ESP_FAIL, or other esp_err_t codes on error
     */
    esp_err_t (*i2c_write_fn)(i2c_port_t port, uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff);
} i2c_interface_st;

/**
 * @brief Initializes I2C port (if needed) and returns function pointers for read/write.
 *
 * @param port      I2C port to use (e.g., I2C_NUM_0)
 * @param iface_out Pointer to struct that will hold I2C function pointers
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t i2c_get_interface(i2c_port_t port, i2c_interface_st *iface_out);
