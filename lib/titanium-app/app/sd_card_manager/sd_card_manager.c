/**
 * @file sd_card_manager.c
 * @brief Manages SD card initialization, mounting, and logging of device reports.
 *
 * This module handles SPI initialization for SD card, mounts FAT filesystem,
 * opens a log file, converts device reports to CSV, and writes them safely.
 * Designed for single-threaded logging of sensor data.
 */
#include "sd_card_manager.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "kernel/inter_task_communication/inter_task_communication.h"
#include "kernel/logger/logger.h"

// This should be temporary, or not who knows
#define PIN_NUM_MISO GPIO_NUM_12
#define PIN_NUM_MOSI GPIO_NUM_13
#define PIN_NUM_CLK GPIO_NUM_14
#define PIN_NUM_CS GPIO_NUM_15

#define FILE_BUFFER_SIZE 512 /**< Size of the temporary buffer for a single CSV line */
#define FILEPATH_SIZE 128    /**< Maximum length of the full file path */

static const char* TAG                    = "SD Card Manager"; /**< Logger tag */
static const char* MOUNT_POINT            = "/sdcard";         /**< Mount point for SD card */
static bool is_sd_card_present            = false;             /**< Tracks SD card presence */
static char filepath[FILEPATH_SIZE]       = {0};               /**< Full path to the file on the SD card */
static char file_buffer[FILE_BUFFER_SIZE] = {0};
static FILE* file                         = NULL; /**< File pointer for open log file */

/**
 * @brief Write the contents of the CSV buffer to the open log file.
 *
 * Uses fputs() to safely write the buffer, and flushes the file to ensure
 * data is persisted to SD card immediately.
 *
 * @return KERNEL_SUCCESS if write succeeds, otherwise an appropriate error code
 */
static kernel_error_st write_to_file(void) {
    if (!file) {
        logger_print(ERR, TAG, "File not open for writing");
        return KERNEL_ERROR_NULL;
    }

    if (fputs(file_buffer, file) == EOF) {
        logger_print(ERR, TAG, "Failed to write to SD card!");
        return KERNEL_ERROR_FAILED_TO_WRITE_TO_FILE;
    }

    fsync(fileno(file));
    fflush(file);
    return KERNEL_SUCCESS;
}

/**
 * @brief Converts a device_report_st into a CSV string in the file_buffer.
 *
 * Format: timestamp,value1,type1,active1,value2,type2,active2,...,num_of_sensors\n
 * Handles multiple sensors and checks buffer size to prevent overflow.
 *
 * @param device_report Pointer to the device report to convert
 * @return KERNEL_SUCCESS if conversion succeeds, otherwise error code
 */
static kernel_error_st device_report_to_csv(const device_report_st* device_report) {
    if (!device_report) {
        return KERNEL_ERROR_NULL;
    }

    int written   = 0;
    int remaining = sizeof(file_buffer);

    int size = snprintf(file_buffer + written, remaining, "%s,", device_report->timestamp);
    if (size < 0 || size >= remaining) {
        return KERNEL_ERROR_BUFFER_TOO_SHORT;
    }
    written += size;
    remaining -= size;

    for (uint8_t i = 0; i < device_report->num_of_sensors; i++) {
        size = snprintf(file_buffer + written,
                        remaining,
                        "%.2f,%d,%d,",
                        device_report->sensors[i].value,
                        (uint8_t)device_report->sensors[i].sensor_type,
                        device_report->sensors[i].active ? 1 : 0);
        if (size < 0 || size >= remaining) {
            return KERNEL_ERROR_BUFFER_TOO_SHORT;
        }
        written += size;
        remaining -= size;
    }

    size = snprintf(file_buffer + written, remaining, "%d\n", device_report->num_of_sensors);
    if (size < 0 || size >= remaining) {
        return KERNEL_ERROR_BUFFER_TOO_SHORT;
    }

    return KERNEL_SUCCESS;
}

/**
 * @brief Mounts the SD card filesystem.
 *
 * Attempts to mount the SD card using FATFS. Prints card info if successful
 * and sets the SD card presence flag.
 *
 * @param host_config_input Pointer to host configuration (SPI/SDMMC)
 * @param slot_config       Pointer to SPI slot configuration
 * @param mount_config      Pointer to FAT mount configuration
 * @param out_card          Pointer to store mounted sdmmc_card_t pointer
 * @return KERNEL_SUCCESS if mount succeeds, otherwise error code
 */
static kernel_error_st mounting_sd_card(const sdmmc_host_t* host_config_input,
                                        const sdspi_device_config_t* slot_config,
                                        const esp_vfs_fat_mount_config_t* mount_config,
                                        sdmmc_card_t** out_card) {
    logger_print(INFO, TAG, "Mounting filesystem");
    esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, host_config_input, slot_config, mount_config, out_card);

    if (ret == ESP_FAIL) {
        logger_print(ERR, TAG, "Failed to mount filesystem.");
        return KERNEL_ERROR_FAILED_TO_MOUNT_SD_CARD;
    } else if (ret != ESP_OK) {
        logger_print(ERR, TAG,
                     "Failed to initialize the card (%s). "
                     "Check SD card connections and pull-up resistors.",
                     esp_err_to_name(ret));
        return KERNEL_ERROR_SD_CARD_NOT_PRESENT;
    }

    logger_print(INFO, TAG, "Filesystem mounted successfully");
    sdmmc_card_print_info(stdout, *out_card);

    is_sd_card_present = true;
    return KERNEL_SUCCESS;
}

/**
 * @brief Initializes the SD card manager and opens the log file.
 *
 * Configures SPI bus, mounts SD card, and opens the log file in append mode.
 *
 * @return KERNEL_SUCCESS if initialization succeeds, otherwise error code
 */
static kernel_error_st sd_card_manager_initialize(void) {
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files              = 5,
        .allocation_unit_size   = (16 * 1024),
    };
    sdmmc_card_t* card = NULL;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_NUM_MOSI,
        .miso_io_num     = PIN_NUM_MISO,
        .sclk_io_num     = PIN_NUM_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        logger_print(ERR, TAG, "Failed to initialize SPI bus");
        return KERNEL_FAILED_INITIALIZE_SPI_BUS;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = PIN_NUM_CS;
    slot_config.host_id               = host.slot;

    kernel_error_st kerr = mounting_sd_card(&host, &slot_config, &mount_config, &card);
    if (kerr != KERNEL_SUCCESS) {
        return kerr;
    }

    size_t filepath_size = snprintf(filepath, sizeof(filepath), "%s/venax.csv", MOUNT_POINT);
    if (filepath_size >= sizeof(filepath)) {
        return KERNEL_ERROR_BUFFER_TOO_SHORT;
    }

    file = fopen(filepath, "a");
    if (!file) {
        logger_print(ERR, TAG, "Failed to open log file");
        return KERNEL_ERROR_FAILED_TO_OPEN_FILE;
    }

    logger_print(INFO, TAG, "Log file opened successfully");
    return KERNEL_SUCCESS;
}

/**
 * @brief Main loop task for SD card manager.
 *
 * Continuously receives device reports from the SD card queue,
 * converts them to CSV, and writes them to the open log file.
 *
 * @param args Task argument (unused)
 */
void sd_card_manager_loop(void* args) {
    kernel_error_st err = sd_card_manager_initialize();
    if (err != KERNEL_SUCCESS) {
        logger_print(ERR, TAG, "Failed to initialize SD card manager! - %d", err);
        vTaskDelete(NULL);
        return;
    }

    QueueHandle_t sd_card_queue = queue_manager_get(SD_CARD_QUEUE_ID);
    if (!sd_card_queue) {
        logger_print(ERR, TAG, "SD Card report queue is NULL");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        device_report_st device_report = {0};

        if (xQueueReceive(sd_card_queue, &device_report, pdMS_TO_TICKS(100)) != pdPASS) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        err = device_report_to_csv(&device_report);
        if (err != KERNEL_SUCCESS) {
            logger_print(ERR, TAG, "Failed to convert device report to CSV - %d", err);
            continue;
        }

        err = write_to_file();
        if (err != KERNEL_SUCCESS) {
            logger_print(ERR, TAG, "Failed to write device report to SD card - %d", err);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
