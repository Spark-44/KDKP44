import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class RearGlobalBrakeSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / "code" / "rear_motor" / "rear_motor.c").read_text(encoding="utf-8")
        cls.header = (ROOT / "code" / "rear_motor" / "rear_motor.h").read_text(encoding="utf-8")

    def test_public_nonblocking_brake_api_is_exposed(self):
        declarations = (
            "void rear_motor_brake_start(void);",
            "void rear_motor_brake_update(void);",
            "uint8 rear_motor_brake_active(void);",
            "uint8 rear_motor_brake_reason(void);",
            "uint32 rear_motor_brake_elapsed_ms(void);",
            "int16 rear_motor_brake_pwm(void);",
            "float rear_motor_brake_end_raw_mps(void);",
        )
        for declaration in declarations:
            self.assertIn(declaration, self.header)

    def test_brake_uses_verified_limits(self):
        expected = {
            "REAR_BRAKE_TIMEOUT_MS": "700u",
            "REAR_BRAKE_HIGH_REVERSE_GUARD_MS": "600u",
            "REAR_BRAKE_HIGH_SPEED_MPS": "3.75f",
            "REAR_BRAKE_STOP_SPEED_MPS": "0.05f",
            "REAR_BRAKE_PWM_HIGH": "2500",
            "REAR_BRAKE_PWM_MID": "1800",
            "REAR_BRAKE_PWM_LOW": "1000",
        }
        for name, value in expected.items():
            self.assertRegex(self.header, rf"(?m)^#define\s+{name}\s+{re.escape(value)}$")

    def test_brake_finishes_through_four_channel_stop(self):
        self.assertIn("static void rear_motor_brake_finish", self.source)
        self.assertIn("rear_motor_stop();", self.source)
        self.assertIn("brake_active = 0;", self.source)


if __name__ == "__main__":
    unittest.main()
