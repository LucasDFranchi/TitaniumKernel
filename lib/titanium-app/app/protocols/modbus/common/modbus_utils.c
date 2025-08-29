/**
 * @file modbus_utils.c
 * @brief Utility functions for Modbus protocol operations, including CRC16 calculation.
 */

#include "modbus_utils.h"

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
uint16_t modbus_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    if (!buf || (len == 0))
    {
        return crc;
    }

    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--)
        {
            if ((crc & 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
                crc >>= 1;
        }
    }
    return crc;
}
