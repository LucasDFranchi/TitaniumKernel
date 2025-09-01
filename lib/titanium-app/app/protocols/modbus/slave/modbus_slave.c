/**
 * @file modbus_slave.c
 * @brief Encode Modbus Read Holding Registers responses for RTU communication.
 *
 * This module provides functions to encode a Modbus slave response frame,
 * including payload and CRC. It handles conversion of register values to
 * big-endian format and validates input parameters.
 */
#include <string.h>

#include "app/protocols/modbus/slave/modbus_slave.h"
#include "app/protocols/modbus/common/modbus_utils.h"
#include "app/protocols/modbus/common/modbus_types.h"

static uint8_t device_slave_id = 0;

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
                              uint8_t *buffer, size_t bufsize)
{
    static const uint8_t PACKET_HEADER_SIZE = sizeof(read_holding_registers_header_response_st);
    static const uint8_t PACKET_CRC_SIZE = sizeof(uint16_t);

    if ((!buffer) || (!regs))
    {
        return 0;
    }

    if (!is_valid_quantity(qty) || !is_valid_slave_id(slave_id))
    {
        return 0;
    }

    size_t frame_len = PACKET_HEADER_SIZE + (qty * sizeof(uint16_t));
    if (bufsize < frame_len)
    {
        return 0;
    }

    read_holding_registers_header_response_st resp = {0};
    resp.slave_id = slave_id;
    resp.function_code = MODBUS_READ_INPUT_REG;
    resp.byte_count = qty * sizeof(uint16_t);

    memcpy(buffer, &resp, sizeof(resp));
    uint8_t *reg_addr = buffer + sizeof(resp);

    for (int i = 0; i < qty; i++)
    {
        uint16_t reg_val = MODBUS_HTONS(regs[i]);
        memcpy(&reg_addr[i * 2], &reg_val, sizeof(reg_val));
    }

    uint16_t crc = modbus_crc16(buffer, frame_len);
    memcpy(buffer + frame_len, &crc, PACKET_CRC_SIZE);

    return frame_len + PACKET_CRC_SIZE;
}

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
int decode_read_request(uint8_t *buffer, size_t bufsize, uint8_t *slave_id, uint16_t *start_addr, uint16_t *qty)
{
    static const uint8_t PACKET_SIZE = sizeof(read_holding_registers_request_st);
    static const uint8_t PACKET_CRC_SIZE = sizeof(uint16_t);

    if (!buffer || !slave_id || !start_addr || !qty) {
        return -1;
    }

    if (bufsize < (PACKET_SIZE + PACKET_CRC_SIZE)) {
        return -2;
    }

    read_holding_registers_request_st req;
    memcpy(&req, buffer, sizeof(req));

    uint16_t qty_req = MODBUS_HTONS(req.qty);
    uint16_t start_addr_req = MODBUS_HTONS(req.starting_address);

    if (!is_valid_quantity(qty_req) || !is_valid_slave_id(req.slave_id) || !is_valid_address_range(start_addr_req, qty_req)) {
        return -3;
    }

    if ((req.slave_id != device_slave_id) && (req.slave_id != BROADCAST_SLAVE_ID)) {
        return -4;
    }

    if (req.function_code != MODBUS_READ_INPUT_REG) {
        return -5;
    }

    uint16_t crc_calc = modbus_crc16(buffer, PACKET_SIZE);
    uint16_t crc_recv = buffer[PACKET_SIZE] | (buffer[PACKET_SIZE + 1] << 8);
    if (crc_calc != crc_recv) {
        return -6;
    }

    *slave_id = req.slave_id;
    *start_addr = start_addr_req;
    *qty = qty_req;

    return 0;
}

/**
 * @brief Set the Modbus slave ID for this device.
 *
 * @param slave_id Modbus slave ID (1..247)
 * @return 0 on success, -1 if the slave ID is invalid
 *
 * This function sets the slave ID that the device will respond to.
 * It validates the input to ensure it is within the allowed range.
 */
int set_device_slave_id(uint8_t slave_id)
{
    if (!is_valid_slave_id(slave_id) || (slave_id == BROADCAST_SLAVE_ID)) {
        return -1;
    }
    device_slave_id = slave_id;
    return 0;
}