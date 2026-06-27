#!/usr/bin/env python3

import argparse
import os
import sys
import termios
import time


def maxim_crc8(data):
    crc = 0
    for byte in data:
        inbyte = byte
        for _ in range(8):
            mix = (crc ^ inbyte) & 0x01
            crc >>= 1
            if mix:
                crc ^= 0x8C
            inbyte >>= 1
    return crc


def build_set_id_command(motor_id):
    command = bytearray([0xAA, 0x55, 0x53, motor_id, 0x00, 0x00, 0x00, 0x00, 0x00])
    command.append(maxim_crc8(command))
    return bytes(command)


def configure_serial(fd):
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] &= ~(termios.PARENB | termios.CSTOPB | termios.CSIZE | termios.CRTSCTS)
    attrs[2] |= termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 10
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)


def main():
    parser = argparse.ArgumentParser(
        description="Assign a DDSM115 motor ID over RS485 without a serial GUI."
    )
    parser.add_argument("port", help="Serial port, for example /dev/ttyUSB0")
    parser.add_argument("id", type=int, help="New motor ID in the range 1..255")
    parser.add_argument(
        "--repeat",
        type=int,
        default=5,
        help="How many times to send the ID command. Waveshare recommends 5.",
    )
    parser.add_argument(
        "--delay",
        type=float,
        default=0.1,
        help="Delay in seconds between repeated commands.",
    )
    args = parser.parse_args()

    if args.id < 1 or args.id > 255:
        parser.error("id must be in the range 1..255")
    if args.repeat < 1:
        parser.error("--repeat must be at least 1")

    command = build_set_id_command(args.id)
    print("Connect only one DDSM115 motor before running this command.")
    print("Sending:", " ".join(f"{byte:02X}" for byte in command))

    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY)
    try:
        configure_serial(fd)
        for index in range(args.repeat):
            os.write(fd, command)
            termios.tcdrain(fd)
            print(f"sent {index + 1}/{args.repeat}")
            time.sleep(args.delay)
    finally:
        os.close(fd)

    print("Done. Power-cycle the motor, then connect the next motor if needed.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except OSError as error:
        print(f"serial error: {error}", file=sys.stderr)
        raise SystemExit(1)
