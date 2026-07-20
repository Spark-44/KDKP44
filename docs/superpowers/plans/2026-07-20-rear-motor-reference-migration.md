# Rear Motor Reference Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate the ADS Workspace rear-motor implementation into the current project while retaining the current project's subject-two, remote-control, telemetry, and calibration behavior.

**Architecture:** The reference `rear_motor` module becomes the single owner of TIM2 rear-encoder sampling, speed windows, odometry pose samples, four-channel HIP PWM, PID, and active braking. Existing current-project call sites are adapted surgically, and current-only full-power and remote speed-limit APIs remain as compatibility entry points into the reference core.

**Tech Stack:** TASKING C99 for Infineon TC264, Seekfree peripheral APIs, Python `unittest` source-contract tests, PowerShell regression scripts, ADS generated makefiles.

---

## File Map And Worktree Rule

- Create `tests/test_hip_motor_mapping.py`: four-channel HIP mapping contract copied from the reference project.
- Create `tests/test_rear_global_brake.py`: reference rear-brake contract.
- Create `tests/test_subject_global_brake.py`: reference subject-one brake-dispatch contract.
- Create `tests/test_kmy_rear_interface.py`: odometry/call-site contract adapted to preserve current `ONE_TICK_DISTANCE`.
- Create `tests/test_rear_current_compatibility.py`: current-only API and sampling-call compatibility contract.
- Create `code/rear_motor/rear_encoder_calibration.h`: reference encoder calibration constants.
- Create `code/rear_motor/rear_odometry_buffer.h`: reference fixed-window accumulator helper.
- Create `code/rear_motor/rear_odometry_pose_buffer.h`: reference pulse/yaw sample queue.
- Modify `code/rear_motor/rear_motor.c`: reference driver core plus current-only compatibility behavior.
- Modify `code/rear_motor/rear_motor.h`: reference public API plus current-only compatibility declarations.
- Modify `code/peripheral.h`: rear-only PWM and encoder aliases; keep current servo, key, and communication pins.
- Modify `code/peripheral.c`: make legacy motor entry points use the same four PWM channels.
- Modify `user/isr.c`: pass `Yaw_1` to the fixed 10 ms sampler.
- Modify `code/guandao.c`: consume pulse/yaw odometry samples without overwriting unrelated route logic.
- Modify `user/cpu0_main.c`: retain subject-two modes, update diagnostics, service braking, and pass yaw to manual samplers.
- Modify `code/remote_control.c`: pass yaw and retain speed-governor calls.

The current worktree was already heavily modified before this migration. Do not replace whole call-site files and do not stage or commit implementation files automatically. Use `git diff` against the pre-migration state after every task so existing changes remain visible and intact.

### Task 1: Add Failing Rear-Driver Contracts

**Files:**
- Create: `tests/test_hip_motor_mapping.py`
- Create: `tests/test_rear_global_brake.py`
- Create: `tests/test_subject_global_brake.py`
- Create: `tests/test_kmy_rear_interface.py`
- Create: `tests/test_rear_current_compatibility.py`

- [ ] **Step 1: Add the two unchanged reference driver tests**

Use the exact UTF-8 contents from:

```text
D:\tools\Infineon\ADS_Workspace\kart_508-kmy\kart_508\tests\test_hip_motor_mapping.py
D:\tools\Infineon\ADS_Workspace\kart_508-kmy\kart_508\tests\test_rear_global_brake.py
D:\tools\Infineon\ADS_Workspace\kart_508-kmy\kart_508\tests\test_subject_global_brake.py
```

These tests require `PWM_L1/PWM_L2/PWM_R1/PWM_R2`, four-channel stop, and the complete nonblocking brake API.

- [ ] **Step 2: Add the adapted interface test**

Create `tests/test_kmy_rear_interface.py` with these assertions:

```python
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class KmyRearInterfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.guandao_c = (ROOT / "code" / "guandao.c").read_text(encoding="utf-8")
        cls.guandao_h = (ROOT / "code" / "guandao.h").read_text(encoding="utf-8")
        cls.main = (ROOT / "user" / "cpu0_main.c").read_text(encoding="utf-8")

    def test_odometry_integrates_pulse_yaw_samples(self):
        start = self.guandao_c.index("void update_state(")
        end = self.guandao_c.index("void portion_1_reset", start)
        body = self.guandao_c[start:end]
        self.assertIn("rear_motor_take_odometry_sample(&odometry_pulses, &sample_yaw)", body)
        self.assertNotIn("Encoder_Get(ecd);", body)

    def test_preserves_current_odometry_calibration(self):
        self.assertIn("#define ONE_TICK_DISTANCE                      0.000378f", self.guandao_h)

    def test_debug_output_uses_reference_odometry_interface(self):
        self.assertNotIn("rear_motor_get_total_encoder_pulses", self.main)
        self.assertNotIn("rear_motor_get_total_distance_m", self.main)
        self.assertIn("rear_motor_get_odometry_total_pulses", self.main)

if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 3: Add current-feature compatibility tests**

Create `tests/test_rear_current_compatibility.py`:

```python
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]

class RearCurrentCompatibilityTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "code/rear_motor/rear_motor.h").read_text(encoding="utf-8")
        cls.source = (ROOT / "code/rear_motor/rear_motor.c").read_text(encoding="utf-8")
        cls.main = (ROOT / "user/cpu0_main.c").read_text(encoding="utf-8")
        cls.isr = (ROOT / "user/isr.c").read_text(encoding="utf-8", errors="replace")
        cls.remote = (ROOT / "code/remote_control.c").read_text(encoding="utf-8")

    def test_current_only_public_apis_remain_available(self):
        for declaration in (
            "void rear_motor_set_full_power(void);",
            "void rear_motor_set_speed_limit_mps(float limit_mps);",
            "void rear_motor_clear_speed_limit(void);",
        ):
            self.assertIn(declaration, self.header)

    def test_full_power_uses_reference_open_loop_path(self):
        body = self.source.split("void rear_motor_set_full_power(void)", 1)[1].split("}", 1)[0]
        self.assertIn("rear_motor_open_loop_update(REAR_PWM_HARD_LIMIT);", body)

    def test_all_runtime_samplers_supply_yaw(self):
        combined = "\n".join((self.main, self.isr, self.remote))
        calls = re.findall(r"rear_motor_encoder_update_10ms\(([^)]*)\)", combined)
        self.assertTrue(calls)
        self.assertTrue(all(call.strip() == "Yaw_1" for call in calls))

    def test_remote_speed_limit_calls_are_preserved(self):
        self.assertIn("rear_motor_set_speed_limit_mps(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS);", self.remote)
        self.assertIn("rear_motor_clear_speed_limit();", self.remote)

if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 4: Run the new tests and verify RED**

Run:

```powershell
python -m unittest tests.test_hip_motor_mapping tests.test_rear_global_brake tests.test_subject_global_brake tests.test_kmy_rear_interface tests.test_rear_current_compatibility -v
```

Expected: FAIL because the current driver still uses two PWM-plus-direction paths, has no brake API or pose queue, and its sampler is still parameterless. Failures must be assertions about those missing behaviors, not import or encoding errors.

### Task 2: Migrate The Reference Driver Core

**Files:**
- Create: `code/rear_motor/rear_encoder_calibration.h`
- Create: `code/rear_motor/rear_odometry_buffer.h`
- Create: `code/rear_motor/rear_odometry_pose_buffer.h`
- Modify: `code/rear_motor/rear_motor.c`
- Modify: `code/rear_motor/rear_motor.h`

- [ ] **Step 1: Add the three reference support headers**

Use the exact UTF-8 contents from the corresponding files under:

```text
D:\tools\Infineon\ADS_Workspace\kart_508-kmy\kart_508\code\rear_motor\
```

Do not change queue capacity, interrupt critical sections, encoder meters-per-pulse, or merge behavior.

- [ ] **Step 2: Replace the rear core with the reference implementation**

Use the reference `rear_motor.c` and `rear_motor.h` as the baseline. Confirm the migrated declarations include:

```c
void rear_motor_encoder_update_10ms(float yaw_deg);
uint8 rear_motor_take_odometry_sample(int32 *pulses, float *yaw_deg);
void rear_motor_open_loop_update(int16 pwm);
void rear_motor_brake_start(void);
void rear_motor_brake_update(void);
```

- [ ] **Step 3: Add current-only compatibility state and APIs**

Add to `rear_motor.c` state:

```c
static float speed_limit_mps = 0.0f;
```

Reset it in `rear_motor_init()` and `rear_motor_stop()`. Add:

```c
void rear_motor_set_full_power(void)
{
    rear_motor_open_loop_update(REAR_PWM_HARD_LIMIT);
}

void rear_motor_set_speed_limit_mps(float limit_mps)
{
    speed_limit_mps = fabsf(limit_mps);
}

void rear_motor_clear_speed_limit(void)
{
    speed_limit_mps = 0.0f;
}
```

Declare all three functions in `rear_motor.h`.

- [ ] **Step 4: Apply the current remote overspeed guard to reference PID output**

Immediately before `rear_motor_set_pwm((int16)pwm_f);` in `rear_motor_pid_update_100ms()`, add:

```c
if(speed_limit_mps > 0.0f)
{
    if(target_mps > 0.0f && actual_mps >= speed_limit_mps && pwm_f > 0.0f) pwm_f = 0.0f;
    if(target_mps < 0.0f && actual_mps <= -speed_limit_mps && pwm_f < 0.0f) pwm_f = 0.0f;
}
```

- [ ] **Step 5: Run driver-only tests**

Run:

```powershell
python -m unittest tests.test_rear_global_brake tests.test_rear_current_compatibility -v
```

Expected: brake and compatibility API tests pass; yaw call-site assertions may remain red until Task 4.

### Task 3: Align Rear Hardware Definitions And Legacy Paths

**Files:**
- Modify: `code/peripheral.h`
- Modify: `code/peripheral.c`
- Test: `tests/test_hip_motor_mapping.py`

- [ ] **Step 1: Replace only the old rear motor macros**

Keep the current `SERVO_MOTOR_*`, keys, and switches unchanged. Replace `PWM_L`, `PWM_R`, and both `MOTOR_GPIO_*` macros with:

```c
#define ENCODER_LEFT        (TIM2_ENCODER)
#define ENCODER_LEFT_A      (TIM2_ENCODER_CH1_P33_7)
#define ENCODER_LEFT_B      (TIM2_ENCODER_CH2_P33_6)
#define ENCODER_QUADDEC     ENCODER_LEFT
#define ENCODER_QUADDEC_A   ENCODER_LEFT_A
#define ENCODER_QUADDEC_B   ENCODER_LEFT_B

#define PWM_L1              (ATOM0_CH2_P21_4)
#define PWM_L2              (ATOM0_CH3_P21_5)
#define PWM_R1              (ATOM0_CH0_P21_2)
#define PWM_R2              (ATOM0_CH1_P21_3)
```

- [ ] **Step 2: Align legacy initialization and signed output**

Ensure `Motor_init()` initializes all four channels. Ensure `Moter_Set()` first drives all four low, then uses `L1/R1` for positive output and `L2/R2` for negative output. It must contain no `MOTOR_GPIO_*` calls.

- [ ] **Step 3: Run HIP mapping tests**

Run:

```powershell
python -m unittest tests.test_hip_motor_mapping -v
```

Expected: PASS, including all four channels low in `rear_motor_stop()`.

### Task 4: Adapt Sampling, Odometry, Diagnostics, And Braking Call Sites

**Files:**
- Modify: `user/isr.c`
- Modify: `code/guandao.c`
- Modify: `user/cpu0_main.c`
- Modify: `code/remote_control.c`
- Test: `tests/test_kmy_rear_interface.py`
- Test: `tests/test_rear_current_compatibility.py`

- [ ] **Step 1: Pass yaw to every runtime sampler**

Change every no-argument call in `isr.c`, `cpu0_main.c`, and `remote_control.c` to:

```c
rear_motor_encoder_update_10ms(Yaw_1);
```

- [ ] **Step 2: Replace only the body of `update_state()` odometry integration**

Use the reference queue-consumer pattern while keeping current `ONE_TICK_DISTANCE`:

```c
int32 odometry_pulses = 0;
int32 total_pulses = 0;
float sample_yaw = 0.0f;

while(rear_motor_take_odometry_sample(&odometry_pulses, &sample_yaw))
{
    float sample_theta = daoche_flag ? sample_yaw + 180.0f : sample_yaw;
    float sample_distance = (float)odometry_pulses * ONE_TICK_DISTANCE;
    angle_plan(&sample_theta);
    if(daoche_flag) sample_distance = -sample_distance;
    state->current_state.x += sample_distance * sinf(sample_theta / 180.0f * M_PI);
    state->current_state.y += sample_distance * cosf(sample_theta / 180.0f * M_PI);
    total_pulses += odometry_pulses;
}
```

Clamp `total_pulses` into `ecd->delta_l/delta_r` as the reference implementation does, then set `state->current_state.theta` from current `Yaw_1`. Do not change `ONE_TICK_DISTANCE` from `0.000378f`.

- [ ] **Step 3: Migrate diagnostics to the reference odometry getter**

Replace `rear_motor_get_total_encoder_pulses()` with `rear_motor_get_odometry_total_pulses()`. Calculate displayed distance with:

```c
(float)rear_motor_get_odometry_total_pulses() * ONE_TICK_DISTANCE
```

Remove all remaining calls to `rear_motor_get_total_distance_m()`.

- [ ] **Step 4: Service active braking before normal rear control**

At the start of `Guandao_Rear_Motor_Update()` add:

```c
if(rear_motor_brake_active())
{
    rear_motor_brake_update();
    return;
}
```

Port the exact one-shot latch declarations and resets used by the reference `code/guandao.c`:

```c
static uint8 portion1_forward_brake_requested = 0;
static uint8 portion1_reverse_brake_requested = 0;
static uint8 portion1_park_brake_requested = 0;
static uint8 guandao_trace_brake_requested = 0;
static guandao_state *guandao_trace_brake_route = NULL;
```

Reset all five in `portion_1_reset()`. At the matching current forward endpoint, reverse-ready transition, reverse completion, route-save-after-`Flash_Store_Mode()`, and generic route endpoint branches, use the reference conditions and this one-shot form:

```c
if(!portion1_forward_brake_requested)
{
    rear_motor_brake_start();
    portion1_forward_brake_requested = 1;
}
```

Use the corresponding reverse, park, and generic-route latch at the other branches. Take the conditions and statement ordering from reference `code/guandao.c`; do not copy unrelated route-planning code. Keep the emergency key path on immediate `rear_motor_stop()` and do not call `rear_motor_brake_start()` from the remote-neutral branch.

- [ ] **Step 5: Run call-site tests**

Run:

```powershell
python -m unittest tests.test_subject_global_brake tests.test_kmy_rear_interface tests.test_rear_current_compatibility -v
```

Expected: PASS with current calibration unchanged and no parameterless sampler calls.

### Task 5: Run Regression And ADS Build Verification

**Files:**
- Verify all modified and created files only.

- [ ] **Step 1: Run all Python source-contract tests**

Run:

```powershell
python -m unittest discover -s tests -p "test_*.py" -v
```

Expected: all discovered tests pass.

- [ ] **Step 2: Run all existing PowerShell checks**

Run:

```powershell
$failed = @()
Get-ChildItem tests -Filter 'check_*.ps1' | Sort-Object Name | ForEach-Object {
    & $_.FullName
    if($LASTEXITCODE -ne 0) { $failed += $_.Name }
}
if($failed.Count -gt 0) { throw "Failed checks: $($failed -join ', ')" }
```

Expected: no script names in `Failed checks`. If a pre-existing unrelated test fails, reproduce it against commit `9011582` before classifying it as unrelated.

- [ ] **Step 3: Regenerate or verify the rear source make fragment**

Confirm `Debug/code/rear_motor/subdir.mk` still compiles only `rear_motor.c`; the three new headers require no object entries.

- [ ] **Step 4: Build with the ADS bundled make**

Run:

```powershell
& 'D:\tools\Infineon\ADS\AURIX-Studio-1.10.32\tools\make\make.exe' -C Debug -j4 all
```

Expected: exit code `0` and a refreshed `Debug/Seekfree_TC264_Opensource_Library.elf`.

- [ ] **Step 5: Perform final structural checks**

Run:

```powershell
rg -n "rear_motor_encoder_update_10ms\(\)" code user
rg -n "rear_motor_get_(total_encoder_pulses|total_distance_m)" code user
rg -n "Encoder_Get\(ecd\);" code/guandao.c
git diff --check
git status --short
```

Expected: the first two searches return no matches; `Encoder_Get(ecd);` is absent from `update_state()`; `git diff --check` exits zero. Review `git status` to confirm no reference-workspace file changed and no unrelated current-workspace file was added by the migration.
