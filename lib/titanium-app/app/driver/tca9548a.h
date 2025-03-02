/**
 * @file TCA9548A.h
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
#pragma once

#include "driver/i2c.h"
#include "esp_log.h"

typedef enum mux_address_e {
    MUX_ADDRESS_0 = 0x70, /*!< TCA9548A I2C address 0x70 */
    MUX_ADDRESS_1 = 0x71, /*!< TCA9548A I2C address 0x71 */
} mux_address_et;

typedef enum mux_channel_e {
    MUX_CHANNEL_0 = 0,
    MUX_CHANNEL_1,
    MUX_CHANNEL_2,
    MUX_CHANNEL_3,
    MUX_CHANNEL_4,
    MUX_CHANNEL_5,
    MUX_CHANNEL_6,
    MUX_CHANNEL_7,
    MUX_CHANNEL_NONE,
} mux_channel_et;

typedef struct tca9548a_config_t {
    mux_address_et dev_address; /*!< I2C address of the TCA9548A multiplexer */
    mux_channel_et channel;     /*!< Currently selected channel (0-7) */
} tca9548a_config_st;

/**
 * @brief Enable a specific channel on the TCA9548A multiplexer.
 *
 * This function enables one of the eight available I2C channels (0–7) on the
 * specified TCA9548A device. Only one channel should be enabled at a time.
 *
 * @param[in] tca9548a_config Pointer to the device configuration.
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG on bad input, or I2C error code.
 */
esp_err_t tca9548a_enable_channel(const tca9548a_config_st *tca9548a_config);

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
esp_err_t tca9548a_disable_all_channels(const tca9548a_config_st *tca9548a_config);


void tca9548a_reset(int reset_pin);
