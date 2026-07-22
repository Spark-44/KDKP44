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


class HipMotorSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "code" / "peripheral.h").read_text(encoding="utf-8", errors="replace")
        cls.peripheral = (ROOT / "code" / "peripheral.c").read_text(encoding="utf-8", errors="replace")
        rear_motor_path = ROOT / "code" / "rear_motor" / "rear_motor.c"
        cls.rear_motor = rear_motor_path.read_text(encoding="utf-8", errors="replace") if rear_motor_path.exists() else None

    def test_hip_pwm_pin_mapping_matches_p5_connector(self):
        self.assertRegex(self.header, r"#define\s+PWM_L1\s+\(ATOM0_CH2_P21_4\)")
        self.assertRegex(self.header, r"#define\s+PWM_L2\s+\(ATOM0_CH3_P21_5\)")
        self.assertRegex(self.header, r"#define\s+PWM_R1\s+\(ATOM0_CH0_P21_2\)")
        self.assertRegex(self.header, r"#define\s+PWM_R2\s+\(ATOM0_CH1_P21_3\)")
        self.assertNotIn("MOTOR_GPIO_L", self.header)
        self.assertNotIn("MOTOR_GPIO_R", self.header)

    def test_motor_paths_use_four_pwm_inputs(self):
        init_body = extract_c_function(self, self.peripheral, "void Motor_init(void)")
        set_body = extract_c_function(
            self,
            self.peripheral,
            "void Moter_Set(int moter_l , int moter_r)",
        )
        for channel in ("PWM_L1", "PWM_L2", "PWM_R1", "PWM_R2"):
            self.assertIn(f"pwm_init({channel}, 17000, 0)", init_body)
            self.assertIn(f"pwm_set_duty({channel}, 0)", set_body)
        self.assertNotIn("gpio_init(MOTOR_GPIO", init_body)
        self.assertNotIn("gpio_set_level(MOTOR_GPIO", set_body)

    def test_stop_path_drives_all_hip_inputs_low(self):
        if self.rear_motor is None:
            self.skipTest("branch has no rear_motor module")
        stop_body = extract_c_function(
            self,
            self.rear_motor,
            "void rear_motor_stop(void)",
        )
        for channel in ("PWM_L1", "PWM_L2", "PWM_R1", "PWM_R2"):
            self.assertIn(f"pwm_set_duty({channel}, 0)", stop_body)


if __name__ == "__main__":
    unittest.main()
