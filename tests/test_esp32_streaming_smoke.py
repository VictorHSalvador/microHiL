#!/usr/bin/env python3
"""Unit tests for the physical streaming smoke harness."""

import importlib.util
import pathlib
import struct
import unittest

MODULE_PATH = pathlib.Path(__file__).parents[1] / "tools" / "esp32_streaming_smoke.py"
SPEC = importlib.util.spec_from_file_location("esp32_streaming_smoke", MODULE_PATH)
SMOKE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SMOKE)


class StreamingSmokeTests(unittest.TestCase):
    def test_encode_command_matches_icd(self):
        self.assertEqual(SMOKE.encode_command(SMOKE.CMD_DISABLE), b"\x59\x72\x01\x01")
        self.assertEqual(SMOKE.encode_command(SMOKE.CMD_ENABLE), b"\x59\x72\x01\x02")
        self.assertEqual(SMOKE.encode_command(SMOKE.CMD_STREAMING), b"\x59\x72\x01\x03")

    def test_encode_read_ack_little_endian(self):
        self.assertEqual(SMOKE.encode_read_ack(0x0102), b"\x59\x72\x03\x02\x01")
        self.assertEqual(SMOKE.encode_read_ack(0), b"\x59\x72\x03\x00\x00")
        self.assertEqual(SMOKE.encode_read_ack(65535), b"\x59\x72\x03\xff\xff")

    def test_parse_data_frame_unpacks_di_and_ai(self):
        seq = 42
        di = [1, 0, 1, 0]
        ai = [1.25, 2.50, 0.00, 3.30, 0.75, 1.10]
        seq_bytes = seq.to_bytes(2, "little")
        di_bytes = bytes(di)
        ai_bytes = struct.pack("<6f", *ai)
        payload = seq_bytes + di_bytes + ai_bytes
        self.assertEqual(len(payload), 30)

        result = SMOKE.parse_data_frame(payload)
        self.assertIsNotNone(result)
        self.assertEqual(result["sequence"], 42)
        self.assertEqual(result["di"], [1, 0, 1, 0])
        self.assertEqual(result["ai"], [1.25, 2.5, 0.0, 3.3, 0.75, 1.1])

    def test_extract_frames_extracts_data_and_read_ack(self):
        data_payload = bytes([0x2A, 0x00]) + bytes(28)  # seq=42
        buffer = bytearray(
            b"\x59\x72\x01\x02\x02" + b"\x59\x72\x02" + data_payload + b"\x59\x72\x03\x2A\x00"
        )
        frames = SMOKE.extract_frames(buffer)
        self.assertEqual(len(frames), 3)
        self.assertEqual(frames[0], (SMOKE.MID_CONFIG, b"\x02\x02"))
        self.assertEqual(frames[1], (SMOKE.MID_DATA, data_payload))
        self.assertEqual(frames[2], (SMOKE.MID_READ_ACK, b"\x2A\x00"))
        self.assertEqual(buffer, bytearray())


if __name__ == "__main__":
    unittest.main()
