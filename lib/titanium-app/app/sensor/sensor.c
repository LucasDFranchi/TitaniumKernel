#include "sensor.h"

#include "kernel/logger/logger.h"

#include "app/driver/ads1115.h"
#include "app/driver/tca9548a.h"
#include "app/error/error_num.h"

/**
 * @brief Configuration array for ADS1115 ADC devices.
 *
 * This static array holds pre-configured ADS1115 register settings for two
 * differential input channels:
 *   - Index 0 configures the ADC to read differential inputs A0 and A1.
 *   - Index 1 configures the ADC to read differential inputs A2 and A3.
 *
 * Each entry includes:
 * - The I2C device address (same for both).
 * - The register configuration bits specifying comparator behavior, data rate,
 *   operating mode, programmable gain amplifier (PGA) setting, multiplexer input,
 *   and operational status/start conversion bit.
 *
 * This allows easy selection between the two ADC input configurations
 * during runtime without needing to rebuild the entire configuration.
 */
static const ads1115_config_st ads1115_config[] = {
    [0] = {
        .dev_addr     = ADS1115_I2C_ADDRESS,
        .reg_cfg.bits = {
            .comp_que  = COMP_QUE_DISABLE,
            .comp_lat  = COMP_LAT_NON_LATCHING,
            .comp_pol  = COMP_POL_ACTIVE_LOW,
            .comp_mode = COMP_MODE_TRADITIONAL,
            .dr        = DR_128SPS,
            .mode      = MODE_SINGLESHOT,
            .pga       = PGA_2_048V,
            .mux       = ADC_CONFIG_DIFF_A0_A1,
            .os        = OS_START_SINGLE_CONV,
        },
    },
    [1] = {
        .dev_addr     = ADS1115_I2C_ADDRESS,
        .reg_cfg.bits = {
            .comp_que  = COMP_QUE_DISABLE,
            .comp_lat  = COMP_LAT_NON_LATCHING,
            .comp_pol  = COMP_POL_ACTIVE_LOW,
            .comp_mode = COMP_MODE_TRADITIONAL,
            .dr        = DR_128SPS,
            .mode      = MODE_SINGLESHOT,
            .pga       = PGA_2_048V,
            .mux       = ADC_CONFIG_DIFF_A2_A3,
            .os        = OS_START_SINGLE_CONV,
        },
    },
};

/**
 * @brief Configuration table mapping each sensor channel to its TCA9548A multiplexer and channel.
 *
 * This static array defines the mapping between sensor indices and their corresponding
 * TCA9548A multiplexer I2C address and channel number. It allows the system to route I2C
 * communication to the correct sensor by enabling the appropriate MUX channel before ADC access.
 *
 */
static const tca9548a_config_st tca9548a_config[] = {
    [0]  = {MUX_ADDRESS_0, MUX_CHANNEL_0},
    [1]  = {MUX_ADDRESS_0, MUX_CHANNEL_1},
    [2]  = {MUX_ADDRESS_0, MUX_CHANNEL_2},
    [3]  = {MUX_ADDRESS_0, MUX_CHANNEL_3},
    [4]  = {MUX_ADDRESS_0, MUX_CHANNEL_4},
    [5]  = {MUX_ADDRESS_0, MUX_CHANNEL_5},
    [6]  = {MUX_ADDRESS_0, MUX_CHANNEL_6},
    [7]  = {MUX_ADDRESS_0, MUX_CHANNEL_7},
    [8]  = {MUX_ADDRESS_1, MUX_CHANNEL_0},
    [9]  = {MUX_ADDRESS_1, MUX_CHANNEL_6},
    [10] = {MUX_ADDRESS_1, MUX_CHANNEL_7},
};

/**
 * @brief Hardware configuration structure for a sensor channel.
 *
 * This structure holds the hardware-specific configuration for a sensor, including:
 * - A pointer to the TCA9548A multiplexer configuration (address and channel).
 * - A pointer to the ADS1115 ADC configuration (MUX mode, PGA, data rate, etc).
 *
 * It abstracts the sensor's I2C routing and ADC behavior into a single reusable object.
 */
typedef struct {
    const tca9548a_config_st *mux; /*!< Pointer to the multiplexer config */
    const ads1115_config_st *adc;  /*!< Pointer to the ADC config */
} sensor_hw_st;

/**
 * @brief Enumeration of all available sensor indices in the system.
 *
 * This enum provides symbolic names for each configured sensor channel,
 * distinguishing between temperature sensors (using A0/A1 or A2/A3 differential inputs)
 * and pressure sensors (typically using A0 or A1 single-ended inputs).
 *
 * This allows high-level code to refer to sensors by name instead of hardcoded integers.
 */
typedef enum {
    SENSOR_TEMP_0_A0A1,
    SENSOR_TEMP_0_A2A3,
    SENSOR_TEMP_1_A0A1,
    SENSOR_TEMP_1_A2A3,
    SENSOR_TEMP_2_A0A1,
    SENSOR_TEMP_2_A2A3,
    SENSOR_TEMP_3_A0A1,
    SENSOR_TEMP_3_A2A3,
    SENSOR_TEMP_4_A0A1,
    SENSOR_TEMP_4_A2A3,
    SENSOR_TEMP_5_A0A1,
    SENSOR_TEMP_5_A2A3,
    SENSOR_TEMP_6_A0A1,
    SENSOR_TEMP_6_A2A3,
    SENSOR_TEMP_7_A0A1,
    SENSOR_TEMP_7_A2A3,
    SENSOR_TEMP_8_A0A1,
    SENSOR_TEMP_8_A2A3,
    SENSOR_TEMP_9_A0A1,
    SENSOR_TEMP_9_A2A3,
    SENSOR_PRESSURE_0_A0,
    SENSOR_PRESSURE_0_A1,
    SENSOR_COUNT
} sensor_index_et;

/**
 * @brief Hardware configuration mapping for sensors.
 *
 * This static array maps each sensor index (from the sensor_index_et enum)
 * to its corresponding hardware configuration, which includes:
 * - The TCA9548A multiplexer configuration (I2C address and channel)
 * - The ADS1115 ADC configuration (device address and input multiplexer setup)
 */
static const sensor_hw_st sensor_hw[] = {
    [SENSOR_TEMP_0_A0A1]   = {&tca9548a_config[0], &ads1115_config[0]},
    [SENSOR_TEMP_0_A2A3]   = {&tca9548a_config[0], &ads1115_config[1]},
    [SENSOR_TEMP_1_A0A1]   = {&tca9548a_config[1], &ads1115_config[0]},
    [SENSOR_TEMP_1_A2A3]   = {&tca9548a_config[1], &ads1115_config[1]},
    [SENSOR_TEMP_2_A0A1]   = {&tca9548a_config[2], &ads1115_config[0]},
    [SENSOR_TEMP_2_A2A3]   = {&tca9548a_config[2], &ads1115_config[1]},
    [SENSOR_TEMP_3_A0A1]   = {&tca9548a_config[3], &ads1115_config[0]},
    [SENSOR_TEMP_3_A2A3]   = {&tca9548a_config[3], &ads1115_config[1]},
    [SENSOR_TEMP_4_A0A1]   = {&tca9548a_config[4], &ads1115_config[0]},
    [SENSOR_TEMP_4_A2A3]   = {&tca9548a_config[4], &ads1115_config[1]},
    [SENSOR_TEMP_5_A0A1]   = {&tca9548a_config[5], &ads1115_config[0]},
    [SENSOR_TEMP_5_A2A3]   = {&tca9548a_config[5], &ads1115_config[1]},
    [SENSOR_TEMP_6_A0A1]   = {&tca9548a_config[6], &ads1115_config[0]},
    [SENSOR_TEMP_6_A2A3]   = {&tca9548a_config[6], &ads1115_config[1]},
    [SENSOR_TEMP_7_A0A1]   = {&tca9548a_config[7], &ads1115_config[0]},
    [SENSOR_TEMP_7_A2A3]   = {&tca9548a_config[7], &ads1115_config[1]},
    [SENSOR_TEMP_8_A0A1]   = {&tca9548a_config[8], &ads1115_config[0]},
    [SENSOR_TEMP_8_A2A3]   = {&tca9548a_config[8], &ads1115_config[1]},
    [SENSOR_TEMP_9_A0A1]   = {&tca9548a_config[9], &ads1115_config[0]},
    [SENSOR_TEMP_9_A2A3]   = {&tca9548a_config[9], &ads1115_config[1]},
    [SENSOR_PRESSURE_0_A0] = {&tca9548a_config[10], &ads1115_config[0]},
    [SENSOR_PRESSURE_0_A1] = {&tca9548a_config[10], &ads1115_config[1]},
};

/**
 * @brief Sensor configuration structure.
 *
 * Defines the properties for each sensor channel, including:
 * - The sensor type (temperature, pressure, etc.)
 * - Pointer to the hardware configuration struct (mux and ADC settings)
 * - Conversion gain applied to the voltage reading
 * - Offset voltage to subtract from the raw measurement
 *
 * This table maps each sensor index to its configuration for easy access and management.
 */
typedef struct sensor_info_s {
    sensor_type_et type;    /**< Type of sensor */
    const sensor_hw_st *hw; /**< Pointer to hardware configuration (MUX and ADC) */
    float conversion_gain;  /**< Gain factor applied after voltage calculation */
    float offset;           /**< Voltage offset to subtract from the measured value */
} sensor_info_st;

static const sensor_info_st sensor_info[] = {
    // --- MUX_ADDRESS_0: Channels 0-7, Temp Sensors ---
    [SENSOR_TEMP_0_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_0_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_0_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_0_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_1_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_1_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_1_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_1_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_2_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_2_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_2_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_2_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_3_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_3_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_3_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_3_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_4_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_4_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_4_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_4_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_5_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_5_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_5_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_5_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_6_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_6_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_6_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_6_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_7_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_7_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_7_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_7_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_8_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_8_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_8_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_8_A2A3], 1.0f, 0.0f},
    [SENSOR_TEMP_9_A0A1]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_9_A0A1], 1.0f, 0.0f},
    [SENSOR_TEMP_9_A2A3]   = {SENSOR_TYPE_TEMPERATURE, &sensor_hw[SENSOR_TEMP_9_A2A3], 1.0f, 0.0f},
    [SENSOR_PRESSURE_0_A0] = {SENSOR_TYPE_PRESSURE, &sensor_hw[SENSOR_PRESSURE_0_A0], 1.0f, 0.0f},
    [SENSOR_PRESSURE_0_A1] = {SENSOR_TYPE_PRESSURE, &sensor_hw[SENSOR_PRESSURE_0_A1], 1.0f, 0.0f},

};

/* Hardware Constant Definitions */
static const uint8_t SDA_IO             = 21;              /*!< gpio number for I2C master data  */
static const uint8_t SCL_IO             = 22;              /*!< gpio number for I2C master clock */
static const uint32_t FREQ_HZ           = 100000;          /*!< I2C master clock frequency */
static const uint8_t TX_BUF_DISABLE     = 0;               /*!< I2C master doesn't need buffer */
static const uint8_t RX_BUF_DISABLE     = 0;               /*!< I2C master doesn't need buffer */
static const uint8_t I2C_NUM            = I2C_NUM_0;       /*!< I2C number */
static const uint8_t I2C_MODE           = I2C_MODE_MASTER; /*!< I2C mode to act as */
static const uint8_t I2C_RX_BUF_STATE   = RX_BUF_DISABLE;  /*!< I2C set rx buffer status */
static const uint8_t I2C_TX_BUF_STATE   = TX_BUF_DISABLE;  /*!< I2C set tx buffer status */
static const uint8_t I2C_INTR_ALOC_FLAG = 0;               /*!< I2C set interrupt allocation flag */

/* Global Variables */
static tca9548a_config_st *last_mux = (tca9548a_config_st *)&tca9548a_config[0]; /*!< Last used multiplexer configuration */
static const char *TAG              = "Sensor Manager";                          /*!< Tag used for logging */

/**
 * @brief Configure the multiplexer and ADC for a specific sensor.
 *
 * This function sets the correct MUX address and channel, configures the ads1115
 * according to the sensor's configuration, and triggers a new conversion.
 *
 * @param sensor_index Index into the sensor_info table.
 * @return kernel_error_st KERNEL_ERROR_NONE on success, or KERNEL_ERROR_INVALID_ARG if index is out of bounds.
 */
kernel_error_st sensor_configure(uint8_t sensor_index) {
    if (sensor_index >= NUM_OF_CHANNELS) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (sensor_info[sensor_index].hw->mux == NULL || sensor_info[sensor_index].hw->adc == NULL) {
        logger_print(ERR, TAG, "Invalid sensor configuration for index %d\n", sensor_index);
        return KERNEL_ERROR_NULL;
    }

    if (sensor_info[sensor_index].hw->mux != last_mux) {
        tca9548a_disable_all_channels(last_mux);
    }
    last_mux = (tca9548a_config_st *)sensor_info[sensor_index].hw->mux;

    if (tca9548a_enable_channel(sensor_info[sensor_index].hw->mux) != ESP_OK) {
        logger_print(ERR, TAG,
                     "Failed to set MUX channel %d for sensor %d\n",
                     sensor_info[sensor_index].hw->mux->channel,
                     sensor_index);
        return APP_ERROR_MUX_CHANNEL_ERROR;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    if (ads1115_update(sensor_info[sensor_index].hw->adc) != ESP_OK) {
        logger_print(ERR, TAG,
                     "Failed to update ADS1115 configuration for sensor %d\n",
                     sensor_index);
        return APP_ERROR_ADC_UPDATE_ERROR;
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Initializes the I2C interface, GPIOs, and ADS1115 ADC.
 *
 * This function sets up GPIO used to reset the TCA9548A multiplexers,
 * configures and installs the I2C driver, and initializes the ADS1115 with
 * default comparator and operation settings.
 *
 * This function must be called before any sensor operations.
 */
void sensor_manager_initialize(void) {
    gpio_config_t io_conf_tca_reset = {
        .pin_bit_mask = (1ULL << GPIO_NUM_27),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE};
    ESP_ERROR_CHECK(gpio_config(&io_conf_tca_reset));

    i2c_config_t i2c_cfg = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = SDA_IO,
        .scl_io_num       = SCL_IO,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &i2c_cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, I2C_MODE, I2C_RX_BUF_STATE, I2C_TX_BUF_STATE, I2C_INTR_ALOC_FLAG));

    tca9548a_reset(GPIO_NUM_27);
}

/**
 * @brief Reads the voltage for a given sensor channel.
 *
 * This function waits for the ADC conversion to complete, retrieves the raw ADC value,
 * and converts it to a voltage using the sensor's configuration.
 *
 * @param sensor_index Index into the sensor_info table.
 * @param voltage Pointer to a float where the resulting voltage will be stored.
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success,
 *         - KERNEL_ERROR_INVALID_ARG if index is invalid,
 *         - KERNEL_ERROR_ADC_CONVERSION_ERROR if the ADC times out.
 */
kernel_error_st sensor_get_voltage(uint8_t sensor_index, float *voltage) {
    if (sensor_index >= NUM_OF_CHANNELS) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    if (sensor_configure(sensor_index) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to configure sensor %d\n", sensor_index);
        return APP_ERROR_ADC_CONFIGURE_ERROR;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    int retries = 10;
    while (!ads1115_get_conversion_state(sensor_info[sensor_index].hw->adc) && retries > 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
        retries--;
    }
    if (retries <= 0) {
        logger_print(ERR, TAG, "ADC conversion timed out for sensor %d\n", sensor_index);
        return APP_ERROR_ADC_CONVERSION_ERROR;
    }

    int16_t raw_value      = ads1115_get_raw_value(sensor_info[sensor_index].hw->adc);
    float internal_voltage = 0.0f;

    switch (sensor_info[sensor_index].hw->adc->reg_cfg.bits.pga) {
        case PGA_2_048V:
            internal_voltage = raw_value * ADS1115_LSB_2_048V;
            break;
        case PGA_4_096V:
            internal_voltage = raw_value * ADS1115_LSB_4_096V;
            break;
        default:
            logger_print(ERR, TAG, "Unsupported PGA setting for sensor %d\n", sensor_index);
            return KERNEL_ERROR_INVALID_ARG;
    }

    internal_voltage -= sensor_info[sensor_index].offset;
    *voltage = internal_voltage * sensor_info[sensor_index].conversion_gain;

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Returns the sensor type (temperature or pressure) for the given index.
 *
 * @param sensor_index Index into the sensor_info table.
 * @return sensor_type_et The sensor type, or SENSOR_TYPE_UNDEFINED if index is invalid.
 */
sensor_type_et sensor_get_type(uint8_t sensor_index) {
    if (sensor_index >= NUM_OF_CHANNELS) {
        return SENSOR_TYPE_UNDEFINED;
    }

    return sensor_info[sensor_index].type;
}
