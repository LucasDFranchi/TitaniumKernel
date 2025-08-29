#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "modbus_defines.h"

/**
 * @brief Convert a 16-bit integer from host byte order to big-endian (Modbus network order)
 *
 * @param x 16-bit integer in host byte order
 * @return uint16_t in big-endian order
 */
#define MODBUS_HTONS(x) (((uint16_t)(x) >> 8) | ((uint16_t)(x) << 8))

/**
 * @brief Convert a 32-bit integer from host byte order to big-endian (Modbus network order)
 *
 * @param x 32-bit integer in host byte order
 * @return uint32_t in big-endian order
 */
#define MODBUS_HTONL(x) (((uint32_t)(x) >> 24) |               \
                         (((uint32_t)(x) & 0x00FF0000) >> 8) | \
                         (((uint32_t)(x) & 0x0000FF00) << 8) | \
                         ((uint32_t)(x) << 24))

/**
 * @brief Compute the Modbus RTU CRC16 for a data buffer.
 *
 * @param buf Pointer to the data buffer
 * @param len Length of the data buffer in bytes
 * @return 16-bit CRC value
 *
 * This function implements the standard Modbus RTU CRC16 algorithm.
 * It can be used for both requests and responses to ensure data integrity.
 */
uint16_t modbus_crc16(const uint8_t *buf, uint16_t len);

/**
 * @brief Check if the register quantity is valid
 *
 * @param qty Number of registers
 * @return true if qty is between 1 and MODBUS_MAX_REGS
 */
static inline bool is_valid_quantity(uint16_t qty)
{
    return (qty >= 1 && qty <= MODBUS_MAX_REGS);
}

/**
 * @brief Check if the byte count is valid for Modbus registers
 *
 * @param byte_count Number of bytes
 * @return true if byte_count is between 2 and MODBUS_MAX_REGS * 2
 */
static inline bool is_valid_byte_count(uint16_t byte_count)
{
    return (byte_count >= 2 && byte_count <= (MODBUS_MAX_REGS * 2));
}

/**
 * @brief Check if the slave ID is valid
 *
 * @param slave Slave ID
 * @return true if slave ID is <= MODBUS_MAX_SLAVES
 */
static inline bool is_valid_slave_id(uint8_t slave)
{
    return (slave <= MODBUS_MAX_SLAVES);
}

/**
 * @brief Check if the register address range is valid
 *
 * @param addr Starting address
 * @param qty Number of registers
 * @return true if the range does not exceed 0xFFFF
 */
static inline bool is_valid_address_range(uint16_t addr, uint16_t qty)
{
    return ((addr + qty - 1) <= 0xFFFF);
}
