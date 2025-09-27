#include "channel.h"

#include "kernel/logger/logger.h"

/**
 * @brief Enable this channel on the multiplexer.
 *
 * Activates the underlying multiplexer hardware channel so that
 * communication with attached sensors is possible.
 *
 * @return kernel_error_st
 *         - KERNEL_SUCCESS if the channel was enabled successfully
 *         - KERNEL_ERROR_NULL if mux_driver_ is not set
 *         - Other error codes depending on driver implementation
 */
kernel_error_st Channel::enable() {
    if (this->mux_driver_ == nullptr) {
        return KERNEL_ERROR_NULL;
    }

    return this->mux_driver_->enable_channel(this->channel_index_);
}

/**
 * @brief Add a sensor to this channel.
 *
 * Registers a sensor pointer so it can be managed by this channel.
 * The channel can hold up to MAX_SENSORS sensors.
 *
 * @param sensor Pointer to the sensor to be added.
 * @return kernel_error_st
 *         - KERNEL_SUCCESS if the sensor was added successfully
 *         - KERNEL_ERROR_NULL if sensor is nullptr
 *         - KERNEL_ERROR_CHANNEL_FULL if channel already has MAX_SENSORS
 */
kernel_error_st Channel::add_sensor(ISensor* sensor) {
    if (sensor == nullptr) {
        return KERNEL_ERROR_NULL;
    }

    if (sensor_count_ >= MAX_SENSORS) {
        return KERNEL_ERROR_CHANNEL_FULL;
    }
    this->sensors_[this->sensor_count_++] = sensor;
    return KERNEL_SUCCESS;
}

/**
 * @brief Initialize all sensors attached to this channel.
 *
 * Iterates over all registered sensors and calls their `initialize()` method.
 * If any sensor fails to initialize, the error is logged and returned immediately.
 *
 * @return kernel_error_st
 *         - KERNEL_SUCCESS if all sensors initialized successfully
 *         - Error code from the first failing sensor otherwise
 */
kernel_error_st Channel::initialize_all() {
    for (size_t i = 0; i < sensor_count_; ++i) {
        auto* sensor = this->sensors_[i];
        if (sensor == nullptr) {
            continue;
        }

        auto err = sensor->initialize();
        if (err != KERNEL_SUCCESS) {
            logger_print(ERR, "Channel",
                         "Failed to initialize sensor on channel %d index %d: %d",
                         static_cast<int>(channel_index_),
                         static_cast<int>(i),
                         err);
            return err;
        }
    }
    return KERNEL_SUCCESS;
}

/**
 * @brief Update all sensors attached to this channel.
 *
 * Iterates over all registered sensors and calls their `update()` method.
 * If any sensor fails to update, the error is logged and returned immediately.
 *
 * @note By default this function fails fast. If you prefer partial success
 *       (i.e., continue updating other sensors after an error), modify the
 *       error handling logic accordingly.
 *
 * @return kernel_error_st
 *         - KERNEL_SUCCESS if all sensors updated successfully
 *         - Error code from the first failing sensor otherwise
 */
kernel_error_st Channel::update_all() {
    kernel_error_st err = this->enable();

    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, "Channel",
                     "Failed to enable channel %d: %d",
                     static_cast<int>(channel_index_),
                     err);
        return err;
    }

    for (size_t i = 0; i < this->sensor_count_; ++i) {
        auto* sensor = this->sensors_[i];
        if (sensor == nullptr) {
            continue;
        }

        auto err = sensor->update();
        if (err != KERNEL_SUCCESS) {
            logger_print(ERR, "Channel",
                         "Failed to update sensor on channel %d index %d: %d",
                         static_cast<int>(channel_index_),
                         static_cast<int>(i),
                         err);
            return err;
        }
    }
    return KERNEL_SUCCESS;
}
