#ifndef MQTT_CLIENT_EXTERNAL_TYPES_H
#define MQTT_CLIENT_EXTERNAL_TYPES_H

#include <stdint.h>
#include "error/error_num.h"
#include "inter_task_communication/inter_task_communication.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"  // Include FreeRTOS semaphore header

#define MQTT_MAXIMUM_TOPIC_LENGTH 64  ///< Defines the maximum length of an MQTT topic string.
#define MQTT_MAXIMUM_TOPIC_COUNT 10   ///< Defines the maximum number of MQTT topics that can be subscribed to.
#define MQTT_MAX_DATA_PAYLOAD_SIZE 9  ///< Defines the maximum size of the data payload to be sent over MQTT.

typedef enum mqtt_data_direction_e {
    PUBLISH,    ///< Publish data to the MQTT broker.
    SUBSCRIBE,  ///< Subscribe to data from the MQTT broker.
} mqtt_data_direction_et;

typedef enum qos_e {
    QOS_0 = 0,  ///< Quality of Service (QoS) level 0 - at most once.
    QOS_1 = 1,  ///< Quality of Service (QoS) level 1 - at least once.
    QOS_2 = 2,  ///< Quality of Service (QoS) level 2 - exactly once.
} qos_et;

typedef struct mqtt_topic_s {
    char topic[MQTT_MAXIMUM_TOPIC_LENGTH];       ///< MQTT topic string.
    QueueHandle_t queue;                         ///< External handle for the sensor data queue.
    qos_et qos;                                  ///< Quality of Service (QoS) level for the topic.
    mqtt_data_direction_et mqtt_data_direction;  ///< Data structure type for the topic.
    SemaphoreHandle_t semaphore;                 ///< Semaphore for thread-safe access.

    /**
     * @brief Parses a JSON string and stores the MQTT topic data.
     *
     * This function should extract relevant data from the provided JSON string 
     * and store it in the corresponding mqtt_topic_st structure.
     * It should return an appropriate kernel_error_st code to indicate success or failure.
     *
     * @param json_str The JSON string containing the topic data.
     * @return kernel_error_st Returns KERNEL_ERROR_NONE on success or a specific error code on failure.
     */
    kernel_error_st (*parse_store_json)(const char *json_str);

    /**
     * @brief Function pointer to publish data to an MQTT topic.
     *
     * This function should handle the publishing of data to the corresponding MQTT topic.
     * It should return an appropriate kernel_error_st code to indicate success or failure.
     *
     * @param json_str The data to be sent to the MQTT topic.
     * @return kernel_error_st Returns KERNEL_ERROR_NONE on success or a specific error code on failure.
     */
    kernel_error_st (*encode_json)(const char *json_str);
} mqtt_topic_st;

#endif /* MQTT_CLIENT_EXTERNAL_TYPES_H */
