import re
from pathlib import Path
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


def find_call_arguments(test_case, source, symbol, path):
    cursor = 0
    while True:
        start = source.find(symbol, cursor)
        if start < 0:
            return
        cursor = start + len(symbol)
        if start > 0 and (source[start - 1].isalnum() or source[start - 1] == "_"):
            continue
        if cursor < len(source) and (source[cursor].isalnum() or source[cursor] == "_"):
            continue

        open_paren = cursor
        while open_paren < len(source) and source[open_paren].isspace():
            open_paren += 1
        if open_paren >= len(source) or source[open_paren] != "(":
            continue

        depth = 0
        quote = None
        escaped = False
        for index in range(open_paren, len(source)):
            char = source[index]
            if quote is not None:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == quote:
                    quote = None
                continue
            if char in ('"', "'"):
                quote = char
            elif char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    after = index + 1
                    while after < len(source) and source[after].isspace():
                        after += 1
                    if after < len(source) and source[after] == ";":
                        yield source[open_paren + 1 : index]
                    cursor = index + 1
                    break
        else:
            test_case.fail(f"unterminated {symbol} call in {path}")


class RearCurrentCompatibilityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "code" / "rear_motor" / "rear_motor.h").read_text(
            encoding="utf-8"
        )
        cls.source = (ROOT / "code" / "rear_motor" / "rear_motor.c").read_text(
            encoding="utf-8"
        )
        cls.remote = (ROOT / "code" / "remote_control.c").read_text(
            encoding="utf-8"
        )
        cls.main = (ROOT / "user" / "cpu0_main.c").read_text(
            encoding="utf-8"
        )
        cls.isr = (ROOT / "user" / "isr.c").read_text(
            encoding="utf-8", errors="replace"
        )

    def test_current_rear_control_apis_remain_declared(self):
        for declaration in (
            "void rear_motor_set_full_power(void);",
            "void rear_motor_set_speed_limit_mps(float limit_mps);",
            "void rear_motor_clear_speed_limit(void);",
        ):
            self.assertIn(declaration, self.header)

    def test_full_power_uses_open_loop_hard_limit(self):
        body = extract_c_function(
            self,
            self.source,
            "void rear_motor_set_full_power(void)",
        )
        self.assertIn(
            "rear_motor_open_loop_update(REAR_PWM_HARD_LIMIT);",
            body,
        )

    def test_encoder_updates_receive_current_yaw(self):
        calls = []
        excluded_directories = {"debug", "build", "generated"}
        rear_motor_path = ROOT / "code" / "rear_motor" / "rear_motor.c"
        for path in sorted(ROOT.rglob("*.c")):
            relative = path.relative_to(ROOT)
            if any(part.lower() in excluded_directories for part in relative.parts[:-1]):
                continue
            source = strip_c_comments(
                path.read_text(encoding="utf-8", errors="replace")
            )
            if path == rear_motor_path:
                definition = extract_c_function(
                    self,
                    source,
                    "rear_motor_encoder_update_10ms(",
                )
                source = source.replace(definition, "", 1)
            for arguments in find_call_arguments(
                self,
                source,
                "rear_motor_encoder_update_10ms",
                relative.as_posix(),
            ):
                calls.append((relative.as_posix(), arguments.strip()))

        self.assertTrue(calls, "expected at least one runtime encoder update call")
        invalid = [f"{path}: ({arguments})" for path, arguments in calls if arguments != "Yaw_1"]
        self.assertFalse(
            invalid,
            "encoder update calls must pass exactly Yaw_1: " + ", ".join(invalid),
        )

    def test_remote_control_retains_speed_limit_calls(self):
        periodic = extract_c_function(
            self,
            self.remote,
            "static void remote_control_periodic_update(void)",
        )
        failsafe = extract_c_function(
            self,
            self.remote,
            "static void remote_control_apply_failsafe(const char *reason)",
        )
        self.assertIn(
            "rear_motor_set_speed_limit_mps(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS);",
            periodic,
        )
        self.assertIn("rear_motor_clear_speed_limit();", failsafe)

    def test_portion2_drive_relies_on_isr_encoder_sampling(self):
        task = extract_c_function(
            self,
            self.main,
            "static void Portion2_Drive_Mode_Task(void)",
        )
        self.assertNotIn("Portion2_Drive_Encoder_Update_10ms", task)
        self.assertNotIn("rear_motor_encoder_update_10ms", task)

        isr = extract_c_function(
            self,
            self.isr,
            "IFX_INTERRUPT(cc61_pit_ch0_isr, 0, CCU6_1_CH0_ISR_PRIORITY)",
        )
        self.assertRegex(
            isr,
            r"conrtol_mode\s*==\s*GUANDAO\s*\|\|\s*conrtol_mode\s*==\s*DAOCHE",
        )
        self.assertIn("rear_motor_encoder_update_10ms(Yaw_1);", isr)


if __name__ == "__main__":
    unittest.main()
