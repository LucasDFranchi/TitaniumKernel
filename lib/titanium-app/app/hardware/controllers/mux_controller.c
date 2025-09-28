/**
 * @file mux_controller.c
 * @brief Multiplexer (TCA9548A) management module.
 *
 * This module handles initialization and channel selection for multiple
 * TCA9548A I2C multiplexers. It tracks the active configuration to avoid
 * unnecessary channel toggling and provides simple APIs to interact with
 * the multiplexer setup.
 */
#include "mux_controller.h"

#include "kernel/logger/logger.h"

static const char *TAG                        = "Mux Controller";
static tca9548a_config_st *current_mux_config = NULL;
static bool is_initialized                    = false;

static const tca9548a_bus_config_st tca9548a_bus_config[] = {
    [MUX_ADDRESS_0] = {.port = I2C_NUM_0, .dev_address = MUX_ADDRESS_0, .reset_pin = GPIO_NUM_27},
    [MUX_ADDRESS_1] = {.port = I2C_NUM_0, .dev_address = MUX_ADDRESS_1, .reset_pin = GPIO_NUM_27},
};

static tca9548a_config_st tca9548a_config[NUM_OF_MUX_ADDRESS][NUM_OF_MUX_CHANNELS] = {
    [MUX_ADDRESS_0] = {
        [MUX_CHANNEL_0] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_0}, .i2c_interface = {0}},
        [MUX_CHANNEL_1] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_1}, .i2c_interface = {0}},
        [MUX_CHANNEL_2] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_2}, .i2c_interface = {0}},
        [MUX_CHANNEL_3] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_3}, .i2c_interface = {0}},
        [MUX_CHANNEL_4] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_4}, .i2c_interface = {0}},
        [MUX_CHANNEL_5] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_5}, .i2c_interface = {0}},
        [MUX_CHANNEL_6] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_6}, .i2c_interface = {0}},
        [MUX_CHANNEL_7] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_0], .channel = MUX_CHANNEL_7}, .i2c_interface = {0}},
    },
    [MUX_ADDRESS_1] = {
        [MUX_CHANNEL_0] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_0}, .i2c_interface = {0}},
        [MUX_CHANNEL_1] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_1}, .i2c_interface = {0}},
        [MUX_CHANNEL_2] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_2}, .i2c_interface = {0}},
        [MUX_CHANNEL_3] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_3}, .i2c_interface = {0}},
        [MUX_CHANNEL_4] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_4}, .i2c_interface = {0}},
        [MUX_CHANNEL_5] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_5}, .i2c_interface = {0}},
        [MUX_CHANNEL_6] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_6}, .i2c_interface = {0}},
        [MUX_CHANNEL_7] = {.hw_config = {.bus_hw_config = &tca9548a_bus_config[MUX_ADDRESS_1], .channel = MUX_CHANNEL_7}, .i2c_interface = {0}},
    },
};

/**
 * @brief Update the current MUX configuration.
 *
 * Validates the provided configuration and sets it as the active one.
 *
 * @param[in] mux_hw_config Pointer to the new MUX hardware configuration.
 * @return KERNEL_SUCCESS on success, error code otherwise.
 */
static kernel_error_st update_current_mux_config(mux_address_et mux_address, mux_channel_et mux_channel) {
    if (mux_address >= NUM_OF_MUX_ADDRESS ||
        mux_channel >= NUM_OF_MUX_CHANNELS) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    current_mux_config = &tca9548a_config[mux_address][mux_channel];

    return KERNEL_SUCCESS;
}

/**
 * @brief Compare two MUX configurations to determine if they represent the same MUX channel.
 *
 * @param[in] mux_hw_config Pointer to a TCA9548A hardware configuration (type: tca9548a_hw_config_st).
 * @param[in] current_mux_config Pointer to a MUX hardware configuration (type: mux_hw_config_st).
 * @return true if both MUX address and channel are the same; false otherwise.
 *
 * @note
 * This function currently takes parameters of two different types representing MUX configurations,
 * which can be confusing and error-prone. It would benefit from refactoring to unify or clarify
 * the configuration types to improve readability and maintainability.
 */
static bool is_same_mux_and_channel(const tca9548a_hw_config_st *mux_hw_config, const mux_hw_config_st *current_mux_config) {
    if (mux_hw_config == NULL || current_mux_config == NULL) {
        return false;
    }

    return (mux_hw_config->bus_hw_config->dev_address == current_mux_config->mux_address &&
            mux_hw_config->channel == current_mux_config->mux_channel);
}

/**
 * @brief Initialize the MUX controller.
 *
 * Initializes I2C interfaces for all configured MUXes and their channels.
 *
 * @return KERNEL_SUCCESS on success.
 */
static kernel_error_st mux_initialize(void) {
    esp_err_t err = ESP_OK;

    for (uint8_t i = 0; i < NUM_OF_MUX_ADDRESS; i++) {
        for (uint8_t j = 0; j < NUM_OF_MUX_CHANNELS; j++) {
            err = i2c_get_interface(tca9548a_bus_config[i].port, &tca9548a_config[i][j].i2c_interface);
            if (err != ESP_OK) {
                logger_print(ERR, TAG, "Failed to get I2C interface for MUX %d channel %d: %s\n",
                             (int)i, (int)j, esp_err_to_name(err));
                return KERNEL_ERROR_INVALID_INTERFACE;
            }
        }
    }

    gpio_config_t io_conf_tca_reset = {
        .pin_bit_mask = (1ULL << tca9548a_bus_config[MUX_ADDRESS_0].reset_pin),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE};
    if (gpio_config(&io_conf_tca_reset) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to configure GPIO for TCA9548A reset pin");
        return KERNEL_ERROR_GPIO_CONFIG_FAIL;
    }

    if (tca9548a_reset(&tca9548a_config[MUX_ADDRESS_0][MUX_CHANNEL_0]) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to reset TCA9548A multiplexer");
        return KERNEL_ERROR_MUX_RESET_ERROR;
    }

    kernel_error_st result = update_current_mux_config(MUX_ADDRESS_0, MUX_CHANNEL_0);
    if (result != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Cannot update the current mux configuration.");
        return result;
    }

    is_initialized = true;

    return KERNEL_SUCCESS;
}

/**
 * @brief Select and activate a specific MUX channel.
 *
 * If the channel is different from the currently active one, it disables the current channel
 * and enables the new one. Updates internal state accordingly.
 *
 * @param[in] mux_hw_config Pointer to desired MUX channel configuration.
 * @return KERNEL_SUCCESS on success, specific error code otherwise.
 */
static kernel_error_st select_channel(const mux_hw_config_st *mux_hw_config) {
    if ((mux_hw_config == NULL) || (current_mux_config == NULL)) {
        return KERNEL_ERROR_NULL;
    }

    kernel_error_st ret = KERNEL_SUCCESS;
    if (!is_same_mux_and_channel(&current_mux_config->hw_config, mux_hw_config)) {
        esp_err_t err = tca9548a_disable_all_channels(current_mux_config);
        if (err != ESP_OK) {
            logger_print(ERR, TAG, "Failed to disable all channels on current MUX %d: %s\n",
                         (int)mux_hw_config->mux_address,
                         esp_err_to_name(err));
            return KERNEL_ERROR_MUX_DISABLECHANNEL_ERROR;
        }
        err = tca9548a_enable_channel(&tca9548a_config[mux_hw_config->mux_address][mux_hw_config->mux_channel]);
        if (err != ESP_OK) {
            logger_print(ERR, TAG, "Failed to enable channel %d on MUX %d: %s\n",
                         (int)mux_hw_config->mux_channel,
                         (int)mux_hw_config->mux_address,
                         esp_err_to_name(err));
            return KERNEL_ERROR_MUX_CHANNEL_ERROR;
        }

        ret = update_current_mux_config(mux_hw_config->mux_address, mux_hw_config->mux_channel);
        if (ret != KERNEL_SUCCESS) {
            logger_print(ERR, TAG, "Failed to update current MUX configuration: %d\n", ret);
            return ret;
        }
    }

    return ret;
}

/**
 * @brief Initialize the mux controller and populate the function pointers.
 *
 * This function initializes the mux controller hardware on the first call and assigns the internal
 * `select_channel` function pointer to the provided mux_controller structure. This allows external
 * code to interact with the mux controller through this abstraction.
 *
 * @param[out] mux_controller Pointer to a mux_controller_st structure to be populated.
 *                            Must not be NULL.
 *
 * @return kernel_error_st Returns KERNEL_SUCCESS on success,
 *                         KERNEL_ERROR_NULL if mux_controller is NULL,
 *                         or an initialization error code if hardware init fails.
 *
 * @note This function performs hardware initialization only once.
 *       Subsequent calls skip initialization but still assign the function pointer.
 */
kernel_error_st mux_controller_init(mux_controller_st *mux_controller) {
    if (!mux_controller) {
        return KERNEL_ERROR_NULL;
    }

    if (!is_initialized) {
        kernel_error_st err = mux_initialize();
        if (err != KERNEL_SUCCESS) {
            return err;
        }
    }

    mux_controller->select_channel = select_channel;

    return KERNEL_SUCCESS;
}
