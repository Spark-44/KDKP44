from pathlib import Path
import re
import unittest


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


class KmyRearInterfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.guandao_c = (ROOT / "code" / "guandao.c").read_text(encoding="utf-8")
        cls.guandao_h = (ROOT / "code" / "guandao.h").read_text(encoding="utf-8")
        cls.main = (ROOT / "user" / "cpu0_main.c").read_text(encoding="utf-8")

    def test_update_state_uses_rear_odometry_sample_queue(self):
        body = extract_c_function(self, self.guandao_c, "void update_state(")
        self.assertIn(
            "rear_motor_take_odometry_sample(&odometry_pulses, &sample_yaw)",
            body,
        )
        self.assertNotIn("Encoder_Get(ecd);", body)

    def test_guandao_preserves_existing_odometry_calibration(self):
        self.assertIn(
            "#define ONE_TICK_DISTANCE                      0.000378f",
            self.guandao_h,
        )

    def test_cpu0_debug_uses_current_rear_odometry_interface(self):
        main = strip_c_comments(self.main)
        diagnostics = "\n".join(
            (
                extract_c_function(
                    self,
                    main,
                    "static void Rear_Motor_Serial_Telemetry_Update(void)",
                ),
                extract_c_function(
                    self,
                    main,
                    "static void Record_Idle_Encoder_Diag_Update(void)",
                ),
            )
        )
        self.assertNotIn("rear_motor_get_total_encoder_pulses", diagnostics)
        self.assertNotIn("rear_motor_get_total_distance_m", diagnostics)
        self.assertRegex(
            diagnostics,
            r"rear_motor_get_odometry_total_pulses\s*\(",
        )


if __name__ == "__main__":
    unittest.main()
