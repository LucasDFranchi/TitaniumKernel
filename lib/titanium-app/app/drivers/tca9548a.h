/**
 * @file TCA9548A.h
 * @brief Driver interface for the TCA9548A I2C multiplexer using ESP-IDF.
 *
 * Provides functions to control the TCA9548A 8-channel I2C multiplexer,
 * allowing communication with multiple devices that share the same I2C address
 * by isolating them on separate mux channels.
 *
 * @author Lucas D. Franchi
 * @license Apache License 2.0
 */
#pragma once

#include "kernel/error/error_num.h"
#include "kernel/hal/gpio/gpio_handler.h"
#include "kernel/hal/i2c/i2c_handler.h"

/**
 * @brief Driver class for the TCA9548A I2C multiplexer.
 *
 * This class provides initialization, channel selection, reset, and
 * enable/disable functionality. The I2C and GPIO handler objects must
 * remain valid for the lifetime of this instance.
 */
class TCA9548A {
   public:
    // Constants
    static constexpr uint8_t DISABLE_ALL_CHANNELS = 0x00;  ///< Control value to disable all channels.
    static constexpr uint8_t RESET_TIME_MS        = 100;   ///< Minimum reset recovery time in ms.

    // Enumerations
    /**
     * @brief Possible I2C addresses of the TCA9548A (A0/A1/A2 pins).
     */
    enum class mux_address_e {
        MUX_ADDRESS_0 = 0x70, /*!< TCA9548A I2C address 0x70 */
        MUX_ADDRESS_1 = 0x71, /*!< TCA9548A I2C address 0x71 */
        MUX_ADDRESS_2 = 0x72, /*!< TCA9548A I2C address 0x72 */
        MUX_ADDRESS_3 = 0x73, /*!< TCA9548A I2C address 0x73 */
        MUX_ADDRESS_4 = 0x74, /*!< TCA9548A I2C address 0x74 */
        MUX_ADDRESS_5 = 0x75, /*!< TCA9548A I2C address 0x75 */
        MUX_ADDRESS_6 = 0x76, /*!< TCA9548A I2C address 0x76 */
        MUX_ADDRESS_7 = 0x77, /*!< TCA9548A I2C address 0x77 */
        NUM_OF_MUX_ADDRESS,   /*!< Invalid TCA9548A I2C address */
    };

    /**
     * @brief Channel indices (0–7).
     */
    enum class channel_index_e {
        MUX_CHANNEL_0 = 0,
        MUX_CHANNEL_1,
        MUX_CHANNEL_2,
        MUX_CHANNEL_3,
        MUX_CHANNEL_4,
        MUX_CHANNEL_5,
        MUX_CHANNEL_6,
        MUX_CHANNEL_7,
        NUM_OF_MUX_CHANNELS,  ///< Invalid channel count marker
    };

    /**
     * @brief Internal register addresses (fixed).
     */
    enum class register_address_e : uint8_t {
        config = 0b00,  ///< Only one control register exists.
    };

    /**
     * @brief Construct a TCA9548A driver instance.
     *
     * @param i2c_handler   Pointer to an initialized I2C handler (must remain valid).
     * @param gpio_handler  Pointer to a GPIO handler controlling the reset pin (must remain valid).
     * @param mux_address   I2C address of the multiplexer.
     */
    TCA9548A(I2CHandler* i2c_handler, GPIOHandler* gpio_handler, mux_address_e mux_address) {
        this->i2c_handler_  = i2c_handler;
        this->gpio_handler_ = gpio_handler;
        this->mux_address_  = mux_address;
    }

    /**
     * @brief Destroy the driver object.
     *
     * Does not release hardware resources. I2C and GPIO handlers are not owned
     * by this class and must be managed externally.
     */
    ~TCA9548A() = default;

    /**
     * @brief Initialize the multiplexer.
     *
     * Configures the device into a known state and disables all channels.
     *
     * @return ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st initialize(void);

    /**
     * @brief Enable a specific channel.
     *
     * @param channel_index Channel index (0–7).
     * @return ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st enable_channel(channel_index_e channel_index);

    /**
     * @brief Disable all channels.
     *
     * @return ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st disable(void);

    /**
     * @brief Reset the multiplexer using the external reset pin.
     *
     * @return ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st reset(void);

   private:
    I2CHandler* i2c_handler_;    ///< I2C handler used for communication.
    GPIOHandler* gpio_handler_;  ///< GPIO handler for controlling reset pin.
    mux_address_e mux_address_;  ///< Device I2C address.
    bool is_initialized_{false};

    /**
     * @brief Write a value to the control register.
     *
     * @param value Value to write.
     * @return ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st write_register(uint8_t value);
};
