#include "health_manager.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "kernel/logger/logger.h"

/** @brief LED states */
typedef enum {
    LED_OFF = 0, /**< LED is off */
    LED_ON  = 1  /**< LED is on */
} led_state_t;

/** @brief Logger tag for Health Manager */
static const char* TAG = "Health Manager";

/** @brief GPIO pin for Health Manager LED */
#define HEALTH_LED_GPIO GPIO_NUM_32

/** @brief LED blink interval in milliseconds */
#define HEALTH_LED_BLINK_MS 1000

/** @brief Current state of the health LED */
static led_state_t led_state = LED_OFF;

/**
 * @brief Initialize the Health Manager hardware resources.
 *
 * Configures the GPIO for the health LED.
 *
 * @param args Unused for now, reserved for future parameters.
 * @return KERNEL_ERROR_NONE on success, or other error codes on failure.
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

    return KERNEL_ERROR_NONE;
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

    if (err != KERNEL_ERROR_NONE) {
        logger_print(ERR, TAG, "Failed to initialize the Health Manager");
        vTaskDelete(NULL);  // Stop the task if initialization fails
    }

    while (1) {
        // Toggle LED
        if (led_state == LED_OFF) {
            gpio_set_level(HEALTH_LED_GPIO, 1);
            led_state = LED_ON;
        } else {
            gpio_set_level(HEALTH_LED_GPIO, 0);
            led_state = LED_OFF;
        }

        vTaskDelay(pdMS_TO_TICKS(HEALTH_LED_BLINK_MS));
    }
}
