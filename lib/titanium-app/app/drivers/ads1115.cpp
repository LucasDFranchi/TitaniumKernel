/*
 * @file ADS1115.cc
 * @brief ADS1115 ADC driver implementation using ESP-IDF I2C APIs.
 *
 * This file provides functions for configuring and using the ADS1115 ADC
 * over I2C, including register-level control and data acquisition.
 *
 * @license Apache License 2.0
 * @author LucasD.Franchi@gmail.com
 */
#include "app/drivers/ads1115.h"

/**
 * @brief Write a 16-bit value to a specific ADS1115 register over I2C.
 *
 * Converts the `value` to big-endian format and sends it to the device
 * at `DEFAULT_I2C_ADDRESS` using the provided I2C handler.
 *
 * @param register_address The target register (enum class).
 * @param value            16-bit value to write.
 *
 * @return Kernel error status:
 *         - KERNEL_SUCCESS: Write successful
 *         - KERNEL_ERROR_NULL: I2C handler not initialized
 *         - Other codes returned by the underlying I2C write function
 */
kernel_error_st ADS1115::write_register(register_address_e register_address, uint16_t value) {
    if (this->i2c_handler_ == nullptr) {
        return KERNEL_ERROR_NULL;
    }

    uint8_t write_buffer[2]      = {0};
    uint8_t register_address_raw = static_cast<uint8_t>(register_address);
    write_buffer[0]              = static_cast<uint8_t>((value >> 8) & 0xFF);
    write_buffer[1]              = static_cast<uint8_t>(value & 0xFF);

    return this->i2c_handler_->write(this->DEFAULT_I2C_ADDRESS,
                                     register_address_raw,
                                     sizeof(write_buffer),
                                     write_buffer);
}

/**
 * @brief Read a 16-bit value from a specific ADS1115 register over I2C.
 *
 * Reads two bytes from the device at `DEFAULT_I2C_ADDRESS` and combines them
 * into a 16-bit value in big-endian order.
 *
 * @param register_address The target register (enum class).
 * @param value            Reference to store the 16-bit result.
 *
 * @return Kernel error status:
 *         - KERNEL_SUCCESS: Read successful
 *         - KERNEL_ERROR_NULL: I2C handler not initialized
 *         - Other codes returned by the underlying I2C read function
 */
kernel_error_st ADS1115::read_register(register_address_e register_address, uint16_t &value) {
    if (this->i2c_handler_ == nullptr) {
        return KERNEL_ERROR_NULL;
    }

    uint8_t read_buffer[2]       = {0};
    uint8_t register_address_raw = static_cast<uint8_t>(register_address);

    kernel_error_st err = this->i2c_handler_->read(this->DEFAULT_I2C_ADDRESS,
                                                   register_address_raw,
                                                   sizeof(read_buffer),
                                                   read_buffer);
    if (err != KERNEL_SUCCESS) {
        return err;
    }

    value = static_cast<uint16_t>((read_buffer[0] << 8) | read_buffer[1]);

    return KERNEL_SUCCESS;
}

kernel_error_st ADS1115::configure(void) {
    return this->write_register(register_address::config, this->config_.value);
}

kernel_error_st ADS1115::get_raw_value(int16_t &raw_value) {
    uint16_t reg_value  = 0;
    kernel_error_st err = this->read_register(register_address::conversion, reg_value);
    if (err != KERNEL_SUCCESS) {
        return err;
    }

    raw_value = static_cast<int16_t>(reg_value);
    return KERNEL_SUCCESS;
}

/**
 * @brief Check if the ADS1115 has completed a conversion.
 *
 * Reads the configuration register and inspects the OS bit:
 * - In single-shot mode: OS = 1 means conversion complete (data ready).
 * - In continuous mode:  OS is always 1, so this function returns true.
 *
 * @return true  If conversion is complete (data ready).
 * @return false If conversion is still in progress or I2C read failed.
 */
bool ADS1115::is_conversion_complete(void) {
    register_config reg_config{};

    if (this->read_register(register_address::config, reg_config.value) != ESP_OK) {
        return false;
    }
    return (reg_config.bits.os == 1) ? true : false;
}

/**
 * @brief Set the ADC input multiplexer configuration.
 *
 * @param mux The input channel configuration to use.
 * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
 */
kernel_error_st ADS1115::set_mux(mux_config mux) {
    if (mux < mux_config::diff_a0_a1 || mux > mux_config::single_a3) {
        return KERNEL_ERROR_INVALID_ARG;
    }
    config_.bits.mux = static_cast<uint16_t>(mux);
    return KERNEL_SUCCESS;
}

/**
 * @brief Set the programmable gain amplifier (PGA) setting.
 *
 * @param gain The PGA gain to use.
 * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
 */
kernel_error_st ADS1115::set_pga(pga_gain gain) {
    if (gain < pga_gain::v6_144 || gain > pga_gain::v0_256_3) {
        return KERNEL_ERROR_INVALID_ARG;
    }
    config_.bits.pga = static_cast<uint16_t>(gain);
    return KERNEL_SUCCESS;
}

/**
 * @brief Set the operating mode of the ADC.
 *
 * @param m The operating mode (continuous or single-shot).
 * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
 */
kernel_error_st ADS1115::set_mode(mode m) {
    if (m != mode::continuous && m != mode::single_shot) {
        return KERNEL_ERROR_INVALID_ARG;
    }
    config_.bits.mode = static_cast<uint16_t>(m);
    return KERNEL_SUCCESS;
}

/**
 * @brief Set the ADC data rate (samples per second).
 *
 * @param rate The data rate setting.
 * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
 */
kernel_error_st ADS1115::set_data_rate(data_rate rate) {
    if (rate < data_rate::sps_8 || rate > data_rate::sps_860) {
        return KERNEL_ERROR_INVALID_ARG;
    }
    config_.bits.dr = static_cast<uint16_t>(rate);
    return KERNEL_SUCCESS;
}

/**
 * @brief Configure the comparator settings for the ADC.
 *
 * @param comp_mode Comparator mode (traditional or window).
 * @param pol Comparator polarity (active low or active high).
 * @param latch Comparator latching mode (non-latching or latching).
 * @param queue Comparator queue/disable setting.
 * @return kernel_error_st KERNEL_SUCCESS if all parameters are valid,
 *                         otherwise a specific error code for the invalid parameter.
 */
kernel_error_st ADS1115::set_comparator(comparator_mode comp_mode,
                                        comparator_polarity pol,
                                        comparator_latching latch,
                                        comparator_queue queue) {
    if (comp_mode != comparator_mode::traditional && comp_mode != comparator_mode::window) {
        return KERNEL_ERROR_ADC_INVALID_CMP_MODE;
    }

    if (pol != comparator_polarity::active_low && pol != comparator_polarity::active_high) {
        return KERNEL_ERROR_ADC_INVALID_CMP_POLARITY;
    }

    if (latch != comparator_latching::non_latching && latch != comparator_latching::latching) {
        return KERNEL_ERROR_ADC_INVALID_CMP_LATCHING;
    }

    if (queue < comparator_queue::assert_1 || queue > comparator_queue::disable) {
        return KERNEL_ERROR_ADC_INVALID_CMP_QUEUE;
    }

    config_.bits.comp_mode = static_cast<uint16_t>(comp_mode);
    config_.bits.comp_pol  = static_cast<uint16_t>(pol);
    config_.bits.comp_lat  = static_cast<uint16_t>(latch);
    config_.bits.comp_que  = static_cast<uint16_t>(queue);

    return KERNEL_SUCCESS;
}
