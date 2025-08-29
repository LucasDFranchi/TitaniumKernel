#pragma once

#include <stdint.h>

#pragma pack(push, 1)
typedef struct read_holding_registers_request_s
{
    uint8_t slave_id;               /**< Modbus slave ID */
    uint8_t function_code;          /**< Function code (0x03 for read holding registers) */
    uint16_t starting_address;      /**< Starting register address (big-endian) */
    uint16_t qty; /**< Number of registers to read (big-endian) */
} read_holding_registers_request_st;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct read_holding_registers_header_response_s
{
    uint8_t slave_id;      /**< Modbus slave ID in response */
    uint8_t function_code; /**< Function code in response */
    uint8_t byte_count;    /**< Number of bytes in the payload */
} read_holding_registers_header_response_st;
#pragma pack(pop)