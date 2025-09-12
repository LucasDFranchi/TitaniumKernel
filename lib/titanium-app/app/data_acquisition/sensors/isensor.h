#pragma once

#include <cstdint>

/**
 * @brief Represents the operational state of a sensor.
 */
enum class SensorStatus {
    DISABLED,   ///< Sensor is not active
    ENABLED     ///< Sensor is active and available
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
     * @brief Read the current sensor value.
     * @return Sensor measurement as a floating-point value.
     */
    virtual float get_value() const = 0;

    /**
     * @brief Set/update the sensor value.
     * @param value New measurement to store in the sensor.
     */
    virtual void set_value(float value) = 0;

    /**
     * @brief Get the index (unique ID) of the sensor.
     * @return Sensor index as an unsigned 16-bit integer.
     */
    virtual uint16_t get_index() const = 0;

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
    virtual SensorStatus get_state() const = 0;

    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~ISensor() {}
};
