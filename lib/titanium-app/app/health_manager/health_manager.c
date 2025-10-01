#include "health_manager.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/manager/task_handler.h"

/** @brief LED states */
typedef enum {
    LED_OFF = 0, /**< LED is off */
    LED_ON  = 1  /**< LED is on */
} led_state_t;

#define HEALTH_LED_GPIO GPIO_NUM_32        /** GPIO pin for Health Manager LED */
#define LED_BLINK_INTERVAL_MS 1000         /** LED blink interval in milliseconds */
#define REPORT_INTERVAL_MS (5 * 60 * 1000) /** Health Report interval in milliseconds */

static const char* TAG         = "Health Manager"; /** Logger tag for Health Manager */
static led_state_t led_state   = LED_OFF;          /** Current state of the health LED */
static health_report_st report = {0};              /** Health report structure */

/**
 * @brief Update the health report task list.
 *
 * Synchronizes the report structure with the current list of tasks.
 * Ensures task names are safely copied and null-terminated.
 */
static void update_health_report_list(void) {
    if (report.num_of_tasks == task_handler_get_task_count()) {
        return;
    }

    for (size_t i = 0; i < task_handler_get_task_count(); i++) {
        size_t task_name_size = snprintf(report.task_health[i].task_name,
                                         TASK_MAXIMUM_NAME_SIZE,
                                         "%s",
                                         task_handler_get_task_name(i));

        if (task_name_size >= TASK_MAXIMUM_NAME_SIZE) {
            logger_print(WARN, TAG, "Task name truncated for task index %d", i);
            report.task_health[i].task_name[TASK_MAXIMUM_NAME_SIZE - 1] = '\0';
        }
    }
    report.num_of_tasks = task_handler_get_task_count();
}

/**
 * @brief Toggle the health LED state.
 *
 * Switches the health LED between ON and OFF.
 */
static void toggle_health_led(void) {
    if (led_state == LED_OFF) {
        gpio_set_level(HEALTH_LED_GPIO, 1);
        led_state = LED_ON;
    } else {
        gpio_set_level(HEALTH_LED_GPIO, 0);
        led_state = LED_OFF;
    }
}

/**
 * @brief Send a health report to the system queue.
 *
 * Updates stack usage for each task and enqueues the health report.
 * If the queue is not available, logs an error.
 */
static void send_health_report(void) {
    update_health_report_list();

    for (size_t i = 0; i < report.num_of_tasks; i++) {
        report.task_health[i].high_water_mark = task_handler_get_highwater(i);
    }

    QueueHandle_t queue = queue_manager_get(HEALTH_REPORT_QUEUE_ID);
    if (queue == NULL) {
        logger_print(ERR, TAG, "Health report queue not found");
        return;
    }

    xQueueSend(queue, &report, pdMS_TO_TICKS(100));
}

/**
 * @brief Initialize the Health Manager hardware resources.
 *
 * Configures the GPIO for the health LED.
 *
 * @param args Unused for now, reserved for future parameters.
 * @return KERNEL_SUCCESS on success, or other error codes on failure.
 */
kernel_error_st health_manager_initialize(void* args) {
    // TODO: Move GPIO configuration to HAL for portability
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << HEALTH_LED_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&io_conf) != ESP_OK) {
        logger_print(ERR, TAG, "Failed to configure Health LED GPIO");
        return KERNEL_ERROR_TASK_INIT;
    }

    update_health_report_list();

    return KERNEL_SUCCESS;
}

/**
 * @brief Health Manager main loop task.
 *
 * Initializes hardware and toggles the health LED every `HEALTH_LED_BLINK_MS`.
 *
 * @param args Unused for now, reserved for future parameters.
 */
void health_manager_loop(void* args) {
    kernel_error_st err = health_manager_initialize(args);

    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize the Health Manager");
        vTaskDelete(NULL);
    }

    TickType_t last_wake_time       = xTaskGetTickCount();
    const TickType_t blink_interval = pdMS_TO_TICKS(LED_BLINK_INTERVAL_MS);

    uint32_t elapsed = 0;

    while (1) {
        toggle_health_led();

        vTaskDelayUntil(&last_wake_time, blink_interval);
        elapsed += LED_BLINK_INTERVAL_MS;

        if (elapsed >= REPORT_INTERVAL_MS) {
            elapsed = 0;
            send_health_report();
        }
    }
}
