from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SubjectGlobalBrakeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.main = (ROOT / "user" / "cpu0_main.c").read_text(encoding="utf-8", errors="replace")
        cls.isr = (ROOT / "user" / "isr.c").read_text(encoding="utf-8", errors="replace")

    def test_dispatcher_services_active_brake_before_mode_commands(self):
        body = self.main.split("static void Guandao_Rear_Motor_Update(void)", 1)[1]
        body = body.split("static void Serial_Debug_Write", 1)[0]
        self.assertLess(body.index("rear_motor_brake_active()"), body.index("conrtol_mode == GUANDAO"))
        self.assertIn("rear_motor_brake_update();", body)

    def test_route_target_transition_starts_brake_once(self):
        body = self.main.split("static void Guandao_Rear_Motor_Update(void)", 1)[1]
        body = body.split("static void Serial_Debug_Write", 1)[0]
        self.assertIn("static uint8 had_drive_target", body)
        self.assertIn("rear_motor_brake_start();", body)

    def test_emergency_key_retains_immediate_stop(self):
        block = self.isr.split("if(Main_Key_Flag == 0)", 1)[1].split("}", 1)[0]
        self.assertIn("rear_motor_stop();", block)
        self.assertNotIn("rear_motor_brake_start", block)


if __name__ == "__main__":
    unittest.main()
