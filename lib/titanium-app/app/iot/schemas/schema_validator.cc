#include "schema_validator.h"

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
 *         - KERNEL_SUCCESS if all fields are present and types match
 *         - KERNEL_ERROR_MISSING_FIELD if any required key is missing
 *         - KERNEL_ERROR_INVALID_TYPE if any value has the wrong type or schema contains unknown type
 */
kernel_error_st validate_json_schema(JsonObject &obj, const json_field_t *schema, size_t schema_len) {
    if (schema == NULL) {
        return KERNEL_ERROR_NULL;
    }

    if (schema_len == 0) {
        return KERNEL_ERROR_INVALID_SIZE;
    }

    for (size_t i = 0; i < schema_len; ++i) {
        const char *key = schema[i].key;

        
        if (!obj.containsKey(key)) {
            return KERNEL_ERROR_MISSING_FIELD;
        }

        JsonVariant value = obj[key];
        switch (schema[i].expected_type) {
            case JSON_TYPE_INT:
                if (!value.is<int>()) return KERNEL_ERROR_INVALID_TYPE;
                break;
            case JSON_TYPE_FLOAT:
                if (!value.is<float>()) return KERNEL_ERROR_INVALID_TYPE;
                break;
            case JSON_TYPE_BOOL:
                if (!value.is<bool>()) return KERNEL_ERROR_INVALID_TYPE;
                break;
            case JSON_TYPE_STRING:
                if (!value.is<const char *>()) return KERNEL_ERROR_INVALID_TYPE;
                break;
            case JSON_TYPE_OBJECT:
                if (!value.is<JsonObject>()) return KERNEL_ERROR_INVALID_TYPE;
                break;
            case JSON_TYPE_ARRAY:
                if (!value.is<JsonArray>()) return KERNEL_ERROR_INVALID_TYPE;
                break;
            default:
                return KERNEL_ERROR_INVALID_TYPE;
        }
    }

    return KERNEL_SUCCESS;
}
