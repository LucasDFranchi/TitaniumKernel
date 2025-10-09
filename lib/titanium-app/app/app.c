/**
 * @file app.c
 * @brief Main application initialization and task management.
 *
 * This module is responsible for:
 * - Initializing the network bridge (Ethernet) and MQTT bridge.
 * - Setting up Sensor, Command, and Health Manager tasks.
 * - Attaching tasks to the FreeRTOS scheduler.
 * - Providing centralized configuration via global structures.
 *
 * It defines static configuration structures for bridges, tasks, and
 * manager modules, which are initialized at runtime. Logging is
 * integrated for initialization steps and error handling.
 */

#include "app.h"

#include "string.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "math.h"

#include "kernel/device/device_info.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/interface/task_interface.h"
#include "kernel/tasks/manager/task_handler.h"
#include "kernel/utils/utils.h"

#include "app/app_extern_types.h"
#include "app/app_tasks_config.h"
#include "app/iot/mqtt_bridge.h"
#include "app/system/network_bridge/network_bridge.h"
// TODO: move to a managers folder
#include "app/command_manager/command_manager.h"
#include "app/health_manager/health_manager.h"
#include "app/sd_card_manager/sd_card_manager.h"
#include "app/sensor_manager/sensor_manager.h"

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

/**
 * @brief Array of constant MQTT topic info structures.
 *
 * Each element defines topic string, QoS, direction, and queue parameters.
 */
static const mqtt_topic_info_st mqtt_topic_infos[] = {
    [SENSOR_REPORT] = {
        .topic               = "sensor/report",
        .qos                 = QOS_1,
        .mqtt_data_direction = PUBLISH,
        .queue_length        = 10,
        .queue_item_size     = sizeof(device_report_st),
        .data_type           = DATA_TYPE_SENSOR_REPORT,
        .message_type        = MESSAGE_TYPE_TARGET,
    },
    [BROADCAST_COMMAND] = {
        .topic               = "all/command",
        .qos                 = QOS_1,
        .mqtt_data_direction = SUBSCRIBE,
        .queue_length        = 10,
        .queue_item_size     = sizeof(command_st),
        .data_type           = DATA_TYPE_COMMAND,
        .message_type        = MESSAGE_TYPE_BROADCAST,
    },
    [TARGET_COMMAND] = {
        .topic               = "command",
        .qos                 = QOS_1,
        .mqtt_data_direction = SUBSCRIBE,
        .queue_length        = 10,
        .queue_item_size     = sizeof(command_st),
        .data_type           = DATA_TYPE_COMMAND,
        .message_type        = MESSAGE_TYPE_TARGET,
    },
    [RESPONSE_COMMAND] = {
        .topic               = "command",
        .qos                 = QOS_1,
        .mqtt_data_direction = PUBLISH,
        .queue_length        = 10,
        .queue_item_size     = sizeof(command_response_st),
        .data_type           = DATA_TYPE_COMMAND_RESPONSE,
        .message_type        = MESSAGE_TYPE_TARGET,
    },
    [HEALTH_REPORT] = {
        .topic               = "health/report",
        .qos                 = QOS_1,
        .mqtt_data_direction = PUBLISH,
        .queue_length        = 10,
        .queue_item_size     = sizeof(health_report_st),
        .data_type           = DATA_TYPE_HEALTH_REPORT,
        .message_type        = MESSAGE_TYPE_TARGET,
    },
};

/**
 * @brief Runtime array of MQTT topics with associated queues.
 *
 * Initialized with pointers to constant topic info and queue handles (NULL initially).
 */
mqtt_topic_st mqtt_topics[TOPIC_COUNT] = {
    [SENSOR_REPORT] = {
        .info        = &mqtt_topic_infos[SENSOR_REPORT],
        .queue_index = SENSOR_REPORT_QUEUE_ID,
    },
    [TARGET_COMMAND] = {
        .info        = &mqtt_topic_infos[TARGET_COMMAND],
        .queue_index = TARGET_COMMAND_QUEUE_ID,
    },
    [BROADCAST_COMMAND] = {
        .info        = &mqtt_topic_infos[BROADCAST_COMMAND],
        .queue_index = BROADCAST_COMMAND_QUEUE_ID,
    },
    [RESPONSE_COMMAND] = {
        .info        = &mqtt_topic_infos[RESPONSE_COMMAND],
        .queue_index = RESPONSE_COMMAND_QUEUE_ID,
    },
    [HEALTH_REPORT] = {
        .info        = &mqtt_topic_infos[HEALTH_REPORT],
        .queue_index = HEALTH_REPORT_QUEUE_ID,
    },
};

/**
 * @brief MQTT bridge instance containing function pointers for MQTT operations.
 *
 * Function pointers are set to NULL initially and assigned during bridge initialization.
 */
mqtt_bridge_st mqtt_bridge = {
    .fetch_publish_data = NULL, /**< Function pointer to fetch publish data */
    .get_topic          = NULL, /**< Function pointer to subscribe to topics */
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

task_interface_st sensor_manager_task = {
    .name         = SENSOR_MANAGER_TASK_NAME,
    .stack_size   = SENSOR_MANAGER_TASK_STACK_SIZE,
    .priority     = SENSOR_MANAGER_TASK_PRIORITY,
    .task_execute = sensor_manager_loop,
    .arg          = NULL,
    .handle       = NULL,
};

task_interface_st command_manager_task = {
    .name         = COMMAND_MANAGER_TASK_NAME,
    .stack_size   = COMMAND_MANAGER_TASK_STACK_SIZE,
    .priority     = COMMAND_MANAGER_TASK_PRIORITY,
    .task_execute = command_manager_loop,
    .arg          = NULL,
    .handle       = NULL,
};

task_interface_st health_manager_task = {
    .name         = HEALTH_MANAGER_TASK_NAME,
    .stack_size   = HEALTH_MANAGER_TASK_STACK_SIZE,
    .priority     = HEALTH_MANAGER_TASK_PRIORITY,
    .task_execute = health_manager_loop,
    .arg          = NULL,
    .handle       = NULL,
};

task_interface_st sd_card_manager_task = {
    .name         = SD_CARD_MANAGER_TASK_NAME,
    .stack_size   = SD_CARD_MANAGER_TASK_STACK_SIZE,
    .priority     = SD_CARD_MANAGER_TASK_PRIORITY,
    .task_execute = sd_card_manager_loop,
    .arg          = NULL,
    .handle       = NULL,
};

static const char *TAG = "Application Task";  ///< Tag used for logging.

/**
 * @brief Initialize the application and attach core tasks.
 *
 * This function sets up the main application components:
 * 1. Validates the global structure.
 * 2. Initializes the network bridge and sends it to its queue.
 * 3. Initializes the MQTT bridge and sends it to its queue.
 * 4. Configures and attaches the Sensor Manager, Command Manager,
 *    and Health Manager tasks to the task manager.
 *
 * @param[in] global_structures Pointer to the global configuration structure.
 *                              Must contain valid queues for network and MQTT bridges.
 *
 * @return kernel_error_st
 *         - KERNEL_SUCCESS on success
 *         - KERNEL_ERROR_INVALID_ARG if the global structures are invalid
 *         - KERNEL_ERROR_xxx if initialization of network bridge, MQTT bridge,
 *           or any manager task fails
 *
 * @note The function must be called once during system startup before any tasks run.
 *       It assumes FreeRTOS is initialized and queues are created.
 */
kernel_error_st app_initialize(global_structures_st *global_structures) {
    logger_print(DEBUG, TAG, "Application initialization started");

    kernel_error_st err = validate_global_structure(global_structures);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Invalid global structure definitions");
        return err;
    }

    err = network_bridge_initialize(&network_bridge_init_struct);
    if (err != KERNEL_SUCCESS) {
        logger_print(INFO, TAG, "Network bridge installed failed!");
        return err;
    }
    QueueHandle_t queue = queue_manager_get(NETWORK_BRIDGE_QUEUE_ID);
    if (queue == NULL) {
        logger_print(ERR, TAG, "Network bridge queue not found");
        return KERNEL_ERROR_QUEUE_NULL;
    }
    xQueueSend(queue,
               network_bridge_init_struct.network_bridge,
               pdMS_TO_TICKS(100));

    err = mqtt_bridge_initialize(&mqtt_bridge_init_struct);
    if (err != KERNEL_SUCCESS) {
        logger_print(INFO, TAG, "MQTT bridge installed failed!");
        return err;
    }

    queue = queue_manager_get(MQTT_BRIDGE_QUEUE_ID);
    if (queue == NULL) {
        logger_print(ERR, TAG, "Network bridge queue not found");
        return KERNEL_ERROR_QUEUE_NULL;
    }
    xQueueSend(queue,
               mqtt_bridge_init_struct.mqtt_bridge,
               pdMS_TO_TICKS(100));

    err = queue_manager_register(SD_CARD_QUEUE_ID, 20, sizeof(device_report_st));
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to register SD Card queue - %d", err);
        return err;
    }

    err = task_handler_attach_task(&sensor_manager_task);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialized Sensor Manager Task - %d", err);
        return err;
    }

    err = task_handler_attach_task(&command_manager_task);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialized Command Manager Task - %d", err);
        return err;
    }

    err = task_handler_attach_task(&health_manager_task);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialized Health Manager Task - %d", err);
        return err;
    }

    err = task_handler_attach_task(&sd_card_manager_task);
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialized Health Manager Task - %d", err);
        return err;
    }

    return KERNEL_SUCCESS;
}
