#include <array>

#include "kernel/error/error_num.h"

#include "app/hal/drivers/tca9548a.h"
#include "app/input_manager/sensors/sensor_interface/isensor.h"

class Channel {
   public:
    Channel(TCA9548A* mux_driver, TCA9548A::channel_index_e channel_index)
        : mux_driver_(mux_driver), channel_index_(channel_index) {}

    Channel()  = default;
    ~Channel() = default;

    static constexpr size_t MAX_SENSORS = 2;

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
    kernel_error_st enable();

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
    kernel_error_st initialize_all();

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
    kernel_error_st update_all();

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
    kernel_error_st add_sensor(ISensor* sensor);

    /**
     * @brief Get the number of sensors currently attached to this channel.
     *
     * @return size_t Number of registered sensors.
     */
    size_t get_sensor_count() const {
        return sensor_count_;
    }

    /**
     * @brief Get a sensor by index.
     *
     * @param i Index of the sensor (0 <= i < get_sensor_count()).
     * @return ISensor* Pointer to the sensor at the given index.
     */
    ISensor* get_sensor(size_t i) const {
        return sensors_[i];
    }

   private:
    TCA9548A* mux_driver_ = nullptr;            ///< Pointer to multiplexer driver
    TCA9548A::channel_index_e channel_index_;   ///< Hardware channel index
    ISensor* sensors_[Channel::MAX_SENSORS]{};  ///< Array of attached sensors
    size_t sensor_count_ = 0;                   ///< Number of registered sensors
};
