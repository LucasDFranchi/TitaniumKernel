#pragma once

#include "stddef.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

kernel_error_st mqtt_serialize_data(mqtt_topic_st *topic, char *buffer, size_t buffer_size);