#include "mqtt_topic_defs.h"

#include "string.h"

typedef struct mqtt_topic_def_s {
    char topic[MQTT_MAXIMUM_TOPIC_LENGTH];
    qos_et qos;
    mqtt_data_direction_et mqtt_data_direction;
} mqtt_topic_def_st;

static const mqtt_topic_def_st mqtt_topic_defs[TOPIC_COUNT] = {
    [SENSOR_REPORT] = {"sensor/report", QOS_1, PUBLISH},
    // [SYSTEM] = {"system", QOS_1, PUBLISH},
};

static const size_t mqtt_topic_defs_count = sizeof(mqtt_topic_defs) / sizeof(mqtt_topic_def_st);

/**
 * @brief Initializes the array of mqtt_topic_st based on static definitions.
 *
 * This function fills the provided mqtt_topics array with statically defined
 * topic metadata, ensuring proper string safety and zero-initialization
 * of runtime fields (queues and function pointers).
 *
 * @param[out] mqtt_topics         Destination array to be initialized.
 * @param[out] mqtt_topics_count   Pointer to receive the number of topics initialized.
 * @param[in]  max_topics          Maximum number of topics the array can hold.
 *
 * @return APP_ERROR_NONE on success.
 *         APP_ERROR_NULL_ARGUMENT if input pointers are NULL.
 *         APP_ERROR_INVALID_MQTT_TOPIC_SIZE if a topic string is too long.
 */
app_error_st mqtt_topics_init(mqtt_topic_st *mqtt_topics, size_t *mqtt_topics_count, size_t max_topics) {
    if (mqtt_topics == NULL || mqtt_topics_count == NULL) {
        return APP_ERROR_NULL;
    }

    size_t initialized_count = 0;

    for (size_t i = 0; i < mqtt_topic_defs_count && initialized_count < max_topics; i++) {
        const char *src = mqtt_topic_defs[i].topic;
        size_t topic_len = strlen(src);

        if (topic_len >= MQTT_MAXIMUM_TOPIC_LENGTH) {
            continue;
        }

        mqtt_topic_st *dest = &mqtt_topics[initialized_count];

        snprintf(dest->topic, MQTT_MAXIMUM_TOPIC_LENGTH, "%s", src);
        dest->qos = mqtt_topic_defs[i].qos;
        dest->mqtt_data_direction = mqtt_topic_defs[i].mqtt_data_direction;

        dest->queue = NULL;
        dest->deserialize_data = NULL;
        dest->serialize_data = NULL;

        initialized_count++;
    }

    *mqtt_topics_count = initialized_count;
    return APP_ERROR_NONE;
}