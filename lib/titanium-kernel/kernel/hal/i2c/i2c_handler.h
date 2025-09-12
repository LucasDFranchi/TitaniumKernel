#pragma once

#include <driver/i2c.h>

#include "kernel/error/error_num.h"

/**
 * @brief Represents one I2C instance (port).
 */
class I2CHandler {
   public:
    I2CHandler(gpio_num_t sda, gpio_num_t scl, i2c_port_t port, uint32_t clock_speed) {
        this->hw_config_.sda         = sda;
        this->hw_config_.scl         = scl;
        this->hw_config_.port        = port;
        this->hw_config_.clock_speed = clock_speed;
    }
    ~I2CHandler() {};

    static constexpr int I2C_CMD_TIMEOUT_MS           = 500;
    static constexpr TickType_t I2C_CMD_TIMEOUT_TICKS = pdMS_TO_TICKS(I2C_CMD_TIMEOUT_MS);

    /**
     * @brief Initialize the I2C port if not already initialized.
     */
    kernel_error_st initialize();


    /**
     * @brief Write bytes to a device register.
     */
    kernel_error_st write(uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff);

    /**
     * @brief Read bytes from a device register.
     */
    kernel_error_st read(uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff);

   private:
    /**
     * @brief Hardware pin + bus config for I²C.
     */
    struct I2CHWConfig {
        gpio_num_t sda;
        gpio_num_t scl;
        i2c_port_t port;
        uint32_t clock_speed;
    };

    I2CHWConfig hw_config_;
    bool is_initialized_{false};
    SemaphoreHandle_t mutex_{nullptr};
};
