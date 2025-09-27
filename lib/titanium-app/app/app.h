#pragma once

#include "kernel/error/error_num.h"
#include "app/app_extern_types.h"
#include "app/iot/mqtt_bridge.h"
#include "app/network/network_bridge.h"
#include "app/health_manager/health_manager.h"
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
    // network_bridge_init_st network_bridge_init_{};
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
    // void configure_network_bridge();
    // void configure_mqtt_bridge();
    // void configure_tasks();
};
