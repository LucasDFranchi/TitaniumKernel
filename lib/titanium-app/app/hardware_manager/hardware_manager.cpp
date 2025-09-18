#include <array>

#include "kernel/hal/gpio/gpio_handler.h"
#include "kernel/hal/i2c/i2c_handler.h"

std::array<I2CHandler::I2CHardwareConfig, 2> i2c_configs = {
    {
        {GPIO_NUM_21, GPIO_NUM_22, I2C_NUM_0, 400000},
    },
};

std::array<GPIOHandler::GPIOHardwareConfig, 2> gpio_configs = {
    {
        {GPIO_NUM_18, GPIO_MODE_OUTPUT, GPIO_PULLUP_ONLY},
    },
};
