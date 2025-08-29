#pragma once

#include "app/sensors/sensor_interface/sensor_interface.h"

kernel_error_st power_sensor_read(sensor_interface_st *ctx, uint8_t sensor_index, float *out_value);