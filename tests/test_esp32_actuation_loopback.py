#!/usr/bin/env python3
"""Unit tests for the ESP32 temporary actuation loopback harness."""

import importlib.util
import pathlib
import struct
import sys
import unittest

TOOLS_DIRECTORY = pathlib.Path(__file__).parents[1] / "tools"
sys.path.insert(0, str(TOOLS_DIRECTORY))
MODULE_PATH = TOOLS_DIRECTORY / "esp32_actuation_loopback.py"
SPEC = importlib.util.spec_from_file_location("esp32_actuation_loopback", MODULE_PATH)
LOOPBACK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LOOPBACK)


class ActuationLoopbackTests(unittest.TestCase):
    def test_default_pwm_duty_is_a_static_high_level_for_adc_loopback(self):
        self.assertEqual(LOOPBACK.DEFAULT_PWM_DUTY, 1.0)

    def test_encode_actuation_matches_fixed_icd_layout(self):
        frame = LOOPBACK.encode_actuation(0x1234, [1, 0, 1, 0, 1], [0.25, 0.75], [1.5, 3.0])
        self.assertEqual(frame[:5], b"\x59\x72\x02\x34\x12")
        payload = frame[5:]
        self.assertEqual(len(payload), LOOPBACK.ACTUATION_PAYLOAD_SIZE)
        self.assertEqual([payload[0], payload[1], payload[10], payload[11], payload[12]], [1, 0, 1, 0, 1])
        self.assertEqual(struct.unpack("<f", payload[2:6])[0], 0.25)
        self.assertEqual(struct.unpack("<f", payload[6:10])[0], 0.75)
        self.assertEqual(struct.unpack("<f", payload[13:17])[0], 1.5)
        self.assertEqual(struct.unpack("<f", payload[17:21])[0], 3.0)

    def test_encode_actuation_rejects_invalid_values(self):
        with self.assertRaises(ValueError):
            LOOPBACK.encode_actuation(0, [2, 0, 0, 0, 0], [0.0, 0.0], [0.0, 0.0])
        with self.assertRaises(ValueError):
            LOOPBACK.encode_actuation(0, [0, 0, 0, 0, 0], [1.1, 0.0], [0.0, 0.0])
        with self.assertRaises(ValueError):
            LOOPBACK.encode_actuation(0, [0, 0, 0, 0, 0], [0.0, 0.0], [3.4, 0.0])

    def test_median_handles_odd_even_and_empty_samples(self):
        self.assertIsNone(LOOPBACK.median([]))
        self.assertEqual(LOOPBACK.median([3.0, 1.0, 2.0]), 2.0)
        self.assertEqual(LOOPBACK.median([4.0, 1.0, 3.0, 2.0]), 2.5)


if __name__ == "__main__":
    unittest.main()
