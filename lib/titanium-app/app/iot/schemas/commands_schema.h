/**
 * @file json_schemas.h
 * @brief Defines JSON schemas used to validate incoming MQTT command payloads.
 *
 * This header declares schemas for validating JSON payloads before they are
 * deserialized into application-specific command structures. Each schema
 * corresponds to a specific command and ensures type and presence validation.
 *
 * These schemas are used in combination with the `validate_json_schema()` function.
 */

#pragma once

#include "schema_validator.h"

/**
 * @brief Schema definition for the CMD_SET_CALIBRATION command.
 *
 * Expected payload structure:
 * {
 *   "sensor_id": int,
 *   "gain": float,
 *   "offset": float
 * }
 */
static const json_field_t calibration_schema[] = {
    {"sensor_id", JSON_TYPE_INT},
    {"gain",      JSON_TYPE_FLOAT},
    {"offset",    JSON_TYPE_FLOAT}
};

// Future command schemas can be added below:
// static const json_field_t reboot_schema[] = {
//     {"delay_ms", JSON_TYPE_INT}
// };
