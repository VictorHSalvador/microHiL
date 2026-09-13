#!/usr/bin/env python3
"""Unit tests for the physical XRCE diagnostic frame parser."""

import importlib.util
import pathlib
import unittest

MODULE_PATH = pathlib.Path(__file__).parents[1] / "tools" / "esp32_xrce_diagnostic.py"
SPEC = importlib.util.spec_from_file_location("esp32_xrce_diagnostic", MODULE_PATH)
DIAGNOSTIC = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DIAGNOSTIC)


class FrameExtractionTests(unittest.TestCase):
    def test_config_data_and_read_ack_keep_their_icd_sizes(self):
        data_payload = bytes([0x10, 0x59, 0x72]) + bytes(range(25))
        buffer = bytearray(
            b"noise" + b"\x59\x72\x01\x01\x01" + b"\x59\x72\x02\x07\x00" + data_payload + b"\x59\x72\x03\x07\x00"
        )
        self.assertEqual(DIAGNOSTIC.extract_frames(buffer), [(0x01, b"\x01\x01"), (0x02, b"\x07\x00" + data_payload), (0x03, b"\x07\x00")])
        self.assertEqual(buffer, bytearray())

    def test_application_reset_keeps_dtr_released_when_rts_is_released(self):
        transitions = []

        class FakePort:
            def __setattr__(self, name, value):
                transitions.append((name, value))

        original_sleep = DIAGNOSTIC.time.sleep
        DIAGNOSTIC.time.sleep = lambda _: None
        try:
            DIAGNOSTIC.reset_application(FakePort())
        finally:
            DIAGNOSTIC.time.sleep = original_sleep
        self.assertEqual(transitions, [("dtr", False), ("rts", True), ("rts", False)])

    def test_fragmented_xrce_waits_for_complete_bounded_payload(self):
        frame = b"\x59\x72\x04\x03\x00\x80\x01\x02"
        buffer = bytearray(frame[:-1])
        self.assertEqual(DIAGNOSTIC.extract_frames(buffer), [])
        buffer.extend(frame[-1:])
        self.assertEqual(DIAGNOSTIC.extract_frames(buffer), [(0x04, b"\x03\x00\x80\x01\x02")])

    def test_oversize_xrce_is_discarded_and_parser_recovers(self):
        buffer = bytearray(b"\x59\x72\x04\x81\x00" + b"\x59\x72\x01\x01\x01")
        self.assertEqual(DIAGNOSTIC.extract_frames(buffer), [(0x01, b"\x01\x01")])

    def test_disable_confirmation_requires_command_and_status(self):
        self.assertEqual(DIAGNOSTIC.encode_disable(), b"\x59\x72\x01\x01")
        self.assertTrue(DIAGNOSTIC.is_disable_confirmation((0x01, b"\x01\x01")))
        self.assertFalse(DIAGNOSTIC.is_disable_confirmation((0x01, b"\x01")))


if __name__ == "__main__":
    unittest.main()
