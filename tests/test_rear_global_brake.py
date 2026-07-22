import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def strip_c_comments(source):
    return re.sub(
        r"//[^\n]*|/\*.*?\*/",
        lambda match: "\n" * match.group(0).count("\n"),
        source,
        flags=re.DOTALL,
    )


def extract_c_function(test_case, source, signature):
    clean = strip_c_comments(source)
    search_from = 0
    while True:
        start = clean.find(signature, search_from)
        if start < 0:
            test_case.fail(f"missing C function definition: {signature}")
        brace = clean.find("{", start + len(signature))
        semicolon = clean.find(";", start + len(signature))
        if brace >= 0 and (semicolon < 0 or brace < semicolon):
            break
        search_from = start + len(signature)

    depth = 0
    for index in range(brace, len(clean)):
        if clean[index] == "{":
            depth += 1
        elif clean[index] == "}":
            depth -= 1
            if depth == 0:
                return clean[start : index + 1]
    test_case.fail(f"unterminated C function: {signature}")


class RearGlobalBrakeSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / "code" / "rear_motor" / "rear_motor.c").read_text(
            encoding="utf-8"
        )
        cls.header = (ROOT / "code" / "rear_motor" / "rear_motor.h").read_text(
            encoding="utf-8"
        )

    def test_public_nonblocking_brake_api_is_exposed(self):
        for declaration in (
            "void rear_motor_brake_start(void);",
            "void rear_motor_brake_update(void);",
            "uint8 rear_motor_brake_active(void);",
            "uint8 rear_motor_brake_reason(void);",
            "uint32 rear_motor_brake_elapsed_ms(void);",
            "int16 rear_motor_brake_pwm(void);",
            "float rear_motor_brake_end_raw_mps(void);",
        ):
            self.assertIn(declaration, self.header)

    def test_brake_uses_verified_limits_and_three_pwm_levels(self):
        expected_defines = {
            "REAR_BRAKE_TIMEOUT_MS": "700u",
            "REAR_BRAKE_HIGH_REVERSE_GUARD_MS": "600u",
            "REAR_BRAKE_HIGH_SPEED_MPS": "3.75f",
            "REAR_BRAKE_STOP_SPEED_MPS": "0.05f",
            "REAR_BRAKE_REVERSE_MPS": "0.05f",
            "REAR_BRAKE_PWM_HIGH": "2500",
            "REAR_BRAKE_PWM_MID": "1800",
            "REAR_BRAKE_PWM_LOW": "1000",
        }
        for name, value in expected_defines.items():
            self.assertRegex(
                self.header,
                rf"(?m)^#define\s+{name}\s+{re.escape(value)}$",
            )

    def test_high_speed_reverse_exit_is_guarded_and_timeout_is_final(self):
        body = extract_c_function(
            self,
            self.source,
            "void rear_motor_brake_update(void)",
        )
        self.assertIn(
            "brake_start_speed_mps >= REAR_BRAKE_HIGH_SPEED_MPS",
            body,
        )
        self.assertIn(
            "brake_elapsed_ms >= REAR_BRAKE_HIGH_REVERSE_GUARD_MS",
            body,
        )
        self.assertIn(
            "brake_elapsed_ms >= REAR_BRAKE_TIMEOUT_MS",
            body,
        )
        self.assertIn("REAR_BRAKE_REASON_TIMEOUT", body)
        self.assertIn("REAR_BRAKE_REASON_REVERSE", body)
        self.assertIn("REAR_BRAKE_REASON_LOW_SPEED", body)

    def test_brake_start_does_not_reverse_an_already_stationary_car(self):
        body = extract_c_function(
            self,
            self.source,
            "void rear_motor_brake_start(void)",
        )
        self.assertIn(
            "brake_start_speed_mps <= REAR_BRAKE_STOP_SPEED_MPS",
            body,
        )
        self.assertIn(
            "fabsf(raw_actual_mps) <= REAR_BRAKE_STOP_SPEED_MPS",
            body,
        )
        self.assertIn(
            "rear_motor_brake_finish(REAR_BRAKE_REASON_LOW_SPEED, raw_actual_mps);",
            body,
        )

    def test_brake_finishes_through_four_channel_stop(self):
        finish_body = extract_c_function(
            self,
            self.source,
            "static void rear_motor_brake_finish(",
        )
        self.assertIn("rear_motor_stop();", finish_body)
        self.assertIn("brake_end_raw_mps = raw_speed_mps;", finish_body)

        stop_body = extract_c_function(
            self,
            self.source,
            "void rear_motor_stop(void)",
        )
        for channel in ("PWM_L1", "PWM_L2", "PWM_R1", "PWM_R2"):
            self.assertIn(f"pwm_set_duty({channel}, 0);", stop_body)
        self.assertIn("brake_active = 0;", stop_body)


if __name__ == "__main__":
    unittest.main()
