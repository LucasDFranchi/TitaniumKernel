#pragma once

#include "kernel/error/error_num.h"

#include "app/third_party/json_handler.h"

typedef enum {
    JSON_TYPE_INT,
    JSON_TYPE_FLOAT,
    JSON_TYPE_BOOL,
    JSON_TYPE_STRING,
    JSON_TYPE_OBJECT,
    JSON_TYPE_ARRAY
} json_field_type_t;

typedef struct {
    const char *key;
    json_field_type_t expected_type;
} json_field_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validates a JSON object against a predefined schema.
 *
 * This function checks that all required keys specified in the schema are present
 * in the given JSON object and that the values match the expected types.
 *
 * @param obj          Reference to the ArduinoJson JsonObject to validate.
 * @param schema       Pointer to an array of `json_field_t` describing expected fields.
 * @param schema_len   Number of elements in the schema array.
 *
 * @return kernel_error_st
 *         - KERNEL_ERROR_NONE if all fields are present and types match
 *         - KERNEL_ERROR_MISSING_FIELD if any required key is missing
 *         - KERNEL_ERROR_INVALID_TYPE if any value has the wrong type or schema contains unknown type
 */
kernel_error_st validate_json_schema(JsonObject &obj, const json_field_t *schema, size_t schema_len);

#ifdef __cplusplus
}
#endif