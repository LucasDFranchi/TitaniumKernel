#include "tca9548a.h"
/**
 * @brief Write a value to the control register.
 *
 * @param value Value to write.
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st TCA9548A::write_register(uint8_t value) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_RESOURCE_NOT_INITIALIZED;
    }

    return this->i2c_handler_->write(static_cast<uint8_t>(this->mux_address_),
                                     static_cast<uint8_t>(register_address_e::config),
                                     sizeof(value),
                                     &value);
}

kernel_error_st TCA9548A::initialize() {
    if (this->is_initialized_) {
        return KERNEL_SUCCESS;
    }

    if ((this->mux_address_ >= mux_address_e::NUM_OF_MUX_ADDRESS) || (this->mux_address_ < mux_address_e::MUX_ADDRESS_0)) {
        return KERNEL_ERROR_INVALID_MUX_ADDRESS;
    }

    if ((this->i2c_handler_ == nullptr) || (this->gpio_handler_ == nullptr)) {
        return KERNEL_ERROR_NULL;
    }

    this->is_initialized_ = true;

    return KERNEL_SUCCESS;
}

/**
 * @brief Enable a specific channel.
 *
 * @param channel_index Channel index (0–7).
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st TCA9548A::enable_channel(channel_index_e channel_index) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_RESOURCE_NOT_INITIALIZED;
    }

    if (channel_index >= channel_index_e::NUM_OF_MUX_CHANNELS) {
        return KERNEL_ERROR_INVALID_MUX_CHANNEL;
    }

    uint8_t data = 1 << static_cast<uint8_t>(channel_index);
    return this->write_register(data);
}
/**
 * @brief Disable all channels.
 *
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st TCA9548A::disable(void) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_RESOURCE_NOT_INITIALIZED;
    }

    return this->write_register(this->DISABLE_ALL_CHANNELS);
}

/**
 * @brief Reset the multiplexer using the external reset pin.
 *
 * @return ::KERNEL_SUCCESS on success, error code otherwise.
 */
kernel_error_st TCA9548A::reset(void) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_RESOURCE_NOT_INITIALIZED;
    }
    
    kernel_error_st kerr = gpio_handler_->set_level(false);
    if (kerr != KERNEL_SUCCESS) {
        return kerr;
    }
    vTaskDelay(pdMS_TO_TICKS(this->RESET_TIME_MS));

    kerr = gpio_handler_->set_level(true);
    if (kerr != KERNEL_SUCCESS) {
        return kerr;
    }
    vTaskDelay(pdMS_TO_TICKS(this->RESET_TIME_MS));

    return KERNEL_SUCCESS;
}
