#include "sensor_manager.h"

#include "kernel/device/device_info.h"
#include "kernel/error/error_num.h"
#include "kernel/hal/i2c/i2c.h"
#include "kernel/logger/logger.h"

#include "app/app_extern_types.h"
#include "app/hardware/controllers/adc_controller.h"
#include "app/hardware/controllers/mux_controller.h"
#include "app/sensors/sensor/ntc_temperature.h"
#include "app/sensors/sensor/power_sensor.h"
#include "app/sensors/sensor/pressure_sensor.h"
#include "app/sensors/sensor_interface/sensor_interface.h"

/* Global Variables */
static const char *TAG                                = "Sensor Manager"; /*!< Tag used for logging */
static sensor_manager_config_st sensor_manager_config = {0};              /*!< Configuration for sensor manager */
static mux_controller_st mux_controller               = {0};
static adc_controller_st adc_controller               = {0};

static sensor_hw_st sensor_hw[NUM_OF_CHANNEL_SENSORS] = {
    [SENSOR_CH_00] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_4},
    },
    [SENSOR_CH_01] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_4},
    },
    [SENSOR_CH_02] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_5},
    },
    [SENSOR_CH_03] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_5},
    },
    [SENSOR_CH_04] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_05] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_06] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_07] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_08] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_09] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_10] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_11] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_12] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_3},
    },
    [SENSOR_CH_13] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_3},
    },
    [SENSOR_CH_14] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_2},
    },
    [SENSOR_CH_15] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_2},
    },
    [SENSOR_CH_16] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_1},
    },
    [SENSOR_CH_17] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_1},
    },
    [SENSOR_CH_18] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_0},
    },
    [SENSOR_CH_19] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_0},
    },
    [SENSOR_CH_20] = {
        .adc_ref_branch    = {0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_0},
    },
    [SENSOR_CH_21] = {
        .adc_ref_branch    = {0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_8SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_0},
    },
    [SENSOR_CH_22] = {
        .adc_ref_branch    = {0},
        .adc_sensor_branch = {0},
        .mux_hw_config     = {0},
    },
};

static sensor_interface_st sensor_interface[NUM_OF_SENSORS] = {
    [SENSOR_ID_00] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_00,
        .hw              = &sensor_hw[SENSOR_CH_00],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_01] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_01,
        .hw              = &sensor_hw[SENSOR_CH_01],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_02] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_02,
        .hw              = &sensor_hw[SENSOR_CH_02],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_03] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_03,
        .hw              = &sensor_hw[SENSOR_CH_03],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_04] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_04,
        .hw              = &sensor_hw[SENSOR_CH_04],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_05] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_05,
        .hw              = &sensor_hw[SENSOR_CH_05],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_06] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_06,
        .hw              = &sensor_hw[SENSOR_CH_06],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_07] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_07,
        .hw              = &sensor_hw[SENSOR_CH_07],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_08] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_08,
        .hw              = &sensor_hw[SENSOR_CH_08],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_09] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_09,
        .hw              = &sensor_hw[SENSOR_CH_09],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_10] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_10,
        .hw              = &sensor_hw[SENSOR_CH_10],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_11] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_11,
        .hw              = &sensor_hw[SENSOR_CH_11],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_12] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_12,
        .hw              = &sensor_hw[SENSOR_CH_12],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_13] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_13,
        .hw              = &sensor_hw[SENSOR_CH_13],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_14] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_14,
        .hw              = &sensor_hw[SENSOR_CH_14],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_15] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_15,
        .hw              = &sensor_hw[SENSOR_CH_15],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_16] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_16,
        .hw              = &sensor_hw[SENSOR_CH_16],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_17] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_17,
        .hw              = &sensor_hw[SENSOR_CH_17],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_18] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_18,
        .hw              = &sensor_hw[SENSOR_CH_18],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_19] = {
        .type            = SENSOR_TYPE_TEMPERATURE,
        .index           = SENSOR_ID_19,
        .hw              = &sensor_hw[SENSOR_CH_19],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_20] = {
        .type            = SENSOR_TYPE_PRESSURE,
        .index           = SENSOR_ID_20,
        .hw              = &sensor_hw[SENSOR_CH_20],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_21] = {
        .type            = SENSOR_TYPE_PRESSURE,
        .index           = SENSOR_ID_21,
        .hw              = &sensor_hw[SENSOR_CH_21],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_22] = {
        .type            = SENSOR_TYPE_VOLTAGE,
        .index           = SENSOR_ID_22,
        .hw              = &sensor_hw[SENSOR_CH_22],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_23] = {
        .type            = SENSOR_TYPE_CURRENT,
        .index           = SENSOR_ID_23,
        .hw              = &sensor_hw[SENSOR_CH_22],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_24] = {
        .type            = SENSOR_TYPE_POWER,
        .index           = SENSOR_ID_24,
        .hw              = &sensor_hw[SENSOR_CH_22],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
    [SENSOR_ID_25] = {
        .type            = SENSOR_TYPE_POWER_FACTOR,
        .index           = SENSOR_ID_25,
        .hw              = &sensor_hw[SENSOR_CH_22],
        .adc_controller  = NULL,
        .mux_controller  = NULL,
        .read            = NULL,
        .conversion_gain = 1.0f,
        .offset          = 0.0f,
    },
};

/**
 * @brief Initialize the sensor manager and its dependencies.
 *
 * This function sets up the sensor manager using the provided configuration.
 * It validates the input parameters, stores the event queue reference,
 * initializes the ADC and multiplexer controllers, and assigns the appropriate
 * sensor interfaces (ADC + MUX + read function) for all available channels.
 *
 * The function must be called once before performing any sensor operations.
 *
 * @param[in] config Pointer to the sensor manager configuration structure.
 *                   Must not be NULL, and must contain a valid event queue.
 *
 * @return
 *     - KERNEL_ERROR_NONE on success
 *     - KERNEL_ERROR_INVALID_ARG if the configuration is invalid
 *     - KERNEL_ERROR_MUX_INIT_ERROR if the multiplexer controller fails to initialize
 *     - KERNEL_ERROR_ADC_INIT_ERROR if the ADC controller fails to initialize
 *
 * @note This function initializes global/static controllers and interfaces.
 *       It should be called only once during system startup.
 */
kernel_error_st sensor_manager_initialize(sensor_manager_config_st *config) {
    if (config == NULL || config->sensor_manager_queue == NULL) {
        logger_print(ERR, TAG, "Invalid sensor manager configuration");
        return KERNEL_ERROR_INVALID_ARG;
    }

    sensor_manager_config.sensor_manager_queue = config->sensor_manager_queue;

    if (adc_controller_init(&adc_controller) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize MUX manager");
        return KERNEL_ERROR_MUX_INIT_ERROR;
    }

    if (mux_controller_init(&mux_controller) != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize ADC manager");
        return KERNEL_ERROR_ADC_INIT_ERROR;
    }

    for (int i = 0; i < NUM_OF_CHANNEL_SENSORS; i++) {
        switch (sensor_interface[i].type) {
            case SENSOR_TYPE_TEMPERATURE:
                sensor_interface[i].adc_controller = &adc_controller;
                sensor_interface[i].mux_controller = &mux_controller;
                sensor_interface[i].read           = temperature_sensor_read;
                break;
            case SENSOR_TYPE_PRESSURE:
                sensor_interface[i].adc_controller = &adc_controller;
                sensor_interface[i].mux_controller = &mux_controller;
                sensor_interface[i].read           = pressure_sensor_read;
                break;
            case SENSOR_TYPE_VOLTAGE:
            case SENSOR_TYPE_CURRENT:
            case SENSOR_TYPE_POWER:
            case SENSOR_TYPE_POWER_FACTOR:
                sensor_interface[i].adc_controller = NULL;
                sensor_interface[i].mux_controller = NULL;
                sensor_interface[i].read           = power_sensor_read;
                break;
            default:
                logger_print(WARN, TAG, "Sensor type %d not supported on channel %d", sensor_interface[i].type, i);
                break;
        }
    }

    return KERNEL_ERROR_NONE;
}

/**
 * @brief Gathers sensor readings and sends a device report to the provided queue.
 *
 * This function:
 * - Retrieves the current device timestamp.
 * - Iterates through all sensor channels, collecting voltage readings.
 * - Marks each successfully read sensor as active and stores the voltage.
 * - Populates a `device_report_st` structure with the results.
 * - Sends the report to the specified FreeRTOS queue.
 *
 * If a sensor reading fails, it logs an error and skips to the next channel.
 *
 * @param device_report_queue FreeRTOS queue to send the generated device report.
 */
void sensor_manager_loop() {
    device_report_st device_report = {0};

    device_info_get_current_time(device_report.timestamp, sizeof(device_report.timestamp));
    device_report.num_of_channels = NUM_OF_SENSORS;

    for (int i = 0; i < NUM_OF_CHANNEL_SENSORS; i++) {
        if (sensor_interface[i].read) {
            kernel_error_st err = sensor_interface[i].read(&sensor_interface[i], device_report.sensors);
            if (err != KERNEL_ERROR_NONE) {
                logger_print(ERR, TAG, "Failed to read sensor at index %d: error %d", i, err);
            }
        }
    }
    logger_print(DEBUG, TAG, "Sensor report generated, sending to queue");
    if (xQueueSend(sensor_manager_config.sensor_manager_queue, &device_report, pdMS_TO_TICKS(100)) != pdPASS) {
        logger_print(ERR, TAG, "Failed to send sensor report to queue");
    }
}

/**
 * @brief Returns the sensor type (temperature or pressure) for the given index.
 *
 * @param sensor_index Index into the sensor_interface table.
 * @return sensor_type_et The sensor type, or SENSOR_TYPE_UNDEFINED if index is invalid.
 */
sensor_type_et sensor_get_type(uint8_t sensor_index) {
    if (sensor_index >= NUM_OF_CHANNEL_SENSORS) {
        return SENSOR_TYPE_UNDEFINED;
    }

    return sensor_interface[sensor_index].type;
}

/**
 * @brief Calibrates a specific sensor by updating its gain and offset.
 *
 * This function updates the calibration parameters for a sensor identified by
 * its index. The gain is used to scale the final measured voltage, and the offset
 * is subtracted from the raw internal voltage before gain is applied.
 *
 * Example:
 * - Measured voltage = (raw_voltage - offset) * gain
 *
 * These values are stored in the `sensor_interface` table and applied in future readings.
 *
 * @param sensor_index Index of the sensor to calibrate (0 to NUM_OF_MUX_CHANNELS - 1).
 * @param offset       Offset value to subtract from the raw measured voltage.
 * @param gain         Gain factor to apply after offset adjustment.
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE on success
 *         - KERNEL_ERROR_INVALID_ARG if the sensor index is out of range
 */
kernel_error_st sensor_calibrate(uint8_t sensor_index, float offset, float gain) {
    if (sensor_index >= NUM_OF_CHANNEL_SENSORS) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    sensor_interface[sensor_index].offset          = offset;
    sensor_interface[sensor_index].conversion_gain = gain;

    return KERNEL_ERROR_NONE;
}