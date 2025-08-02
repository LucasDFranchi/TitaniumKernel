#pragma once

#include "app/hardware/controllers/adc_controller.h"
#include "app/hardware/controllers/mux_controller.h"

/**
 * @enum sensor_type_et
 * @brief Enumerates the types of supported sensors.
 *
 * Used to categorize sensors by their measurement domain. This type is also
 * embedded in sensor reports to indicate how the raw value should be interpreted.
 */
typedef enum sensor_type_e {
    SENSOR_TYPE_TEMPERATURE = 0, /**< Temperature sensor (e.g., NTC thermistor) */
    SENSOR_TYPE_PRESSURE,        /**< Pressure sensor */
    SENSOR_TYPE_VOLTAGE,         /**< Direct voltage measurement */
    SENSOR_TYPE_CURRENT,         /**< Current measurement */
    SENSOR_TYPE_POWER_FACTOR,    /**< Power factor measurement */
    SENSOR_TYPE_UNDEFINED,       /**< Unknown or unsupported sensor */
} sensor_type_et;

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
 * @param[in] sensor_index Logical index of the sensor in the system.
 * @param[out] out_value   Pointer to store the resulting measurement.
 *
 * @return
 *     - KERNEL_ERROR_NONE on success
 *     - Appropriate kernel_error_st code on failure
 */
typedef kernel_error_st (*sensor_read_fn)(sensor_interface_st *ctx, uint8_t sensor_index, float *out_value);

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
 *
 * Members:
 * - @c type : The sensor type (temperature, voltage, etc.).
 * - @c hw   : Hardware configuration for this sensor channel.
 * - @c adc_controller : Shared ADC controller instance.
 * - @c mux_controller : Shared multiplexer controller instance.
 * - @c read : Function pointer to the driver’s read routine.
 * - @c conversion_gain : Calibration gain applied after measurement.
 * - @c offset : Calibration offset subtracted from the raw value.
 */
struct sensor_interface_s {
    sensor_type_et type;
    sensor_hw_st *hw;                  /*!< Per-sensor hardware config */
    adc_controller_st *adc_controller; /*!< Pointer to shared ADC controller */
    mux_controller_st *mux_controller; /*!< Pointer to shared MUX controller */
    sensor_read_fn read;               /*!< Function pointer to read sensor value */
    float conversion_gain;             /*!< Gain factor applied after voltage calculation */
    float offset;                      /*!< Voltage offset to subtract from the measured value */
};
