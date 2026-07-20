from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class MotorPidMigrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pid_h = (ROOT / "code" / "PID.h").read_text(encoding="utf-8", errors="replace")
        cls.pid_c = (ROOT / "code" / "PID.c").read_text(encoding="utf-8", errors="replace")
        cls.angle_h = (ROOT / "code" / "angle_control.h").read_text(encoding="utf-8", errors="replace")
        cls.angle_c = (ROOT / "code" / "angle_control.c").read_text(encoding="utf-8", errors="replace")
        cls.rear_h = (ROOT / "code" / "rear_motor" / "rear_motor.h").read_text(encoding="utf-8", errors="replace")

    def test_pid_initializer_accepts_integral_limit(self):
        signature = "void PID_Init(PID_TypeDef *pid, float kp, float ki, float kd, float max_output, float integral_max)"
        self.assertIn(signature, self.pid_h)
        self.assertIn(signature, self.pid_c)
        self.assertIn("pid->IntegralMax = integral_max;", self.pid_c)

    def test_front_steering_uses_reference_parameters(self):
        expected = ("500.0f", "18.0f", "27.0f", "65.0f", "1000.0f")
        names = ("ANGLE_DEFAULT_KP", "ANGLE_DEFAULT_KI", "ANGLE_DEFAULT_KD", "ANGLE_FEED_FORWARD", "ANGLE_INTEGRAL_MAX")
        for name, value in zip(names, expected):
            self.assertRegex(self.angle_h, rf"#define\s+{name}\s+{value}")
        self.assertIn("ANGLE_OUTPUT_MAX, ANGLE_INTEGRAL_MAX", self.angle_c)

    def test_rear_speed_loop_uses_reference_parameters(self):
        expected = {
            "REAR_KP": "4.0f", "REAR_KI": "0.6f", "REAR_KD": "0.12f",
            "REAR_FF_GAIN": "8.0f", "REAR_PWM_RATE_LIMIT": "600",
            "REAR_DIFF_PWM_GAIN": "600.0f", "REAR_REVERSE_PWM_MIN": "1800",
            "REAR_INTEGRAL_THRESHOLD": "500.0f", "REAR_SPEED_FILTER_ALPHA": "0.15f",
        }
        for name, value in expected.items():
            self.assertRegex(self.rear_h, rf"#define\s+{name}\s+{value}")


if __name__ == "__main__":
    unittest.main()
