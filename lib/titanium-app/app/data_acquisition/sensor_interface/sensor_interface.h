#pragma once

#include "app/hardware/controllers/adc_controller.h"
#include "app/hardware/controllers/mux_controller.h"

#include "app/sensor_manager/sensor_manager.h"

typedef struct sensor_interface_s sensor_interface_st;

/**
 * @typedef sensor_read_fn
 * @brief Function pointer type for sensor read operations.
 *
 * Each sensor driver implements this interface. A read function acquires the
 * raw measurement (via MUX + ADC), applies calibration (offset + gain), and
 * returns a floating-point value in physical units.
 *
 * @param[in] ctx          Pointer to the sensor interface instance.
 * @param[out] sensor_report   Pointer to store the resulting measurement.
 *
 * @return
 *     - KERNEL_SUCCESS on success
 *     - Appropriate kernel_error_st code on failure
 */
typedef kernel_error_st (*sensor_read_fn)(sensor_interface_st *ctx, sensor_report_st *sensor_report);

/**
 * @brief Hardware configuration structure for a sensor channel.
 *
 * Encapsulates the low-level hardware bindings required for a sensor channel:
 * - Reference ADC branch (constant side of the divider or bridge).
 * - Sensor ADC branch (variable side of the divider or measurement line).
 * - Multiplexer configuration (used to route the sensor to the ADC).
 *
 * This abstraction allows a sensor interface to be reused across different
 * hardware setups while keeping the routing and ADC behavior configurable.
 */
typedef struct {
    const adc_hw_config_st adc_ref_branch; /*!< ADC config for reference branch of Wheatstone bridge */
    adc_hw_config_st adc_sensor_branch;    /*!< ADC config for measurement (variable) branch */
    const mux_hw_config_st mux_hw_config;  /*!< Multiplexer config (for routing the sensor input) */
} sensor_hw_st;

/**
 * @brief Generic sensor interface structure.
 *
 * Represents one logical sensor in the system. It associates hardware
 * configuration with shared controller instances and a driver-specific
 * read function.
 */
struct sensor_interface_s {
    sensor_type_et type;
    sensor_index_et index;
    sensor_hw_st *hw;                  /*!< Per-sensor hardware config */
    adc_controller_st *adc_controller; /*!< Pointer to shared ADC controller */
    mux_controller_st *mux_controller; /*!< Pointer to shared MUX controller */
    sensor_read_fn read;               /*!< Function pointer to read sensor value */
    float conversion_gain;             /*!< Gain factor applied after voltage calculation */
    float offset;                      /*!< Voltage offset to subtract from the measured value */
    sensor_state_et state;               /*!< Indicates if the sensor is currently active */
    SemaphoreHandle_t mutex;           /*!< Mutex to protect access to this sensor */
};
