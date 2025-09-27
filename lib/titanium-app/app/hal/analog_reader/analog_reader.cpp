#include "analog_reader.h"

#include "kernel/logger/logger.h"

/**
 * @brief Initialize the AnalogReader with the desired ADC configuration.
 *
 * This function applies data rate, gain, and mux settings to the ADC driver
 * using the safe setters. If any of these fail, the initialization aborts
 * and the corresponding error is returned.
 *
 * @return KERNEL_SUCCESS on success, or the error code from the failing setter.
 */
kernel_error_st AnalogReader::initialize() {
    if (!this->adc_driver_)
        return KERNEL_ERROR_NULL;

    kernel_error_st kerr;
    if ((kerr = this->set_data_rate(this->data_rate_e_)) != KERNEL_SUCCESS) {
        return kerr;
    }
    if ((kerr = this->set_pga_gain(this->pga_gain_e_)) != KERNEL_SUCCESS) {
        return kerr;
    }
    if ((kerr = this->set_mux_config(this->mux_config_e_)) != KERNEL_SUCCESS) {
        return kerr;
    }

    this->initialized_ = true;

    return KERNEL_SUCCESS;
}

/**
 * @brief Perform a single ADC conversion and return the raw value.
 *
 * This function configures the ADC, waits for conversion to complete
 * (with retries and a delay), and then retrieves the raw conversion result.
 *
 * @param[out] raw_value Reference where the raw ADC result will be stored.
 * @return KERNEL_SUCCESS if the read succeeded, or an error code on failure:
 *         - KERNEL_ERROR_NULL if adc_driver_ is null
 *         - KERNEL_ERROR_TIMEOUT if the conversion does not complete in time
 *         - Driver-specific error codes from configure() or get_raw_value()
 */
kernel_error_st AnalogReader::read_adc(int16_t& raw_value) {
    if (this->adc_driver_ == nullptr) {
        return KERNEL_ERROR_NULL;
    }

    if (!initialized_) {
        logger_print(ERR, TAG, "ADC reader not initialized");
        return KERNEL_ERROR_INVALID_STATE;
    }

    kernel_error_st kerr = this->adc_driver_->configure();
    if (kerr != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to configure ADC (error code %d)", kerr);
        return kerr;
    }

    uint8_t retries = MAX_ADC_READ_RETRIES;
    while (retries-- > 0) {
        if (this->adc_driver_->is_conversion_complete()) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(ADC_READ_DELAY_MS));
    }
    if (retries == 0) {
        logger_print(ERR, TAG, "ADC conversion did not complete in time");
        return KERNEL_ERROR_TIMEOUT;
    }

    kerr = this->adc_driver_->get_raw_value(raw_value);

    if (kerr != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to read ADC value (error code %d)", kerr);
        return kerr;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Set the ADC data rate.
 *
 * Attempts to apply the requested data rate to the ADC driver. If successful,
 * the cached data_rate_e_ member is updated to match.
 *
 * @param rate Desired ADC data rate.
 * @return KERNEL_SUCCESS if applied successfully, or driver error code otherwise.
 */
kernel_error_st AnalogReader::set_data_rate(ADS1115::data_rate_e rate) {
    kernel_error_st kerr = this->adc_driver_->set_data_rate(rate);
    if (kerr != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to set data rate (error code %d)", kerr);
        return kerr;
    }
    data_rate_e_ = rate;
    return KERNEL_SUCCESS;
}

/**
 * @brief Set the ADC programmable gain amplifier (PGA) gain.
 *
 * Attempts to apply the requested gain to the ADC driver. If successful,
 * the cached pga_gain_e_ member is updated to match.
 *
 * @param gain Desired PGA gain setting.
 * @return KERNEL_SUCCESS if applied successfully, or driver error code otherwise.
 */
kernel_error_st AnalogReader::set_pga_gain(ADS1115::pga_gain_e gain) {
    kernel_error_st kerr = this->adc_driver_->set_pga(gain);
    if (kerr != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to set PGA gain (error code %d)", kerr);
        return kerr;
    }

    pga_gain_e_ = gain;
    return KERNEL_SUCCESS;
}

/**
 * @brief Set the ADC multiplexer configuration.
 *
 * Attempts to apply the requested MUX setting to the ADC driver. If successful,
 * the cached mux_config_e_ member is updated to match.
 *
 * @param cfg Desired multiplexer configuration.
 * @return KERNEL_SUCCESS if applied successfully, or driver error code otherwise.
 */
kernel_error_st AnalogReader::set_mux_config(ADS1115::mux_config_e cfg) {
    kernel_error_st kerr = this->adc_driver_->set_mux(cfg);
    if (kerr != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to set MUX (error code %d)", kerr);
        return kerr;
    }
    mux_config_e_ = cfg;
    return KERNEL_SUCCESS;
}

/**
 * @brief Get the conversion delay (in ms) for a given ADS1115 data rate.
 *
 * This function returns a safe delay time (rounded up) based on the selected data rate.
 *
 * @return uint16_t Recommended delay in milliseconds.
 */
uint16_t AnalogReader::get_conversion_delay() {
    if (this->adc_driver_ == nullptr) {
        return 0;
    }

    return this->adc_driver_->get_conversion_delay(this->data_rate_e_);
}

/**
 * @brief Get the LSB size (in microvolts) for a given PGA gain setting.
 *
 * This function returns the resolution per bit (µV) corresponding to the PGA gain setting.
 *
 * @return float LSB size in microvolts.
 */
float AnalogReader::get_lsb_size() {
    if (this->adc_driver_ == nullptr) {
        return 0.0f;
    }

    return this->adc_driver_->get_lsb_size(this->pga_gain_e_);
}
