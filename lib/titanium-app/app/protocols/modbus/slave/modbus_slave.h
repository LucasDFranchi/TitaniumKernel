#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Encode a Modbus Read Holding Registers response frame.
 *
 * @param slave_id Modbus slave ID (1..247)
 * @param regs Array of register values to include in response
 * @param qty Number of registers to include (must be <= MODBUS_MAX_REGS)
 * @param buffer Output buffer to store the encoded response
 * @param bufsize Size of the output buffer
 * @return Length of the encoded response in bytes (including CRC), or 0 on failure
 *
 * This function validates the input parameters, converts registers to big-endian
 * format, and appends the CRC16 checksum at the end of the frame.
 */
uint16_t encode_read_response(uint8_t slave_id,
                              const uint16_t *regs, uint16_t qty,
                              uint8_t *buffer, size_t bufsize);

/**
 * @brief Decode a Modbus Read Holding Registers request.
 *
 * @param buffer Pointer to the incoming Modbus request frame
 * @param bufsize Size of the incoming buffer
 * @param slave_id Pointer to store the decoded slave ID
 * @param start_addr Pointer to store the decoded starting register address
 * @param qty Pointer to store the decoded quantity of registers
 * @return 0 on success, negative value on error
 *
 * This function validates the Modbus request frame, checks the CRC,
 * ensures the slave ID and quantity are valid, and outputs the decoded values.
 */
int decode_read_request(uint8_t *buffer, size_t bufsize, uint8_t *slave_id, uint16_t *start_addr, uint16_t *qty);

/**
 * @brief Set the Modbus slave ID for this device.
 *
 * @param slave_id Modbus slave ID (1..247)
 * @return 0 on success, -1 if the slave ID is invalid
 *
 * This function sets the slave ID that the device will respond to.
 * It validates the input to ensure it is within the allowed range.
 */
int set_device_slave_id(uint8_t slave_id);
