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
#include "app/hal/drivers/ads1115.h"

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
    return this->write_register(register_address_e::config, this->config_.value);
}

kernel_error_st ADS1115::get_raw_value(int16_t &raw_value) {
    uint16_t reg_value  = 0;
    kernel_error_st err = this->read_register(register_address_e::conversion, reg_value);
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

    if (this->read_register(register_address_e::config, reg_config.value) != ESP_OK) {
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
kernel_error_st ADS1115::set_mux(mux_config_e mux) {
    if (mux < mux_config_e::diff_a0_a1 || mux > mux_config_e::single_a3) {
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
kernel_error_st ADS1115::set_pga(pga_gain_e gain) {
    if (gain < pga_gain_e::v6_144 || gain > pga_gain_e::v0_256_3) {
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
kernel_error_st ADS1115::set_mode(mode_e mode) {
    if ((mode != mode_e::continuous) && (mode != mode_e::single_shot)) {
        return KERNEL_ERROR_INVALID_ARG;
    }
    config_.bits.mode = static_cast<uint16_t>(mode);
    return KERNEL_SUCCESS;
}

/**
 * @brief Set the ADC data rate (samples per second).
 *
 * @param rate The data rate setting.
 * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
 */
kernel_error_st ADS1115::set_data_rate(data_rate_e rate) {
    if (rate < data_rate_e::sps_8 || rate > data_rate_e::sps_860) {
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
kernel_error_st ADS1115::set_comparator(comparator_mode_e comp_mode,
                                        comparator_polarity_e pol,
                                        comparator_latching_e latch,
                                        comparator_queue_e queue) {
    if (comp_mode != comparator_mode_e::traditional && comp_mode != comparator_mode_e::window) {
        return KERNEL_ERROR_ADC_INVALID_CMP_MODE;
    }

    if (pol != comparator_polarity_e::active_low && pol != comparator_polarity_e::active_high) {
        return KERNEL_ERROR_ADC_INVALID_CMP_POLARITY;
    }

    if (latch != comparator_latching_e::non_latching && latch != comparator_latching_e::latching) {
        return KERNEL_ERROR_ADC_INVALID_CMP_LATCHING;
    }

    if (queue < comparator_queue_e::assert_1 || queue > comparator_queue_e::disable) {
        return KERNEL_ERROR_ADC_INVALID_CMP_QUEUE;
    }

    config_.bits.comp_mode = static_cast<uint16_t>(comp_mode);
    config_.bits.comp_pol  = static_cast<uint16_t>(pol);
    config_.bits.comp_lat  = static_cast<uint16_t>(latch);
    config_.bits.comp_que  = static_cast<uint16_t>(queue);

    return KERNEL_SUCCESS;
}

/**
 * @brief Get the LSB size (in microvolts) for a given PGA gain setting.
 *
 * This function returns the resolution per bit (µV) corresponding to the PGA gain setting.
 *
 * @param pga_gain PGA gain enum value from ADS1115 config.
 * @return float LSB size in microvolts.
 */
float ADS1115::get_lsb_size(pga_gain_e pga_gain) {
    switch (pga_gain) {
        case pga_gain_e::v6_144:
            return (this->FULL_SCALE_MV_6_144 / this->ADC_RESOLUTION);
        case pga_gain_e::v4_096:
            return (this->FULL_SCALE_MV_4_096 / this->ADC_RESOLUTION);
        case pga_gain_e::v2_048:
            return (this->FULL_SCALE_MV_2_048 / this->ADC_RESOLUTION);
        case pga_gain_e::v1_024:
            return (this->FULL_SCALE_MV_1_024 / this->ADC_RESOLUTION);
        case pga_gain_e::v0_512:
            return (this->FULL_SCALE_MV_0_512 / this->ADC_RESOLUTION);
        case pga_gain_e::v0_256_1:
        case pga_gain_e::v0_256_2:
        case pga_gain_e::v0_256_3:
            return (this->FULL_SCALE_MV_0_256 / this->ADC_RESOLUTION);
        default:
            return (this->FULL_SCALE_MV_6_144 / this->ADC_RESOLUTION);
    }
}

/**
 * @brief Get the conversion delay (in ms) for a given ADS1115 data rate.
 *
 * This function returns a safe delay time (rounded up) based on the selected data rate.
 *
 * @param data_rate Data rate setting from the ADS1115 config.
 * @return uint16_t Recommended delay in milliseconds.
 */
uint16_t ADS1115::get_conversion_delay(data_rate_e data_rate) {
    switch (data_rate) {
        case data_rate_e::sps_8:
            return this->DELAY_8SPS;
        case data_rate_e::sps_16:
            return this->DELAY_16SPS;
        case data_rate_e::sps_32:
            return this->DELAY_32SPS;
        case data_rate_e::sps_64:
            return this->DELAY_64SPS;
        case data_rate_e::sps_128:
            return this->DELAY_128SPS;
        case data_rate_e::sps_250:
            return this->DELAY_250SPS;
        case data_rate_e::sps_475:
            return this->DELAY_475SPS;
        case data_rate_e::sps_860:
            return this->DELAY_860SPS;
        default:
            return this->DELAY_DEFAULT;
    }
}
