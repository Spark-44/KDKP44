import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


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


def assert_tokens_in_order(test_case, source, tokens, context):
    cursor = 0
    for token in tokens:
        position = source.find(token, cursor)
        if position < 0:
            test_case.fail(f"{context}: missing or out-of-order token: {token}")
        cursor = position + len(token)


class SubjectGlobalBrakeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.guandao = (ROOT / "code" / "guandao.c").read_text(encoding="utf-8")
        cls.main = (ROOT / "user" / "cpu0_main.c").read_text(encoding="utf-8")
        cls.isr = (ROOT / "user" / "isr.c").read_text(encoding="latin-1")
        cls.remote = (ROOT / "code" / "remote_control.c").read_text(
            encoding="utf-8"
        )

    def test_dispatcher_services_active_brake_before_control_modes(self):
        body = extract_c_function(
            self,
            self.main,
            "static void Guandao_Rear_Motor_Update(void)",
        )
        branch = re.search(
            r"\b(?:if|else\s+if)\s*\(\s*conrtol_mode\s*==\s*[A-Za-z_]\w*",
            body,
        )
        self.assertIsNotNone(branch, "dispatcher has no conrtol_mode branch")
        self.assertIn("rear_motor_brake_active()", body)
        self.assertIn("rear_motor_brake_update()", body)
        self.assertLess(body.index("rear_motor_brake_active()"), branch.start())
        self.assertLess(body.index("rear_motor_brake_update()"), branch.start())

    def test_remote_periodic_and_failsafe_paths_do_not_start_active_brake(self):
        for signature in (
            "static void remote_control_periodic_update(void)",
            "static void remote_control_apply_failsafe(const char *reason)",
            "void remote_control_stop(void)",
        ):
            body = extract_c_function(self, self.remote, signature)
            self.assertNotIn("rear_motor_brake_start", body, signature)

    def test_subject_one_stop_events_start_brake_once(self):
        portion = extract_c_function(self, self.guandao, "void portion_1(void)")
        reset = extract_c_function(self, self.guandao, "void portion_1_reset(void)")
        for latch in (
            "portion1_forward_brake_requested",
            "portion1_reverse_brake_requested",
            "portion1_park_brake_requested",
        ):
            assert_tokens_in_order(
                self,
                portion,
                (
                    f"if(!{latch})",
                    "rear_motor_brake_start();",
                    f"{latch} = 1;",
                ),
                f"portion_1 {latch}",
            )
            self.assertIn(f"{latch} = 0;", reset, f"portion_1_reset {latch}")

    def test_route_save_starts_brake_after_flash_write(self):
        body = extract_c_function(self, self.guandao, "void guandao_recode(")
        pattern = r"Flash_Store_Mode\(route_setting_choice\);\s*rear_motor_brake_start\(\);"
        self.assertEqual(
            len(re.findall(pattern, body)),
            1,
            "the current KEY1 save branch must start braking after Flash write",
        )

    def test_generic_route_endpoint_has_one_shot_brake_latch(self):
        trace = extract_c_function(
            self,
            self.guandao,
            "void guandao_trace(guandao_state * state)",
        )
        self.assertIn("guandao_debug_stop_reason == 1", trace)
        self.assertIn("guandao_debug_stop_reason == 4", trace)
        assert_tokens_in_order(
            self,
            trace,
            (
                "if(!guandao_trace_brake_requested || guandao_trace_brake_route != p)",
                "rear_motor_brake_start();",
                "guandao_trace_brake_requested = 1;",
                "guandao_trace_brake_route = p;",
                "guandao_trace_brake_requested = 0;",
                "guandao_trace_brake_route = NULL;",
            ),
            "guandao_trace one-shot brake",
        )

    def test_emergency_key_keeps_immediate_stop(self):
        body = extract_c_function(
            self,
            self.isr,
            "IFX_INTERRUPT(cc61_pit_ch1_isr, 0, CCU6_1_CH1_ISR_PRIORITY)",
        )
        self.assertIn("if(Main_Key_Flag == 0)", body)
        emergency = body[body.index("if(Main_Key_Flag == 0)") :]
        self.assertIn("rear_motor_stop();", emergency)
        self.assertNotIn("rear_motor_brake_start", emergency)


if __name__ == "__main__":
    unittest.main()
