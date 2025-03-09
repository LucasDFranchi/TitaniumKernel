#include "kernel/kernel.h"


/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_events_st global_events = {0};

void app_main() {
    // kernel_global_events_initialize(&global_events);

    // data_info_st command_config_info = {
    //     .type      = DATA_STRUCT_COMMAND_CONFIG,
    //     .size      = sizeof(command_config_st),
    //     .direction = SUBSCRIBE,
    // };
    // ESP_ERROR_CHECK(mqtt_topic_initialize(&global_events, "tag/command/config", &command_config_info));

    // data_info_st command_write_info = {
    //     .type      = DATA_STRUCT_COMMAND_WRITE,
    //     .size      = sizeof(command_write_st),
    //     .direction = SUBSCRIBE,
    // };
    // ESP_ERROR_CHECK(mqtt_topic_initialize(&global_events, "tag/command/write", &command_write_info));

    // data_info_st response_read_info = {
    //     .type      = DATA_STRUCT_RESPONSE_READ,
    //     .size      = sizeof(response_read_st),
    //     .direction = PUBLISH,
    // };
    // ESP_ERROR_CHECK(mqtt_topic_initialize(&global_events, "tag/response/read", &response_read_info));

    // data_info_st response_write_info = {
    //     .type      = DATA_STRUCT_RESPONSE_WRITE,
    //     .size      = sizeof(response_write_st),
    //     .direction = PUBLISH,
    // };
    // ESP_ERROR_CHECK(mqtt_topic_initialize(&global_events, "tag/response/write", &response_write_info));

    // global_events.allow_external_logs = true;


    kernel_initialize(SERIAL, &global_events);
    kernel_enable_network(&global_events);

    kernel_start_tasks();
}