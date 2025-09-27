#pragma once

#include "kernel/hal/gpio/gpio_handler.h"
#include "kernel/hal/i2c/i2c_handler.h"

#include "app/hal/analog_reader/analog_reader.h"
#include "app/hal/multiplexer/mux.h"

namespace hw {
    enum class GpioId { StatusLed,
                        MuxReset };

    GPIOHandler& get_gpio(GpioId id);

    enum class MuxId { Mux0,
                       Mux1 };

    Multiplexer& get_multiplexer(MuxId id);

    enum class ChannelId { Channel0,
                           Channel1,
                           Channel2,
                           Channel3,
                           Channel4,
                           Channel5,
                           Channel6,
                           Channel7 };

    enum class AnalogReaderId { AnalogReader0,
                                AnalogReader1,
                                AnalogReader2,
                                AnalogReader3 };

    AnalogReader& get_analog_reader(AnalogReaderId id);
}  // namespace hw
