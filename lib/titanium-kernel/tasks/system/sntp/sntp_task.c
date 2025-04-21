/**
 * @file sntp_task.c
 * @brief Implementation of SNTP synchronization task for ESP32.
 *
 * This file contains the implementation of a task that synchronizes the
 * system time using the Simple Network Time Protocol (SNTP). It includes
 * functions for initializing SNTP, retrieving time from an NTP server, and
 * signaling successful synchronization via an event group. This task ensures
 * that the system clock is accurately synchronized for time-dependent operations.
 */

#include "sntp_task.h"

#include "logger/logger.h"
#include "inter_task_communication/events/events_definition.h"

#include "lwip/apps/sntp.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_events_st *global_events = NULL;

static const char *TAG          = "SNTP Task";  ///< Tag for logging
static bool is_sntp_initialized = false;        ///< Tracks if SNTP has been initialized
static bool is_sntp_synced      = false;        ///< Tracks if time synchronization is successful

/**
 * @brief Initialize SNTP and attempt to synchronize time with the server.
 *
 * This function configures the SNTP client, initializes it if not already done, and
 * retrieves the current time. If the time is successfully synchronized, an event
 * bit (TIME_SYNCED) is set in the event group.
 */
static void sntp_task_sync_time_obtain_time(void) {
    time_t now         = 0;
    struct tm timeinfo = {0};

    time(&now);
    localtime_r(&now, &timeinfo);

    if (!is_sntp_initialized) {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();

        setenv("TZ", "GMT+3", 1);
        tzset();
        is_sntp_initialized = true;
    }

    if (timeinfo.tm_year < (2020 - 1900)) {
        xEventGroupClearBits(global_events->firmware_event_group, TIME_SYNCED);
    } else {
        xEventGroupSetBits(global_events->firmware_event_group, TIME_SYNCED);
        is_sntp_synced = true;
    }
}

/**
 * @brief Task to manage SNTP time synchronization.
 *
 * This task waits for the system to connect to the Wi-Fi network and then
 * attempts to synchronize the system time with an SNTP server. The task
 * exits once synchronization is successful.
 *
 * @param pvParameters Pointer to the event group handle for synchronization.
 */
void sntp_task_execute(void *pvParameters) {
    logger_print(INFO, TAG, "Starting SNTP task execution...");

    global_events = (global_events_st *)pvParameters;
    if (global_events == NULL || global_events->firmware_event_group == NULL) {
        logger_print(ERR, TAG, "Failed to initialize SNTP task");
        vTaskDelete(NULL);
    }

    logger_print(DEBUG, TAG, "Waiting for Wi-Fi connection...");
    EventBits_t firmware_event_bits = xEventGroupWaitBits(global_events->firmware_event_group,
                                                          WIFI_CONNECTED_STA,
                                                          pdFALSE,
                                                          pdFALSE,
                                                          portMAX_DELAY);

    while (1) {
        if (firmware_event_bits & WIFI_CONNECTED_STA) {
            logger_print(DEBUG, TAG, "Trying to synchronize time...");
            sntp_task_sync_time_obtain_time();
        }

        vTaskDelay(pdMS_TO_TICKS(SNTP_TASK_DELAY));

        if (is_sntp_synced) {
            logger_print(INFO, TAG, "Time synchronization successful. Exiting SNTP task.");
            break;
        }
    }

    logger_print(INFO, TAG, "SNTP task completed. Deleting task...");
    vTaskDelete(NULL);
}