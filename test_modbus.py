"""
Pure PySerial Modbus RTU simulator for PZEM
"""

import serial
import struct
import time

# Serial port configuration
PORT = "COM8"          # Change to your port
BAUDRATE = 115200
TIMEOUT = 0.1          # seconds

# PZEM holding registers (16-bit)
registers = {
    0x0000: 230,        # Voltage
    0x0001: 5,          # Current high
    0x0002: 0,          # Current low
    0x0003: 1150,       # Power high
    0x0004: 0,          # Power low
    0x0005: 1234,       # Energy high
    0x0006: 0,          # Energy low
    0x0007: 50,         # Frequency
    0x0008: 95,         # Power factor
}

# CRC16 calculation for Modbus RTU
def crc16(data: bytes):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if (crc & 0x0001):
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return crc

# Function to handle Modbus RTU read holding registers (function 0x03)
def handle_request(frame):
    if len(frame) < 8:
        return None

    slave_addr = frame[0]
    func_code = frame[1]
    start_addr = frame[2] << 8 | frame[3]
    qty = frame[4] << 8 | frame[5]
    recv_crc = frame[6] | (frame[7] << 8)
    calc_crc = crc16(frame[:-2])

    if recv_crc != calc_crc:
        print("CRC mismatch")
        return None

    if func_code != 0x04:
        print(f"Unsupported function: {func_code}")
        return None

    # Build response
    response = bytearray()
    response.append(slave_addr)
    response.append(func_code)
    response.append(qty * 2)  # byte count

    for i in range(qty):
        val = registers.get(start_addr + i, 0)
        response += struct.pack(">H", val)

    crc = crc16(response)
    response += struct.pack("<H", crc)
    return response

# Main loop
with serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT) as ser:
    print(f"PZEM RTU simulator running on {PORT} at {BAUDRATE} bps")
    while True:
        if ser.in_waiting >= 8:  # minimum frame length for read holding registers
            frame = ser.read(ser.in_waiting)
            
            # Print received data in hex format
            print("Received data:", " ".join(f"{b:02X}" for b in frame))
            
            resp = handle_request(frame)
            if resp:
                ser.write(resp)
                ser.flush()
                # Optionally print response
                print("Sent response:", " ".join(f"{b:02X}" for b in resp))
