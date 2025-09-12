#include "kernel/hal/i2c/i2c_handler.h"

/**
 * @brief Write bytes to a device register over I2C.
 *
 * Constructs an I2C command to write `w_len` bytes from `buff` to the device
 * at `dev_adr`, starting at register `w_adr`. Uses a mutex to ensure exclusive
 * access to the I2C bus and always cleans up the command handle.
 *
 * @param dev_adr 7-bit I2C device address.
 * @param w_adr   Register address to write to.
 * @param w_len   Number of bytes to write.
 * @param buff    Pointer to data buffer to write.
 *
 * @return Kernel error status:
 *         - KERNEL_SUCCESS: Write successful
 *         - KERNEL_ERROR_INVALID_ARG: Null buffer or zero length
 *         - KERNEL_ERROR_NO_MEM: Failed to allocate I2C command link
 *         - KERNEL_ERROR_I2C_START: Failed to send I2C START
 *         - KERNEL_ERROR_I2C_WRITE_BYTE: Failed writing byte(s)
 *         - KERNEL_ERROR_I2C_STOP: Failed sending I2C STOP
 *         - KERNEL_ERROR_I2C_EXEC: I2C command execution failed
 */
kernel_error_st I2CHandler::write(uint8_t dev_adr,
                                  uint8_t w_adr,
                                  uint8_t w_len,
                                  uint8_t *buff) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_I2C_NOT_INITALIZED;
    }
    
    if (!buff || w_len == 0) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    kernel_error_st kerr = KERNEL_SUCCESS;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return KERNEL_ERROR_NO_MEM;
    }

    esp_err_t ret = i2c_master_start(cmd);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_START;
        goto cleanup;
    }

    ret = i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    if (w_len == 1) {
        ret |= i2c_master_write_byte(cmd, buff[0], true);
    } else {
        ret |= i2c_master_write_byte(cmd, w_adr, true);
        ret |= i2c_master_write(cmd, buff, w_len, true);
    }
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_WRITE_BYTE;
        goto cleanup;
    }

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_STOP;
        goto cleanup;
    }

    if (xSemaphoreTake(this->mutex_, portMAX_DELAY)) {
        ret = i2c_master_cmd_begin(this->hw_config_.port, cmd, this->hw_config_.clock_speed);
        xSemaphoreGive(this->mutex_);
    }

    kerr = (ret == ESP_OK) ? KERNEL_SUCCESS : KERNEL_ERROR_I2C_EXEC;

cleanup:
    if (cmd) {
        i2c_cmd_link_delete(cmd);
    }
    return kerr;
}

/**
 * @brief Read bytes from a device register over I2C.
 *
 * Constructs an I2C command to read `r_len` bytes into `buff` from the device
 * at `dev_adr`, starting at register `r_adr`. Uses a mutex to ensure exclusive
 * access to the I2C bus and always cleans up the command handle.
 *
 * @param dev_adr 7-bit I2C device address.
 * @param r_adr   Register address to read from.
 * @param r_len   Number of bytes to read.
 * @param buff    Pointer to buffer where read data will be stored.
 *
 * @return Kernel error status:
 *         - KERNEL_SUCCESS: Read successful
 *         - KERNEL_ERROR_INVALID_ARG: Null buffer or zero length
 *         - KERNEL_ERROR_NO_MEM: Failed to allocate I2C command link
 *         - KERNEL_ERROR_I2C_START: Failed to send I2C START
 *         - KERNEL_ERROR_I2C_WRITE_BYTE: Failed writing byte(s)
 *         - KERNEL_ERROR_I2C_READ: Failed reading byte(s)
 *         - KERNEL_ERROR_I2C_STOP: Failed sending I2C STOP
 *         - KERNEL_ERROR_I2C_EXEC: I2C command execution failed
 */
kernel_error_st I2CHandler::read(uint8_t dev_adr,
                                 uint8_t r_adr,
                                 uint8_t r_len,
                                 uint8_t *buff) {
    if (!this->is_initialized_) {
        return KERNEL_ERROR_I2C_NOT_INITALIZED;
    }

    if (!buff || r_len == 0) {
        return KERNEL_ERROR_INVALID_ARG;
    }

    kernel_error_st kerr = KERNEL_SUCCESS;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return KERNEL_ERROR_NO_MEM;
    }

    esp_err_t ret = i2c_master_start(cmd);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_START;
        goto cleanup;
    }

    ret = i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_WRITE, true);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_WRITE_BYTE;
        goto cleanup;
    }

    ret = i2c_master_write_byte(cmd, r_adr, true);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_WRITE_BYTE;
        goto cleanup;
    }

    ret = i2c_master_start(cmd);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_START;
        goto cleanup;
    }

    ret = i2c_master_write_byte(cmd, (dev_adr << 1) | I2C_MASTER_READ, true);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_WRITE_BYTE;
        goto cleanup;
    }

    if (r_len > 1) {
        ret = i2c_master_read(cmd, buff, r_len - 1, I2C_MASTER_ACK);
        if (ret != ESP_OK) {
            kerr = KERNEL_ERROR_I2C_READ;
            goto cleanup;
        }
    }

    ret = i2c_master_read_byte(cmd, buff + r_len - 1, I2C_MASTER_NACK);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_READ;
        goto cleanup;
    }

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_STOP;
        goto cleanup;
    }

    if (xSemaphoreTake(this->mutex_, portMAX_DELAY)) {
        ret = i2c_master_cmd_begin(this->hw_config_.port, cmd, I2C_CMD_TIMEOUT_TICKS);
        xSemaphoreGive(this->mutex_);
    }

    kerr = (ret == ESP_OK) ? KERNEL_SUCCESS : KERNEL_ERROR_I2C_EXEC;

cleanup:
    if (cmd) {
        i2c_cmd_link_delete(cmd);
    }
    return kerr;
}

kernel_error_st I2CHandler::initialize() {
    if (this->is_initialized_) {
        return KERNEL_SUCCESS;
    }

    if (this->hw_config_.port < 0 || this->hw_config_.port >= I2C_NUM_MAX) {
        return KERNEL_ERROR_INVALID_I2C_PORT;
    }

    if (hw_config_.sda >= GPIO_NUM_MAX || hw_config_.scl >= GPIO_NUM_MAX) {
        return KERNEL_ERROR_INVALID_GPIO;
    }

    if (!this->mutex_) {
        this->mutex_ = xSemaphoreCreateMutex();
        if (!this->mutex_) {
            return KERNEL_ERROR_NO_MEM;
        }
    }

    i2c_config_t conf{};
    conf.mode             = I2C_MODE_MASTER;
    conf.sda_io_num       = this->hw_config_.sda;
    conf.scl_io_num       = this->hw_config_.scl;
    conf.sda_pullup_en    = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en    = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = this->hw_config_.clock_speed;

    kernel_error_st kerr = KERNEL_SUCCESS;

    esp_err_t err = i2c_param_config(this->hw_config_.port, &conf);
    if (err != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_CONFIG;
    }

    err = i2c_driver_install(this->hw_config_.port, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        kerr = KERNEL_ERROR_I2C_DRIVER_INSTALL;
    }

    if (kerr == KERNEL_SUCCESS) {
        this->is_initialized_ = true;
    } else {
        vSemaphoreDelete(this->mutex_);
        this->mutex_ = nullptr;
    }

    return kerr;
}
