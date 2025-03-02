/**
 * @file TCA9548A.c
 * @brief Driver interface for the TCA9548A I2C multiplexer using ESP-IDF.
 *
 * This file provides functions to control the TCA9548A 8-channel I2C multiplexer,
 * allowing communication with multiple devices that share the same I2C address
 * by isolating them on separate mux channels.
 *
 * Supported features:
 * - Selecting a single active I2C channel
 * - Disabling all channels
 * - Basic error handling and logging
 *
 * Example usage:
 * @code
 * tca9548a_config_st config = { .dev_address = 0x70 };
 * tca9548a_enable_channel(&config, MUX_CHANNEL_3);
 * // Communicate with device behind channel 3
 * tca9548a_disable_all_channels(&config);
 * @endcode
 *
 * @author Lucas D. Franchi
 * @license Apache License 2.0
 */
#include "tca9548a.h"

/**
 * @brief Send a single-byte command to the TCA9548A over I2C.
 *
 * This function handles the low-level I2C transmission to send one command byte
 * (used for channel selection or disabling) to a TCA9548A device.
 *
 * @param[in] tca9548a_config Pointer to device configuration containing I2C address.
 * @param[in] data Command byte to send (bitmask for channel selection).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if config is NULL, or other I2C errors.
 */
static esp_err_t i2c_send_one_byte(const tca9548a_config_st *tca9548a_config, uint8_t data) {
    esp_err_t ret_err = ESP_OK;

    if (!tca9548a_config) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret_err += i2c_master_start(cmd);
    ret_err += i2c_master_write_byte(cmd, (tca9548a_config->dev_address << 1) | I2C_MASTER_WRITE, true);
    ret_err += i2c_master_write_byte(cmd, data, true);
    ret_err += i2c_master_stop(cmd);

    ret_err += i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(500));
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Enable a specific channel on the TCA9548A multiplexer.
 *
 * This function enables one of the eight available I2C channels (0–7) on the
 * specified TCA9548A device. Only one channel should be enabled at a time.
 *
 * @param[in] tca9548a_config Pointer to the device configuration.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG on bad input, or I2C error code.
 */
esp_err_t tca9548a_enable_channel(const tca9548a_config_st *tca9548a_config) {
    if (!tca9548a_config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (tca9548a_config->channel >= MUX_CHANNEL_NONE) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data = 1 << tca9548a_config->channel;
    return i2c_send_one_byte(tca9548a_config, data);
}

/**
 * @brief Disable all channels on the TCA9548A multiplexer.
 *
 * This function sends a command to disconnect all downstream I2C channels
 * from the TCA9548A. This is useful before switching between multiple muxes
 * or avoiding bus contention with duplicate addresses.
 *
 * @param[in] tca9548a_config Pointer to the device configuration.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if config is NULL, or I2C error code.
 */
esp_err_t tca9548a_disable_all_channels(const tca9548a_config_st *tca9548a_config) {
    const uint8_t disable_all_channels_cmd = 0x00;

    if (!tca9548a_config) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_send_one_byte(tca9548a_config, disable_all_channels_cmd);
}


void tca9548a_reset(int reset_pin) {
    const uint8_t reset_time = 100;  // Reset pulse duration in milliseconds

    gpio_set_level(reset_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(reset_time));

    gpio_set_level(reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(reset_time));
}