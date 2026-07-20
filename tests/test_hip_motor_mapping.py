from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class HipMotorSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "code" / "peripheral.h").read_text(encoding="utf-8", errors="replace")
        cls.peripheral = (ROOT / "code" / "peripheral.c").read_text(encoding="utf-8", errors="replace")
        cls.rear_motor = (ROOT / "code" / "rear_motor" / "rear_motor.c").read_text(encoding="utf-8", errors="replace")

    def test_hip_pwm_pin_mapping_matches_p5_connector(self):
        self.assertRegex(self.header, r"#define\s+PWM_L1\s+\(ATOM0_CH2_P21_4\)")
        self.assertRegex(self.header, r"#define\s+PWM_L2\s+\(ATOM0_CH3_P21_5\)")
        self.assertRegex(self.header, r"#define\s+PWM_R1\s+\(ATOM0_CH0_P21_2\)")
        self.assertRegex(self.header, r"#define\s+PWM_R2\s+\(ATOM0_CH1_P21_3\)")
        self.assertNotIn("MOTOR_GPIO_L", self.header)
        self.assertNotIn("MOTOR_GPIO_R", self.header)

    def test_motor_paths_initialize_four_pwm_inputs(self):
        combined = self.peripheral + self.rear_motor
        for channel in ("PWM_L1", "PWM_L2", "PWM_R1", "PWM_R2"):
            self.assertIn(f"pwm_init({channel}, 17000, 0)", combined)
        self.assertNotIn("gpio_init(MOTOR_GPIO", combined)
        self.assertNotIn("gpio_set_level(MOTOR_GPIO", combined)

    def test_stop_path_drives_all_hip_inputs_low(self):
        body = self.rear_motor.split("void rear_motor_stop(void)", 1)[1]
        body = body.split("void rear_motor_set_full_power", 1)[0]
        for channel in ("PWM_L1", "PWM_L2", "PWM_R1", "PWM_R2"):
            self.assertIn(f"pwm_set_duty({channel}, 0);", body)


if __name__ == "__main__":
    unittest.main()
