#pragma once

#include "app/app_extern_types.h"
#include "app/health_manager/health_manager.h"
#include "app/iot/mqtt_bridge.h"
#include "app/network/network_bridge.h"
#include "kernel/error/error_num.h"
#include "kernel/tasks/manager/task_handler.h"

/**
 * @brief Main application class.
 *
 * Encapsulates initialization of network bridge, MQTT bridge, and core manager tasks.
 * All previous static global structs are moved into private members.
 */
class Application {
   public:
    /**
     * @brief Construct a new Application instance.
     *
     * All members are zero-initialized by default.
     */
    Application() = default;

    static constexpr char TAG[] = "Application";  ///< Tag used for logging.

    /**
     * @brief Initialize the application and attach core tasks.
     *
     * @param global_structures Pointer to the global configuration structure containing queues.
     * @return kernel_error_st
     *         - KERNEL_SUCCESS on success
     *         - error code otherwise
     */
    kernel_error_st initialize(global_structures_st* global_structures);

   private:
    // // Bridges
    // network_bridge_st network_bridge_{};
    // network_bridge_init_st network_bridge_init_struct_ = {
    //     {
    //         // ethernet_device
    //         {
    //             // ethernet_hardware_config
    //             {
    //                 // ethernet_spi_config
    //                 GPIO_NUM_19,  // miso          - SPI MISO pin number
    //                 GPIO_NUM_23,  // mosi          - SPI MOSI pin number
    //                 GPIO_NUM_18,  // sclk          - SPI Clock pin number
    //                 GPIO_NUM_5,   // cs            - SPI Chip Select pin number
    //                 SPI3_HOST,    // spi_host      - SPI host to use (SPI3)
    //                 10            // spi_clock_mhz - SPI clock frequency in MHz
    //             },
    //             -1,          // irq_gpio       - IRQ GPIO pin (-1 if unused)
    //             GPIO_NUM_26  // phy_reset_gpio - PHY reset GPIO pin number
    //         },
    //         10,       // poll_period_ms - Polling period in milliseconds
    //         1,        // phy_addr       - PHY address on the MDIO bus
    //         {0},      // mac_addr       - MAC address placeholder (initialized to zero)
    //         2048 * 4  // rx_stack_size  - RX stack size in bytes
    //     },
    //     &network_bridge_  // network_bridge   - Pointer to the network bridge interface instance
    // };
    // mqtt_bridge_st mqtt_bridge_{};
    // mqtt_bridge_init_struct_st mqtt_bridge_init_{};

    // // Managers
    // sensor_manager_init_st sensor_manager_init_{};
    // command_manager_init_st command_manager_init_{};
    // health_manager_init_st health_manager_init_{};

    // // Tasks
    // task_interface_st sensor_manager_task_{};
    // task_interface_st command_manager_task_{};
    // task_interface_st health_manager_task_{};

    // // MQTT topics
    // static constexpr size_t TOPIC_COUNT = 4;
    // mqtt_topic_info_st mqtt_topic_infos_[TOPIC_COUNT]{};
    // mqtt_topic_st mqtt_topics_[TOPIC_COUNT]{};

    // static constexpr const char* TAG = "Application";

    // // Helpers
    void configure_network_bridge();
    // void configure_mqtt_bridge();
    // void configure_tasks();
};
