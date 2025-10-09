#include "pressure_sensor.h"

#include "kernel/logger/logger.h"

static const char *TAG = "Pressure Sensor";

/**
 * @brief Convert sensor voltage (in millivolts) to pressure in Pascals.
 *
 * This function linearly maps the input sensor voltage to a pressure value.
 * The transfer function assumes:
 * - 600 mV corresponds to 0 Pa
 * - 3000 mV corresponds to 2400 Pa
 *
 * Any voltage below 600 mV is clamped to 0 Pa, and any voltage above 3000 mV
 * is clamped to 2400 Pa. Out-of-range conditions are logged as warnings.
 *
 * @param[in] voltage_mv   Measured sensor voltage in millivolts.
 * @param[in] sensor_index Index of the sensor (used only for logging context).
 *
 * @return float Pressure value in Pascals, constrained to [0, 2400].
 */
static float voltage_to_pressure(uint16_t voltage_mv, int sensor_index) {
    const uint16_t MIN_VOLTAGE_MV = 600;
    const uint16_t MAX_VOLTAGE_MV = 3000;
    const float MAX_PRESSURE_PA   = 2400.0f;

    if (voltage_mv < MIN_VOLTAGE_MV) {
        logger_print(WARN, TAG,
                     "[Sensor %d] Voltage too low (%u mV), returning 0 Pa",
                     sensor_index, voltage_mv);
        return 0.0f;
    }

    if (voltage_mv > MAX_VOLTAGE_MV) {
        logger_print(WARN, TAG,
                     "[Sensor %d] Voltage too high (%u mV), returning max pressure %.1f Pa",
                     sensor_index, voltage_mv, MAX_PRESSURE_PA);
        return MAX_PRESSURE_PA;
    }

    return ((float)(voltage_mv - MIN_VOLTAGE_MV) /
            (MAX_VOLTAGE_MV - MIN_VOLTAGE_MV)) *
           MAX_PRESSURE_PA;
}

/**
 * @brief Read pressure sensor data and populate the sensor report.
 *
 * This function performs the full read sequence for a pressure sensor:
 * - Validates input parameters.
 * - Selects the correct multiplexer channel for the sensor.
 * - Configures the ADC channel for measurement.
 * - Reads the raw ADC value from the sensor branch.
 * - Converts the raw ADC reading into a voltage, then into pressure (Pa).
 * - Applies calibration (gain and offset) from the sensor context.
 * - Updates the corresponding entry in the @p sensor_report array.
 *
 * @param[in]  ctx            Pointer to the sensor interface context.
 *                            Must provide valid MUX and ADC controller handles.
 * @param[out] sensor_report  Array of sensor reports to update. The entry at
 *                            @p ctx->index will be updated with the pressure data.
 *
 * @return kernel_error_st
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_NULL if @p ctx or @p sensor_report is NULL
 *         - KERNEL_ERROR_xxx if MUX selection, ADC configuration, or ADC read fails
 *
 * @note The measured pressure value is scaled by @p ctx->conversion_gain and shifted
 *       by @p ctx->offset to apply calibration.
 */
kernel_error_st pressure_sensor_read(sensor_interface_st *ctx, sensor_report_st *sensor_report) {
    kernel_error_st err    = KERNEL_SUCCESS;
    int16_t sensor_raw_adc = 0;

    uint8_t sensor_index = ctx->index;

    if ((!sensor_report) || (!ctx)) {
        return KERNEL_ERROR_NULL;
    }

    sensor_report[sensor_index].value       = 0;
    sensor_report[sensor_index].active      = false;

    err = ctx->mux_controller->select_channel(&ctx->hw->mux_hw_config);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to select MUX for sensor %d", sensor_index);
        return err;
    }

    err = ctx->adc_controller->configure(&ctx->hw->adc_sensor_branch);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to configure sensor branch ADC for sensor %d", sensor_index);
        return err;
    }
    err = ctx->adc_controller->read(&ctx->hw->adc_sensor_branch, &sensor_raw_adc);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to read sensor branch ADC for sensor %d", sensor_index);
        return err;
    }

    float pga_sensor_branch = ctx->adc_controller->get_lsb_size(ctx->hw->adc_sensor_branch.pga_gain);
    int16_t voltage_sensor  = (int16_t)((sensor_raw_adc * pga_sensor_branch));

    float pressure                          = voltage_to_pressure(voltage_sensor, sensor_index);
    sensor_report[sensor_index].value       = (pressure * ctx->conversion_gain) + ctx->offset;
    sensor_report[sensor_index].active      = true;

    return KERNEL_SUCCESS;
}