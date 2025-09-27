#pragma once

#include "kernel/error/error_num.h"

#include "app/hal/analog_reader/analog_reader.h"
#include "app/input_manager/sensors/sensor_interface/isensor.h"
#include "app/input_manager/sensors/sensor_report.h"

class TemperatureSensor : public ISensor {
   public:
    TemperatureSensor(uint16_t index, float gain, float offset, AnalogReader* ref_adc, AnalogReader* ntc_adc)
        : index_(index),
          gain_(gain),
          offset_(offset),
          state_(SensorStatus::Disabled),
          ref_adc_(ref_adc),
          ntc_adc_(ntc_adc) {}

    static constexpr uint16_t SENSOR_SLOTS = 1;  ///< Number of slots that this sensor uses in a report

    /**
     * @brief Initialize the temperature sensor by configuring its reference and NTC ADCs.
     *
     * This method performs the following steps:
     *  - Verifies that ADC references are set (ref_adc_ and ntc_adc_ are not null).
     *  - Ensures the sensor is not already initialized.
     *  - Initializes both reference and NTC ADC drivers.
     *  - Reads the reference ADC once to calculate the reference voltage.
     *  - Updates the internal sensor state to ENABLED if all steps succeed.
     *
     * @note The function is idempotent: calling it multiple times will not reinitialize
     *       the sensor, but it will log a warning if already initialized.
     *
     * @return
     *  - KERNEL_SUCCESS on success.
     *  - KERNEL_ERROR_NULL if ADC references are missing.
     *  - ADC driver error codes if initialization or reading fails.
     */
    kernel_error_st initialize() override;

    /**
     * @brief Update the temperature reading from the NTC thermistor.
     *
     * This function reads the raw ADC value from the NTC channel, calculates the
     * corresponding thermistor voltage, converts it to temperature, and applies
     * any calibration gain and offset. Updates the internal temperature state.
     *
     * @return
     *  - KERNEL_SUCCESS if the update succeeded.
     *  - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
     *  - KERNEL_ERROR_FAIL if the ADC read failed.
     */
    kernel_error_st update() override;

    /**
     * @brief Convert measured thermistor voltage to temperature.
     *
     * Uses the reference branch to compensate measurement errors, computes the thermistor resistance,
     * and maps it to temperature using the lookup table.
     *
     * @return Interpolated temperature in degrees Celsius.
     */
    float voltage_to_temperature();

    /**
     * @brief Convert thermistor resistance to temperature using lookup table interpolation.
     *
     * Performs a binary search over the NTC lookup table and applies linear interpolation
     * between two nearest resistance values. Handles out-of-range resistance values by
     * returning the closest valid temperature.
     *
     * @param resistance_kohm Thermistor resistance in kΩ.
     * @return Interpolated temperature in degrees Celsius.
     */
    float resistance_to_temperature(float resistance_kohm);

    /**
     * @brief Calculate thermistor resistance in kΩ based on voltage divider output.
     *
     * This function estimates the thermistor resistance by applying the voltage divider formula,
     * compensating for reference branch error. The result is used as input for temperature interpolation.
     *
     * @return Calculated thermistor resistance in kΩ.
     */
    float calculate_resistance_kohm();

    /**
     * @brief Calibrate the temperature sensor.
     *
     * This function sets the gain and offset used to adjust the measured temperature.
     * The calibration is applied to subsequent temperature updates.
     *
     * @param gain  Multiplicative factor to adjust the measured temperature.
     * @param offset Additive offset to adjust the measured temperature.
     *
     * @return
     *  - KERNEL_SUCCESS if calibration succeeded.
     *  - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
     */
    kernel_error_st calibrate(float gain, float offset);

    /**
     * @brief Fill a sensor report array with the current temperature measurement.
     *
     * Populates each entry of the provided sensor report array with the current
     * temperature, sensor type, and active status. The number of entries filled
     * is determined by SENSOR_SLOTS.
     *
     * @param sensor_list Pointer to an array of sensor_report_st to be filled.
     *
     * @return
     *  - KERNEL_SUCCESS if the report was successfully generated.
     *  - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
     *  - KERNEL_ERROR_NULL if sensor_list is a null pointer.
     */
    kernel_error_st get_report(sensor_report_st* report_list);

    SensorType get_type() const override {
        return SensorType::Temperature;
    }

    /**
     * @brief Get the unique sensor index.
     *
     * @return The index of this sensor.
     */
    uint16_t get_index() const override {
        return index_;
    }

    /**
     * @brief Get the calibration gain factor.
     *
     * @return Current gain applied to sensor readings.
     */
    float get_gain() const override {
        return gain_;
    }

    /**
     * @brief Get the calibration offset.
     *
     * @return Current offset applied to sensor readings.
     */
    float get_offset() const override {
        return offset_;
    }

    /**
     * @brief Get the current sensor state.
     *
     * Indicates whether the sensor is enabled, disabled, or in another state.
     *
     * @return SensorStatus enum representing the current state.
     */
    SensorStatus get_status() const override {
        return state_;
    }

   private:
    uint16_t index_        = 0xFFFF;
    float gain_            = 1.0f;
    float offset_          = 0.0f;
    SensorStatus state_    = SensorStatus::Disabled;
    AnalogReader* ref_adc_ = nullptr;
    AnalogReader* ntc_adc_ = nullptr;
    float ref_adc_voltage  = 0.0f;
    float ntc_adc_voltage  = 0.0f;
    float temperature      = -273.15f;
};