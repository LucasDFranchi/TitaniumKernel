#pragma once

/**
 * @file modbus_defines.h
 * @brief Modbus protocol constants for registers, slaves, and function codes.
 */

/** @brief Maximum number of registers that can be read/written in one Modbus frame */
#define MODBUS_MAX_REGS 125

/** @brief Maximum allowed Modbus slave ID (1..247) */
#define MODBUS_MAX_SLAVES 247

/** @brief Modbus function code for "Read Input Registers" */
#define MODBUS_READ_INPUT_REG 0x04

/** @brief Modbus broadcast slave ID (0) */
#define BROADCAST_SLAVE_ID 0

/**
 * @note Additional function codes can be added here as needed, for example:
 *       #define MODBUS_WRITE_SINGLE_REG 0x06
 *       #define MODBUS_WRITE_MULTIPLE_REGS 0x10
 */
