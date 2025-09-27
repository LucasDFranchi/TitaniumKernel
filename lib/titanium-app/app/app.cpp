#include "app.h"
// #include "kernel/logger/logger.h"
// #include "kernel/utils/utils.h"
// #include "kernel/tasks/interface/task_interface.h"

// #include "driver/gpio.h"
// #include "driver/i2c.h"

#include "app/input_manager/sensor_manager.h"

/**
 * @brief Initialize the application and attach tasks.
 */
kernel_error_st Application::initialize(global_structures_st* global_structures) {

    sensors::SensorManager sensor_manager;
    // logger_print(DEBUG, TAG, "Application initialization started");

    // kernel_error_st err = validate_global_structure(global_structures);
    // if (err != KERNEL_SUCCESS) {
    //     logger_print(ERR, TAG, "Invalid global structure definitions");
    //     return err;
    // }

    // configure_network_bridge();
    // err = network_bridge_initialize(&network_bridge_init_);
    // if (err != KERNEL_SUCCESS) return err;
    // xQueueSend(global_structures->global_queues.network_bridge_queue,
    //            network_bridge_init_.network_bridge,
    //            pdMS_TO_TICKS(100));

    // configure_mqtt_bridge();
    // err = mqtt_bridge_initialize(&mqtt_bridge_init_);
    // if (err != KERNEL_SUCCESS) return err;
    // xQueueSend(global_structures->global_queues.mqtt_bridge_queue,
    //            mqtt_bridge_init_.mqtt_bridge,
    //            pdMS_TO_TICKS(100));

    // configure_tasks();

    // // Attach tasks
    // err = task_handler_attach_task(&sensor_manager_task_);
    // if (err != KERNEL_SUCCESS) return err;
    // err = task_handler_attach_task(&command_manager_task_);
    // if (err != KERNEL_SUCCESS) return err;
    // err = task_handler_attach_task(&health_manager_task_);
    // if (err != KERNEL_SUCCESS) return err;

    return KERNEL_SUCCESS;
}

// /**
//  * @brief Configure the network bridge initialization struct.
//  */
// void Application::configure_network_bridge() {
//     // Fill network_bridge_init_ with SPI, PHY, MAC, etc.
//     network_bridge_init_.ethernet_device = {
//         .ethernet_hardware_config = {
//             .ethernet_spi_config = {
//                 .miso          = GPIO_NUM_19,
//                 .mosi          = GPIO_NUM_23,
//                 .sclk          = GPIO_NUM_18,
//                 .cs            = GPIO_NUM_5,
//                 .spi_host      = SPI3_HOST,
//                 .spi_clock_mhz = 10,
//             },
//             .irq_gpio       = -1,
//             .phy_reset_gpio = GPIO_NUM_26,
//         },
//         .poll_period_ms = 10,
//         .rx_stack_size  = (2048 * 4),
//         .phy_addr       = 1,
//         .mac_addr       = {0},
//     };
//     network_bridge_init_.network_bridge = &network_bridge_;
// }

// /**
//  * @brief Configure MQTT bridge and topics.
//  */
// void Application::configure_mqtt_bridge() {
//     mqtt_topic_infos_[SENSOR_REPORT] = {
//         .topic               = "sensor/report",
//         .qos                 = QOS_1,
//         .mqtt_data_direction = PUBLISH,
//         .queue_length        = 10,
//         .queue_item_size     = sizeof(device_report_st),
//         .data_type           = DATA_TYPE_REPORT,
//         .message_type        = MESSAGE_TYPE_TARGET,
//     };
//     mqtt_topic_infos_[BROADCAST_COMMAND] = {
//         .topic               = "all/command",
//         .qos                 = QOS_1,
//         .mqtt_data_direction = SUBSCRIBE,
//         .queue_length        = 10,
//         .queue_item_size     = sizeof(command_st),
//         .data_type           = DATA_TYPE_COMMAND,
//         .message_type        = MESSAGE_TYPE_BROADCAST,
//     };
//     mqtt_topic_infos_[TARGET_COMMAND] = {
//         .topic               = "command",
//         .qos                 = QOS_1,
//         .mqtt_data_direction = SUBSCRIBE,
//         .queue_length        = 10,
//         .queue_item_size     = sizeof(command_st),
//         .data_type           = DATA_TYPE_COMMAND,
//         .message_type        = MESSAGE_TYPE_TARGET,
//     };
//     mqtt_topic_infos_[RESPONSE_COMMAND] = {
//         .topic               = "command",
//         .qos                 = QOS_1,
//         .mqtt_data_direction = PUBLISH,
//         .queue_length        = 10,
//         .queue_item_size     = sizeof(command_response_st),
//         .data_type           = DATA_TYPE_COMMAND_RESPONSE,
//         .message_type        = MESSAGE_TYPE_TARGET,
//     };

//     for (size_t i = 0; i < TOPIC_COUNT; ++i) {
//         mqtt_topics_[i] = { .info = &mqtt_topic_infos_[i], .queue = nullptr };
//     }

//     mqtt_bridge_init_ = {
//         .mqtt_bridge = &mqtt_bridge_,
//         .topic_count = TOPIC_COUNT,
//         .topics      = mqtt_topics_,
//     };
// }

// /**
//  * @brief Configure manager tasks and their init structs.
//  */
// void Application::configure_tasks() {
//     sensor_manager_init_.sensor_manager_queue = mqtt_topics_[SENSOR_REPORT].queue;
//     sensor_manager_task_ = {
//         .name         = SENSOR_MANAGER_TASK_NAME,
//         .stack_size   = SENSOR_MANAGER_TASK_STACK_SIZE,
//         .priority     = SENSOR_MANAGER_TASK_PRIORITY,
//         .task_execute = nullptr,
//         .arg          = &sensor_manager_init_,
//         .handle       = nullptr,
//     };

//     command_manager_init_.target_command_queue    = mqtt_topics_[TARGET_COMMAND].queue;
//     command_manager_init_.broadcast_command_queue = mqtt_topics_[BROADCAST_COMMAND].queue;
//     command_manager_init_.response_command_queue  = mqtt_topics_[RESPONSE_COMMAND].queue;
//     command_manager_task_ = {
//         .name         = COMMAND_MANAGER_TASK_NAME,
//         .stack_size   = COMMAND_MANAGER_TASK_STACK_SIZE,
//         .priority     = COMMAND_MANAGER_TASK_PRIORITY,
//         .task_execute = nullptr,
//         .arg          = &command_manager_init_,
//         .handle       = nullptr,
//     };

//     health_manager_task_ = {
//         .name         = HEALTH_MANAGER_TASK_NAME,
//         .stack_size   = HEALTH_MANAGER_TASK_STACK_SIZE,
//         .priority     = HEALTH_MANAGER_TASK_PRIORITY,
//         .task_execute = nullptr,
//         .arg          = &health_manager_init_,
//         .handle       = nullptr,
//     };
// }
