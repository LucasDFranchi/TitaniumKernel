#pragma once

#include "kernel/error/error_num.h"
#include <driver/gpio.h>

/**
 * @brief Wrapper class to manage GPIO pins with ESP-IDF.
 *
 * Provides initialization and basic control for digital input/output pins.
 * This class is intentionally thin: it uses ESP-IDF enums and structures directly
 * instead of wrapping them in custom types.
 */
class GPIOHandler {
   public:
    /**
     * @brief Hardware pin configuration for GPIO.
     */
    struct GPIOHardwareConfig {
        gpio_num_t pin;      ///< GPIO pin number
        gpio_mode_t mode;    ///< GPIO mode (input/output/etc.)
        gpio_pullup_t pull;  ///< Pull-up / pull-down configuration
    };

    /**
     * @brief Construct a GPIOHandler object.
     *
     * The object is not immediately active; call initialize() before using
     * any other methods.
     *
     * @param pin  GPIO pin number to control.
     * @param mode Pin mode (e.g., GPIO_MODE_INPUT, GPIO_MODE_OUTPUT).
     * @param pull Pull-up/pull-down configuration (e.g., GPIO_PULLUP_ONLY).
     */
    GPIOHandler(gpio_num_t pin, gpio_mode_t mode, gpio_pullup_t pull)
        : pin_(pin), mode_(mode), pull_(pull) {}

    /**
     * @brief Destroy the GPIOHandler object.
     *
     * Does not release hardware resources. The pin remains configured
     * as last set by ESP-IDF.
     */
    ~GPIOHandler() = default;

    /**
     * @brief Initialize the GPIO pin with the configured mode and pull.
     *
     * This must be called once before using set_level(), get_level(),
     * or toggle().
     *
     * @return ::KERNEL_SUCCESS on success, error code otherwise.
     */
    kernel_error_st initialize();

    /**
     * @brief Set the pin to logical high or low.
     *
     * @param level true = high, false = low.
     * @return ::KERNEL_SUCCESS on success, error code otherwise.
     */
    kernel_error_st set_level(bool level);

    /**
     * @brief Read the current logic level from the pin.
     *
     * @param level Reference to store pin state (true = high, false = low).
     * @return ::KERNEL_SUCCESS on success, error code otherwise.
     */
    kernel_error_st get_level(bool &level);

    /**
     * @brief Toggle the pin state.
     *
     * Only valid if the pin was configured as output.
     *
     * @return ::KERNEL_SUCCESS on success, error code otherwise.
     */
    kernel_error_st toggle();

   private:
    gpio_num_t pin_;              ///< ESP-IDF pin identifier (GPIO number).
    gpio_mode_t mode_;            ///< ESP-IDF pin mode configuration.
    gpio_pullup_t pull_;          ///< ESP-IDF pull-up/pull-down configuration.
    bool is_initialized_{false};  ///< Tracks whether initialize() has been called.
};
