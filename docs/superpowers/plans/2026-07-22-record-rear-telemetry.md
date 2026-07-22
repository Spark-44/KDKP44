# Record Rear-Motor Telemetry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Print rear-wheel actual speed and signed PWM every 200 ms while remote control is active in Record mode.

**Architecture:** Add one rate-limited telemetry function beside the existing rear-motor diagnostics in `cpu0_main.c`. Schedule it only from the Record-mode branch, retaining the idle encoder diagnostic unchanged. A focused PowerShell check statically verifies guards, data sources, output format, scheduling, and rate limit.

**Tech Stack:** TC264 C99, TASKING toolchain, PowerShell static regression tests, debug UART at 115200 baud.

---

### Task 1: Add the Record telemetry regression check

**Files:**
- Create: `tests/check_record_rear_motor_telemetry.ps1`
- Inspect: `user/cpu0_main.c`

- [ ] **Step 1: Write the failing test**

```powershell
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw (Join-Path $root 'user/cpu0_main.c')

function Assert-Match([string]$Text, [string]$Pattern, [string]$Message) {
    if($Text -notmatch $Pattern) { throw $Message }
}

Assert-Match $main '#define\s+RECORD_REAR_TELEMETRY_PERIOD_MS\s+\(200U\)' `
    'Record rear telemetry must use a 200ms period.'
Assert-Match $main 'static\s+void\s+Record_Rear_Motor_Telemetry_Update\s*\(\s*void\s*\)' `
    'Record rear telemetry update function is missing.'
Assert-Match $main 'Record_Rear_Motor_Telemetry_Update[\s\S]*?main_mode\s*!=\s*Guandao_Portion2_Recode[\s\S]*?return\s*;' `
    'Record rear telemetry must be guarded by Record mode.'
Assert-Match $main 'Record_Rear_Motor_Telemetry_Update[\s\S]*?!remote_control_is_active\s*\(\s*\)[\s\S]*?return\s*;' `
    'Record rear telemetry must require active remote control.'
Assert-Match $main '\[REC-REAR\]\s+speed=%\.3fmps\s+pwm=%d' `
    'Record rear telemetry output format is missing.'
Assert-Match $main '\[REC-REAR\][\s\S]*?rear_motor_get_speed_mps\s*\(\s*\)[\s\S]*?rear_motor_get_pwm\s*\(\s*\)' `
    'Record rear telemetry must read actual speed and PWM.'
Assert-Match $main 'case\s+Guandao_Portion2_Recode\s*:[\s\S]*?portion2_record_task\s*\(\s*\)\s*;[\s\S]*?Record_Rear_Motor_Telemetry_Update\s*\(\s*\)\s*;' `
    'Record mode must schedule rear telemetry after the record task.'
Assert-Match $main 'case\s+Guandao_Portion2_Recode\s*:[\s\S]*?Record_Idle_Encoder_Diag_Update\s*\(\s*\)\s*;' `
    'Existing idle encoder diagnostics must remain scheduled.'

Write-Host 'Record rear motor telemetry checks passed.'
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/check_record_rear_motor_telemetry.ps1
```

Expected: FAIL with `Record rear telemetry must use a 200ms period.`

- [ ] **Step 3: Commit the failing test**

```powershell
git add -- tests/check_record_rear_motor_telemetry.ps1
git commit -m "test: cover record rear motor telemetry"
```

### Task 2: Implement and schedule Record telemetry

**Files:**
- Modify: `user/cpu0_main.c:31-176`
- Modify: `user/cpu0_main.c:1090-1102`
- Test: `tests/check_record_rear_motor_telemetry.ps1`

- [ ] **Step 1: Add the period constant**

Add beside the other telemetry periods:

```c
#define RECORD_REAR_TELEMETRY_PERIOD_MS (200U)
```

- [ ] **Step 2: Add the guarded telemetry function**

Add after `Rear_Motor_Serial_Telemetry_Update()`:

```c
static void Record_Rear_Motor_Telemetry_Update(void)
{
    static uint32 last_report_ms = 0;
    uint32 now_ms = system_getval_ms();

    if(main_mode != Guandao_Portion2_Recode)
    {
        return;
    }
    if(!remote_control_is_active())
    {
        return;
    }
    if((uint32)(now_ms - last_report_ms) < RECORD_REAR_TELEMETRY_PERIOD_MS)
    {
        return;
    }

    last_report_ms = now_ms;
    {
        char line[80];
        int len = sprintf(line,
                          "[REC-REAR] speed=%.3fmps pwm=%d\r\n",
                          rear_motor_get_speed_mps(),
                          (int)rear_motor_get_pwm());
        if(len > 0)
        {
            Serial_Debug_Write(line);
        }
    }
}
```

- [ ] **Step 3: Schedule it in Record mode**

Change the Record branch to:

```c
case Guandao_Portion2_Recode:
    voice_inited = 0;
    Record_Idle_Encoder_Diag_Update();
    portion2_record_task();
    Record_Rear_Motor_Telemetry_Update();
    if(remote_control_is_active())
    {
        continue;
    }
    break;
```

- [ ] **Step 4: Run the focused test and verify GREEN**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/check_record_rear_motor_telemetry.ps1
```

Expected: PASS with `Record rear motor telemetry checks passed.`

- [ ] **Step 5: Run adjacent regression checks**

Run:

```powershell
$checks = @(
  'tests/check_record_idle_encoder_diag.ps1',
  'tests/check_rear_motor_serial_telemetry.ps1',
  'tests/check_remote_control_port.ps1'
)
foreach($check in $checks) {
  & powershell -ExecutionPolicy Bypass -File $check
  if($LASTEXITCODE -ne 0) { throw "Failed: $check" }
}
```

Expected: all three scripts exit 0.

- [ ] **Step 6: Verify formatting and commit**

```powershell
git diff --check
git add -- user/cpu0_main.c tests/check_record_rear_motor_telemetry.ps1
git commit -m "feat: print rear telemetry while recording"
```

### Task 3: Hardware acceptance

**Files:**
- Verify only; no source changes.

- [ ] **Step 1: Build and flash from ADS**

Expected: TASKING build completes without errors and the firmware is flashed to the TC264.

- [ ] **Step 2: Verify live Record telemetry**

Enter Record mode, activate remote driving, and inspect UART0 at 115200 baud.

Expected: approximately five lines per second in this form:

```text
[REC-REAR] speed=1.234mps pwm=3500
```

Release remote control.

Expected: `[REC-REAR]` output stops, while the existing idle diagnostic behavior remains available.
