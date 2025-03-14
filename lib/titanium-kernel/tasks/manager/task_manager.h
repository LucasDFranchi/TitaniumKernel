#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "error/error_num.h"
#include "inter_task_communication/events/events_definition.h"
#include "tasks/interface/task_interface.h"

kernel_error_st task_manager_enqueue_task(task_interface_st *task);
kernel_error_st task_manager_start_queued_tasks(void);
int task_manager_get_task_count(void);

#endif /* TASK_MANAGER_H */
