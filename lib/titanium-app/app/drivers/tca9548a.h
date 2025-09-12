/**
 * @file TCA9548A.h
 * @brief Driver interface for the TCA9548A I2C multiplexer using ESP-IDF.
 *
 * This file provides functions to control the TCA9548A 8-channel I2C multiplexer,
 * allowing communication with multiple devices that share the same I2C address
 * by isolating them on separate mux channels.
 *
 * @author Lucas D. Franchi
 * @license Apache License 2.0
 */
#pragma once

#include "esp_log.h"

#include "kernel/hal/i2c/i2c.h"

typedef enum tca9548a_address_e {
    MUX_ADDRESS_0 = 0x70, /*!< TCA9548A I2C address 0x70 */
    MUX_ADDRESS_1 = 0x71, /*!< TCA9548A I2C address 0x71 */
    NUM_OF_MUX_ADDRESS,   /*!< Invalid TCA9548A I2C address */
} mux_address_et;

typedef enum tca9548a_channel_e {
    MUX_CHANNEL_0 = 0,
    MUX_CHANNEL_1,
    MUX_CHANNEL_2,
    MUX_CHANNEL_3,
    MUX_CHANNEL_4,
    MUX_CHANNEL_5,
    MUX_CHANNEL_6,
    MUX_CHANNEL_7,
    NUM_OF_MUX_CHANNELS,
} mux_channel_et;

typedef struct tca9548a_bus_config_s {
    i2c_port_t port;            /*!< I2C port number (e.g., I2C_NUM_0 or I2C_NUM_1) */
    mux_address_et dev_address; /*!< I2C address of the TCA9548A multiplexer */
    int reset_pin;              /*!< GPIO pin used for hardware reset */
} tca9548a_bus_config_st;

typedef struct tca9548a_hw_config_s {
    const tca9548a_bus_config_st *bus_hw_config; /*!< Hardware bus configuration */
    mux_channel_et channel;                      /*!< Currently selected channel (0-7) */
} tca9548a_hw_config_st;

typedef struct tca9548a_config_t {
    const tca9548a_hw_config_st hw_config; /*!< Pointer to hardware configuration */
    i2c_interface_st i2c_interface;        /**< Function pointer for I2C write operations */
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

/**
 * @brief Reset the TCA9548A I2C multiplexer via the designated GPIO reset pin.
 *
 * This function pulls the reset pin low and then high with a delay in between to trigger a hardware reset
 * on the TCA9548A device. The pin must be connected to the RESET input of the multiplexer.
 *
 * @param tca9548a_config Pointer to the TCA9548A configuration structure containing the reset GPIO pin.
 *
 * @return
 *     - ESP_OK: Reset completed successfully.
 *     - ESP_ERR_INVALID_ARG: Null pointer provided for configuration.
 *     - Other esp_err_t values: Errors returned by gpio_set_level.
 */
esp_err_t tca9548a_reset(const tca9548a_config_st *tca9548a_config);
