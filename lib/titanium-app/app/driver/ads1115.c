/* 
 * @file ads1115.c
 * @brief ads1115 ADC driver implementation using ESP-IDF I2C APIs.
 * 
 * This file provides functions for configuring and using the ads1115 ADC 
 * over I2C, including register-level control and data acquisition.
 * 
 * @license Apache License 2.0
 * @author LucasD.Franchi@gmail.com
 */
#include <driver/i2c.h>
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

#include "ads1115.h"

#define ADS_RW_BUFF_SIZE 2  // Size of the read/write buffer

static uint8_t write_buffer[ADS_RW_BUFF_SIZE] = {0};
static uint8_t read_buffer[ADS_RW_BUFF_SIZE]  = {0};

/**
 * @brief Write a sequence of bytes to a specific register over I2C.
 *
 * Constructs and sends an I2C write transaction to a given device and register address.
 *
 * @param dev_adr 7-bit I2C address of the target device.
 * @param w_adr   Register address within the device to write to.
 * @param w_len   Number of bytes to write from the buffer.
 * @param buff    Pointer to the buffer containing data to be written.
 *
 * @return
 *     - ESP_OK: Write successful
 *     - ESP_ERR_INVALID_ARG, ESP_FAIL, or other esp_err_t codes on error
 */
static esp_err_t i2c_handle_write(uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff) {
    esp_err_t ret_err = ESP_OK;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret_err += i2c_master_start(cmd);

    ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    ret_err += i2c_master_write_byte(cmd, w_adr, true);
    ret_err += i2c_master_write(cmd, buff, w_len, true);
    ret_err += i2c_master_stop(cmd);

    ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Read a sequence of bytes from a specific register over I2C.
 *
 * Sends a register address to the device, then reads the specified number of bytes.
 * The received data is stored in the provided buffer.
 *
 * @param dev_adr 7-bit I2C address of the target device.
 * @param r_adr   Register address within the device to read from.
 * @param r_len   Number of bytes to read into the buffer.
 * @param buff    Pointer to the buffer where read data will be stored.
 *
 * @return
 *     - ESP_OK: Read successful
 *     - ESP_ERR_INVALID_ARG, ESP_FAIL, or other esp_err_t codes on error
 */
static esp_err_t i2c_handle_read(uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff) {
    memset(buff, 0, ADS_RW_BUFF_SIZE);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret_err    = ESP_OK;

    ret_err += i2c_master_start(cmd);

    ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    ret_err += i2c_master_write_byte(cmd, r_adr, true);
    ret_err += i2c_master_start(cmd);
    ret_err += i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_READ, true);

    if (r_len > 1)
        ret_err += i2c_master_read(cmd, buff, r_len - 1, I2C_MASTER_ACK);
    ret_err += i2c_master_read_byte(cmd, buff + r_len - 1, I2C_MASTER_NACK);
    ret_err += i2c_master_stop(cmd);

    ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Write a 16-bit value to a specified ADS1115 register.
 *
 * Converts the given 16-bit value into a byte buffer and sends it to the
 * specified register of the ADS1115 via I2C.
 *
 * @param dev_addr         7-bit I2C address of the ADS1115 device.
 * @param value            16-bit value to write to the register.
 * @param register_address Target register address in the ADS1115.
 *
 * @return
 *     - ESP_OK: Register written successfully
 *     - ESP_FAIL or other esp_err_t codes on communication failure
 */
static esp_err_t write_register(uint16_t dev_addr, uint16_t value, uint16_t register_address) {
    write_buffer[0] = (uint8_t)(value >> 8) & 0xFF;
    write_buffer[1] = (uint8_t)value & 0xFF;

    return i2c_handle_write(dev_addr, register_address, sizeof(write_buffer), write_buffer);
}

/**
 * @brief Read a 16-bit value from a specified ADS1115 register.
 *
 * Initiates a read from the given register address and stores the result
 * into the internal `read_buffer` array.
 *
 * @param dev_addr         7-bit I2C address of the ADS1115 device.
 * @param register_address Register address to read from.
 *
 * @return
 *     - ESP_OK: Register read successfully
 *     - ESP_FAIL or other esp_err_t codes on communication failure
 */
static esp_err_t read_register(uint16_t dev_addr, uint8_t register_address) {
    return i2c_handle_read(dev_addr, register_address, sizeof(read_buffer), read_buffer);
}

/**
 * @brief Update the ADS1115 configuration register.
 *
 * Writes the current configuration from the `reg_cfg.value` field of the given
 * `ads1115_config_st` structure to the ADS1115 configuration register over I2C.
 *
 * This function is typically used after modifying the bitfields to apply the new
 * settings such as input mux, gain, or operating mode.
 *
 * @param[in] dev Pointer to an initialized ADS1115 configuration structure.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL or other esp_err_t codes on I2C failure
 */
esp_err_t ads1115_update(const ads1115_config_st *dev) {
    if (!dev) {
        return ESP_ERR_INVALID_ARG;  // Ensure the device pointer is valid
    }
    return write_register(dev->dev_addr, dev->reg_cfg.value, REG_ADDR_CONFIG);
}

/**
 * @brief Read the raw ADC conversion result.
 *
 * Retrieves the latest conversion value from the ADS1115 conversion register.
 * The value returned is a 16-bit signed integer in the ADS1115's native format.
 *
 * This function assumes a conversion has already been triggered and completed.
 * It does not initiate a new conversion.
 *
 * @param[in] dev Pointer to an ADS1115 configuration structure.
 *
 * @return 16-bit raw ADC value read from the conversion register.
 */
uint16_t ads1115_get_raw_value(const ads1115_config_st *dev) {
    read_register(dev->dev_addr, REG_ADDR_CONVERSION);
    return (uint16_t)(((read_buffer[0] << 8) & 0xFF00) | read_buffer[1]);
}

/**
 * @brief Check if the ADS1115 conversion is complete.
 *
 * Returns the current operational status of the ADS1115 conversion process.
 * In single-shot mode, it indicates whether the latest conversion is finished.
 * In continuous mode, it is always considered "ready".
 *
 * Internally, it reads the OS (Operational Status) bit from the configuration register.
 *
 * @param[in] dev Pointer to an ADS1115 configuration structure.
 *
 * @return
 *     - true: Conversion complete (data ready to read)
 *     - false: Conversion still in progress
 */
bool ads1115_get_conversion_state(const ads1115_config_st *dev)
{
    read_register(dev->dev_addr, REG_ADDR_CONFIG);
    return (read_buffer[0] & 0x80) ? true : false;
}
