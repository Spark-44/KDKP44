from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class KmyRearInterfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "code" / "rear_motor" / "rear_motor.h").read_text(encoding="utf-8", errors="replace")
        cls.guandao_c = (ROOT / "code" / "guandao.c").read_text(encoding="utf-8", errors="replace")
        cls.guandao_h = (ROOT / "code" / "guandao.h").read_text(encoding="utf-8", errors="replace")
        cls.isr = (ROOT / "user" / "isr.c").read_text(encoding="utf-8", errors="replace")

    def test_subject_two_compatibility_api_is_preserved(self):
        declarations = (
            "void rear_motor_set_full_power(void);",
            "void rear_motor_set_speed_limit_mps(float limit_mps);",
            "void rear_motor_clear_speed_limit(void);",
            "int32  rear_motor_get_total_encoder_pulses(void);",
            "float  rear_motor_get_total_distance_m(void);",
            "void   rear_motor_clear_odometer(void);",
        )
        for declaration in declarations:
            normalized_header = " ".join(self.header.split())
            normalized_declaration = " ".join(declaration.split())
            self.assertIn(normalized_declaration, normalized_header)

    def test_odometry_uses_yaw_tagged_samples(self):
        self.assertIn("rear_motor_encoder_update_10ms(Yaw_1);", self.isr)
        self.assertIn("rear_motor_take_odometry_sample(&odometry_pulses, &sample_yaw)", self.guandao_c)

    def test_guandao_keeps_subject_two_distance_calibration(self):
        self.assertRegex(self.guandao_h, r"#define\s+ONE_TICK_DISTANCE\s+0\.000378f")


if __name__ == "__main__":
    unittest.main()
