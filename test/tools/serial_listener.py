import serial
import argparse
from datetime import datetime

def main():
    parser = argparse.ArgumentParser(description="Capture serial data and store in a file.")
    parser.add_argument("--port", "-p", help="Serial port (e.g., COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baudrate", "-b", type=int, help="Baud rate (e.g., 9600, 115200)")
    parser.add_argument("--output", "-o", default="serial_capture.txt", help="Output file name")
    args = parser.parse_args()

    try:
        with serial.Serial(port=args.port, baudrate=args.baudrate, timeout=1) as ser, \
            open(args.output, "w", encoding="utf-8") as f:
            print(f"Listening on {args.port} at {args.baudrate} baud. Saving to {args.output}...")
            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode(errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                        entry = f"[{timestamp}] {line}"
                        print(entry)
                        f.write(entry + "\n")
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except KeyboardInterrupt:
        print("\nSerial capture stopped.")

if __name__ == "__main__":
    main()
