#pragma once

#include "kernel/error/error_num.h"

#include "app/hardware/drivers/tca9548a.h"

/**
 * @brief Hardware configuration for a multiplexer channel.
 */
typedef struct mux_hw_config_s {
    mux_address_et mux_address; /*!< Multiplexer I2C address */
    mux_channel_et mux_channel; /*!< Multiplexer channel (0–7) */
} mux_hw_config_st;

/**
 * @brief Select and activate a specific MUX channel.
 *
 * If the channel is different from the currently active one, it disables the current channel
 * and enables the new one. Updates internal state accordingly.
 *
 * @param[in] mux_hw_config Pointer to desired MUX channel configuration.
 * @return KERNEL_SUCCESS on success, specific error code otherwise.
 */
typedef kernel_error_st (*mux_select_fn_st)(const mux_hw_config_st *mux);

/**
 * @brief MUX controller struct holding pointers to MUX operations.
 */
typedef struct mux_controller_s {
    mux_select_fn_st select_channel; /**< Select and activate a specific MUX channel */
} mux_controller_st;

/**
 * @brief Initialize the mux controller and populate the function pointers.
 *
 * This function initializes the mux controller hardware on the first call and assigns the internal
 * `select_channel` function pointer to the provided mux_controller structure. This allows external
 * code to interact with the mux controller through this abstraction.
 *
 * @param[out] mux_controller Pointer to a mux_controller_st structure to be populated.
 *                            Must not be NULL.
 *
 * @return kernel_error_st Returns KERNEL_SUCCESS on success,
 *                         KERNEL_ERROR_NULL if mux_controller is NULL,
 *                         or an initialization error code if hardware init fails.
 *
 * @note This function performs hardware initialization only once.
 *       Subsequent calls skip initialization but still assign the function pointer.
 */
kernel_error_st mux_controller_init(mux_controller_st *mux_controller);
