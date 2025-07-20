/**
 * @file app.c
 * @brief Temperature monitoring and logging for temperature and humidity data.
 *
 * This module interfaces with the AHT10 temperature and humidity sensor,
 * reads the data, and logs the temperature and humidity values periodically.
 * It provides functions to initialize the sensor, execute continuous readings,
 * and log the results.
 */
#include "app.h"

#include "string.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "math.h"

#include "kernel/device/device_info.h"
#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/interface/task_interface.h"
#include "kernel/utils/utils.h"

#include "app/app_extern_types.h"
#include "app/command_dispatcher/command_dispatcher.h"
#include "app/iot/mqtt_bridge.h"
#include "app/sensor/sensor.h"
#include "app/system/network_bridge/network_bridge.h"

/**
 * @brief Network bridge interface with function pointers for Ethernet operations.
 *
 * This static instance holds the function pointers for driver initialization
 * and Ethernet event handling. It is initialized with NULLs and configured
 * during network bridge initialization.
 */
static network_bridge_st network_bridge = {
    .initialize_driver = NULL,
};

/**
 * @brief Static initialization structure for the network bridge configuration.
 *
 * Contains the Ethernet device hardware configuration including SPI pins,
 * interrupt and PHY reset pins, polling parameters, and MAC address storage.
 * It also holds a pointer to the associated network_bridge interface.
 */
static network_bridge_init_st network_bridge_init_struct = {
    .ethernet_device = {
        .ethernet_hardware_config = {
            .ethernet_spi_config = {
                .miso          = GPIO_NUM_19,  ///< SPI MISO pin number
                .mosi          = GPIO_NUM_23,  ///< SPI MOSI pin number
                .sclk          = GPIO_NUM_18,  ///< SPI Clock pin number
                .cs            = GPIO_NUM_5,   ///< SPI Chip Select pin number
                .spi_host      = SPI3_HOST,    ///< SPI host to use (SPI3)
                .spi_clock_mhz = 10,           ///< SPI clock frequency in MHz
            },
            .irq_gpio       = -1,           ///< IRQ GPIO pin (-1 if unused)
            .phy_reset_gpio = GPIO_NUM_26,  ///< PHY reset GPIO pin number
        },
        .poll_period_ms = 10,          ///< Polling period in milliseconds
        .phy_addr       = 1,           ///< PHY address on the MDIO bus
        .mac_addr       = {0},         ///< MAC address placeholder (initialized to zero)
        .rx_stack_size  = (2048 * 4),  ///< RX stack size in bytes
    },
    .network_bridge = &network_bridge,  ///< Pointer to the network bridge interface instance
};

/* MQTT Topics Definition */
/**
 * @brief Enum listing the MQTT topic indexes.
 */
typedef enum mqtt_topic_index_e {
    DEVICE_REPORT = 0, /**< Sensor data reports */
    COMMAND,           /**<*/
    COMMAND_RESPONSE,  /**<*/
    TOPIC_COUNT,       /**< Total number of defined topics */
} mqtt_topic_index_et;

/**
 * @brief Array of constant MQTT topic info structures.
 *
 * Each element defines topic string, QoS, direction, and queue parameters.
 */
static const mqtt_topic_info_st mqtt_topic_infos[] = {
    [DEVICE_REPORT] = {
        .topic               = "sensor/report",
        .qos                 = QOS_1,
        .mqtt_data_direction = PUBLISH,
        .queue_length        = 10,
        .queue_item_size     = sizeof(device_report_st),
        .data_type           = DATA_TYPE_REPORT,
    },
    [COMMAND] = {
        .topic               = "command",
        .qos                 = QOS_1,
        .mqtt_data_direction = SUBSCRIBE,
        .queue_length        = 10,
        .queue_item_size     = sizeof(command_st),
        .data_type           = DATA_TYPE_COMMAND,
    },
    [COMMAND_RESPONSE] = {
        .topic               = "command",
        .qos                 = QOS_1,
        .mqtt_data_direction = PUBLISH,
        .queue_length        = 10,
        .queue_item_size     = sizeof(command_response_st),
        .data_type           = DATA_TYPE_COMMAND_RESPONSE,
    },
};

/**
 * @brief Runtime array of MQTT topics with associated queues.
 *
 * Initialized with pointers to constant topic info and queue handles (NULL initially).
 */
mqtt_topic_st mqtt_topics[MAX_MQTT_TOPICS] = {
    [DEVICE_REPORT] = {
        .info  = &mqtt_topic_infos[DEVICE_REPORT],
        .queue = NULL,
    },
    [COMMAND] = {
        .info  = &mqtt_topic_infos[COMMAND],
        .queue = NULL,
    },
    [COMMAND_RESPONSE] = {
        .info  = &mqtt_topic_infos[COMMAND_RESPONSE],
        .queue = NULL,
    },
};

/**
 * @brief MQTT bridge instance containing function pointers for MQTT operations.
 *
 * Function pointers are set to NULL initially and assigned during bridge initialization.
 */
mqtt_bridge_st mqtt_bridge = {
    .fetch_publish_data = NULL, /**< Function pointer to fetch publish data */
    .subscribe          = NULL, /**< Function pointer to subscribe to topics */
    .handle_event_data  = NULL, /**< Function pointer to handle incoming MQTT data */
    .get_topics_count   = NULL, /**< Function pointer to get the number of registered topics */
};

/**
 * @brief Initialization struct for the MQTT bridge.
 *
 * Holds a pointer to the MQTT bridge instance, the array of runtime topics, and the topic count.
 */
mqtt_bridge_init_struct_st mqtt_bridge_init_struct = {
    .mqtt_bridge = &mqtt_bridge, /**< Pointer to the mqtt_bridge instance */
    .topic_count = TOPIC_COUNT,  /**< Number of topics in the topics array */
    .topics      = mqtt_topics,  /**< Pointer to the mqtt_topics array */
};

/* Application Global Variables */
/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st *_global_structures = NULL;                ///< Pointer to the global configuration structure.
static const char *TAG                          = "Application Task";  ///< Tag used for logging.

static esp_err_t app_task_initialize() {
    sensor_manager_initialize();

    if (mqtt_bridge_initialize(&mqtt_bridge_init_struct) != KERNEL_ERROR_NONE) {
        logger_print(INFO, TAG, "MQTT bridge installed failed!");
        return ESP_FAIL;
    }
    xQueueSend(_global_structures->global_queues.mqtt_bridge_queue,
               mqtt_bridge_init_struct.mqtt_bridge,
               pdMS_TO_TICKS(100));

    if (network_bridge_initialize(&network_bridge_init_struct) != KERNEL_ERROR_NONE) {
        logger_print(INFO, TAG, "Network bridge installed failed!");
        return ESP_FAIL;
    }
    xQueueSend(_global_structures->global_queues.network_bridge_queue,
               network_bridge_init_struct.network_bridge,
               pdMS_TO_TICKS(100));

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_32),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);

    return ESP_OK;
}

void app_task_execute(void *pvParameters) {
    _global_structures = (global_structures_st *)pvParameters;
    if ((app_task_initialize() != ESP_OK) || validate_global_structure(_global_structures)) {
        logger_print(ERR, TAG, "Failed to initialize app task");
        vTaskDelete(NULL);
    }
    static bool led_on = false;

    while (1) {
        handle_incoming_command(mqtt_topics[COMMAND].queue, mqtt_topics[COMMAND_RESPONSE].queue);
        handle_device_report(mqtt_topics[DEVICE_REPORT].queue);

        led_on = !led_on;
        gpio_set_level(GPIO_NUM_32, led_on);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}