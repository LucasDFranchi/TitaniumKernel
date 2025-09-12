#pragma once
#include "kernel/error/error_num.h"
#include "kernel/hal/i2c/i2c_handler.h"
#include <stdbool.h>
#include <cstdint>

/**
 * @brief ADS1115 ADC driver class using ESP-IDF I2C APIs.
 *
 * Provides access to single-ended and differential measurements,
 * programmable gain, data rate, and comparator options.
 */
class Ads1115Driver {
   public:
    // Constants
    static constexpr float LSB_2_048V            = 0.0000625f;
    static constexpr float LSB_4_096V            = LSB_2_048V * 2;
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x48;
    static constexpr uint8_t ADS_RW_BUFF_SIZE    = 2;  // Size of the read/write buffer

    // Enumerations
    enum class register_address : uint8_t {
        conversion   = 0b00,
        config       = 0b01,
        lo_threshold = 0b10,
        hi_threshold = 0b11
    };

    enum class os_status : uint8_t {
        no_effect               = 0,
        start_single_conversion = 1
    };

    enum class mux_config : uint8_t {
        diff_a0_a1 = 0,
        diff_a0_a3,
        diff_a1_a3,
        diff_a2_a3,
        single_a0,
        single_a1,
        single_a2,
        single_a3,
        none
    };

    enum class pga_gain : uint8_t {
        v6_144   = 0b000,
        v4_096   = 0b001,
        v2_048   = 0b010,
        v1_024   = 0b011,
        v0_512   = 0b100,
        v0_256_1 = 0b101,
        v0_256_2 = 0b110,
        v0_256_3 = 0b111
    };

    enum class mode : uint8_t {
        continuous  = 0,
        single_shot = 1
    };

    enum class data_rate : uint8_t {
        sps_8 = 0b000,
        sps_16,
        sps_32,
        sps_64,
        sps_128,
        sps_250,
        sps_475,
        sps_860
    };

    enum class comparator_mode : uint8_t {
        traditional = 0,
        window
    };

    enum class comparator_polarity : uint8_t {
        active_low = 0,
        active_high
    };

    enum class comparator_latching : uint8_t {
        non_latching = 0,
        latching
    };

    enum class comparator_queue : uint8_t {
        assert_1 = 0b00,
        assert_2 = 0b01,
        assert_4 = 0b10,
        disable  = 0b11
    };

    /**
     * @brief Union representing the 16-bit ADS1115 configuration register.
     */
    union register_config {
        struct {
            uint16_t comp_que : 2;
            uint16_t comp_lat : 1;
            uint16_t comp_pol : 1;
            uint16_t comp_mode : 1;
            uint16_t dr : 3;
            uint16_t mode : 1;
            uint16_t pga : 3;
            uint16_t mux : 3;
            uint16_t os : 1;
        } bits;
        uint16_t value = 0;
    };

    /**
     * @brief Construct an Ads1115Driver.
     * @param driver Pointer to an initialized I2C Hal (must remain valid for lifetime of this object)
     */
    explicit Ads1115Driver(I2CHandler* i2c_hal) : i2c_handler_(i2c_hal) {}

    ~Ads1115Driver() = default;

    /**
     * @brief Apply current configuration to the ADS1115 device.
     *
     * @return esp_err_t ESP_OK on success, otherwise I2C error code.
     */
    kernel_error_st configure();

    /**
     * @brief Read raw 16-bit ADC value from the ADS1115.
     *
     * @param raw_value Reference to store the ADC value.
     * @return esp_err_t ESP_OK on success, otherwise I2C error code.
     */
    kernel_error_st get_raw_value(int16_t& raw_value);

    /**
     * @brief Check whether the ADC conversion is complete.
     *
     * @return true Conversion complete.
     * @return false Conversion in progress.
     */
    bool is_conversion_complete();

    /**
     * @brief Set the ADC input multiplexer configuration.
     *
     * @param mux The input channel configuration to use.
     * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
     */
    kernel_error_st set_mux(mux_config mux);

    /**
     * @brief Set the programmable gain amplifier (PGA) setting.
     *
     * @param gain The PGA gain to use.
     * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
     */
    kernel_error_st set_pga(pga_gain gain);

    /**
     * @brief Set the operating mode of the ADC.
     *
     * @param m The operating mode (continuous or single-shot).
     * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
     */
    kernel_error_st set_mode(mode mode);

    /**
     * @brief Set the ADC data rate (samples per second).
     *
     * @param rate The data rate setting.
     * @return kernel_error_st KERNEL_SUCCESS if valid, KERNEL_ERROR_INVALID_ARG otherwise.
     */
    kernel_error_st set_data_rate(data_rate rate);
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
    kernel_error_st set_comparator(comparator_mode mode, comparator_polarity pol, comparator_latching latch, comparator_queue queue);

   private:
    register_config config_;
    I2CHandler* i2c_handler_;

    kernel_error_st write_register(register_address address, uint16_t value);
    kernel_error_st read_register(register_address address, uint16_t& value);
};
