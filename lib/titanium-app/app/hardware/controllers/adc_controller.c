#include "adc_controller.h"

#include "kernel/hal/i2c/i2c.h"
#include "kernel/logger/logger.h"

static const uint16_t PGA_256_LIMIT_VOLTAGE  = 256;   ///< Max measurable voltage for ±0.256 V PGA in mV
static const uint16_t PGA_512_LIMIT_VOLTAGE  = 512;   ///< Max measurable voltage for ±0.512 V PGA in mV
static const uint16_t PGA_1024_LIMIT_VOLTAGE = 1024;  ///< Max measurable voltage for ±1.024 V PGA in mV
static const uint16_t PGA_2048_LIMIT_VOLTAGE = 2048;  ///< Max measurable voltage for ±2.048 V PGA in mV
static const uint16_t PGA_4096_LIMIT_VOLTAGE = 4096;  ///< Max measurable voltage for ±4.096 V PGA in mV

static const uint16_t FULL_SCALE_MV_6_144 = 6144;  ///< Full-scale voltage for ±6.144 V PGA
static const uint16_t FULL_SCALE_MV_4_096 = 4096;  ///< Full-scale voltage for ±4.096 V PGA
static const uint16_t FULL_SCALE_MV_2_048 = 2048;  ///< Full-scale voltage for ±2.048 V PGA
static const uint16_t FULL_SCALE_MV_1_024 = 1024;  ///< Full-scale voltage for ±1.024 V PGA
static const uint16_t FULL_SCALE_MV_0_512 = 512;   ///< Full-scale voltage for ±0.512 V PGA
static const uint16_t FULL_SCALE_MV_0_256 = 256;   ///< Full-scale voltage for ±0.256 V PGA

static const float ADC_RESOLUTION = 32768.0f;  ///< 16-bit signed ADC range (±32768 counts)

static const uint16_t DELAY_8SPS    = 125;  ///< Delay in ms for 8 samples/sec (~125 ms conversion time)
static const uint16_t DELAY_16SPS   = 63;   ///< Delay in ms for 16 samples/sec (~62.5 ms conversion time)
static const uint16_t DELAY_32SPS   = 32;   ///< Delay in ms for 32 samples/sec (~31.25 ms conversion time)
static const uint16_t DELAY_64SPS   = 16;   ///< Delay in ms for 64 samples/sec (~15.625 ms conversion time)
static const uint16_t DELAY_128SPS  = 8;    ///< Delay in ms for 128 samples/sec (~7.8125 ms conversion time)
static const uint16_t DELAY_250SPS  = 4;    ///< Delay in ms for 250 samples/sec (~4 ms conversion time)
static const uint16_t DELAY_475SPS  = 3;    ///< Delay in ms for 475 samples/sec (~2.105 ms conversion time)
static const uint16_t DELAY_860SPS  = 2;    ///< Delay in ms for 860 samples/sec (~1.163 ms conversion time)
static const uint16_t DELAY_DEFAULT = 125;  ///< Fallback delay for unknown data rates

/**
 * @brief Static configuration instance for the ADS1115 ADC.
 *
 * This structure holds the I2C communication details, ADS1115 register configuration,
 * and the resolved I2C interface for communication. It is initialized once and used
 * throughout the ADC controller module.
 */
static ads1115_config_st ads1115_config = {
    .hw_config = {
        .port        = I2C_NUM_0,           /*!< I2C hardware port used by the ADS1115 (e.g., I2C_NUM_0 or I2C_NUM_1) */
        .dev_address = ADS1115_I2C_ADDRESS, /*!< 7-bit I2C address of the ADS1115 device (usually 0x48–0x4B) */
    },
    .config.bits = {
        .comp_que  = COMP_QUE_DISABLE,           /*!< Disable comparator functionality */
        .comp_lat  = COMP_LAT_NON_LATCHING,      /*!< Non-latching comparator output */
        .comp_pol  = COMP_POL_ACTIVE_LOW,        /*!< Comparator active low polarity */
        .comp_mode = COMP_MODE_TRADITIONAL,      /*!< Traditional comparator (vs. window) */
        .dr        = DR_8SPS,                    /*!< Data rate: 8 samples per second (maximum conversion time ~125 ms) */
        .mode      = MODE_SINGLESHOT,            /*!< Perform single-shot conversion and power down between reads */
        .pga       = PGA_2_048V,                 /*!< Programmable gain amplifier: ±2.048V full-scale range */
        .mux       = ADC_CONFIG_SINGLE_ENDED_A1, /*!< Input MUX: single-ended read from channel A1 */
        .os        = OS_START_SINGLE_CONV,       /*!< Start a single conversion on write */
    },
    .i2c_interface = {0}, /*!< Filled at runtime by the I2C HAL layer with a valid driver reference */
};

/* Global Variables */
static const char *TAG     = "ADC Controller";
static bool is_initialized = false;

/**
 * @brief Get the conversion delay (in ms) for a given ADS1115 data rate.
 *
 * This function returns a safe delay time (rounded up) based on the selected data rate.
 *
 * @param data_rate Data rate setting from the ADS1115 config.
 * @return uint16_t Recommended delay in milliseconds.
 */
static uint16_t get_conversion_delay(data_rate_et data_rate) {
    switch (data_rate) {
        case DR_8SPS:
            return DELAY_8SPS;
        case DR_16SPS:
            return DELAY_16SPS;
        case DR_32SPS:
            return DELAY_32SPS;
        case DR_64SPS:
            return DELAY_64SPS;
        case DR_128SPS:
            return DELAY_128SPS;
        case DR_250SPS:
            return DELAY_250SPS;
        case DR_475SPS:
            return DELAY_475SPS;
        case DR_860SPS:
            return DELAY_860SPS;
        default:
            return DELAY_DEFAULT;
    }
}

/**
 * @brief Initialize the ADC controller by acquiring the I2C interface.
 *
 * This must be called once before using any other ADC controller functions.
 *
 * @return kernel_error_st KERNEL_ERROR_NONE on success, otherwise error code.
 */
static kernel_error_st adc_initialize(void) {
    esp_err_t err = i2c_get_interface(ads1115_config.hw_config.port, &ads1115_config.i2c_interface);
    if (err != ESP_OK) {
        logger_print(ERR, TAG, "Failed to get I2C interface for ADC: %s\n", esp_err_to_name(err));
        return KERNEL_ERROR_INVALID_INTERFACE;
    }

    is_initialized = true;

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Configure the ADS1115 with new ADC settings.
 *
 * Updates the internal ADS1115 config structure and writes the configuration over I2C.
 *
 * @param adc_hw_config Pointer to hardware config with desired settings.
 * @return kernel_error_st KERNEL_ERROR_NONE on success, otherwise error code.
 */
static kernel_error_st configure(const adc_hw_config_st *adc_hw_config) {
    if (!adc_hw_config) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    ads1115_config.config.bits.pga = adc_hw_config->pga_gain;
    ads1115_config.config.bits.dr  = adc_hw_config->data_rate;
    ads1115_config.config.bits.mux = adc_hw_config->adc_mux_config;

    esp_err_t err = ads1115_configure(&ads1115_config);

    if (err != ESP_OK) {
        logger_print(ERR, TAG, "Failed to configure ADC: %s\n", esp_err_to_name(err));
        return KERNEL_ERROR_ADC_CONFIGURE_ERROR;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Read the raw ADC value after starting a conversion.
 *
 * Assumes configuration has already been applied. Waits for conversion completion
 * using polling and returns the raw 16-bit ADC result.
 *
 * @param adc_hw_config Pointer to configuration (must match previous configure call).
 * @param raw_value Pointer to store the raw ADC value.
 * @return kernel_error_st KERNEL_ERROR_NONE on success, otherwise error code.
 */
static kernel_error_st read(const adc_hw_config_st *adc_hw_config, int16_t *raw_value) {
    if ((adc_hw_config == NULL) || (raw_value == NULL)) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (adc_hw_config->pga_gain != ads1115_config.config.bits.pga ||
        adc_hw_config->data_rate != ads1115_config.config.bits.dr ||
        adc_hw_config->adc_mux_config != ads1115_config.config.bits.mux) {
        logger_print(ERR, TAG, "ADC configuration mismatch. Please call adc_controller_configure first.\n");

        logger_print(ERR, TAG,
                     "Expected: PGA=%d, DR=%d, MUX=%d; Got: PGA=%d, DR=%d, MUX=%d\n",
                     ads1115_config.config.bits.pga,
                     ads1115_config.config.bits.dr,
                     ads1115_config.config.bits.mux,
                     adc_hw_config->pga_gain,
                     adc_hw_config->data_rate,
                     adc_hw_config->adc_mux_config);

        return KERNEL_ERROR_ADC_CONFIG_MISMATCH_ERROR;
    }

    uint8_t retries = 3;
    while (!ads1115_get_conversion_state(&ads1115_config) && retries > 0) {
        vTaskDelay(pdMS_TO_TICKS(get_conversion_delay(adc_hw_config->data_rate)));
        retries--;
    }

    if (retries <= 0) {
        logger_print(ERR, TAG, "ADC conversion timed out for sensor\n");
        return KERNEL_ERROR_ADC_CONVERSION_ERROR;
    }

    esp_err_t err = ads1115_get_raw_value(&ads1115_config, raw_value);
    if (err != ESP_OK) {
        logger_print(ERR, TAG, "Failed to read ADC: %s\n", esp_err_to_name(err));
        return KERNEL_ERROR_ADC_READ_ERROR;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Get the LSB size (in microvolts) for a given PGA gain setting.
 *
 * This function returns the resolution per bit (µV) corresponding to the PGA gain setting.
 *
 * @param pga_gain PGA gain enum value from ADS1115 config.
 * @return float LSB size in microvolts.
 */
static float get_lsb_size(pga_gain_et pga_gain) {
    switch (pga_gain) {
        case PGA_6_144V:
            return (FULL_SCALE_MV_6_144 / ADC_RESOLUTION);
        case PGA_4_096V:
            return (FULL_SCALE_MV_4_096 / ADC_RESOLUTION);
        case PGA_2_048V:
            return (FULL_SCALE_MV_2_048 / ADC_RESOLUTION);
        case PGA_1_024V:
            return (FULL_SCALE_MV_1_024 / ADC_RESOLUTION);
        case PGA_0_512V:
            return (FULL_SCALE_MV_0_512 / ADC_RESOLUTION);
        case PGA_0_256V_1:
        case PGA_0_256V_2:
        case PGA_0_256V_3:
            return (FULL_SCALE_MV_0_256 / ADC_RESOLUTION);
        default:
            return (FULL_SCALE_MV_6_144 / ADC_RESOLUTION);
    }
}

/**
 * @brief Determine the optimal PGA gain setting based on a coarse ADC voltage measurement.
 *
 * This function selects the smallest possible Programmable Gain Amplifier (PGA) range
 * on the ADS1115 that can still measure the given voltage without clipping. A configurable
 * clipping percentage is applied to avoid saturation near the PGA's maximum range.
 *
 * The input voltage is in millivolts and is compared against predefined PGA limit voltages.
 *
 * @param coarse_voltage Measured coarse voltage in millivolts.
 * @return pga_gain_et The selected PGA gain enumeration value.
 */
static pga_gain_et get_pga_gain(uint16_t coarse_voltage) {
    static const uint8_t CLIPPING_PERCENT = 95;

    uint16_t clipped_voltage = (coarse_voltage * CLIPPING_PERCENT) / 100;

    if (clipped_voltage <= PGA_256_LIMIT_VOLTAGE) {
        return PGA_0_256V_1;
    } else if (clipped_voltage <= PGA_512_LIMIT_VOLTAGE) {
        return PGA_0_512V;
    } else if (clipped_voltage <= PGA_1024_LIMIT_VOLTAGE) {
        return PGA_1_024V;
    } else if (clipped_voltage <= PGA_2048_LIMIT_VOLTAGE) {
        return PGA_2_048V;
    } else if (clipped_voltage <= PGA_4096_LIMIT_VOLTAGE) {
        return PGA_4_096V;
    }

    return PGA_6_144V;
}

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
kernel_error_st adc_controller_init(adc_controller_st *adc_controller) {
    if (!adc_controller) {
        return KERNEL_ERROR_NULL;
    }

    if (!is_initialized) {
        kernel_error_st err = adc_initialize();
        if (err != KERNEL_ERROR_NONE) {
            return err;
        }
    }

    adc_controller->configure    = configure;
    adc_controller->read         = read;
    adc_controller->get_lsb_size = get_lsb_size;
    adc_controller->get_pga_gain = get_pga_gain;

    return KERNEL_ERROR_NONE;
}
