#!/usr/bin/env python3
"""Bridge MID 04 between one CH340 and a local Micro-ROS Agent UDP port."""

import argparse
import selectors
import socket
import sys
import time

SYNC = b"\x59\x72"
MID_CONFIG = 0x01
MID_DATA = 0x02
MID_READ_ACK = 0x03
MID_XRCE = 0x04
DATA_FRAME_SIZE = 33
XRCE_MTU = 128


def extract_frames(buffer):
    """Return complete ICD frames and retain only an incomplete suffix."""
    frames = []
    while True:
        start = buffer.find(SYNC)
        if start < 0:
            del buffer[:-1]
            return frames
        if start:
            del buffer[:start]
        if len(buffer) < 3:
            return frames
        mid = buffer[2]
        if mid == MID_CONFIG:
            total = 5
        elif mid == MID_DATA:
            total = DATA_FRAME_SIZE
        elif mid == MID_READ_ACK:
            total = 5
        elif mid == MID_XRCE:
            if len(buffer) < 5:
                return frames
            payload_size = buffer[3] | (buffer[4] << 8)
            if payload_size > XRCE_MTU:
                del buffer[0]
                continue
            total = 5 + payload_size
        else:
            del buffer[0]
            continue
        if len(buffer) < total:
            return frames
        frames.append((mid, bytes(buffer[3:total])))
        del buffer[:total]


def reset_application(port):
    """Reset EN while keeping IO0 released; DTR is never asserted at RTS release."""
    port.dtr = False
    port.rts = True
    time.sleep(0.1)
    port.rts = False
    time.sleep(0.2)


def encode_disable():
    return SYNC + bytes([MID_CONFIG, 0x01])


def is_disable_confirmation(frame):
    return frame == (MID_CONFIG, b"\x01\x01")


def read_disable_confirmation(port, buffer, capture, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        chunk = port.read(256)
        if chunk:
            if capture:
                capture.write(chunk)
                capture.flush()
            for frame in extract_frames(buffer):
                if frame[0] == MID_CONFIG:
                    print("daqc-config", frame[1].hex(), flush=True)
                if is_disable_confirmation(frame):
                    return True
        time.sleep(0.01)
    return False


def confirm_disable(port, buffer, capture, label):
    command = encode_disable()
    port.write(command)
    port.flush()
    print(f"{label}-disable-command", command.hex(), flush=True)
    if not read_disable_confirmation(port, buffer, capture, 2.0):
        raise RuntimeError(f"{label} CONFIG DISABLE confirmation missing")
    print(f"{label}-disable-confirmed", flush=True)


def parse_arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=152000)
    parser.add_argument("--agent-port", type=int, default=8888)
    parser.add_argument("--duration", type=float, default=15.0)
    parser.add_argument("--no-reset", action="store_true")
    parser.add_argument("--capture", help="path for raw DAQC-to-host bytes")
    return parser.parse_args()


def main():
    arguments = parse_arguments()
    try:
        import serial
    except ImportError as error:
        raise RuntimeError("pyserial is required only to run the physical diagnostic") from error
    port = serial.Serial()
    port.port = arguments.port
    port.baudrate = arguments.baud
    port.timeout = 0
    port.dtr = False
    port.rts = False
    port.open()
    capture = open(arguments.capture, "wb") if arguments.capture else None
    boot_confirmed = False
    try:
        if not arguments.no_reset:
            reset_application(port)
        buffer = bytearray()
        confirm_disable(port, buffer, capture, "boot")
        boot_confirmed = True
        udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            udp.bind(("127.0.0.1", 0))
            udp.connect(("127.0.0.1", arguments.agent_port))
            selector = selectors.DefaultSelector()
            selector.register(port, selectors.EVENT_READ, "serial")
            selector.register(udp, selectors.EVENT_READ, "udp")
            deadline = time.monotonic() + arguments.duration
            print("bridge-ready", flush=True)
            while time.monotonic() < deadline:
                for key, _ in selector.select(timeout=0.2):
                    if key.data == "serial":
                        chunk = port.read(256)
                        if capture and chunk:
                            capture.write(chunk)
                            capture.flush()
                        buffer.extend(chunk)
                        for mid, payload in extract_frames(buffer):
                            if mid == MID_XRCE:
                                xrce_payload = payload[2:]
                                print("daqc-to-agent", len(xrce_payload), xrce_payload[:8].hex(), flush=True)
                                udp.send(xrce_payload)
                            else:
                                print("daqc-frame", f"mid={mid:02x}", f"raw={SYNC.hex()}{mid:02x}{payload.hex()}", flush=True)
                    else:
                        payload = udp.recv(XRCE_MTU + 1)
                        if len(payload) > XRCE_MTU:
                            print("agent-oversize", len(payload), flush=True)
                            continue
                        frame = SYNC + bytes([MID_XRCE]) + len(payload).to_bytes(2, "little") + payload
                        port.write(frame)
                        port.flush()
                        print("agent-to-daqc", len(payload), payload[:8].hex(), flush=True)
        finally:
            udp.close()
    finally:
        try:
            if boot_confirmed:
                confirm_disable(port, buffer, capture, "final")
        finally:
            if capture:
                capture.close()
            port.close()


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError) as error:
        print(f"diagnostic-error: {error}", file=sys.stderr)
        raise SystemExit(1)
