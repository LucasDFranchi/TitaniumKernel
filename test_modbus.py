import serial
import struct

# Configure your serial port
ser = serial.Serial(
    port="COM8",      # change to your serial port
    baudrate=115200,
    bytesize=8,
    parity="N",
    stopbits=1,
    timeout=1
)

# Compute Modbus CRC16
def modbus_crc16(data: bytes) -> bytes:
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for _ in range(8):
            if crc & 1:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return struct.pack("<H", crc)  # little-endian

# Fixed float value to reply (32-bit float)
value = 1234.56
def float_to_registers(val):
    # Convert float to 4 bytes big-endian
    b = struct.pack('>f', val)
    # Split into 2 registers (high word first)
    return [b[0] << 8 | b[1], b[2] << 8 | b[3]]

registers = float_to_registers(value)

print("Modbus RTU slave running...")
while True:
    request = ser.read(8)  # read exactly 8 bytes (address + function + start + quantity + CRC)
    if len(request) != 8:
        continue

    slave_addr = request[0]
    func_code = request[1]
    start_addr = request[2] << 8 | request[3]
    quantity = request[4] << 8 | request[5]

    # Check for the exact request you specified
    if slave_addr == 0x01 and func_code == 0x03 and start_addr == 0x0000 and quantity == 1:
        # Build response
        response = bytearray()
        response.append(slave_addr)
        response.append(func_code)
        response.append(2)  # 4 bytes for 2 registers (32-bit float)
        response += struct.pack('>H', registers[0])
        response += modbus_crc16(response)
        ser.write(response)
        print("Sent response:", response.hex(" "))
    else:
        # Optional: ignore or send exception
        pass
