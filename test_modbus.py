"""
PZEM Modbus RTU simulator over serial.
Python 3.11, PyModbus 3.x
"""

import asyncio
from pymodbus.server.async_io import StartSerialServer
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext
from pymodbus.datastore import ModbusSequentialDataBlock
from pymodbus.transaction import ModbusRtuFramer

# Define PZEM registers (16-bit values)
VOLTAGE_REGISTERS = [230]          # 1 register
CURRENT_REGISTERS = [5, 0]         # 2 registers
POWER_REGISTERS   = [1150, 0]      # 2 registers
ENERGY_REGISTERS  = [1234, 0]      # 2 registers
FREQUENCY_REGISTERS = [50]         # 1 register
FACTOR_POWER_REGISTERS = [95]      # 1 register

# Combine all registers sequentially
registers = (
    VOLTAGE_REGISTERS +
    CURRENT_REGISTERS +
    POWER_REGISTERS +
    ENERGY_REGISTERS +
    FREQUENCY_REGISTERS +
    FACTOR_POWER_REGISTERS
)

# Create Modbus datastore
store = ModbusSlaveContext(
    hr=ModbusSequentialDataBlock(0, registers)  # holding registers
)
context = ModbusServerContext(slaves=store, single=True)

def main():
    print("Starting PZEM RTU simulator on COM3, 9600 bps...")
    StartSerialServer(
        context=context,
        framer=ModbusRtuFramer,
        port='COM8',          # change to your serial port
        baudrate=115200,
        stopbits=1,
        bytesize=8,
        parity='N',
        timeout=1
    )

main()