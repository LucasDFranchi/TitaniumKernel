#include "pressure_sensor.h"

#include "kernel/logger/logger.h"

static const char *TAG = "Pressure Sensor";

static float voltage_to_pressure(uint16_t voltage, int sensor_index) {
    const float minimal_mili_voltage = 0.6f;
    const float maximal_mili_voltage = 3.0f;
    const float conversion_curve_gain = 2.4f;

    if (voltage < minimal_mili_voltage) {
        logger_print(WARN, TAG, "[Sensor %d] Voltage too low (%d mV), returning 0 Pa", sensor_index, voltage);
        return 0.0f;
    }

    if (voltage > maximal_mili_voltage) {
        logger_print(WARN, TAG, "[Sensor %d] Voltage too high (%d mV), returning max pressure 2400 Pa", sensor_index, voltage);
        return 100.0f;
    }

    return minimal_mili_voltage + (conversion_curve_gain * voltage) / 100.0f;
}

kernel_error_st pressure_sensor_read(sensor_interface_st *ctx, uint8_t sensor_index, float *out_value) {
    int16_t sensor_raw_adc    = 0;

    if ((!out_value) || (!ctx)) {
        return KERNEL_ERROR_NULL;
    }

    kernel_error_st err = KERNEL_ERROR_NONE;

    err = ctx->mux_controller->select_channel(&ctx->hw->mux_hw_config);
    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to select MUX for sensor %d", sensor_index);
        return err;
    }

    err = ctx->adc_controller->configure(&ctx->hw->adc_sensor_branch);
    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to configure sensor branch ADC for sensor %d", sensor_index);
        return err;
    }
    err = ctx->adc_controller->read(&ctx->hw->adc_sensor_branch, &sensor_raw_adc);
    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to read sensor branch ADC for sensor %d", sensor_index);
        return err;
    }

    // This part needs improvement to handle the voltage conversion
    float pga_sensor_branch   = ctx->adc_controller->get_lsb_size(ctx->hw->adc_sensor_branch.pga_gain);
    int16_t voltage_sensor    = (int16_t)((sensor_raw_adc * pga_sensor_branch));

    *out_value = voltage_to_pressure(voltage_sensor, sensor_index);

    return KERNEL_ERROR_NONE;
}