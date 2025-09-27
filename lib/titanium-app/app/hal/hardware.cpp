#include "hardware.hpp"

#include "app/hal/drivers/ads1115.h"
#include "app/hal/drivers/tca9548a.h"

namespace hw {

    GPIOHandler& get_gpio(GpioId id) {
        static GPIOHandler status_led(GPIO_NUM_27, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE);
        static GPIOHandler mux_reset(GPIO_NUM_32, GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE);

        switch (id) {
            case GpioId::StatusLed:
                return status_led;
            case GpioId::MuxReset:
                return mux_reset;
        }
        return status_led;  // fallback
    }

    I2CHandler& get_i2c() {
        static I2CHandler i2c(GPIO_NUM_21, GPIO_NUM_22, I2C_NUM_0, 100000);
        return i2c;
    }

    Multiplexer& get_multiplexer(MuxId id) {
        static GPIOHandler gpio(GPIO_NUM_27, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE);

        static TCA9548A mux_0(&get_i2c(), &gpio, TCA9548A::mux_address_e::MUX_ADDRESS_0);
        static TCA9548A mux_1(&get_i2c(), &gpio, TCA9548A::mux_address_e::MUX_ADDRESS_1);

        static Multiplexer multiplexer_0(&mux_0);
        static Multiplexer multiplexer_1(&mux_1);

        switch (id) {
            case MuxId::Mux0:
                return multiplexer_0;
            case MuxId::Mux1:
                return multiplexer_1;
        }
        return multiplexer_0;
    }

    AnalogReader& get_analog_reader(AnalogReaderId id) {
        static ADS1115 adc(&get_i2c());

        static AnalogReader reader_0(&adc,
                                     ADS1115::data_rate_e::sps_128,
                                     ADS1115::pga_gain_e::v4_096,
                                     ADS1115::mux_config_e::single_a0);
        static AnalogReader reader_1(&adc,
                                     ADS1115::data_rate_e::sps_128,
                                     ADS1115::pga_gain_e::v2_048,
                                     ADS1115::mux_config_e::single_a1);
        static AnalogReader reader_2(&adc,
                                     ADS1115::data_rate_e::sps_128,
                                     ADS1115::pga_gain_e::v4_096,
                                     ADS1115::mux_config_e::single_a2);
        static AnalogReader reader_3(&adc,
                                     ADS1115::data_rate_e::sps_128,
                                     ADS1115::pga_gain_e::v2_048,
                                     ADS1115::mux_config_e::single_a3);
        switch (id) {
            case AnalogReaderId::AnalogReader0:
                return reader_0;
            case AnalogReaderId::AnalogReader1:
                return reader_1;
            case AnalogReaderId::AnalogReader2:
                return reader_2;
            case AnalogReaderId::AnalogReader3:
                return reader_3;
        }
        return reader_0;
    }

}  // namespace hw
