#pragma once

#include <driver/i2c.h>

#include "kernel/error/error_num.h"

/**
 * @brief Represents one I2C instance (port).
 *
 * This class encapsulates initialization and basic read/write
 * operations for an ESP-IDF I²C peripheral. Each I2CHandler
 * instance manages one port (I2C_NUM_0 or I2C_NUM_1).
 */
class I2CHandler {
   public:
    /**
     * @brief Hardware pin + bus configuration for I²C.
     */
    struct I2CHardwareConfig {
        gpio_num_t sda;
        gpio_num_t scl;
        i2c_port_t port;
        uint32_t clock_speed;
    };

    /**
     * @brief Construct a new I2CHandler object.
     *
     * @param sda SDA pin number.
     * @param scl SCL pin number.
     * @param port I2C port (I2C_NUM_0 or I2C_NUM_1).
     * @param clock_speed I2C clock frequency in Hz.
     */
    I2CHandler(gpio_num_t sda, gpio_num_t scl, i2c_port_t port, uint32_t clock_speed) {
        this->hw_config_.sda         = sda;
        this->hw_config_.scl         = scl;
        this->hw_config_.port        = port;
        this->hw_config_.clock_speed = clock_speed;
    }

    /**
     * @brief Destroy the I2CHandler object.
     *
     * Frees internal resources such as the semaphore. Does not
     * deinitialize the ESP-IDF I²C driver.
     */
    ~I2CHandler() {};

    // Constants
    static constexpr int I2C_CMD_TIMEOUT_MS           = 500;                                /// @brief Timeout in milliseconds for I²C commands.
    static constexpr TickType_t I2C_CMD_TIMEOUT_TICKS = pdMS_TO_TICKS(I2C_CMD_TIMEOUT_MS);  /// @brief Timeout in FreeRTOS ticks for I²C commands.

    /**
     * @brief Initialize the I²C port if not already initialized.
     *
     * Configures pins, clock speed, and installs the I²C driver
     * with mutex protection.
     *
     * @return kernel_error_st ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st initialize();

    /**
     * @brief Write bytes to a device register.
     *
     * @param dev_adr 7-bit I²C device address.
     * @param w_adr Register address to write.
     * @param w_len Number of bytes to write.
     * @param buff Pointer to buffer containing data.
     * @return kernel_error_st ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st write(uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff);

    /**
     * @brief Read bytes from a device register.
     *
     * @param dev_adr 7-bit I²C device address.
     * @param r_adr Register address to read.
     * @param r_len Number of bytes to read.
     * @param buff Pointer to buffer where data will be stored.
     * @return kernel_error_st ::KERNEL_ERROR_NONE on success, error code otherwise.
     */
    kernel_error_st read(uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff);

   private:
    I2CHardwareConfig hw_config_;
    bool is_initialized_{false};
    SemaphoreHandle_t mutex_{nullptr};
};
