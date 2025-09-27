// sensors.cpp
#include "sensors.h"

#include "app/hal/hardware.hpp"

namespace sensors {

    TemperatureSensor& get_temperature_sensor(SensorId id) {
        static constexpr float DEFAULT_GAIN   = 1.0f;
        static constexpr float DEFAULT_OFFSET = 0.0f;

        static std::array<TemperatureSensor, static_cast<size_t>(SensorId::Count)> sensors = {
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp0),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp1),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp2),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp3),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp4),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp5),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp6),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp7),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp8),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp9),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp10),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp11),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp12),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp13),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp14),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp15),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp16),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp17),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp18),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader0),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader1)),
            TemperatureSensor(static_cast<uint16_t>(sensors::SensorId::Temp19),
                              DEFAULT_GAIN, DEFAULT_OFFSET,
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader2),
                              &hw::get_analog_reader(hw::AnalogReaderId::AnalogReader3)),
        };

        auto idx = static_cast<size_t>(id);
        if (idx < sensors.size()) {
            return sensors[idx];
        }
        return sensors[0];  // fallback
    }

}  // namespace sensors
