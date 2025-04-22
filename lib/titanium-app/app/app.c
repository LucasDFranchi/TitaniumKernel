#include "app.h"

#include "app/error/error_num.h"

#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/tasks/interface/task_interface.h"

mqtt_topic_st mqtt_topic = {
    .topic               = "temperature",
    .data_size           = sizeof(uint32_t),
    .qos                 = QOS_1,
    .mqtt_data_direction = PUBLISH,
    .parse_store_json    = NULL,
    .encode_json         = NULL,
};

static int app_task_initialize() {
    return 0;
}

void app_task_execute() {
    if (app_task_initialize() != 0) {
    }

    while (1) {
        // Main task loop
        printf("Main Task Running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1 second
    }
}