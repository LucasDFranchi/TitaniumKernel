#pragma once

#include "app/input_manager/sensors/sensor_interface/ntc_temperature.h"

namespace sensors {
    // clang-format off
    enum class SensorId { Temp0,  Temp1,  Temp2,  Temp3,  Temp4,
                          Temp5,  Temp6,  Temp7,  Temp8,  Temp9,
                          Temp10, Temp11, Temp12, Temp13, Temp14,
                          Temp15, Temp16, Temp17, Temp18, Temp19,
                          Count };
    // clang-format on

    /**
     * @brief Get a reference to a statically allocated temperature sensor.
     *
     * The function lazily constructs the TemperatureSensor and reuses the same
     * instance for each call. The mapping between SensorId and underlying hardware
     * is fixed here in one place.
     *
     * @param id Identifier of the sensor to retrieve.
     * @return Reference to the corresponding TemperatureSensor instance.
     */
    TemperatureSensor& get_temperature_sensor(SensorId id);

}  // namespace sensors
