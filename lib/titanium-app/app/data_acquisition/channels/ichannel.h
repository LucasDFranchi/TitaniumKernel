#pragma once

#include <cstdint>
#include <cstddef>

#include "app/data_acquisition/sensors/isensor.h"

/**
 * @brief Abstract interface for hardware channels.
 *
 * A channel represents a hardware interface (e.g., ADC, Modbus)
 * that can update one or more sensors with new readings.
 */
class IChannel {
public:
    /**
     * @brief Perform a hardware read and update associated sensors.
     */
    virtual void update() = 0;

    /**
     * @brief Get the number of sensors attached to this channel.
     * @return Number of sensors.
     */
    virtual std::size_t sensor_count() const = 0;

    /**
     * @brief Get a sensor by index.
     * @param index Sensor index within the channel.
     * @return Pointer to ISensor, or nullptr if index is invalid.
     */
    virtual const ISensor* get_sensor(std::size_t index) const = 0;

    /**
     * @brief Get the channel identifier.
     * @return Unsigned 16-bit channel index.
     */
    virtual uint16_t get_index() const = 0;

    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IChannel() {}
};
