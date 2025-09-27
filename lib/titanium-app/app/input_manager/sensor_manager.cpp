#include "sensor_manager.h"

#include <array>

#include "kernel/logger/logger.h"

#include "app/input_manager/sensors/sensors.h"

namespace sensors {

    kernel_error_st SensorManager::initialize() {
        for (auto const &cfg : this->SENSOR_CHANNEL_MAP_) {
            auto &mux    = hw::get_multiplexer(cfg.mux);
            auto &chan   = mux.channel(static_cast<size_t>(cfg.channel));
            auto &sensor = sensors::get_temperature_sensor(cfg.sensor);

            auto err = chan.add_sensor(&sensor);
            if (err != KERNEL_SUCCESS) {
                logger_print(ERR, TAG,
                             "Failed to add sensor %d to mux %d channel %d: %d",
                             static_cast<int>(cfg.sensor),
                             static_cast<int>(cfg.mux),
                             static_cast<int>(cfg.channel),
                             err);
                return err;
            }
        }

        for (Multiplexer *mux : this->muxes_) {
            mux->disable();
            for (size_t i = 0; i < mux->channelCount(); ++i) {
                auto err = mux->channel(i).initialize_all();
                if (err != KERNEL_SUCCESS) {
                    logger_print(ERR, TAG,
                                 "Failed to initialize sensors on mux channel %d: %d",
                                 static_cast<int>(i),
                                 err);
                    return err;
                }
            }
        }

        return KERNEL_SUCCESS;
    }

    void SensorManager::run() {
        while (1) {
            for (Multiplexer *mux : this->muxes_) {
                for (size_t i = 0; i < mux->channelCount(); ++i) {
                    auto err = mux->channel(i).update_all();
                    if (err != KERNEL_SUCCESS) {
                        logger_print(ERR, TAG,
                                     "Failed to update sensors on mux channel %d: %d",
                                     static_cast<int>(i),
                                     err);
                    }
                }
            }

            vTaskDelay(pdMS_TO_TICKS(10000));  // TODO: Improve this
        }
    }

    /**
     * @brief Get the type of a specific sensor.
     *
     * Retrieves the sensor type (Temperature, Pressure, etc.) for the given sensor ID.
     *
     * @param id Identifier of the sensor.
     * @return SensorType The type of the sensor.
     */
    SensorType SensorManager::get_type(SensorId id) const {
        ISensor *sensor = &sensors::get_temperature_sensor(id);
        return sensor->get_type();
    }

    /**
     * @brief Get the calibration gain of a specific sensor.
     *
     * Returns the gain factor currently applied to the sensor measurements.
     *
     * @param id Identifier of the sensor.
     * @return float Calibration gain of the sensor.
     */
    float SensorManager::get_gain(SensorId id) const {
        ISensor *sensor = &sensors::get_temperature_sensor(id);
        return sensor->get_gain();
    }

    /**
     * @brief Get the calibration offset of a specific sensor.
     *
     * Returns the offset currently applied to the sensor measurements.
     *
     * @param id Identifier of the sensor.
     * @return float Calibration offset of the sensor.
     */
    float SensorManager::get_offset(SensorId id) const {
        ISensor *sensor = &sensors::get_temperature_sensor(id);
        return sensor->get_gain();
    }

    /**
     * @brief Get the current operational status of a specific sensor.
     *
     * Returns whether the sensor is enabled or disabled.
     *
     * @param id Identifier of the sensor.
     * @return SensorStatus Current state of the sensor.
     */
    SensorStatus SensorManager::get_status(SensorId id) const {
        ISensor *sensor = &sensors::get_temperature_sensor(id);
        return sensor->get_status();
    }

    /**
     * @brief Apply calibration to the sensor.
     *
     * Updates the internal calibration parameters such as gain and offset
     * to correct the measured values. This function should be called after
     * any manual calibration procedure or adjustment.
     *
     * @return kernel_error_st
     *         - KERNEL_SUCCESS if calibration succeeded.
     *         - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
     *         - Other sensor-specific error codes as applicable.
     */
    kernel_error_st SensorManager::calibrate(SensorId id, float gain, float offset) {
        ISensor *sensor = &sensors::get_temperature_sensor(id);
        return sensor->calibrate(gain, offset);
    }
}  // namespace sensors
