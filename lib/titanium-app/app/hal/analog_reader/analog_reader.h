#pragma once
#include "kernel/error/error_num.h"

#include "app/hal/drivers/ads1115.h"

class AnalogReader {
   public:
    AnalogReader(ADS1115* adc_driver, ADS1115::data_rate_e data_rate_e, ADS1115::pga_gain_e pga_gain_e, ADS1115::mux_config_e mux_config_e)
        : adc_driver_(adc_driver),
          data_rate_e_(data_rate_e),
          pga_gain_e_(pga_gain_e),
          mux_config_e_(mux_config_e) {}

    ~AnalogReader() = default;

    static constexpr char TAG[]               = "AnalogReader";
    static constexpr int MAX_ADC_READ_RETRIES = 10;
    static constexpr int ADC_READ_DELAY_MS    = 10;

    /**
     * @brief Initialize the AnalogReader with the desired ADC configuration.
     *
     * This function applies data rate, gain, and mux settings to the ADC driver
     * using the safe setters. If any of these fail, the initialization aborts
     * and the corresponding error is returned.
     *
     * @return KERNEL_SUCCESS on success, or the error code from the failing setter.
     */
    kernel_error_st initialize();

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
    kernel_error_st read_adc(int16_t& raw_value);

    /**
     * @brief Get the current ADC data rate setting.
     *
     * This returns the cached data rate that was last successfully applied
     * to the ADC via set_data_rate() or init().
     *
     * @return The current ADS1115::data_rate_e value.
     */
    ADS1115::data_rate_e get_data_rate() const {
        return data_rate_e_;
    }

    /**
     * @brief Get the current ADC PGA (programmable gain amplifier) gain.
     *
     * This returns the cached PGA gain that was last successfully applied
     * to the ADC via set_pga_gain() or init().
     *
     * @return The current ADS1115::pga_gain_e value.
     */
    ADS1115::pga_gain_e get_pga_gain() const {
        return pga_gain_e_;
    }

    /**
     * @brief Get the current ADC multiplexer configuration.
     *
     * This returns the cached MUX configuration that was last successfully applied
     * to the ADC via set_mux_config() or init().
     *
     * @return The current ADS1115::mux_config_e value.
     */
    ADS1115::mux_config_e get_mux_config() const {
        return mux_config_e_;
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
    kernel_error_st set_data_rate(ADS1115::data_rate_e rate);

    /**
     * @brief Set the ADC programmable gain amplifier (PGA) gain.
     *
     * Attempts to apply the requested gain to the ADC driver. If successful,
     * the cached pga_gain_e_ member is updated to match.
     *
     * @param gain Desired PGA gain setting.
     * @return KERNEL_SUCCESS if applied successfully, or driver error code otherwise.
     */
    kernel_error_st set_pga_gain(ADS1115::pga_gain_e gain);

    /**
     * @brief Set the ADC multiplexer configuration.
     *
     * Attempts to apply the requested MUX setting to the ADC driver. If successful,
     * the cached mux_config_e_ member is updated to match.
     *
     * @param cfg Desired multiplexer configuration.
     * @return KERNEL_SUCCESS if applied successfully, or driver error code otherwise.
     */
    kernel_error_st set_mux_config(ADS1115::mux_config_e cfg);

    /**
     * @brief Get the conversion delay (in ms) for a given ADS1115 data rate.
     *
     * This function returns a safe delay time (rounded up) based on the selected data rate.
     *
     * @return uint16_t Recommended delay in milliseconds.
     */
    uint16_t get_conversion_delay();

    /**
     * @brief Get the LSB size (in microvolts) for a given PGA gain setting.
     *
     * This function returns the resolution per bit (µV) corresponding to the PGA gain setting.
     *
     * @return float LSB size in microvolts.
     */
    float get_lsb_size();

   private:
    ADS1115* adc_driver_ = nullptr;
    ADS1115::data_rate_e data_rate_e_;
    ADS1115::pga_gain_e pga_gain_e_;
    ADS1115::mux_config_e mux_config_e_;
    bool initialized_ = false;
};
