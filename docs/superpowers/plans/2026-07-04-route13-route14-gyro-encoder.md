# Route 13 and Route 14 Gyroscope/Encoder Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add compiled-in route 13 and route 14 controllers extracted from the supplied serial record, using only encoder distance and gyroscope yaw at runtime while leaving recorded routes 11 and 12 unchanged.

**Architecture:** A focused `subject_2_gyro_route` module owns immutable distance/relative-yaw profiles, interpolation, PID steering, speed, safety, and diagnostics. The existing command dispatcher starts and schedules this controller; existing recorded-route and fixed-action start paths cancel it so only one controller owns the motor and steering outputs.

**Tech Stack:** Infineon TC264 embedded C, existing encoder/IMU/motor APIs, PowerShell source-verification tests.

---

## File Map

- Create `code/subject_2_gyro_route.h`: public logical route IDs and start/stop/task/status API.
- Create `code/subject_2_gyro_route.c`: extracted profiles and complete runtime controller.
- Create `tests/check_gyro_routes.ps1`: profile, command, control, safety, and regression checks.
- Modify `user/cpu0_main.c`: X/x/y/Y/S dispatch and main-loop scheduling.
- Modify `code/guandao.c`: cancel a gyroscope route before starting a recorded route.
- Modify `code/subject_2_fixed_action.c`: cancel a gyroscope route before starting a fixed action.

### Task 1: Lock Profile Data and Public API With a Failing Test

**Files:**
- Create: `tests/check_gyro_routes.ps1`
- Create: `code/subject_2_gyro_route.h`
- Create: `code/subject_2_gyro_route.c`

- [ ] **Step 1: Write the profile/API test before production files exist**

Create a PowerShell check that reads the future header/source and requires the public API, exact sample counts, monotonic profile distances, route endpoints, and no `x/y` runtime fields:

```powershell
$ErrorActionPreference = 'Stop'
$root = Join-Path $PSScriptRoot '..'
$header = Get-Content -Raw (Join-Path $root 'code\subject_2_gyro_route.h')
$source = Get-Content -Raw (Join-Path $root 'code\subject_2_gyro_route.c')

foreach($pattern in @(
    '#define\s+SUBJECT_2_GYRO_ROUTE_13\s+13U',
    '#define\s+SUBJECT_2_GYRO_ROUTE_14\s+14U',
    'void\s+subject_2_gyro_route_start\s*\(\s*uint8\s+route_number\s*,\s*uint8\s+reverse\s*\)',
    'void\s+subject_2_gyro_route_stop\s*\(\s*const\s+char\s*\s*reason\s*\)',
    'void\s+subject_2_gyro_route_task\s*\(\s*void\s*\)',
    'uint8\s+subject_2_gyro_route_is_active\s*\(\s*void\s*\)'
)) { if($header -notmatch $pattern) { throw "Missing gyro-route API: $pattern" } }

if(([regex]::Matches($source, '(?m)^\s+GYRO_ROUTE_13_SAMPLE\(')).Count -ne 42) { throw 'Route 13 must have 42 samples.' }
if(([regex]::Matches($source, '(?m)^\s+GYRO_ROUTE_14_SAMPLE\(')).Count -ne 44) { throw 'Route 14 must have 44 samples.' }
if($source -notmatch '16\.7[0-9]f' -or $source -notmatch '17\.7[0-9]f') { throw 'Profile terminal distances are missing.' }
if($source -match 'typedef\s+struct\s*\{[^}]*\b(x|y)\b[^}]*\}\s*subject_2_gyro_route_sample_t') { throw 'Runtime profile must not store x/y.' }
```

- [ ] **Step 2: Run the test and verify RED**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_gyro_routes.ps1`

Expected: FAIL because `code/subject_2_gyro_route.h` and `.c` do not exist.

- [ ] **Step 3: Add the smallest public header**

Create `subject_2_gyro_route.h` with include guards, `zf_common_typedef.h`, logical route numbers 13/14, and these declarations:

```c
#define SUBJECT_2_GYRO_ROUTE_13 13U
#define SUBJECT_2_GYRO_ROUTE_14 14U

void subject_2_gyro_route_start(uint8 route_number, uint8 reverse);
void subject_2_gyro_route_stop(const char *reason);
void subject_2_gyro_route_task(void);
uint8 subject_2_gyro_route_is_active(void);
```

- [ ] **Step 4: Extract and add immutable profiles**

In `subject_2_gyro_route.c`, define:

```c
typedef struct
{
    float distance_m;
    float relative_yaw_deg;
} subject_2_gyro_route_sample_t;
```

Populate 42 route-13 and 44 route-14 samples from the `[P2-REC]` records. Compute each distance as cumulative `hypotf(dx, dy)`. The expected rounded endpoints are 16.75 m for route 13 (source cumulative distance 16.7480 m) and 17.71 m for route 14 (source cumulative distance 17.7074 m). Compute each yaw as the continuously unwrapped `theta - first_theta`; retain two decimal places. Use `GYRO_ROUTE_13_SAMPLE(distance, yaw)` and `GYRO_ROUTE_14_SAMPLE(distance, yaw)` macros on each entry so the verification script can count them. Do not copy GPS, latitude, longitude, or absolute `x/y` into production.

- [ ] **Step 5: Complete profile parsing checks and run GREEN**

Extend the PowerShell test to parse every macro pair as invariant-culture floats, require strictly increasing distance after sample zero, require first distance/yaw to be zero, and compare the final distances to the cumulative values within 0.02 m.

Run: `powershell -ExecutionPolicy Bypass -File tests/check_gyro_routes.ps1`

Expected: PASS for profile/API checks.

- [ ] **Step 6: Commit the profile boundary**

```powershell
git add code/subject_2_gyro_route.h code/subject_2_gyro_route.c tests/check_gyro_routes.ps1
git commit -m "feat: add extracted gyro route profiles"
```

### Task 2: Implement Distance/Yaw Control With Safety

**Files:**
- Modify: `tests/check_gyro_routes.ps1`
- Modify: `code/subject_2_gyro_route.c`

- [ ] **Step 1: Add failing behavior checks**

Require these concrete controller elements in the source:

```powershell
foreach($pattern in @(
    'subject_2_gyro_route_interpolate_yaw\s*\(',
    'while\s*\(\s*delta\s*>\s*180\.0f\s*\)\s*delta\s*-=\s*360\.0f',
    'while\s*\(\s*delta\s*<\s*-180\.0f\s*\)\s*delta\s*\+=\s*360\.0f',
    'calculate_delta\s*\(',
    'ONE_TICK_DISTANCE',
    'SUBJECT_2_GYRO_ROUTE_MAX_ENCODER_DELTA\s+1000',
    'SUBJECT_2_GYRO_ROUTE_STALL_MS\s+3000U',
    'SUBJECT_2_GYRO_ROUTE_SLOWDOWN_M',
    'if\s*\(\s*reverse\s*\)[\s\S]*?steer_command\s*=\s*-steer_command',
    'subject_2_gyro_route_finish\s*\(\s*"DISTANCE"\s*\)',
    'subject_2_gyro_route_finish\s*\(\s*"STALL"\s*\)'
)) { if($source -notmatch $pattern) { throw "Missing gyro-route control behavior: $pattern" } }
```

- [ ] **Step 2: Run and verify RED**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_gyro_routes.ps1`

Expected: FAIL on the first missing control behavior.

- [ ] **Step 3: Implement interpolation and angle helpers**

Implement a bounded linear scan from the cached sample index. Clamp before the first and after the final sample. Interpolate with:

```c
ratio = (distance_m - a->distance_m) / (b->distance_m - a->distance_m);
return a->relative_yaw_deg + ratio * (b->relative_yaw_deg - a->relative_yaw_deg);
```

Normalize only the final target/error comparison; keep the table unwrapped.

- [ ] **Step 4: Implement controller lifecycle and PID**

At start, reject route numbers other than 13/14 and reject forward route 13. Capture `Yaw_Straight_1`, `l_ecdcounter()`, and `system_getval_ms()`. Route 13 always uses negative speed. Route 14 uses speed sign from `reverse`.

Each task update must:

1. accumulate absolute valid encoder delta times `ONE_TICK_DISTANCE`;
2. update the progress timestamp after at least 0.002 m;
3. interpolate relative yaw and add the captured start yaw;
4. run the encoder-yaw PID shape (KP 0.80, KI 0.20, KD 0.04), integral clamp 20 degrees, steering clamp 25 degrees, and per-update steering rate clamp 0.5 degrees; the 25-degree clamp covers the extracted route-13 peak theoretical demand of approximately 22.9 degrees;
5. negate steering for reverse drive;
6. command `out_v_l`, `out_v_r`, and `out_servo` using the existing `GUANDAO_SPEED_TO_MPS` convention;
7. slow over the final 1.0 m, finish at terminal distance, and stall after 3000 ms without progress.

- [ ] **Step 5: Add diagnostics and safe stop**

Emit rate-limited lines beginning `[GYRO-ROUTE]` with `route`, `dir`, `dist`, `total`, `target`, `yaw`, `err`, `steer`, `enc`, and `reason`. Start/stop lines bypass rate limiting. Stop must clear active state, set all three outputs to zero, and call `rear_motor_set_target_mps(0.0f)`.

- [ ] **Step 6: Run focused test and commit**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_gyro_routes.ps1`

Expected: PASS.

```powershell
git add code/subject_2_gyro_route.c tests/check_gyro_routes.ps1
git commit -m "feat: control gyro routes by encoder distance"
```

### Task 3: Wire Commands and Exclusive Controller Ownership

**Files:**
- Modify: `tests/check_gyro_routes.ps1`
- Modify: `user/cpu0_main.c`
- Modify: `code/guandao.c`
- Modify: `code/subject_2_fixed_action.c`

- [ ] **Step 1: Add failing integration checks**

Require `X/x -> start(13,1)`, `y -> start(14,0)`, `Y -> start(14,1)`, `S -> stop`, gyro-route-first task scheduling, and cancellation calls in both existing start paths. Also capture the original V/v/W/w command bodies and assert they still call the same route-11/12 APIs.

```powershell
foreach($pattern in @(
    "data\s*==\s*'X'\s*\|\|\s*data\s*==\s*'x'[\s\S]*?subject_2_gyro_route_start\s*\(\s*SUBJECT_2_GYRO_ROUTE_13\s*,\s*1U\s*\)",
    "data\s*==\s*'y'[\s\S]*?subject_2_gyro_route_start\s*\(\s*SUBJECT_2_GYRO_ROUTE_14\s*,\s*0U\s*\)",
    "data\s*==\s*'Y'[\s\S]*?subject_2_gyro_route_start\s*\(\s*SUBJECT_2_GYRO_ROUTE_14\s*,\s*1U\s*\)",
    "data\s*==\s*'S'[\s\S]*?subject_2_gyro_route_stop\s*\(\s*\"COMMAND\"\s*\)",
    'subject_2_gyro_route_is_active\s*\(\s*\)[\s\S]*?subject_2_gyro_route_task\s*\(\s*\)',
    'void\s+portion2_run_select_route[\s\S]*?subject_2_gyro_route_stop\s*\(\s*"RECORDED_ROUTE"\s*\)',
    'void\s+voice_drive_action_start[\s\S]*?subject_2_gyro_route_stop\s*\(\s*"FIXED_ACTION"\s*\)'
)) { if(($main + $guandao + $fixed) -notmatch $pattern) { throw "Missing integration: $pattern" } }
```

- [ ] **Step 2: Run and verify RED**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_gyro_routes.ps1`

Expected: FAIL because commands and ownership hooks are absent.

- [ ] **Step 3: Wire command dispatch**

Include `subject_2_gyro_route.h` in `cpu0_main.c`. Add X/x/y/Y cases before the final `else`. Each case clears pending prefixes, records/echoes the received byte, stops voice and recorded-route controllers, then starts the requested compiled route. Add `subject_2_gyro_route_stop("COMMAND")` to S.

- [ ] **Step 4: Schedule the active controller**

In `Guandao_Voice`, use this exclusive order:

```c
if(subject_2_gyro_route_is_active())
{
    subject_2_gyro_route_task();
}
else if(voice_drive_action_get_mode() != VOICE_DRIVE_ACTION_NONE)
{
    voice_drive_action_task();
}
else
{
    portion2_run_task();
}
```

- [ ] **Step 5: Add cancellation hooks without changing route 11/12 policy**

Call `subject_2_gyro_route_stop("RECORDED_ROUTE")` at the start of recorded route selectors and `subject_2_gyro_route_stop("FIXED_ACTION")` at the start of `voice_drive_action_start`. Do not change `PORTION2_ROUTE_COUNT`, Flash dimensions, route 11/12 constants, or V/v/W/w dispatch.

- [ ] **Step 6: Run integration test and commit**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_gyro_routes.ps1`

Expected: PASS.

```powershell
git add user/cpu0_main.c code/guandao.c code/subject_2_fixed_action.c tests/check_gyro_routes.ps1
git commit -m "feat: dispatch gyro routes 13 and 14"
```

### Task 4: Regression Verification and Handoff

**Files:**
- Modify: `tests/check_gyro_routes.ps1` only if a verification assertion is incomplete.

- [ ] **Step 1: Run the focused check fresh**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_gyro_routes.ps1`

Expected: `Gyroscope route 13/14 checks passed.` and exit code 0.

- [ ] **Step 2: Run all project PowerShell checks**

Run:

```powershell
Get-ChildItem tests\*.ps1 | ForEach-Object {
    & powershell -ExecutionPolicy Bypass -File $_.FullName
    if($LASTEXITCODE -ne 0) { throw "Failed: $($_.Name)" }
}
& powershell -ExecutionPolicy Bypass -File tools\verify_portion2_serial_debug.ps1
```

Expected: every script exits 0.

- [ ] **Step 3: Run source hygiene checks**

Run:

```powershell
git diff --check
rg -n "TBD|TODO|PLACEHOLDER" code\subject_2_gyro_route.* tests\check_gyro_routes.ps1
git status --short
```

Expected: no whitespace errors or placeholders; status contains only intentional feature files plus the user's pre-existing modifications.

- [ ] **Step 4: Inspect the final diff against the design**

Confirm 42/44 profile entries, no runtime GPS/x/y dependency, original route count and Flash layout unchanged, correct X/x/y/Y mapping, exclusive ownership, diagnostics, slowdown, distance stop, stall stop, and unchanged V/v/W/w behavior.

- [ ] **Step 5: Commit any final test-only correction**

If Step 4 required a test correction, rerun Steps 1–3 and commit only that correction:

```powershell
git add tests/check_gyro_routes.ps1
git commit -m "test: complete gyro route regression coverage"
```
