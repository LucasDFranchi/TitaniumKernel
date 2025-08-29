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
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

#include "app/hardware/drivers/ads1115.h"

#define ADS_RW_BUFF_SIZE 2  // Size of the read/write buffer

static uint8_t write_buffer[ADS_RW_BUFF_SIZE] = {0};
static uint8_t read_buffer[ADS_RW_BUFF_SIZE]  = {0};

/**
 * @brief Write a 16-bit value to a specified ADS1115 register.
 *
 * Converts the given 16-bit value into a byte buffer and sends it to the
 * specified register of the ADS1115 via I2C.
 *
 * @param[in] dev Pointer to an initialized ADS1115 configuration structure.
 * @param[in] register_address Target register address in the ADS1115.
 *
 * @return
 *     - ESP_OK: Register written successfully
 *     - ESP_FAIL or other esp_err_t codes on communication failure
 */
static esp_err_t write_register(const ads1115_config_st *dev, uint16_t register_address) {
    if ((!dev) || (dev->i2c_interface.i2c_write_fn == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t dev_addr = dev->hw_config.dev_address;

    write_buffer[0] = (uint8_t)(dev->config.value >> 8) & 0xFF;
    write_buffer[1] = (uint8_t)dev->config.value & 0xFF;

    return dev->i2c_interface.i2c_write_fn(dev->hw_config.port, dev_addr, register_address, sizeof(write_buffer), write_buffer);
}

/**
 * @brief Read a 16-bit value from a specified ADS1115 register.
 *
 * Initiates a read from the given register address and stores the result
 * into the internal `read_buffer` array.
 *
 * @param[in] dev Pointer to an initialized ADS1115 configuration structure.
 *
 * @return
 *     - ESP_OK: Register read successfully
 *     - ESP_FAIL or other esp_err_t codes on communication failure
 */
static esp_err_t read_register(const ads1115_config_st *dev, uint8_t register_address) {
    if ((!dev) || (dev->i2c_interface.i2c_read_fn == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t dev_addr = dev->hw_config.dev_address;

    return dev->i2c_interface.i2c_read_fn(dev->hw_config.port, dev_addr, register_address, sizeof(read_buffer), read_buffer);
}

/**
 * @brief Configure the ADS1115 configuration register.
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
esp_err_t ads1115_configure(const ads1115_config_st *dev) {
    if (!dev) {
        return ESP_ERR_INVALID_ARG;
    }
    return write_register(dev, REG_ADDR_CONFIG);
}

/**
 * @brief Read the raw ADC conversion result from the ADS1115.
 *
 * Retrieves the latest conversion value from the ADS1115 conversion register.
 * The value returned is a 16-bit signed integer in the ADS1115's native format.
 *
 * @param[in]  dev        Pointer to the ADS1115 configuration structure.
 * @param[out] raw_value  Pointer to store the 16-bit signed raw ADC result.
 *
 * @return
 *     - ESP_OK:     Conversion result read successfully
 *     - ESP_FAIL or other esp_err_t: I2C communication error
 */
esp_err_t ads1115_get_raw_value(const ads1115_config_st *dev, int16_t *raw_value) {
    if (!dev || !raw_value) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = read_register(dev, REG_ADDR_CONVERSION);
    if (err != ESP_OK) {
        return err;
    }

    *raw_value = (int16_t)((read_buffer[0] << 8) | read_buffer[1]);
    return ESP_OK;
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
bool ads1115_get_conversion_state(const ads1115_config_st *dev) {
    if (read_register(dev, REG_ADDR_CONFIG) != ESP_OK) {
        return false;
    }
    return (read_buffer[0] & 0x80) ? true : false;
}
