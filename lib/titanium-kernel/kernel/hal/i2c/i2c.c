#include "kernel/hal/i2c/i2c.h"

#define I2C_DEFAULT_CLK_SPEED_HZ 400000
#define I2C_CMD_TIMEOUT_MS 500
#define I2C_CMD_TIMEOUT_TICKS pdMS_TO_TICKS(I2C_CMD_TIMEOUT_MS)

/**
 * @brief I2C hardware pin configuration.
 *
 * Holds GPIO pin assignments for the I2C SDA and SCL lines.
 */
typedef struct i2c_hw_config_t {
    gpio_num_t sda; /*!< GPIO number for SDA line */
    gpio_num_t scl; /*!< GPIO number for SCL line */
} i2c_hw_config_st;

/**
 * @brief I2C driver instance state.
 *
 * Represents the state and configuration of an I2C port.
 */
typedef struct i2c_instance_t {
    bool is_initialized;        /*!< Indicates if the I2C port is initialized */
    i2c_hw_config_st hw_config; /*!< Hardware configuration (SDA/SCL pins) */
    SemaphoreHandle_t mutex;
} i2c_instance_st;

/* Global I2C HAL variables */
static i2c_instance_st i2c_instance[I2C_NUM_MAX] = {
    [I2C_NUM_0] = {
        .is_initialized = false,
        .hw_config      = {
                 .sda = GPIO_NUM_21,  // Default SDA pin for I2C_NUM_0
                 .scl = GPIO_NUM_22,  // Default SCL pin for I2C_NUM_0
        },
    },
    [I2C_NUM_1] = {
        .is_initialized = false,
        .hw_config      = {
                 .sda = GPIO_NUM_25,  // Default SDA pin for I2C_NUM_1
                 .scl = GPIO_NUM_26,  // Default SCL pin for I2C_NUM_1
        },
    },
};

/**
 * @brief Write a sequence of bytes to a specific register over I2C.
 *
 * Constructs and sends an I2C write transaction to a given device and register address.
 *
 * @param port    I2C port number to use (e.g., I2C_NUM_0 or I2C_NUM_1).
 * @param dev_adr 7-bit I2C address of the target device.
 * @param w_adr   Register address within the device to write to.
 * @param w_len   Number of bytes to write from the buffer.
 * @param buff    Pointer to the buffer containing data to be written.
 *
 * @return
 *     - ESP_OK: Write successful
 *     - ESP_ERR_INVALID_ARG: Invalid argument (null buffer or zero length)
 *     - ESP_ERR_NO_MEM: Failed to allocate I2C command link
 *     - ESP_FAIL or other esp_err_t codes on I2C communication error
 */
static esp_err_t i2c_handle_write(i2c_port_t port, uint8_t dev_adr, uint8_t w_adr, uint8_t w_len, uint8_t *buff) {
    if (!buff || w_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret_err = i2c_master_start(cmd);
    ret_err |= i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    ret_err |= i2c_master_write_byte(cmd, w_adr, true);
    ret_err |= i2c_master_write(cmd, buff, w_len, true);
    ret_err |= i2c_master_stop(cmd);

    if (ret_err == ESP_OK) {
        if (xSemaphoreTake(i2c_instance[port].mutex, portMAX_DELAY)) {
            ret_err = i2c_master_cmd_begin(port, cmd, I2C_CMD_TIMEOUT_TICKS);
            xSemaphoreGive(i2c_instance[port].mutex);
        }
    }
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Read a sequence of bytes from a specific register over I2C.
 *
 * Sends the register address to the device, then reads the specified number of bytes.
 * The received data is stored in the provided buffer.
 *
 * @param port    I2C port number to use (e.g., I2C_NUM_0 or I2C_NUM_1).
 * @param dev_adr 7-bit I2C address of the target device.
 * @param r_adr   Register address within the device to read from.
 * @param r_len   Number of bytes to read into the buffer.
 * @param buff    Pointer to the buffer where read data will be stored.
 *
 * @return
 *     - ESP_OK: Read successful
 *     - ESP_ERR_INVALID_ARG: Invalid argument (null buffer or zero length)
 *     - ESP_ERR_NO_MEM: Failed to allocate I2C command link
 *     - ESP_FAIL or other esp_err_t codes on I2C communication error
 */
static esp_err_t i2c_handle_read(i2c_port_t port, uint8_t dev_adr, uint8_t r_adr, uint8_t r_len, uint8_t *buff) {
    if (!buff || r_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret_err = i2c_master_start(cmd);
    ret_err |= i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    ret_err |= i2c_master_write_byte(cmd, r_adr, true);
    ret_err |= i2c_master_start(cmd);
    ret_err |= i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_READ, true);

    if (r_len > 1) {
        ret_err |= i2c_master_read(cmd, buff, r_len - 1, I2C_MASTER_ACK);
    }
    ret_err |= i2c_master_read_byte(cmd, buff + r_len - 1, I2C_MASTER_NACK);
    ret_err |= i2c_master_stop(cmd);

    if (ret_err == ESP_OK) {
        if (xSemaphoreTake(i2c_instance[port].mutex, portMAX_DELAY)) {
            ret_err = i2c_master_cmd_begin(port, cmd, I2C_CMD_TIMEOUT_TICKS);
            xSemaphoreGive(i2c_instance[port].mutex);
        }
    }
    i2c_cmd_link_delete(cmd);

    return ret_err;
}

/**
 * @brief Initialize the specified I2C port if not already initialized.
 *
 * Configures and installs the I2C driver for the given port using the
 * hardware pin configuration stored in `i2c_instance[]`.
 *
 * This function is idempotent: it performs initialization only once.
 *
 * @param port I2C port to initialize (e.g., I2C_NUM_0 or I2C_NUM_1)
 *
 * @return
 *     - ESP_OK: Initialization successful or already initialized
 *     - ESP_ERR_INVALID_ARG: Invalid port number
 *     - ESP_FAIL or other esp_err_t codes from underlying I2C config/driver calls
 */
static esp_err_t i2c_init_once(i2c_port_t port) {
    if (i2c_instance[port].is_initialized) {
        return ESP_OK;
    }

    if (port < 0 || port >= I2C_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!i2c_instance[port].mutex) {
        i2c_instance[port].mutex = xSemaphoreCreateMutex();
        if (!i2c_instance[port].mutex)
            return ESP_ERR_NO_MEM;
    }

    i2c_config_t conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = i2c_instance[port].hw_config.sda,
        .scl_io_num       = i2c_instance[port].hw_config.scl,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_DEFAULT_CLK_SPEED_HZ};

    esp_err_t err = i2c_param_config(port, &conf);
    if (err != ESP_OK)
        return err;

    err = i2c_driver_install(port, conf.mode, 0, 0, 0);
    if (err == ESP_OK) {
        i2c_instance[port].is_initialized = true;
    }

    return err;
}

/**
 * @brief Initializes the I2C port if necessary and returns a functional interface.
 *
 * Ensures the I2C hardware is initialized for the specified port and fills in a struct
 * with function pointers for read and write operations.
 *
 * @param port      I2C port to use (e.g., I2C_NUM_0)
 * @param iface_out Pointer to the output interface structure (must not be NULL)
 *
 * @return
 *     - ESP_OK: Interface ready to use
 *     - ESP_ERR_INVALID_ARG: iface_out is NULL or port is invalid
 *     - Other esp_err_t: I2C initialization or driver install failure
 */
esp_err_t i2c_get_interface(i2c_port_t port, i2c_interface_st *iface_out) {
    if (!iface_out)
        return ESP_ERR_INVALID_ARG;

    esp_err_t err = i2c_init_once(port);
    if (err != ESP_OK)
        return err;

    iface_out->i2c_read_fn  = i2c_handle_read;
    iface_out->i2c_write_fn = i2c_handle_write;

    return ESP_OK;
}