#include "ntc_temperature.h"

#include "kernel/logger/logger.h"

static const char* TAG = "NTC Sensor";

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
 * @brief Calculate thermistor resistance in kΩ based on voltage divider output.
 *
 * This function estimates the thermistor resistance by applying the voltage divider formula,
 * compensating for reference branch error. The result is used as input for temperature interpolation.
 *
 * @return Calculated thermistor resistance in kΩ, or -1.0f on error.
 */
float TemperatureSensor::calculate_resistance_kohm() {
    float v_supply       = 3.3f;
    float v_error        = 1.65 - this->ref_adc_voltage;
    float adjusted_v_ntc = this->ntc_adc_voltage + v_error;
    float v_gain         = adjusted_v_ntc / v_supply;

    float denominator = 1 - v_gain;
    if (denominator <= 0) {
        logger_print(ERR, TAG,
                     "Sensor %d invalid divider: adjusted_v_ntc=%.3f V (v_gain=%.3f)",
                     this->index_, adjusted_v_ntc, v_gain);
        return -1.0f;
    }

    float resistance_ohm  = (100000.0f * v_gain) / denominator;
    float resistance_kohm = resistance_ohm / 1000.0f;

    logger_print(DEBUG, TAG,
                 "Sensor %d resistance: %.1f Ohm (%.3f kOhm)",
                 this->index_, resistance_ohm, resistance_kohm);

    return resistance_kohm;
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
float TemperatureSensor::resistance_to_temperature(float resistance_kohm) {
    const size_t ntc_table_size = sizeof(ntc_table) / sizeof(ntc_table[0]);

    if (resistance_kohm < 0) {
        logger_print(ERR, TAG, "[Sensor %d] Invalid resistance: %.3f kΩ", this->index_, resistance_kohm);
        return -273.15f;
    }

    if (resistance_kohm >= ntc_table[0].resistance_kohm) {
        logger_print(WARN, TAG, "[Sensor %d] Resistance too high (%.3f kΩ), returning min temp %d°C",
                     this->index_, resistance_kohm, ntc_table[0].temperature_c);
        return ntc_table[0].temperature_c;
    }

    int last = ntc_table_size - 1;
    if (resistance_kohm <= ntc_table[last].resistance_kohm) {
        logger_print(WARN, TAG, "[Sensor %d] Resistance too low (%.3f kΩ), returning max temp %d°C",
                     this->index_, resistance_kohm, ntc_table[last].temperature_c);
        return ntc_table[last].temperature_c;
    }

    size_t low  = 0;
    size_t high = last;
    while (high - low > 1) {
        size_t mid = (low + high) / 2;
        if (resistance_kohm > ntc_table[mid].resistance_kohm)
            high = mid;
        else
            low = mid;
    }

    float r1 = ntc_table[low].resistance_kohm;
    float r2 = ntc_table[high].resistance_kohm;
    float t1 = ntc_table[low].temperature_c;
    float t2 = ntc_table[high].temperature_c;

    return t1 + (resistance_kohm - r1) * (t2 - t1) / (r2 - r1);
}

/**
 * @brief Convert measured thermistor voltage to temperature.
 *
 * Uses the reference branch to compensate measurement errors, computes the thermistor resistance,
 * and maps it to temperature using the lookup table.
 *
 * @return Interpolated temperature in degrees Celsius.
 */
float TemperatureSensor::voltage_to_temperature() {
    float r_kohm = this->calculate_resistance_kohm();
    return resistance_to_temperature(r_kohm);
}

/**
 * @brief Initialize the temperature sensor by configuring its reference and NTC ADCs.
 *
 * This method performs the following steps:
 *  - Verifies that ADC references are set (ref_adc_ and ntc_adc_ are not null).
 *  - Ensures the sensor is not already initialized.
 *  - Initializes both reference and NTC ADC drivers.
 *  - Reads the reference ADC once to calculate the reference voltage.
 *  - Updates the internal sensor state to ENABLED if all steps succeed.
 *
 * @note The function is idempotent: calling it multiple times will not reinitialize
 *       the sensor, but it will log a warning if already initialized.
 *
 * @return
 *  - KERNEL_SUCCESS on success.
 *  - KERNEL_ERROR_NULL if ADC references are missing.
 *  - ADC driver error codes if initialization or reading fails.
 */
kernel_error_st TemperatureSensor::initialize() {
    if (this->ref_adc_ == nullptr || this->ntc_adc_ == nullptr) {
        logger_print(ERR, TAG, "ADC references not set for temperature sensor %d", this->index_);
        return KERNEL_ERROR_NULL;
    }

    if (this->state_ == SensorStatus::Enabled) {
        logger_print(WARN, TAG, "Sensor %d already initialized", this->index_);
        return KERNEL_SUCCESS;
    }

    if (this->ref_adc_->initialize() != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize reference ADC for sensor %d", this->index_);
        return KERNEL_ERROR_FAIL;  // TODO: Improve error handling
    }

    if (this->ntc_adc_->initialize() != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize NTC ADC for sensor %d", this->index_);
        return KERNEL_ERROR_FAIL;  // TODO: Improve error handling
    }

    int16_t ref_adc_raw = 0;
    if (this->ref_adc_->read_adc(ref_adc_raw) != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to read reference ADC for sensor %d", this->index_);
        return KERNEL_ERROR_FAIL;  // TODO: Improve error handling
    }

    float pga_gain        = this->ref_adc_->get_lsb_size();
    this->ref_adc_voltage = static_cast<float>((ref_adc_raw * pga_gain));

    this->state_ = SensorStatus::Enabled;

    return KERNEL_SUCCESS;
}

/**
 * @brief Update the temperature reading from the NTC thermistor.
 *
 * This function reads the raw ADC value from the NTC channel, calculates the
 * corresponding thermistor voltage, converts it to temperature, and applies
 * any calibration gain and offset. Updates the internal temperature state.
 *
 * @return
 *  - KERNEL_SUCCESS if the update succeeded.
 *  - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
 *  - KERNEL_ERROR_FAIL if the ADC read failed.
 */
kernel_error_st TemperatureSensor::update() {
    if (this->state_ != SensorStatus::Enabled) {
        logger_print(ERR, TAG, "Sensor %d not initialized", this->index_);
        return KERNEL_ERROR_SENSOR_NOT_INITIALIZED;
    }

    int16_t ntc_adc_raw = 0;
    if (this->ntc_adc_->read_adc(ntc_adc_raw) != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to read NTC ADC for sensor %d", this->index_);
        return KERNEL_ERROR_FAIL;  // TODO: Improve error handling
    }

    float pga_gain        = this->ntc_adc_->get_lsb_size();
    this->ntc_adc_voltage = static_cast<float>((ntc_adc_raw * pga_gain));
    float temperature_c   = this->voltage_to_temperature();
    this->temperature     = (temperature_c * this->gain_) + this->offset_;

    return KERNEL_SUCCESS;
}

/**
 * @brief Calibrate the temperature sensor.
 *
 * This function sets the gain and offset used to adjust the measured temperature.
 * The calibration is applied to subsequent temperature updates.
 *
 * @param gain  Multiplicative factor to adjust the measured temperature.
 * @param offset Additive offset to adjust the measured temperature.
 *
 * @return
 *  - KERNEL_SUCCESS if calibration succeeded.
 *  - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
 */
kernel_error_st TemperatureSensor::calibrate(float gain, float offset) {
    if (this->state_ != SensorStatus::Enabled) {
        logger_print(ERR, TAG, "Sensor %d not initialized", this->index_);
        return KERNEL_ERROR_SENSOR_NOT_INITIALIZED;
    }

    this->gain_   = gain;
    this->offset_ = offset;

    logger_print(DEBUG, TAG, "Sensor %d calibrated: gain=%.3f, offset=%.3f", this->index_, gain, offset);
    return KERNEL_SUCCESS;
}

/**
 * @brief Fill a sensor report array with the current temperature measurement.
 *
 * Populates each entry of the provided sensor report array with the current
 * temperature, sensor type, and active status. The number of entries filled
 * is determined by SENSOR_SLOTS.
 *
 * @param sensor_list Pointer to an array of sensor_report_st to be filled.
 *
 * @return
 *  - KERNEL_SUCCESS if the report was successfully generated.
 *  - KERNEL_ERROR_SENSOR_NOT_INITIALIZED if the sensor is not enabled.
 *  - KERNEL_ERROR_NULL if sensor_list is a null pointer.
 */
kernel_error_st TemperatureSensor::get_report(sensor_report_st* sensor_list) {
    if (this->state_ != SensorStatus::Enabled) {
        logger_print(ERR, TAG, "Sensor %d not initialized", this->index_);
        return KERNEL_ERROR_SENSOR_NOT_INITIALIZED;
    }

    if (sensor_list == nullptr) {
        logger_print(ERR, TAG, "Sensor report list is null");
        return KERNEL_ERROR_NULL;
    }

    for (size_t i = 0; i < this->SENSOR_SLOTS; i++) {
        sensor_list[i].sensor_type = SENSOR_TYPE_TEMPERATURE;
        sensor_list[i].active      = (this->state_ == SensorStatus::Enabled);
        sensor_list[i].value       = this->temperature;
    }

    return KERNEL_SUCCESS;
}
