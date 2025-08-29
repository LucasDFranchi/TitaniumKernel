#pragma once

#include <stdint.h>

/**
 * @brief Encode a Modbus Read Holding Registers request.
 *
 * @param slave_id Modbus slave ID (1..247)
 * @param addr Starting register address
 * @param qty Number of registers to read
 * @param buffer Output buffer to store the encoded request
 * @param bufsize Size of the output buffer
 * @return Length of the encoded request in bytes, or 0 on failure
 *
 * This function validates the input parameters and fills the buffer with the
 * Modbus RTU request frame, including CRC.
 */
uint16_t encode_read_request(uint8_t slave_id, uint16_t addr, uint16_t qty, uint8_t *buffer, size_t bufsize);

/**
 * @brief Decode a Modbus Read Holding Registers response.
 *
 * @param buffer Buffer containing the response
 * @param bufsize Size of the buffer
 * @param regs Output array to store decoded register values
 * @param regs_len Length of the output array
 * @return Number of registers decoded on success, or a negative error code:
 *         -1: Invalid input pointers
 *         -2: Slave ID mismatch
 *         -3: Function code mismatch
 *         -4: Invalid byte count
 *         -5: Buffer too small
 *         -6: Output array too small
 *         -7: CRC mismatch
 *
 * The function validates the response header, checks CRC, and converts
 * register values from big-endian to host byte order.
 */
int decode_read_response(uint8_t *buffer, size_t bufsize, uint16_t *regs, uint8_t regs_len);