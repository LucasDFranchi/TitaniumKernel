#pragma once

#include "kernel/inter_task_communication/inter_task_communication.h"

#include "app/app_extern_types.h"

/**
 * @brief Main loop task for SD card manager.
 *
 * Continuously receives device reports from the SD card queue,
 * converts them to CSV, and writes them to the open log file.
 *
 * @param args Task argument (unused)
 */
void sd_card_manager_loop(void* args);