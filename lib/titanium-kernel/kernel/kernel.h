#ifndef KERNEL_H
#define KERNEL_H

#include "kernel/inter_task_communication/events/events_definition.h"
#include "kernel/inter_task_communication/queues/queues_definition.h"

#include "kernel/tasks/interface/task_interface.h"
#include "kernel/tasks/iot/http_server/http_server_task.h"
#include "kernel/tasks/iot/mqtt/mqtt_client_task.h"
#include "kernel/tasks/system/network/network_task.h"
#include "kernel/tasks/system/sntp/sntp_task.h"
#include "kernel/tasks/system/watchdog/watchdog_task.h"

#include "error/error_num.h"
#include "kernel/logger/logger.h"

kernel_error_st kernel_initialize(log_output_et log_output, global_structures_st *global_structures);
kernel_error_st kernel_enable_network(global_structures_st *global_structures);
kernel_error_st kernel_enable_http_server(global_structures_st *global_structures);
kernel_error_st kernel_enable_mqtt(global_structures_st *global_structures);
kernel_error_st kernel_start_tasks(void);
kernel_error_st kernel_enqueue_task(task_interface_st *task);

#endif /* KERNEL_H */