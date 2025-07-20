#include "http_server_task.h"

#include "kernel/error/error_num.h"
#include "kernel/inter_task_communication/events/events_definition.h"
#include "kernel/inter_task_communication/queues/queues_definition.h"
#include "kernel/logger/logger.h"
#include "kernel/tasks/system/network/network_task.h"
#include "kernel/utils/utils.h"

#include "esp_ota_ops.h"
#include "esp_system.h"
#include "nvs_flash.h"

/**
 * @brief Pointer to the global configuration structure.
 *
 * This variable is used to synchronize and manage all FreeRTOS events and queues
 * across the system. It provides a centralized configuration and state management
 * for consistent and efficient event handling. Ensure proper initialization before use.
 */
static global_structures_st* _global_structures = NULL;  ///< Pointer to the global configuration structure.

static const char* TAG            = "HTTP Server Task";      ///< Logging tag for HTTPServerProcess class.
static httpd_config_t config      = HTTPD_DEFAULT_CONFIG();  ///< HTTP server configuration structure.
static httpd_handle_t http_server = NULL;                    ///< Handle for the HTTP server instance.
static bool is_server_connected   = false;

extern const uint8_t bin_data_index_html_start[] asm("_binary_index_html_start"); /**< Start of index.html binary data. */
extern const uint8_t bin_data_index_html_end[] asm("_binary_index_html_end");     /**< End of index.html binary data. */
extern const uint8_t bin_data_schema_start[] asm("_binary_schema_json_start");    /**< Start of json schema binary data. */
extern const uint8_t bin_data_schema_end[] asm("_binary_schema_json_end");        /**< End of json schema binary data. */

credentials_st cred = {0};

/**
 * @brief HTTP GET handler for serving index.html.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t get_uri_index_html(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req,
                    (const char*)(bin_data_index_html_start),
                    bin_data_index_html_end - bin_data_index_html_start);

    return ESP_OK;
}

/**
 * @brief HTTP GET handler that returns Wi-Fi connection status in JSON format.
 *
 * This function reads the firmware event group to check if the device is
 * connected to Wi-Fi (via the STA_GOT_IP event bit). It responds to
 * the client with a simple JSON object indicating the connection status.
 *
 * The expected output is:
 *   - `{"connected": true}` if connected to Wi-Fi
 *   - `{"connected": false}` otherwise
 *
 * The response is sent with the `application/json` content type.
 *
 * @param req Pointer to the HTTP request.
 * @return esp_err_t ESP_OK on success, or an appropriate error code on failure.
 */
esp_err_t status_get_handler(httpd_req_t* req) {
    EventBits_t firmware_event_bits = xEventGroupGetBits(_global_structures->global_events.firmware_event_group);

    bool is_status_green = false;
    if ((firmware_event_bits & STA_GOT_IP) == 1) {
        is_status_green = true;
    }
    const char* status_json = is_status_green
                                  ? "{\"connected\": true}"
                                  : "{\"connected\": false}";

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, status_json);
}

/**
 * @brief HTTP POST handler for processing WiFi credentials.
 *
 * @param[in] req HTTP request object.
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t post_uri_wifi_credentials(httpd_req_t* req) {
    kernel_error_st result = KERNEL_ERROR_NONE;

    char buf[128] = {0};
    int received  = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive POST data");
        return KERNEL_ERROR_RECEIVING_FORM_DATA;
    }

    if (httpd_query_key_value(buf, "ssid", cred.ssid, sizeof(cred.ssid)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid SSID");
        return KERNEL_ERROR_READING_SSID;
    }

    if (httpd_query_key_value(buf, "password", cred.password, sizeof(cred.password)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid Password");
        return KERNEL_ERROR_READING_PASSWORD;
    }

    if (strlen(cred.ssid) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID cannot be empty");
        return KERNEL_ERROR_EMPTY_SSID;
    }

    if (strlen(cred.password) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Password cannot be empty");
        return KERNEL_ERROR_EMPTY_PASSWORD;
    }

    size_t ssid_len = strlen(cred.ssid);
    if (ssid_len >= sizeof(cred.ssid)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID is too long");
        return KERNEL_ERROR_STA_SSID_TOO_LONG;
    }

    size_t password_len = strlen(cred.password);
    if (password_len >= sizeof(cred.password)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Password is too long");
        return KERNEL_ERROR_STA_PASSWORD_TOO_LONG;
    }

    logger_print(DEBUG, TAG, "SSID: %s, Password: %s", cred.ssid, cred.password);

    cred.ssid[ssid_len + 1]         = '\0';
    cred.password[password_len + 1] = '\0';

    if (xQueueSend(_global_structures->global_queues.credentials_queue, &cred, pdMS_TO_TICKS(100)) != pdPASS) {
        return KERNEL_ERROR_QUEUE_FULL;
    }

    httpd_resp_set_status(req, "303");
    httpd_resp_set_hdr(req, "Location", "/?status=ok");
    httpd_resp_send(req, NULL, 0);

    return result;
}

/**
 * @brief Handles OTA firmware upload via HTTP POST request.
 *
 * This function receives a firmware image through a POST request and writes it
 * to the next available OTA partition. It expects the firmware to be wrapped in
 * a multipart/form-data request, and it skips the HTTP header by searching for
 * the "\r\n\r\n" sequence before starting the OTA write.
 *
 * After successfully writing the firmware, it finalizes the OTA process,
 * sets the new partition as bootable, and triggers a device restart.
 *
 * @param req Pointer to the HTTP request containing the firmware image.
 * @return esp_err_t ESP_OK on success, or an appropriate error code on failure.
 *
 * @note This implementation assumes the firmware is uploaded using multipart
 *       encoding and skips the header manually. This logic is tightly coupled
 *       to the transfer format and should be refactored into smaller helper
 *       functions for improved readability and maintainability.
 */
static esp_err_t ota_post_handler(httpd_req_t* req) {
    const esp_partition_t* ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        logger_print(ERR, TAG, "No OTA partition found");
        return ESP_FAIL;
    }

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        logger_print(ERR, TAG, "OTA begin failed");
        return err;
    }

    char buf[1024];
    int remaining = req->content_len;
    bool started  = false;

    while (remaining > 0) {
        int to_read = remaining < sizeof(buf) ? remaining : sizeof(buf);
        int read    = httpd_req_recv(req, buf, to_read);
        if (read <= 0) {
            logger_print(ERR, TAG, "Failed to receive firmware data");
            esp_ota_abort(ota_handle);
            return ESP_FAIL;
        }

        if (!started) {
            char* firmware_start = strstr(buf, "\r\n\r\n");
            if (firmware_start) {
                firmware_start += 4;
                int skip_len = firmware_start - buf;
                int bin_size = read - skip_len;

                err = esp_ota_write(ota_handle, firmware_start, bin_size);
                if (err != ESP_OK) {
                    esp_ota_abort(ota_handle);
                    logger_print(ERR, TAG, "OTA write failed at start");
                    return err;
                }

                started = true;
            } else {
                continue;
            }
        } else {
            err = esp_ota_write(ota_handle, buf, read);
            if (err != ESP_OK) {
                esp_ota_abort(ota_handle);
                logger_print(ERR, TAG, "OTA write failed");
                return err;
            }
        }

        remaining -= read;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        logger_print(ERR, TAG, "OTA end failed");
        return err;
    }

    err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK) {
        logger_print(ERR, TAG, "OTA set boot failed");
        return err;
    }

    httpd_resp_sendstr(req, "Upload successful. Rebooting...");
    logger_print(INFO, TAG, "OTA complete, restarting...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

/**
 * @brief Initializes the list of HTTP request URIs and their corresponding handlers.
 */
void initialize_request_list(void) {
    static const httpd_uri_t uri_index_html = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = get_uri_index_html,
        .user_ctx = NULL,
    };

    static const httpd_uri_t uri_post_credentials = {
        .uri      = "/save",
        .method   = HTTP_POST,
        .handler  = post_uri_wifi_credentials,
        .user_ctx = NULL,
    };

    static const httpd_uri_t uri_get_status = {
        .uri      = "/status",
        .method   = HTTP_GET,
        .handler  = status_get_handler,
        .user_ctx = NULL};

    static const httpd_uri_t uri_post_ota = {
        .uri      = "/upload",
        .method   = HTTP_POST,
        .handler  = ota_post_handler,
        .user_ctx = NULL};

    esp_err_t result = httpd_register_uri_handler(http_server, &uri_index_html);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result = httpd_register_uri_handler(http_server, &uri_get_status);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result = httpd_register_uri_handler(http_server, &uri_post_credentials);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
    result = httpd_register_uri_handler(http_server, &uri_post_ota);
    ESP_ERROR_CHECK_WITHOUT_ABORT(result);
}

/**
 * @brief Starts the HTTP server and registers URI handlers.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    esp_err_t result      = httpd_start(&http_server, &config);
    if (result == ESP_OK) {
        logger_print(INFO, TAG, "HTTP server started successfully");
        initialize_request_list();
        is_server_connected = true;
    } else {
        logger_print(ERR, TAG, "Failed to start HTTP server: %s", esp_err_to_name(result));
    }
    return result;
}

/**
 * @brief Stops the HTTP server.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
void stop_http_server(void) {
    if (http_server) {
        httpd_stop(http_server);
        http_server         = NULL;
        is_server_connected = false;
        logger_print(INFO, TAG, "HTTP server stopped");
    }
}

/**
 * @brief Initializes the HTTPManager by configuring server parameters and initializing request handlers.
 *
 * @return ESP_OK on success, or an error code on failure.
 */
static esp_err_t http_server_task_initialize(void) {
    esp_err_t result = ESP_OK;

    config.send_wait_timeout = 10;
    config.recv_wait_timeout = 10;
    config.max_uri_handlers  = 20;

    return result;
}

/**
 * @brief Main execution function for the HTTP server.
 *
 * This function initializes and starts the HTTP server, enabling the ESP32 to
 * handle incoming web requests. It processes requests in a FreeRTOS task.
 *
 * @param[in] pvParameters Pointer to task parameters (TaskHandle_t).
 */
void http_server_task_execute(void* pvParameters) {
    _global_structures = (global_structures_st*)pvParameters;
    if ((http_server_task_initialize() != ESP_OK) || validate_global_structure(_global_structures)) {
        logger_print(ERR, TAG, "Failed to initialize HTTP Server task");
        vTaskDelete(NULL);
    }

    while (1) {
        EventBits_t firmware_event_bits = xEventGroupGetBits(_global_structures->global_events.firmware_event_group);

        if (is_server_connected) {
            if ((firmware_event_bits & WIFI_CONNECTED_AP) == 0) {
                logger_print(INFO, TAG, "STOP SERVER");
                stop_http_server();
            }
        } else {
            if (firmware_event_bits & WIFI_CONNECTED_AP) {
                logger_print(INFO, TAG, "START SERVER");
                start_http_server();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(HTTP_SERVER_TASK_DELAY));
    }
}
