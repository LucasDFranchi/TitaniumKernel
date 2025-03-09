#ifndef KERNEL_H
#define KERNEL_H

#include "inter_task_communication/events/events_definition.h"

#include "tasks/interface/task_interface.h"
#include "tasks/system/network/network_task.h"
#include "tasks/iot/http_server/http_server_task.h"
#include "tasks/iot/mqtt/mqtt_client_task.h"
#include "tasks/system/sntp/sntp_task.h"
#include "tasks/system/watchdog/watchdog_task.h"

#include "logger/logger.h"
#include "error/error_num.h"

kernel_error_st kernel_global_events_initialize(global_events_st *global_events);
kernel_error_st kernel_initialize(log_output_et log_output, global_events_st *global_events) ;
kernel_error_st kernel_enable_network(global_events_st *global_events);
kernel_error_st kernel_enable_http_server(global_events_st *global_events);
kernel_error_st kernel_enable_mqtt(global_events_st *global_events);
kernel_error_st kernel_start_tasks(void);

#endif /* KERNEL_H */