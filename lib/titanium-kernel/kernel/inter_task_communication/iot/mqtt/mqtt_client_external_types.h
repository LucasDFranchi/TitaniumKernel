#ifndef MQTT_CLIENT_EXTERNAL_TYPES_H
#define MQTT_CLIENT_EXTERNAL_TYPES_H

#include <stdint.h>

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

#define MQTT_MAXIMUM_TOPIC_LENGTH 64      ///< Defines the maximum length of an MQTT topic string.
#define MQTT_MAXIMUM_PAYLOAD_LENGTH 1024  ///< Defines the maximum length of an MQTT payload string.
#define MAX_MQTT_TOPICS 10                ///< Maximum number of MQTT topics that can be subscribed to.

typedef uint32_t data_type_et;  ///< Type of the data used in the topic, used for serialization and routing.

typedef enum mqtt_data_direction_e {
    PUBLISH,    ///< Publish data to the MQTT broker.
    SUBSCRIBE,  ///< Subscribe to data from the MQTT broker.
} mqtt_data_direction_et;

typedef enum qos_e {
    QOS_0 = 0,  ///< Quality of Service (QoS) level 0 - at most once.
    QOS_1 = 1,  ///< Quality of Service (QoS) level 1 - at least once.
    QOS_2 = 2,  ///< Quality of Service (QoS) level 2 - exactly once.
} qos_et;

/**
 * @brief MQTT buffer abstraction.
 *
 * Represents a generic character buffer and its size. This is used to pass
 * topic names and payloads around the system in a flexible and consistent way.
 */
typedef struct mqtt_buffer_t {
    char *buffer;  ///< Pointer to the buffer memory.
    size_t size;   ///< Size of the buffer in bytes.
} mqtt_buffer_st;

/**
 * @brief Static configuration for an MQTT topic.
 *
 * This structure holds the compile-time configuration of an MQTT topic,
 * including its topic string, QoS level, data direction, and associated queue parameters.
 * It also includes a data type identifier used by the serialization/deserialization layer.
 *
 * This structure is meant to be immutable and should be shared across runtime
 * topic instances. One instance should exist for each unique topic definition.
 */
typedef struct {
    const char topic[MQTT_MAXIMUM_TOPIC_LENGTH];  ///< MQTT topic string (null-terminated).
    qos_et qos;                                   ///< Quality of Service (QoS) level for the topic.
    mqtt_data_direction_et mqtt_data_direction;   ///< Data direction (PUBLISH or SUBSCRIBE).
    size_t queue_length;                          ///< Number of items the associated queue can hold.
    uint32_t queue_item_size;                     ///< Size in bytes of each item in the queue.
    data_type_et data_type;                       ///< Type of the data used in the topic, used for serialization and routing.
} mqtt_topic_info_st;

/**
 * @brief Runtime state for an MQTT topic.
 *
 * Links the static configuration (`mqtt_topic_info_st`) with the runtime data
 * structures needed to process messages (e.g., FreeRTOS queue). One instance of
 * this struct should exist per MQTT topic in use.
 */
typedef struct {
    const mqtt_topic_info_st *info;  ///< Pointer to constant topic info.
    QueueHandle_t queue;             ///< Runtime queue handle.
} mqtt_topic_st;

/**
 * @brief Function pointer for fetching data to publish.
 *
 * This function is called to prepare topic and payload data for publishing to the broker.
 */
typedef kernel_error_st (*fetch_func_t)(uint8_t mqtt_index, mqtt_buffer_st *topic, mqtt_buffer_st *payload, qos_et *qos);

/**
 * @brief Function pointer for subscribing to topics.
 *
 * Called during initialization to set up MQTT subscriptions.
 */
typedef kernel_error_st (*subscribe_t)(uint8_t mqtt_index, mqtt_buffer_st *topic, qos_et *qos);

/**
 * @brief Function pointer for handling incoming MQTT data.
 *
 * Called whenever an MQTT event with incoming data is received.
 */
typedef kernel_error_st (*handle_event_data_t)(char *topic, mqtt_buffer_st *payload);

/**
 * @brief Function pointer to get the number of active topics.
 *
 * Returns the number of topics currently registered or supported by the bridge.
 */
typedef size_t (*get_topics_count_t)(void);

/**
 * @brief Represents the MQTT communication bridge.
 *
 * Encapsulates the logic and function pointers required to interact with
 * the MQTT layer. This includes functions to fetch data to publish, subscribe
 * to topics, and handle incoming messages.
 *
 * This struct does not expose the internal topic list directly — only the
 * application and the bridge implementation are allowed to manage topics.
 * The MQTT task interacts solely via function pointers.
 */
typedef struct mqtt_bridge_s {
    fetch_func_t fetch_publish_data;        ///< Function to fetch data for publishing.
    subscribe_t subscribe;                  ///< Function to subscribe to topics (optional).
    handle_event_data_t handle_event_data;  ///< Function to handle incoming MQTT data (optional).
    get_topics_count_t get_topics_count;    ///< Function to retrieve the number of registered topics.
} mqtt_bridge_st;

#endif /* MQTT_CLIENT_EXTERNAL_TYPES_H */
