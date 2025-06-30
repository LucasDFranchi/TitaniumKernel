#pragma once

#include "stddef.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/inter_task_communication.h"

/**
 * @brief Serializes a 'device_report_st' structure into a JSON string.
 *
 * This function formats the contents of the device report into a human-readable JSON format
 * using 'snprintf'. It avoids dynamic memory allocation, making it suitable for embedded use.
 *
 * @param queue Handle to the queue from which data will be received.
 * @param out_buffer Pre-allocated buffer to write the resulting JSON string into.
 * @param buffer_size Size of the output buffer.
 * @return 'APP_ERROR_NONE' on success, or an appropriate error code if serialization fails.
 */
kernel_error_st serialize_device_report(QueueHandle_t queue, char *out_buffer, size_t buffer_size);