#ifndef KERNEL_H
#define KERNEL_H

#include "GlobalConfig/global_config.h"
#include "HTTPServer/http_server_task.h"
#include "MQTT/mqtt_client_task.h"
#include "Network/network_task.h"
#include "SNTP/sntp_task.h"
#include "Watchdog/watchdog_task.h"

#include "Logger/logger.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct task_info_st {
    const char *name;
    const uint32_t stack_size;
    const uint32_t priority;
    void (*task_execute)(void *arg);
} task_info_st;

#endif /* KERNEL_H */