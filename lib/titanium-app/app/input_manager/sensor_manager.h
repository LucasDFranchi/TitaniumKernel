#pragma once

#include <cstdint>

#include "kernel/error/error_num.h"
#include "kernel/tasks/task_template.h"

#include "app/hal/hardware.hpp"
#include "app/input_manager/sensor_types.h"
#include "app/input_manager/sensors/sensors.h"

namespace sensors {

    class SensorManager : public TaskTemplate {
       public:
        SensorManager() = default;

        SensorManager(const SensorManager&)            = delete;
        SensorManager& operator=(const SensorManager&) = delete;

        static constexpr char TAG[] = "Sensor Manager"; /*!< Tag used for logging */

        /**
         * @brief Main loop for the Sensor Manager task.
         *
         * Periodically reads data from all available sensors, builds a device report,
         * and sends it to the sensor manager queue.
         */
        static void loop(void* args);
        
        /**
         * @brief Initialize the sensor.
         *
         * Configures the underlying hardware, sets up ADCs or other peripherals,
         * and prepares the sensor for reading. After a successful call, the
         * sensor state should be ENABLED.
         *
         * @return kernel_error_st
         *         - KERNEL_SUCCESS if initialization succeeded.
         *         - KERNEL_ERROR_NULL if required hardware references are missing.
         *         - Other sensor-specific error codes if hardware initialization fails.
         */
        kernel_error_st initialize();

        /**
         * @brief Main execution routine for the sensor manager.
         *
         * This function is called repeatedly in the FreeRTOS task loop to
         * perform periodic updates on all managed sensors. It may read sensor
         * values, apply calibration, and update internal state.
         */
        void run() override;

        /**
         * @brief Get the type of a specific sensor.
         *
         * Retrieves the sensor type (Temperature, Pressure, etc.) for the given sensor ID.
         *
         * @param id Identifier of the sensor.
         * @return SensorType The type of the sensor.
         */
        SensorType get_type(SensorId id) const;

        /**
         * @brief Get the calibration gain of a specific sensor.
         *
         * Returns the gain factor currently applied to the sensor measurements.
         *
         * @param id Identifier of the sensor.
         * @return float Calibration gain of the sensor.
         */
        float get_gain(SensorId id) const;

        /**
         * @brief Get the calibration offset of a specific sensor.
         *
         * Returns the offset currently applied to the sensor measurements.
         *
         * @param id Identifier of the sensor.
         * @return float Calibration offset of the sensor.
         */
        float get_offset(SensorId id) const;

        /**
         * @brief Get the current operational status of a specific sensor.
         *
         * Returns whether the sensor is enabled or disabled.
         *
         * @param id Identifier of the sensor.
         * @return SensorStatus Current state of the sensor.
         */
        SensorStatus get_status(SensorId id) const;

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
        kernel_error_st calibrate(SensorId id, float gain, float offset);

       private:
        std::array<Multiplexer*, 2> muxes_ = {
            &hw::get_multiplexer(hw::MuxId::Mux0),
            &hw::get_multiplexer(hw::MuxId::Mux1)};

        struct SensorChannelConfig {
            hw::MuxId mux;
            hw::ChannelId channel;
            SensorId sensor;
        };

        static constexpr SensorChannelConfig SENSOR_CHANNEL_MAP_[] = {
            {hw::MuxId::Mux0, hw::ChannelId::Channel0, SensorId::Temp0},
            {hw::MuxId::Mux0, hw::ChannelId::Channel0, SensorId::Temp1},
            {hw::MuxId::Mux0, hw::ChannelId::Channel1, SensorId::Temp2},
            {hw::MuxId::Mux0, hw::ChannelId::Channel1, SensorId::Temp3},
            {hw::MuxId::Mux0, hw::ChannelId::Channel2, SensorId::Temp4},
            {hw::MuxId::Mux0, hw::ChannelId::Channel2, SensorId::Temp5},
            {hw::MuxId::Mux0, hw::ChannelId::Channel3, SensorId::Temp6},
            {hw::MuxId::Mux0, hw::ChannelId::Channel3, SensorId::Temp7},
            {hw::MuxId::Mux0, hw::ChannelId::Channel4, SensorId::Temp8},
            {hw::MuxId::Mux0, hw::ChannelId::Channel4, SensorId::Temp9},
            {hw::MuxId::Mux0, hw::ChannelId::Channel5, SensorId::Temp10},
            {hw::MuxId::Mux0, hw::ChannelId::Channel5, SensorId::Temp11},
            {hw::MuxId::Mux0, hw::ChannelId::Channel6, SensorId::Temp12},
            {hw::MuxId::Mux0, hw::ChannelId::Channel6, SensorId::Temp13},
            {hw::MuxId::Mux0, hw::ChannelId::Channel7, SensorId::Temp14},
            {hw::MuxId::Mux0, hw::ChannelId::Channel7, SensorId::Temp15},
            {hw::MuxId::Mux1, hw::ChannelId::Channel0, SensorId::Temp16},
            {hw::MuxId::Mux1, hw::ChannelId::Channel0, SensorId::Temp17},
            {hw::MuxId::Mux1, hw::ChannelId::Channel1, SensorId::Temp18},
            {hw::MuxId::Mux1, hw::ChannelId::Channel1, SensorId::Temp19},
        };
    };

}  // namespace sensors
