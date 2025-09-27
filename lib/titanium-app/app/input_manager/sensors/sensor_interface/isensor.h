#pragma once

#include <cstdint>

#include "kernel/error/error_num.h"

/**
 * @brief Represents the operational state of a sensor.
 */
enum class SensorStatus {
    Disabled,  ///< Sensor is not active
    Enabled    ///< Sensor is active and available
};

// Declare only the base enum
enum class SensorType {
    Undefined = 0,
    Temperature,
    Pressure,
    Power,
};

/**
 * @brief Abstract interface for all sensor types.
 *
 * Defines the minimal set of functions that any sensor
 * must implement in order to be used in the system.
 */
class ISensor {
   public:
    /**
     * @brief Initialize the sensor and prepare it for reading.
     *
     * This function should configure the hardware, set up any
     * required registers, and perform any calibration needed.
     *
     * @return kernel_error_st Error code indicating success or failure
     */
    virtual kernel_error_st initialize() = 0;

    /**
     * @brief Read the current value from the sensor.
     *
     * This function should return the latest measurement, applying
     * any calibration gain and offset if required. It may update
     * internal cached values.
     *
     * @return float The sensor measurement value
     */
    virtual kernel_error_st update() = 0;

    /**
     * @brief Get the index (unique ID) of the sensor.
     * @return Sensor index as an unsigned 16-bit integer.
     */
    virtual uint16_t get_index() const = 0;

    /**
     * @brief Get the type of the sensor.
     * @return Sensor type (e.g., temperature, pressure)
     */
    virtual SensorType get_type() const = 0;

    /**
     * @brief Get the calibration gain applied to the sensor.
     * @return Gain factor as a floating-point value.
     */
    virtual float get_gain() const = 0;

    /**
     * @brief Get the calibration offset applied to the sensor.
     * @return Offset value as a floating-point number.
     */
    virtual float get_offset() const = 0;

    /**
     * @brief Get the current state of the sensor.
     * @return SensorStatus enum (ENABLED or DISABLED).
     */
    virtual SensorStatus get_status() const = 0;

    /**
     * @brief Apply calibration to the sensor.
     *
     * Updates the sensor's internal calibration parameters (gain and offset)
     * to correct future measurements. Should be called after manual calibration
     * or when updating sensor-specific correction factors.
     *
     * @param gain   Multiplicative factor to scale the measured values.
     * @param offset Additive offset to apply to the raw measurements.
     * @return kernel_error_st
     *         - KERNEL_SUCCESS if calibration was applied successfully.
     *         - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
     *         - Other sensor-specific error codes depending on implementation.
     */
    virtual kernel_error_st calibrate(float gain, float offset) = 0;

    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~ISensor() {}
};
