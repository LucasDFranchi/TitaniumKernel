/**
 * @file sensor_manager.c
 * @brief Sensor Manager implementation.
 *
 * This module initializes and manages all sensors in the system, including:
 * - Multiplexer (MUX) and ADC controller initialization
 * - Sensor interface configuration (temperature, pressure, power, etc.)
 * - Periodic acquisition of sensor data and reporting to the application queue
 * - Calibration utilities and getters for sensor parameters
 *
 * The sensor manager runs in its own loop task (`sensor_manager_loop`) and
 * periodically collects sensor readings to publish them to higher-level
 * application modules.
 */

#include "sensor_manager.h"

#include "kernel/device/device_info.h"
#include "kernel/error/error_num.h"
#include "kernel/hal/i2c/i2c.h"
#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"

#include "app/app_extern_types.h"
#include "app/app_tasks_config.h"
#include "app/hardware/controllers/adc_controller.h"
#include "app/hardware/controllers/mux_controller.h"
#include "app/sensor_manager/sensor/ntc_temperature.h"
#include "app/sensor_manager/sensor/power_sensor.h"
#include "app/sensor_manager/sensor/pressure_sensor.h"
#include "app/sensor_manager/sensor_interface/sensor_interface.h"

/* Global Variables */
static const char *TAG                  = "Sensor Manager"; /*!< Tag used for logging */
static mux_controller_st mux_controller = {0};
static adc_controller_st adc_controller = {0};

static sensor_hw_st sensor_hw[NUM_OF_CHANNEL_SENSORS] = {
    [SENSOR_CH_00] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_4},
    },
    [SENSOR_CH_01] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_4},
    },
    [SENSOR_CH_02] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_5},
    },
    [SENSOR_CH_03] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_5},
    },
    [SENSOR_CH_04] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_05] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_06] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_07] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_08] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_09] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_6},
    },
    [SENSOR_CH_10] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_11] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_7},
    },
    [SENSOR_CH_12] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_3},
    },
    [SENSOR_CH_13] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_3},
    },
    [SENSOR_CH_14] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_2},
    },
    [SENSOR_CH_15] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_2},
    },
    [SENSOR_CH_16] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_1},
    },
    [SENSOR_CH_17] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_1},
    },
    [SENSOR_CH_18] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_0},
    },
    [SENSOR_CH_19] = {
        .adc_ref_branch    = {.pga_gain = PGA_2_048V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A2},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A3},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_0, .mux_channel = MUX_CHANNEL_0},
    },
    [SENSOR_CH_20] = {
        .adc_ref_branch    = {0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A1},
        .mux_hw_config     = {.mux_address = MUX_ADDRESS_1, .mux_channel = MUX_CHANNEL_0},
    },
    [SENSOR_CH_21] = {
        .adc_ref_branch    = {0},
        .adc_sensor_branch = {.pga_gain = PGA_4_096V, .data_rate = DR_128SPS, .adc_mux_config = ADC_CONFIG_SINGLE_ENDED_A0},
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
 * Sets up the sensor manager with the given MQTT topics and configures
 * its event queue, ADC, and multiplexer controllers. Each sensor channel
 * is assigned the proper interface (ADC + MUX + read function).
 *
 * This function must be called once before using any sensor operations.
 *
 * @return
 *     - KERNEL_SUCCESS on success
 *     - KERNEL_ERROR_INVALID_ARG if the input is invalid
 *     - KERNEL_ERROR_MUX_INIT_ERROR if the multiplexer initialization fails
 *     - KERNEL_ERROR_ADC_INIT_ERROR if the ADC initialization fails
 *
 * @note Initializes global/static controllers and should be called only once
 *       at system startup.
 */
static kernel_error_st sensor_manager_initialize(void *args) {
    if (adc_controller_init(&adc_controller) != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize ADC manager");
        return KERNEL_ERROR_MUX_INIT_ERROR;
    }

    if (mux_controller_init(&mux_controller) != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize MUX manager");
        return KERNEL_ERROR_ADC_INIT_ERROR;
    }

    for (int i = 0; i < NUM_OF_SENSORS; i++) {
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

        sensor_interface[i].mutex = xSemaphoreCreateMutex();
        if (sensor_interface[i].mutex == NULL) {
            logger_print(ERR, TAG, "Unable to allocate sensor mutex!");
            return KERNEL_ERROR_NO_MEM;
        }
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Get the type of a sensor.
 *
 * Safely returns the sensor type (temperature, pressure, voltage, etc.)
 * for the sensor at the given index in the sensor interface table.
 *
 * @param sensor_index Index of the sensor (0..NUM_OF_SENSORS-1).
 * @return sensor_type_et The sensor type, or SENSOR_TYPE_UNDEFINED if
 *         the index is invalid or the mutex could not be taken.
 */
sensor_type_et sensor_get_type(uint8_t sensor_index) {
    if (sensor_index >= NUM_OF_SENSORS) {
        return SENSOR_TYPE_UNDEFINED;
    }

    sensor_interface_st *sensor = &sensor_interface[sensor_index];
    sensor_type_et type         = SENSOR_TYPE_UNDEFINED;

    if (xSemaphoreTake(sensor->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        type = sensor->type;
        xSemaphoreGive(sensor->mutex);
    } else {
        type = SENSOR_TYPE_UNDEFINED;
    }

    return type;
}

/**
 * @brief Get the calibration gain of a sensor.
 *
 * Safely returns the conversion gain applied to the sensor readings.
 *
 * @param sensor_index Index of the sensor (0..NUM_OF_SENSORS-1).
 * @return float The conversion gain, or 1.0f if the index is invalid
 *               or the mutex could not be taken.
 */
float sensor_get_gain(uint8_t sensor_index) {
    if (sensor_index >= NUM_OF_SENSORS) {
        return 1.0f;
    }

    sensor_interface_st *sensor = &sensor_interface[sensor_index];
    float gain                  = 1.0f;

    if (xSemaphoreTake(sensor->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        gain = sensor->conversion_gain;
        xSemaphoreGive(sensor->mutex);
    } else {
        gain = 1.0f;
    }

    return gain;
}

/**
 * @brief Get the calibration offset of a sensor.
 *
 * Safely returns the offset applied to the raw sensor readings.
 *
 * @param sensor_index Index of the sensor (0..NUM_OF_SENSORS-1).
 * @return float The offset, or 0.0f if the index is invalid or
 *               the mutex could not be taken.
 */
float sensor_get_offset(uint8_t sensor_index) {
    if (sensor_index >= NUM_OF_SENSORS) {
        return 0.0f;
    }

    sensor_interface_st *sensor = &sensor_interface[sensor_index];
    float offset                = 0.0f;

    if (xSemaphoreTake(sensor->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        offset = sensor->offset;
        xSemaphoreGive(sensor->mutex);
    } else {
        offset = 0.0f;
    }

    return offset;
}

/**
 * @brief Get the current state of a sensor.
 *
 * Safely returns the runtime state of the sensor (enabled, disabled, etc.).
 *
 * @param sensor_index Index of the sensor (0..NUM_OF_SENSORS-1).
 * @return sensor_state_et The sensor state, or SENSOR_DISABLED if the
 *                         index is invalid or the mutex could not be taken.
 */
sensor_state_et sensor_get_state(uint8_t sensor_index) {
    if (sensor_index >= NUM_OF_SENSORS) {
        return SENSOR_DISABLED;
    }

    sensor_interface_st *sensor = &sensor_interface[sensor_index];
    sensor_state_et state       = SENSOR_DISABLED;

    if (xSemaphoreTake(sensor->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        state = sensor->state;
        xSemaphoreGive(sensor->mutex);
    } else {
        state = SENSOR_DISABLED;
    }

    return state;
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
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_INVALID_ARG if the sensor index is out of range
 */
kernel_error_st sensor_calibrate(uint8_t sensor_index, float offset, float gain) {
    if (sensor_index >= NUM_OF_SENSORS) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    sensor_interface_st *sensor = &sensor_interface[sensor_index];

    if (xSemaphoreTake(sensor->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        sensor_interface[sensor_index].offset          = offset;
        sensor_interface[sensor_index].conversion_gain = gain;
        xSemaphoreGive(sensor->mutex);
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Main loop for the Sensor Manager task.
 *
 * Periodically reads data from all available sensors, builds a device report,
 * and sends it to the sensor manager queue.
 *
 * @param args Pointer to a `sensor_manager_init_st` structure containing
 *             the queue handle for sending reports.
 *
 * @note Runs indefinitely as an RTOS task. This function should be registered
 *       with the RTOS task scheduler at startup.
 */
void sensor_manager_loop(void *args) {
    kernel_error_st err = sensor_manager_initialize(args);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize the sensor manager! - %d", err);
        return;
    }

    QueueHandle_t sensor_queue = queue_manager_get(SENSOR_REPORT_QUEUE_ID);
    if (sensor_queue == NULL) {
        logger_print(ERR, TAG, "Sensor report queue is NULL");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        device_report_st device_report = {0};

        TickType_t last_wake_time       = xTaskGetTickCount();
        const TickType_t interval_ticks = pdMS_TO_TICKS(5000);

        device_info_get_current_time(device_report.timestamp, sizeof(device_report.timestamp));
        device_report.num_of_channels = NUM_OF_SENSORS;

        for (int i = 0; i < NUM_OF_CHANNEL_SENSORS; i++) {
            if (sensor_interface[i].read == NULL) {
                continue;
            }
            kernel_error_st err = sensor_interface[i].read(&sensor_interface[i], device_report.sensors);
            if (err != KERNEL_SUCCESS) {
                logger_print(ERR, TAG, "Failed to read sensor at index %d: error %d", i, err);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        logger_print(DEBUG, TAG, "Sensor report generated, sending to queue");

        if (xQueueSend(sensor_queue, &device_report, pdMS_TO_TICKS(100)) != pdPASS) {
            logger_print(ERR, TAG, "Failed to send sensor report to queue");
        }

        vTaskDelayUntil(&last_wake_time, interval_ticks);
    }
}
