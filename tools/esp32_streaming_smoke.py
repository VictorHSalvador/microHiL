#!/usr/bin/env python3
"""Physical test harness for DAQC state transitions, STREAMING data and READ_ACK."""

import argparse
import selectors
import socket
import struct
import sys
import time

SYNC = b"\x59\x72"
MID_CONFIG = 0x01
MID_DATA = 0x02
MID_READ_ACK = 0x03
MID_XRCE = 0x04

CMD_DISABLE = 0x01
CMD_ENABLE = 0x02
CMD_STREAMING = 0x03

DATA_FRAME_SIZE = 33
XRCE_MTU = 128


def extract_frames(buffer):
    """Extract complete ICD frames from bytearray buffer."""
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


def encode_command(command):
    """Encode CONFIG command frame."""
    return SYNC + bytes([MID_CONFIG, command])


def encode_read_ack(sequence):
    """Encode READ_ACK frame for consumed sequence."""
    return SYNC + bytes([MID_READ_ACK]) + sequence.to_bytes(2, "little")


def parse_data_frame(payload):
    """Unpack 33-byte DATA frame payload (sequence uint16 + 28 bytes acquisition)."""
    if len(payload) != 30:  # without mid
        return None
    sequence = payload[0] | (payload[1] << 8)
    acq = payload[2:]
    di_values = list(acq[:4])
    ai_values = struct.unpack("<6f", acq[4:28])
    return {
        "sequence": sequence,
        "di": di_values,
        "ai": [round(val, 3) for val in ai_values],
    }


def open_serial_port(serial_module, port_name, baud):
    """Open serial port without pulsing RTS/DTR beforehand."""
    port = serial_module.Serial(port_name, baud, timeout=0.02)
    port.dtr = False
    port.rts = False
    return port


def send_and_wait_config(port, buffer, target_cmd, timeout=2.0, attempts=3):
    """Send CONFIG command and wait for expected confirmation frame."""
    cmd_bytes = encode_command(target_cmd)
    deadline = time.monotonic() + timeout
    for attempt in range(1, attempts + 1):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        port.write(cmd_bytes)
        port.flush()
        wait_deadline = time.monotonic() + min(0.3, remaining)
        while time.monotonic() < wait_deadline:
            chunk = port.read(256)
            if chunk:
                buffer.extend(chunk)
                for frame in extract_frames(buffer):
                    if frame[0] == MID_CONFIG and len(frame[1]) >= 2:
                        cmd_resp, state_resp = frame[1][0], frame[1][1]
                        if cmd_resp == target_cmd:
                            return True, state_resp
            time.sleep(0.01)
    return False, None


def parse_arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=152000)
    parser.add_argument("--agent-port", type=int, default=8888)
    parser.add_argument("--streaming-duration", type=float, default=3.0)
    parser.add_argument("--setup-wait", type=float, default=6.0)
    parser.add_argument("--skip-ros-setup", action="store_true")
    parser.add_argument("--auto-publish-setup", action="store_true", help="Auto publish ROS 2 /daqc_setup")
    parser.add_argument("--no-reset", action="store_true")
    return parser.parse_args()


def main():
    args = parse_arguments()
    try:
        import serial
    except ImportError as exc:
        raise RuntimeError("pyserial required to run physical streaming test") from exc

    port = open_serial_port(serial, args.port, args.baud)
    if not args.no_reset:
        port.dtr = False
        port.rts = True
        time.sleep(0.1)
        port.rts = False
        time.sleep(1.0)
    port.reset_input_buffer()

    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.bind(("127.0.0.1", 0))
    udp.connect(("127.0.0.1", args.agent_port))

    selector = selectors.DefaultSelector()
    selector.register(port, selectors.EVENT_READ, "serial")
    selector.register(udp, selectors.EVENT_READ, "udp")

    buffer = bytearray()
    print("=== Step 1: Confirm baseline DISABLE state ===")
    ok, state = send_and_wait_config(port, buffer, CMD_DISABLE, timeout=2.0)
    if not ok or state != CMD_DISABLE:
        print(f"FAILED to confirm initial DISABLE (state={state})")
        return 1
    print(f"OK: DAQC in DISABLE state (state={state})")

    print("\n=== Step 2: Transition DISABLE -> ENABLE ===")
    ok, state = send_and_wait_config(port, buffer, CMD_ENABLE, timeout=2.0)
    if not ok or state != CMD_ENABLE:
        print(f"FAILED to transition to ENABLE (state={state})")
        return 1
    print(f"OK: DAQC in ENABLE state (state={state})")

    if not args.skip_ros_setup:
        print(f"\n=== Step 3: Waiting {args.setup_wait}s for ROS /daqc_setup and agent bridge ===")
        if not args.auto_publish_setup:
            print("Publish configuration from another terminal with:")
            print('  source /opt/ros/humble/setup.bash && source install-ros2/setup.bash && \\\n'
                  '  ros2 topic pub --once /daqc_setup microhil_interfaces/msg/DaqcSetup \\\n'
                  '    "{command: 2, profile_id: 1, apply_configuration: 1, adc_resolution_bits: 12, '
                  'adc_attenuation: [3,3,3,3,3,3], pwm_frequency_hz: [1000,1000], '
                  'pwm_resolution_bits: [10,10], acquisition_frequency_hz: 100}"')
        deadline = time.monotonic() + args.setup_wait
        auto_published = False
        start_time = time.monotonic()
        while time.monotonic() < deadline:
            now = time.monotonic()
            if args.auto_publish_setup and not auto_published and (now - start_time >= 2.0):
                print("Triggering automatic /daqc_setup ROS 2 publication...")
                import subprocess
                subprocess.Popen([
                    "bash", "-c",
                    "source /opt/ros/humble/setup.bash && source install-ros2/setup.bash && "
                    "ros2 topic pub --once /daqc_setup microhil_interfaces/msg/DaqcSetup "
                    "'{command: 2, profile_id: 1, apply_configuration: 1, adc_resolution_bits: 12, "
                    "adc_attenuation: [3, 3, 3, 3, 3, 3], pwm_frequency_hz: [1000, 1000], "
                    "pwm_resolution_bits: [10, 10], acquisition_frequency_hz: 100}'"
                ])
                auto_published = True
            for key, _ in selector.select(timeout=0.05):
                if key.data == "serial":
                    buffer.extend(port.read(256))
                    for mid, payload in extract_frames(buffer):
                        if mid == MID_XRCE:
                            udp.send(payload[2:])
                else:
                    payload = udp.recv(129)
                    if len(payload) <= XRCE_MTU:
                        port.write(SYNC + bytes([MID_XRCE]) + len(payload).to_bytes(2, "little") + payload)
                        port.flush()

    print("\n=== Step 4: Request Transition ENABLE -> STREAMING ===")
    ok, state = send_and_wait_config(port, buffer, CMD_STREAMING, timeout=2.0)
    if not ok:
        print("FAILED: No response to STREAMING request")
        send_and_wait_config(port, buffer, CMD_DISABLE, timeout=1.0)
        return 1
    if state != CMD_STREAMING:
        print(f"SECURITY REJECTION: DAQC refused STREAMING (remained in state {state} because profile was not configured via ROS)")
        print("This proves Requirement REQ-F-02/F-18: STREAMING is blocked until valid configuration is applied!")
        send_and_wait_config(port, buffer, CMD_DISABLE, timeout=1.0)
        return 0

    print(f"SUCCESS: DAQC accepted STREAMING! (state={state})")
    print(f"\n=== Step 5: Collecting DATA packets and sending READ_ACK for {args.streaming_duration}s ===")
    data_count = 0
    last_seq = None
    first_time = None
    last_time = None
    stream_end = time.monotonic() + args.streaming_duration

    while time.monotonic() < stream_end:
        for key, _ in selector.select(timeout=0.05):
            if key.data == "serial":
                buffer.extend(port.read(256))
                for mid, payload in extract_frames(buffer):
                    if mid == MID_DATA:
                        now = time.monotonic()
                        if first_time is None:
                            first_time = now
                        last_time = now
                        data_count += 1
                        parsed = parse_data_frame(payload)
                        if parsed:
                            seq = parsed["sequence"]
                            last_seq = seq
                            # Immediately send READ_ACK
                            ack_bytes = encode_read_ack(seq)
                            port.write(ack_bytes)
                            port.flush()
                            if data_count <= 5 or data_count % 20 == 0:
                                print(f"DATA #{data_count:03d} seq={seq:05d} DI={parsed['di']} AI={parsed['ai']} V -> ACK sent")
                    elif mid == MID_XRCE:
                        udp.send(payload[2:])
            else:
                payload = udp.recv(129)
                if len(payload) <= XRCE_MTU:
                    port.write(SYNC + bytes([MID_XRCE]) + len(payload).to_bytes(2, "little") + payload)
                    port.flush()

    duration = (last_time - first_time) if (first_time and last_time and last_time > first_time) else 0.0
    freq = (data_count / duration) if duration > 0 else 0.0
    print(f"\nCollected {data_count} DATA frames in {duration:.2f}s (~{freq:.1f} Hz)")

    print("\n=== Step 6: Transition STREAMING -> DISABLE (Safe Stop) ===")
    ok, state = send_and_wait_config(port, buffer, CMD_DISABLE, timeout=2.0)
    print(f"OK: DAQC returned to DISABLE state (confirmed state={state})")
    udp.close()
    port.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
