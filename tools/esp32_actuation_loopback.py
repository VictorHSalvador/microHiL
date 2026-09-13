#!/usr/bin/env python3
"""Exercise DAQC actuation through temporary ESP32 GPIO loopbacks."""

import argparse
import pathlib
import selectors
import socket
import struct
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).parent))
import esp32_streaming_smoke as smoke

ACTUATION_PAYLOAD_SIZE = 21
DEFAULT_DAC_VOLTS = 1.65
DEFAULT_PWM_DUTY = 1.0
DEFAULT_PWM_FREQUENCY_HZ = 937


def encode_actuation(sequence, do_values, pwm_values, analog_values):
    """Encode the fixed 21-byte host-to-DAQC payload from the ICD."""
    if len(do_values) != 5 or len(pwm_values) != 2 or len(analog_values) != 2:
        raise ValueError("expected five digital, two PWM, and two analog output values")
    if any(value not in (0, 1) for value in do_values):
        raise ValueError("digital output values must be binary")
    if any(value < 0.0 or value > 1.0 for value in pwm_values):
        raise ValueError("PWM duty must be in the [0, 1] interval")
    if any(value < 0.0 or value > 3.3 for value in analog_values):
        raise ValueError("analog output voltage must be in the [0, 3.3] interval")

    payload = bytearray(ACTUATION_PAYLOAD_SIZE)
    payload[0] = do_values[0]
    payload[1] = do_values[1]
    payload[2:6] = struct.pack("<f", pwm_values[0])
    payload[6:10] = struct.pack("<f", pwm_values[1])
    payload[10] = do_values[2]
    payload[11] = do_values[3]
    payload[12] = do_values[4]
    payload[13:17] = struct.pack("<f", analog_values[0])
    payload[17:21] = struct.pack("<f", analog_values[1])
    return smoke.SYNC + bytes([smoke.MID_DATA]) + sequence.to_bytes(2, "little") + bytes(payload)


def median(values):
    """Return a middle sample without adding a numerical package to the harness."""
    if not values:
        return None
    ordered = sorted(values)
    middle = len(ordered) // 2
    if len(ordered) % 2:
        return ordered[middle]
    return (ordered[middle - 1] + ordered[middle]) / 2.0


def bridge_until(port, udp, buffer, duration_s):
    """Forward XRCE, acknowledge consumed acquisition frames, and return DATA samples."""
    selector = selectors.DefaultSelector()
    selector.register(port, selectors.EVENT_READ, "serial")
    selector.register(udp, selectors.EVENT_READ, "udp")
    samples = []
    deadline = time.monotonic() + duration_s
    try:
        while time.monotonic() < deadline:
            for key, _ in selector.select(timeout=0.02):
                if key.data == "serial":
                    buffer.extend(port.read(256))
                    for mid, payload in smoke.extract_frames(buffer):
                        if mid == smoke.MID_DATA:
                            sample = smoke.parse_data_frame(payload)
                            if sample:
                                samples.append(sample)
                                port.write(smoke.encode_read_ack(sample["sequence"]))
                                port.flush()
                        elif mid == smoke.MID_XRCE:
                            udp.send(payload[2:])
                else:
                    payload = udp.recv(129)
                    if len(payload) <= smoke.XRCE_MTU:
                        port.write(smoke.SYNC + bytes([smoke.MID_XRCE]) + len(payload).to_bytes(2, "little") + payload)
                        port.flush()
    finally:
        selector.close()
    return samples


def wait_for_setup(port, udp, buffer, setup_wait_s, pwm_frequency_hz):
    """Publish the temporary test setup while preserving the serial XRCE bridge."""
    command = (
        "source /opt/ros/humble/setup.bash && source install-ros2/setup.bash && "
        "ros2 topic pub --once /daqc_setup microhil_interfaces/msg/DaqcSetup "
        "'{command: 2, profile_id: 1, apply_configuration: 1, adc_resolution_bits: 12, "
        "adc_attenuation: [3, 3, 3, 3, 3, 3], "
        f"pwm_frequency_hz: [{pwm_frequency_hz}, {pwm_frequency_hz}], "
        "pwm_resolution_bits: [10, 10], acquisition_frequency_hz: 100}'"
    )
    subprocess.Popen(["bash", "-c", command])
    bridge_until(port, udp, buffer, setup_wait_s)


def parse_arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=152000)
    parser.add_argument("--agent-port", type=int, default=8888)
    parser.add_argument("--setup-wait", type=float, default=6.0)
    parser.add_argument("--sample-duration", type=float, default=1.0)
    parser.add_argument("--dac-volts", type=float, default=DEFAULT_DAC_VOLTS)
    parser.add_argument("--pwm-duty", type=float, default=DEFAULT_PWM_DUTY)
    parser.add_argument("--pwm-frequency", type=int, default=DEFAULT_PWM_FREQUENCY_HZ)
    parser.add_argument("--no-reset", action="store_true")
    return parser.parse_args()


def main():
    args = parse_arguments()
    try:
        import serial
    except ImportError as exc:
        raise RuntimeError("pyserial required to run physical actuation loopback test") from exc

    port = smoke.open_serial_port(serial, args.port, args.baud)
    buffer = bytearray()
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.bind(("127.0.0.1", 0))
    udp.connect(("127.0.0.1", args.agent_port))
    if not args.no_reset:
        port.dtr = False
        port.rts = True
        time.sleep(0.1)
        port.rts = False
        time.sleep(1.0)
    port.reset_input_buffer()

    try:
        print("=== Step 1: Confirm DISABLE and enter ENABLE ===")
        ok, state = smoke.send_and_wait_config(port, buffer, smoke.CMD_DISABLE)
        if not ok or state != smoke.CMD_DISABLE:
            print(f"FAILED: expected initial DISABLE, got {state}")
            return 1
        ok, state = smoke.send_and_wait_config(port, buffer, smoke.CMD_ENABLE)
        if not ok or state != smoke.CMD_ENABLE:
            print(f"FAILED: expected ENABLE, got {state}")
            return 1

        print("=== Step 2: Apply temporary loopback setup through ROS ===")
        wait_for_setup(port, udp, buffer, args.setup_wait, args.pwm_frequency)
        ok, state = smoke.send_and_wait_config(port, buffer, smoke.CMD_STREAMING)
        if not ok or state != smoke.CMD_STREAMING:
            print(f"FAILED: expected STREAMING after setup, got {state}")
            return 1

        print("=== Step 3: Acquire baseline with safe outputs ===")
        port.write(encode_actuation(0, [0, 0, 0, 0, 0], [0.0, 0.0], [0.0, 0.0]))
        port.flush()
        baseline = bridge_until(port, udp, buffer, args.sample_duration)

        print("=== Step 4: Apply GPIO16=1, GPIO25 DAC and GPIO18 PWM ===")
        command = encode_actuation(1, [1, 0, 0, 0, 0], [args.pwm_duty, 0.0], [args.dac_volts, 0.0])
        for _ in range(3):
            port.write(command)
            port.flush()
            time.sleep(0.01)
        observed = bridge_until(port, udp, buffer, args.sample_duration)

        baseline_ai0 = median([sample["ai"][0] for sample in baseline])
        baseline_ai1 = median([sample["ai"][1] for sample in baseline])
        observed_ai0 = median([sample["ai"][0] for sample in observed])
        observed_ai1 = median([sample["ai"][1] for sample in observed])
        di_high_count = sum(sample["di"][0] == 1 for sample in observed)
        print(f"Baseline samples={len(baseline)} AI32={baseline_ai0} V AI33={baseline_ai1} V")
        print(f"Observed samples={len(observed)} DI4 high={di_high_count}/{len(observed)} AI32={observed_ai0} V AI33={observed_ai1} V")

        if not baseline or not observed:
            print("FAILED: no acquisition samples were received")
            return 1
        if di_high_count == 0:
            print("FAILED: GPIO16-to-GPIO4 loopback did not report a high digital input")
            return 1
        if observed_ai0 is None or baseline_ai0 is None or observed_ai0 - baseline_ai0 < 0.5:
            print("FAILED: GPIO25-to-GPIO32 loopback did not show an analog step")
            return 1
        if observed_ai1 is None or baseline_ai1 is None or observed_ai1 - baseline_ai1 < 0.5:
            print("FAILED: GPIO18-to-GPIO33 loopback did not show a PWM high-level step")
            return 1
        print("SUCCESS: digital, DAC, and PWM high-level loopbacks changed their corresponding DAQC acquisitions")
        return 0
    finally:
        print("=== Step 5: Request DISABLE and close the test bridge ===")
        try:
            smoke.request_safe_disable(port, buffer)
        finally:
            udp.close()
            port.close()


if __name__ == "__main__":
    sys.exit(main())
