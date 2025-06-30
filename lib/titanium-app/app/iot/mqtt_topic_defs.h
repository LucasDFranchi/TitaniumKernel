#pragma once

#include "app/error/error_num.h"
#include "kernel/inter_task_communication/iot/mqtt/mqtt_client_external_types.h"

/**
 * @brief Enumerates the known MQTT topic identifiers.
 */
typedef enum mqtt_topic_index_e {
    SENSOR_REPORT, /**< Sensor data reports */
    // SYSTEM,        /**< System status updates */
    TOPIC_COUNT,   /**< Total number of defined topics */
} mqtt_topic_index_et;

/**
 * @brief Initializes an array of MQTT topic structures with static definitions.
 *
 * This function populates a runtime array of `mqtt_topic_st` instances using predefined
 * topic definitions (e.g., topic strings, QoS, and direction). It ensures all topic names
 * are safely copied and null-terminated, up to the maximum number of topics specified.
 *
 * Runtime-only fields such as queue handles and function pointers are set to NULL.
 *
 * @param[out] mqtt_topics         Pointer to the array where topics will be initialized.
 * @param[out] mqtt_topics_count   Pointer to store the number of initialized topics.
 * @param[in]  max_topics          Maximum number of topics the output array can hold.
 *
 * @return APP_ERROR_NONE on success,
 *         APP_ERROR_NULL_ARGUMENT if a pointer argument is NULL,
 *         APP_ERROR_INVALID_MQTT_TOPIC_SIZE if a topic string is too long.
 */
app_error_st mqtt_topics_init(mqtt_topic_st *mqtt_topics, size_t *mqtt_topics_count, size_t max_topics);
