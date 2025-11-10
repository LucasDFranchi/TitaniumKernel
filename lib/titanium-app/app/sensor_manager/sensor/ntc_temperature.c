#include "ntc_temperature.h"

#include "kernel/logger/logger.h"

static const char* TAG               = "NTC Sensor";
static const uint32_t FIXED_RESISTOR = (100 * 1000);  // 100k Ohms

/**
 * @brief Structure representing a single entry in the NTC thermistor lookup table.
 *
 * This structure maps a specific resistance value (in kilo-ohms) to its corresponding
 * temperature (in degrees Celsius). It is used for converting measured resistance
 * from a thermistor into a temperature using a lookup table with interpolation.
 */
typedef struct ntc_entry_s {
    float resistance_kohm; /*!< Resistance of the thermistor in kilo-ohms (kΩ) */
    int temperature_c;     /*!< Corresponding temperature in degrees Celsius (°C) */
} ntc_entry_st;

// clang-format off
static const ntc_entry_st ntc_table[] = {
    {7011.86, -55}, {6670.29, -54}, {6335.44, -53}, {6008.77, -52}, {5691.51, -51},
    {5384.66, -50}, {5088.98, -49}, {4805.02, -48}, {4533.16, -47}, {4273.61, -46},
    {4026.43, -45}, {3791.58, -44}, {3568.88, -43}, {3358.10, -42}, {3158.93, -41},
    {2971.00, -40}, {2793.89, -39}, {2627.18, -38}, {2470.40, -37}, {2323.09, -36},
    {2184.77, -35}, {2054.98, -34}, {1933.24, -33}, {1819.11, -32}, {1712.14, -31},
    {1611.90, -30}, {1517.98, -29}, {1430.00, -28}, {1347.57, -27}, {1270.35, -26},
    {1198.00, -25}, {1130.19, -24}, {1066.65, -23}, {1007.07, -22}, {951.22, -21},
    {898.82, -20},  {849.66, -19},  {803.51, -18},  {760.19, -17},  {719.50, -16},
    {681.26, -15},  {645.31, -14},  {611.51, -13},  {579.70, -12},  {549.77, -11},
    {521.58, -10},  {495.02, -9},   {469.99, -8},   {446.40, -7},   {424.13, -6},
    {403.12, -5},   {383.29, -4},   {364.55, -3},   {346.84, -2},   {330.10, -1},
    {315.68, 0},    {299.28, 1},    {285.10, 2},    {271.67, 3},    {258.95, 4},
    {246.89, 5},    {235.46, 6},    {224.61, 7},    {214.33, 8},    {204.56, 9},
    {195.29, 10},   {186.48, 11},   {178.12, 12},   {170.17, 13},   {162.61, 14},
    {155.42, 15},   {148.58, 16},   {142.07, 17},   {135.87, 18},   {129.98, 19},
    {124.36, 20},   {119.01, 21},   {113.91, 22},   {109.05, 23},   {104.42, 24},
    {100.00, 25},   {95.79, 26},    {91.77, 27},    {87.93, 28},    {84.27, 29},
    {80.78, 30},    {77.44, 31},    {74.26, 32},    {71.22, 33},    {68.31, 34},
    {65.53, 35},    {62.88, 36},    {60.34, 37},    {57.92, 38},    {55.60, 39},
    {53.38, 40},    {51.26, 41},    {49.23, 42},    {47.29, 43},    {45.43, 44},
    {43.65, 45},    {41.95, 46},    {40.32, 47},    {38.76, 48},    {37.27, 49},
    {35.84, 50},    {34.47, 51},    {33.16, 52},    {31.90, 53},    {30.69, 54},
    {29.54, 55},    {28.43, 56},    {27.37, 57},    {26.35, 58},    {25.37, 59},
    {24.44, 60},    {23.54, 61},    {22.68, 62},    {21.85, 63},    {21.06, 64},
    {20.30, 65},    {19.56, 66},    {18.86, 67},    {18.19, 68},    {17.54, 69},
    {16.92, 70},    {16.32, 71},    {15.75, 72},    {15.20, 73},    {14.67, 74},
    {14.16, 75},    {13.67, 76},    {13.20, 77},    {12.75, 78},    {12.32, 79},
    {11.90, 80},    {11.50, 81},    {11.11, 82},    {10.74, 83},    {10.38, 84},
    {10.04, 85},    {9.70, 86},     {9.38, 87},     {9.08, 88},     {8.78, 89},
    {8.50, 90},     {8.22, 91},     {7.96, 92},     {7.70, 93},     {7.45, 94},
    {7.22, 95},     {6.99, 96},     {6.77, 97},     {6.56, 98},     {6.35, 99},
    {6.17, 100},    {5.97, 101},    {5.78, 102},    {5.61, 103},    {5.44, 104},
    {5.27, 105},    {5.11, 106},    {4.96, 107},    {4.81, 108},    {4.67, 109},
    {4.53, 110},    {4.39, 111},    {4.26, 112},    {4.14, 113},    {4.02, 114},
    {3.90, 115},    {3.79, 116},    {3.68, 117},    {3.58, 118},    {3.47, 119},
    {3.38, 120},    {3.28, 121},    {3.19, 122},    {3.10, 123},    {3.01, 124},
    {2.93, 125},    {2.85, 126},    {2.77, 127},    {2.70, 128},    {2.62, 129},
    {2.55, 130},    {2.48, 131},    {2.42, 132},    {2.35, 133},    {2.29, 134},
    {2.23, 135},    {2.17, 136},    {2.11, 137},    {2.06, 138},    {2.00, 139},
    {1.95, 140},    {1.90, 141},    {1.85, 142},    {1.80, 143},    {1.76, 144},
    {1.71, 145},    {1.67, 146},    {1.63, 147},    {1.59, 148},    {1.55, 149},
    {1.51, 150},
};
// clang-format on

/**
 * @brief Apply 5-region polynomial correction to match measured resistance.
 *
 * Regions defined descending (high -> low) and contiguous to avoid gaps.
 *
 * @param r_in Calculated resistance (kΩ) from voltage divider.
 * @return Calibrated resistance (kΩ) corrected to match measured values.
 */
static float correct_resistance_kohm(float r_in) {
    typedef struct {
        float r_high;  // inclusive upper bound (kΩ)
        float r_low;   // exclusive lower bound (kΩ) except last region
        float a, b, c; // quadratic coefficients
    } region_fit_t;

    static const region_fit_t regions[] = {
        /* Region 1: 3361.887 -> 329.300 kΩ */
        {3361.887f, 329.300f, 3.050603e-06f, 9.680608e-01f, 1.101766e+01f},

        /* Region 2: 329.300 -> 87.474 kΩ */
        {329.300f, 87.474f, 3.750742e-04f, 8.410913e-01f, 1.230265e+01f},

        /* Region 3: 87.474 -> 22.259 kΩ */
        {87.474f, 22.259f, -4.009059e-05f, 9.984124e-01f, -2.474721e-01f},

        /* Region 4: 22.259 -> 6.731 kΩ */
        {22.259f, 6.731f, -3.474550e-04f, 1.032403e+00f, -1.619189e-01f},

        /* Region 5: 6.731 -> 2.232 kΩ (lowest region: include lower bound) */
        {6.731f, 2.232f, -2.576672e-03f, 1.038778e+00f, -1.142167e-01f},
    };

    const size_t region_count = sizeof(regions) / sizeof(regions[0]);
    const size_t last_index = region_count - 1;

    for (size_t i = 0; i < region_count; ++i) {
        float high = regions[i].r_high;
        float low  = regions[i].r_low;

        if ((r_in <= high) && (r_in > low)) {
            float a = regions[i].a;
            float b = regions[i].b;
            float c = regions[i].c;
            return (a * r_in * r_in) + (b * r_in) + c;
        }
        else if ((r_in > high) && (i == 0)) {
            // Above highest region
            float a = regions[i].a;
            float b = regions[i].b;
            float c = regions[i].c;
            return (a * r_in * r_in) + (b * r_in) + c;
        }
        else if (r_in <= low && i == last_index) {
            // Below lowest region
            float a = regions[i].a;
            float b = regions[i].b;
            float c = regions[i].c;
            return (a * r_in * r_in) + (b * r_in) + c;
        }
    }
    
    return r_in;
}


/**
 * @brief Calculate thermistor resistance in kΩ based on voltage divider output.
 *
 * This function estimates the thermistor resistance by applying the voltage divider formula,
 * compensating for reference branch error. The result is used as input for temperature interpolation.
 *
 * @param v_ref Reference voltage measured from the divider (mV).
 * @param v_ntc Voltage measured across the thermistor branch (mV).
 * @param sensor_index Index of the sensor (for logging purposes).
 * @return Calculated thermistor resistance in kΩ.
 */
static float calculate_resistance_kohm(float v_ref, float v_ntc, uint16_t sensor_index) {
    float v_supply       = 3300.0f;
    float v_error        = 1650.0f - v_ref;
    float adjusted_v_ntc = v_ntc + v_error;
    float v_gain         = (adjusted_v_ntc / v_supply);

    uint32_t resistance_ohm = (FIXED_RESISTOR * v_gain) / (1 - v_gain);

    logger_print(DEBUG, TAG, "Calculated resistance %d: %d Ohm (%.3f kOhm)", sensor_index, resistance_ohm, resistance_ohm / 1000.0f);
    return correct_resistance_kohm(resistance_ohm / 1000.0f);
}

/**
 * @brief Convert thermistor resistance to temperature using lookup table interpolation.
 *
 * Performs a binary search over the NTC lookup table and applies linear interpolation
 * between two nearest resistance values. Handles out-of-range resistance values by
 * returning the closest valid temperature.
 *
 * @param resistance_kohm Thermistor resistance in kΩ.
 * @param sensor_index Index of the sensor (for logging and error reporting).
 * @return Interpolated temperature in degrees Celsius.
 */
static float resistance_to_temperature(float resistance_kohm, int sensor_index) {
    const size_t ntc_table_size = sizeof(ntc_table) / sizeof(ntc_table[0]);

    if (resistance_kohm < 0) {
        logger_print(ERR, TAG, "[Sensor %d] Invalid resistance: %.3f kΩ", sensor_index, resistance_kohm);
        return -273.15f;
    }

    if (resistance_kohm >= ntc_table[0].resistance_kohm) {
        logger_print(WARN, TAG, "[Sensor %d] Resistance too high (%.3f kΩ), returning min temp %d°C",
                     sensor_index, resistance_kohm, ntc_table[0].temperature_c);
        return ntc_table[0].temperature_c;
    }

    int last = ntc_table_size - 1;
    if (resistance_kohm <= ntc_table[last].resistance_kohm) {
        logger_print(WARN, TAG, "[Sensor %d] Resistance too low (%.3f kΩ), returning max temp %d°C",
                     sensor_index, resistance_kohm, ntc_table[last].temperature_c);
        return ntc_table[last].temperature_c;
    }

    int low  = 0;
    int high = last;
    while (high - low > 1) {
        int mid = (low + high) / 2;
        if (resistance_kohm > ntc_table[mid].resistance_kohm)
            high = mid;
        else
            low = mid;
    }

    float r1 = ntc_table[low].resistance_kohm;
    float r2 = ntc_table[high].resistance_kohm;
    float t1 = ntc_table[low].temperature_c;
    float t2 = ntc_table[high].temperature_c;

    if (r1 == r2)
        return (t1 + t2) / 2.0f;

    return t1 + (resistance_kohm - r1) * (t2 - t1) / (r2 - r1);
}

/**
 * @brief Convert measured thermistor voltage to temperature.
 *
 * Uses the reference branch to compensate measurement errors, computes the thermistor resistance,
 * and maps it to temperature using the lookup table.
 *
 * @param v_ref Reference branch voltage (mV).
 * @param v_ntc Sensor branch voltage (mV).
 * @param sensor_index Index of the sensor (for logging).
 * @return Interpolated temperature in degrees Celsius.
 */
static float voltage_to_temperature(float v_ref, float v_ntc, int sensor_index) {
    float r_kohm = calculate_resistance_kohm(v_ref, v_ntc, sensor_index);
    return resistance_to_temperature(r_kohm, sensor_index);
}

/**
 * @brief Acquire and convert temperature reading from an NTC sensor.
 *
 * This is the main entry point for sensor reading. The function:
 *  - Selects the appropriate MUX channel for the sensor.
 *  - Configures and samples both reference and sensor ADC branches.
 *  - Dynamically adjusts PGA gain if required to improve accuracy.
 *  - Converts raw ADC readings into voltages, then into resistance and finally temperature.
 *
 * @param ctx Sensor interface context containing hardware configuration and driver callbacks.
 * @param[out] out_value Pointer to store the resulting temperature in Celsius.
 * @return kernel_error_st Error code indicating success or failure.
 */
kernel_error_st temperature_sensor_read(sensor_interface_st* ctx, sensor_report_st* sensor_report) {
    kernel_error_st err       = KERNEL_SUCCESS;
    int16_t reference_raw_adc = 0;
    int16_t sensor_raw_adc    = 0;

    if ((!sensor_report) || (!ctx)) {
        return KERNEL_ERROR_NULL;
    }

    uint8_t sensor_index = ctx->index;

    sensor_report[sensor_index].value  = 0;
    sensor_report[sensor_index].active = false;

    err = ctx->mux_controller->select_channel(&ctx->hw->mux_hw_config);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to select MUX for sensor %d", sensor_index);
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    /* First we measure the reference branch to estimate the error based on the voltage input */
    err = ctx->adc_controller->configure(&ctx->hw->adc_ref_branch);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to configure reference branch ADC for sensor %d - %d", sensor_index, err);
        return err;
    }
    err = ctx->adc_controller->read(&ctx->hw->adc_ref_branch, &reference_raw_adc);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to read reference branch ADC for sensor %d - %d", sensor_index, err);
        return err;
    }

    /* Them we try to measure using the maximum PGA*/
    err = ctx->adc_controller->configure(&ctx->hw->adc_sensor_branch);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to configure sensor branch ADC for sensor %d - %d", sensor_index, err);
        return err;
    }
    err = ctx->adc_controller->read(&ctx->hw->adc_sensor_branch, &sensor_raw_adc);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to read sensor branch ADC for sensor %d - %d", sensor_index, err);
        return err;
    }

    // This part needs improvement to handle the voltage conversion
    float pga_ref_branch    = ctx->adc_controller->get_lsb_size(ctx->hw->adc_ref_branch.pga_gain);
    float pga_sensor_branch = ctx->adc_controller->get_lsb_size(ctx->hw->adc_sensor_branch.pga_gain);
    float voltage_reference = (float)((reference_raw_adc * pga_ref_branch));
    float voltage_sensor    = (float)((sensor_raw_adc * pga_sensor_branch));

    pga_gain_et fine_pga_gain = ctx->adc_controller->get_pga_gain(voltage_sensor);

    logger_print(DEBUG, TAG, "Voltage sensor: %f mV, Current PGA: %d, Fine PGA: %d", voltage_sensor, ctx->hw->adc_sensor_branch.pga_gain, fine_pga_gain);

    if (fine_pga_gain != ctx->hw->adc_sensor_branch.pga_gain) {
        // ctx->hw->adc_sensor_branch.pga_gain = fine_pga_gain;

        err = ctx->adc_controller->configure(&ctx->hw->adc_sensor_branch);
        if (err != KERNEL_SUCCESS) {
            logger_print(ERR, TAG, "Failed to configure sensor branch ADC for sensor %d - %d", sensor_index, err);
            return err;
        }
        err = ctx->adc_controller->read(&ctx->hw->adc_sensor_branch, &sensor_raw_adc);
        if (err != KERNEL_SUCCESS) {
            logger_print(ERR, TAG, "Failed to read sensor branch ADC for sensor %d - %d", sensor_index, err);
            return err;
        }
        pga_sensor_branch = ctx->adc_controller->get_lsb_size(ctx->hw->adc_sensor_branch.pga_gain);
        voltage_sensor    = (float)((sensor_raw_adc * pga_sensor_branch));
    }

    logger_print(DEBUG, TAG,
                 "Sensor %d: Reference ADC: %d, Sensor ADC: %d, Reference Voltage: %f mV, Sensor Voltage: %f mV",
                 sensor_index, reference_raw_adc, sensor_raw_adc, voltage_reference, voltage_sensor);

    float temperature_c = voltage_to_temperature(voltage_reference, voltage_sensor, sensor_index);

    sensor_report[sensor_index].value  = (temperature_c * ctx->conversion_gain) + ctx->offset;
    sensor_report[sensor_index].active = true;

    return KERNEL_SUCCESS;
}
