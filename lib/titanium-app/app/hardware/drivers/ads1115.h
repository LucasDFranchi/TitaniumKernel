#pragma once
/*
 * @file ads1115.h
 * @brief ADS1115 ADC driver interface using ESP-IDF I2C APIs.
 *
 * This header defines types, constants, and function prototypes for interacting with
 * the ADS1115 analog-to-digital converter over I2C. It provides fine-grained control
 * of the device via register-level configuration and supports both single-ended and
 * differential measurements, as well as programmable gain, data rate, and comparator options.
 *
 * @details
 * Supported features include:
 * - Full register configuration via bitfield structures
 * - Adjustable PGA gain, sample rate, and input mux selection
 * - Support for single-shot and continuous conversion modes
 * - Basic comparator setup
 * - Low-level read/write access to the device via ESP-IDF I2C
 *
 * @note
 * This driver assumes a working I2C master setup using ESP-IDF. The I2C bus and GPIOs
 * must be configured before using the provided functions.
 *
 * @license Apache License 2.0
 * @author Lucas D. Franchi <LucasD.Franchi@gmail.com>
 */

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#include "kernel/hal/i2c/i2c.h"

/**
 * @def ADS1115_LSB_2_048V
 * @brief LSB (Least Significant Bit) value for ±2.048V PGA setting on ADS1115.
 */
#define ADS1115_LSB_2_048V 0.0000625f

/**
 * @def ADS1115_LSB_4_096V
 * @brief LSB value for ±4.096V PGA setting on ADS1115.
 */
#define ADS1115_LSB_4_096V (ADS1115_LSB_2_048V * 2)

/**
 * @def ADS1115_I2C_ADDRESS
 * @brief Default I2C address for ADS1115 (A0 = GND)
 */
#define ADS1115_I2C_ADDRESS 0x48

/**
 * @enum register_address_et
 * @brief Address pointer values for selecting internal registers.
 */
typedef enum register_address_e {
    REG_ADDR_CONVERSION = 0b00, /**< Conversion register (00b) */
    REG_ADDR_CONFIG     = 0b01, /**< Config register     (01b) */
    REG_ADDR_LO_THRESH  = 0b10, /**< Lo_thresh register   (10b) */
    REG_ADDR_HI_THRESH  = 0b11  /**< Hi_thresh register   (11b) */
} register_address_et;

/**
 * @enum os_status_t
 * @brief Operational status or single-shot conversion control.
 */
typedef enum os_status_ {
    OS_NO_EFFECT         = 0, /**< Write: No effect | Read: Conversion ongoing */
    OS_START_SINGLE_CONV = 1  /**< Write: Start conversion | Read: Idle */
} os_status_t;

/**
 * @enum adc_mux_configuration_et
 * @brief Enumerates ADC input configurations for the ads1115.
 */
typedef enum adc_configuration_e {
    ADC_CONFIG_DIFF_A0_A1 = 0,  /**< Differential input between AIN0 and AIN1 */
    ADC_CONFIG_DIFF_A0_A3,      /**< Differential input between AIN0 and AIN3 */
    ADC_CONFIG_DIFF_A1_A3,      /**< Differential input between AIN1 and AIN3 */
    ADC_CONFIG_DIFF_A2_A3,      /**< Differential input between AIN2 and AIN3 */
    ADC_CONFIG_SINGLE_ENDED_A0, /**< Single-ended input on AIN0 */
    ADC_CONFIG_SINGLE_ENDED_A1, /**< Single-ended input on AIN1 */
    ADC_CONFIG_SINGLE_ENDED_A2, /**< Single-ended input on AIN2 */
    ADC_CONFIG_SINGLE_ENDED_A3, /**< Single-ended input on AIN3 */
    ADC_CONFIG_NONE,            /**< No valid configuration */
} adc_mux_configuration_et;

/**
 * @enum pga_gain_et
 * @brief Programmable Gain Amplifier settings.
 */
typedef enum pga_gain_e {
    PGA_6_144V   = 0b000, /**< ±6.144V (not on ADS1113) */
    PGA_4_096V   = 0b001, /**< ±4.096V */
    PGA_2_048V   = 0b010, /**< ±2.048V (default) */
    PGA_1_024V   = 0b011, /**< ±1.024V */
    PGA_0_512V   = 0b100, /**< ±0.512V */
    PGA_0_256V_1 = 0b101, /**< ±0.256V */
    PGA_0_256V_2 = 0b110, /**< ±0.256V */
    PGA_0_256V_3 = 0b111  /**< ±0.256V */
} pga_gain_et;

/**
 * @enum operating_mode_et
 * @brief Operating mode.
 */
typedef enum operating_mode_e {
    MODE_CONTINUOUS = 0, /**< Continuous conversion */
    MODE_SINGLESHOT = 1  /**< Single-shot mode (default) */
} operating_mode_et;

/**
 * @enum data_rate_et
 * @brief Data rate (samples per second).
 */
typedef enum data_rate_e {
    DR_8SPS   = 0b000, /**< 8 SPS */
    DR_16SPS  = 0b001, /**< 16 SPS */
    DR_32SPS  = 0b010, /**< 32 SPS */
    DR_64SPS  = 0b011, /**< 64 SPS */
    DR_128SPS = 0b100, /**< 128 SPS (default) */
    DR_250SPS = 0b101, /**< 250 SPS */
    DR_475SPS = 0b110, /**< 475 SPS */
    DR_860SPS = 0b111  /**< 860 SPS */
} data_rate_et;

/**
 * @enum comparator_mode_t
 * @brief Comparator mode.
 */
typedef enum comparator_mode_e {
    COMP_MODE_TRADITIONAL = 0, /**< Traditional (default) */
    COMP_MODE_WINDOW      = 1  /**< Window */
} comparator_mode_et;

/**
 * @enum comparator_polarity_et
 * @brief Comparator polarity.
 */
typedef enum comparator_polarity_e {
    COMP_POL_ACTIVE_LOW  = 0, /**< Active low (default) */
    COMP_POL_ACTIVE_HIGH = 1  /**< Active high */
} comparator_polarity_et;

/**
 * @enum comparator_latching_t
 * @brief Latching comparator mode.
 */
typedef enum comparator_latching_e {
    COMP_LAT_NON_LATCHING = 0, /**< Non-latching (default) */
    COMP_LAT_LATCHING     = 1  /**< Latching */
} comparator_latching_et;

/**
 * @enum comparator_queue_et
 * @brief Comparator queue/disable.
 */
typedef enum comparator_queue_e {
    COMP_QUE_ASSERT_1 = 0b00, /**< Assert after 1 conv */
    COMP_QUE_ASSERT_2 = 0b01, /**< Assert after 2 conv */
    COMP_QUE_ASSERT_4 = 0b10, /**< Assert after 4 conv */
    COMP_QUE_DISABLE  = 0b11  /**< Disable (default) */
} comparator_queue_et;

/**
 * @union register_config_ut
 * @brief 16-bit configuration register for ADS111x devices.
 */
typedef union register_config_u {
    /**
     * @struct register_config_u::bits
     * @brief Bit-field access to ADS111x configuration register.
     */
    struct {
        uint16_t comp_que : 2;  /**< Comparator queue/disable (COMP_QUE[1:0]) */
        uint16_t comp_lat : 1;  /**< Latching comparator */
        uint16_t comp_pol : 1;  /**< Comparator polarity (active low/high) */
        uint16_t comp_mode : 1; /**< Comparator mode (traditional/window) */
        uint16_t dr : 3;        /**< Data rate selection (DR[2:0]) */
        uint16_t mode : 1;      /**< Device operating mode (MODE) */
        uint16_t pga : 3;       /**< Programmable Gain Amplifier (PGA[2:0]) */
        uint16_t mux : 3;       /**< Input multiplexer selection (MUX[2:0]) */
        uint16_t os : 1;        /**< Operational status / start single conversion */
    } bits;
    uint16_t value; /**< Raw 16-bit register value */
} register_config_ut;

/**
 * @brief Hardware configuration for an ADS1115 device.
 *
 * Contains the physical connection details required to communicate
 * with the ADS1115 via I2C.
 */
typedef struct ads1115_hw_config_s {
    i2c_port_t port;     /**< I2C port number (e.g., I2C_NUM_0 or I2C_NUM_1) */
    uint8_t dev_address; /**< 7-bit I2C address of the ADS1115 device */
} ads1115_hw_config_st;

/**
 * @brief Complete configuration structure for an ADS1115 device.
 *
 * This structure encapsulates both the hardware configuration (I2C port and address)
 * and the software configuration (register bitfields), along with the I2C interface
 * used to communicate with the device.
 */
typedef struct ads1115_config_s {
    const ads1115_hw_config_st hw_config; /**< I2C port and address for the ADS1115 device */
    register_config_ut config;            /**< ADS1115 configuration register (OS, MUX, PGA, etc.) */
    i2c_interface_st i2c_interface;       /**< I2C interface used to communicate with the device */
} ads1115_config_st;

/**
 * @brief Configure the ADS1115 configuration register.
 *
 * Writes the current configuration from the `reg_cfg.value` field of the given
 * `ads1115_config_st` structure to the ADS1115 configuration register over I2C.
 *
 * This function is typically used after modifying the bitfields to apply the new
 * settings such as input mux, gain, or operating mode.
 *
 * @param[in] dev Pointer to an initialized ADS1115 configuration structure.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL or other esp_err_t codes on I2C failure
 */
esp_err_t ads1115_configure(const ads1115_config_st *dev);

/**
 * @brief Read the raw ADC conversion result from the ADS1115.
 *
 * Retrieves the latest conversion value from the ADS1115 conversion register.
 * The value returned is a 16-bit signed integer in the ADS1115's native format.
 *
 * @param[in]  dev        Pointer to the ADS1115 configuration structure.
 * @param[out] raw_value  Pointer to store the 16-bit signed raw ADC result.
 *
 * @return
 *     - ESP_OK:     Conversion result read successfully
 *     - ESP_FAIL or other esp_err_t: I2C communication error
 */
esp_err_t ads1115_get_raw_value(const ads1115_config_st *dev, int16_t *raw_value);

/**
 * @brief Check if the ADS1115 conversion is complete.
 *
 * Returns the current operational status of the ADS1115 conversion process.
 * In single-shot mode, it indicates whether the latest conversion is finished.
 * In continuous mode, it is always considered "ready".
 *
 * Internally, it reads the OS (Operational Status) bit from the configuration register.
 *
 * @param[in] dev Pointer to an ADS1115 configuration structure.
 *
 * @return
 *     - true: Conversion complete (data ready to read)
 *     - false: Conversion still in progress
 */
bool ads1115_get_conversion_state(const ads1115_config_st *dev);
