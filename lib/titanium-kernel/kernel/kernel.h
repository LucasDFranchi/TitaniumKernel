#ifndef KERNEL_H
#define KERNEL_H

#include "kernel/inter_task_communication/events/events_definition.h"

#include "kernel/tasks/interface/task_interface.h"
#include "kernel/tasks/system/network/network_task.h"
#include "kernel/tasks/iot/http_server/http_server_task.h"
#include "kernel/tasks/iot/mqtt/mqtt_client_task.h"
#include "kernel/tasks/system/sntp/sntp_task.h"
#include "kernel/tasks/system/watchdog/watchdog_task.h"

#include "kernel/logger/logger.h"
#include "error/error_num.h"

kernel_error_st kernel_global_events_initialize(global_events_st *global_events);
kernel_error_st kernel_initialize(log_output_et log_output, global_events_st *global_events) ;
kernel_error_st kernel_enable_network(global_events_st *global_events);
kernel_error_st kernel_enable_http_server(global_events_st *global_events);
kernel_error_st kernel_enable_mqtt(global_events_st *global_events);
kernel_error_st kernel_start_tasks(void);
kernel_error_st kernel_enqueue_task(task_interface_st *task);

#endif /* KERNEL_H */