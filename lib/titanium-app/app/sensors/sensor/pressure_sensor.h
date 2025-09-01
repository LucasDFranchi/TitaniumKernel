#pragma once

#include "app/sensors/sensor_interface/sensor_interface.h"

kernel_error_st pressure_sensor_read(sensor_interface_st *ctx, sensor_report_st *out_value);