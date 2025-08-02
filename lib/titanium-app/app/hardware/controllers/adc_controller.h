#pragma once

#include "kernel/error/error_num.h"

#include "app/hardware/drivers/ads1115.h"

/**
 * @brief Hardware configuration for an ADC.
 */
typedef struct adc_hw_config_s {
    pga_gain_et pga_gain;                    /*!< Programmable Gain Amplifier setting */
    data_rate_et data_rate;                  /*!< Data rate for ADC conversions */
    adc_mux_configuration_et adc_mux_config; /*!< ADC input configuration (single-ended or differential) */
} adc_hw_config_st;

/**
 * @brief Function pointer type for configuring the ADC hardware.
 *
 * Updates the ADC configuration and writes it over I2C.
 *
 * @param adc_hw_config Pointer to hardware config with desired settings.
 * @return kernel_error_st KERNEL_ERROR_NONE on success, otherwise error code.
 */
typedef kernel_error_st (*adc_configure_fn_st)(const adc_hw_config_st *adc_hw_config);

/**
 * @brief Function pointer type for reading raw ADC values.
 *
 * Waits for conversion completion and returns the raw 16-bit ADC result.
 *
 * @param adc_hw_config Pointer to ADC configuration.
 * @param raw_value Pointer to store the raw ADC value.
 * @return kernel_error_st KERNEL_ERROR_NONE on success, otherwise error code.
 */
typedef kernel_error_st (*adc_read_fn_st)(const adc_hw_config_st *adc_hw_config, int16_t *raw_value);

/**
 * @brief Function pointer type for getting the LSB size (resolution) of the ADC for a given PGA gain.
 *
 * @param pga_gain PGA gain enum value from ADS1115 config.
 * @return float LSB size in micro volts.
 */
typedef float (*adc_get_lsb_size_fn_st)(pga_gain_et pga_gain);

/**
 * @brief Function pointer type for selecting the optimal PGA gain based on a coarse voltage measurement.
 *
 * @param coarse_voltage Measured coarse voltage in millivolts.
 * @return pga_gain_et The selected PGA gain enumeration value.
 */
typedef pga_gain_et (*adc_get_pga_gain_fn_st)(uint16_t coarse_voltage);

/**
 * @brief ADC controller struct holding pointers to ADC operations.
 */
typedef struct adc_controller_s {
    adc_configure_fn_st configure;       /**< Configure ADC hardware */
    adc_read_fn_st read;                 /**< Read raw ADC value */
    adc_get_lsb_size_fn_st get_lsb_size; /**< Get LSB size for given PGA */
    adc_get_pga_gain_fn_st get_pga_gain; /**< Get PGA gain setting based on voltage */
} adc_controller_st;

/**
 * @brief Initialize the ADC controller and populate the function pointers.
 *
 * This function initializes the ADC hardware on the first call and assigns the ADC controller’s
 * internal static functions to the provided adc_controller_st structure. This allows the caller
 * to access and use the ADC controller's API via the returned structure.
 *
 * @param adc_controller Pointer to the adc_controller_st struct to be populated.
 *                       Must not be NULL.
 *
 * @return kernel_error_st Returns KERNEL_ERROR_NONE on success,
 *                         KERNEL_ERROR_NULL if adc_controller is NULL,
 *                         or an initialization error code if hardware init fails.
 *
 * @note This function performs hardware initialization only once.
 *       Subsequent calls will skip initialization but still populate the function pointers.
 */
kernel_error_st adc_controller_init(adc_controller_st *adc_controller);
