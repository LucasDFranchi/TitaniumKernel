#include "gpio_handler.h"

/**
 * @brief Initialize the GPIO pin with the configured mode and pull.
 *
 * This method configures the GPIO hardware according to the mode and pull
 * parameters provided in the constructor. It must be called before using
 * set_level(), get_level(), or toggle().
 *
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st GPIOHandler::initialize() {
    gpio_config_t config = {};
    config.pin_bit_mask  = (1ULL << this->pin_);
    config.mode          = this->mode_;

    switch (this->pull_) {
        case GPIO_PULLUP_ONLY: {
            config.pull_up_en   = GPIO_PULLUP_ENABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        }
        case GPIO_PULLDOWN_ONLY: {
            config.pull_up_en   = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        }
        default:
            config.pull_up_en   = GPIO_PULLUP_DISABLE;
            config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    }

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return KERNEL_ERROR_INITIALIZE_GPIO;
    }

    this->is_initialized_ = true;

    return KERNEL_SUCCESS;
}

/**
 * @brief Set the pin to logical high or low.
 *
 * This function is only valid if the pin is configured as output.
 *
 * @param level true = high, false = low.
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st GPIOHandler::set_level(bool level) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_RESOURCE_NOT_INITIALIZED;
    }
    esp_err_t ret = gpio_set_level(this->pin_, level ? 1 : 0);
    return (ret == ESP_OK) ? KERNEL_SUCCESS : KERNEL_ERROR_GPIO_SET_LEVEL_FAIL;
}

/**
 * @brief Read the current logic level from the pin.
 *
 * @param level Reference to store pin state (true = high, false = low).
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st GPIOHandler::get_level(bool &level) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_RESOURCE_NOT_INITIALIZED;
    }

    level = (gpio_get_level(pin_) != 0);

    return KERNEL_SUCCESS;
}

/**
 * @brief Toggle the pin state.
 *
 * Reads the current state and writes the opposite value.
 * Only valid if the pin is configured as output.
 *
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st GPIOHandler::toggle() {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_RESOURCE_NOT_INITIALIZED;
    }

    bool current = false;

    kernel_error_st err = get_level(current);
    if (err != KERNEL_SUCCESS) {
        return err;
    }
    return set_level(!current);
}
