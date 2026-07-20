# HIP4082 Motor Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate the reference project's HIP4082 rear motor drive, rear speed loop, active braking, and front steering PID into the current subject-2 project without removing its routes or peripheral features.

**Architecture:** Keep subject-2 route and mode ownership intact, but replace the low-level rear actuator with four PWM channels and port the reference controller state machine behind a compatibility-preserving `rear_motor` API. Feed yaw-tagged encoder samples from the 10 ms ISR into route odometry, and service nonblocking braking before normal motor dispatch.

**Tech Stack:** Infineon TC264 C99, Seekfree peripheral APIs, TASKING toolchain, Python and PowerShell static regression tests.

---

### Task 1: Lock Down The Hardware Contract

**Files:**
- Create: `tests/test_hip_motor_mapping.py`
- Modify: `code/peripheral.h`
- Modify: `code/peripheral.c`

- [ ] **Step 1: Port the failing hardware mapping test**

Assert that `PWM_L1/L2/R1/R2` map to P21.4/P21.5/P21.2/P21.3, all four channels initialize at 17 kHz, direction GPIO macros disappear, and stop clears every channel.

- [ ] **Step 2: Run the test and verify RED**

Run: `python -m unittest tests.test_hip_motor_mapping -v`

Expected: FAIL because the current project only defines `PWM_L`, `PWM_R`, `MOTOR_GPIO_L`, and `MOTOR_GPIO_R`.

- [ ] **Step 3: Implement the four-PWM mapping**

Replace the old pin macros and update `Motor_init()`/`Moter_Set()` so each wheel clears both inputs before enabling the requested direction.

- [ ] **Step 4: Run the test and verify GREEN**

Run: `python -m unittest tests.test_hip_motor_mapping -v`

Expected: PASS.

### Task 2: Port The Rear Controller And Compatibility API

**Files:**
- Create: `tests/test_rear_global_brake.py`
- Create: `tests/test_kmy_rear_interface.py`
- Create: `code/rear_motor/rear_encoder_calibration.h`
- Create: `code/rear_motor/rear_odometry_pose_buffer.h`
- Modify: `code/rear_motor/rear_motor.h`
- Modify: `code/rear_motor/rear_motor.c`

- [ ] **Step 1: Port failing controller/interface tests**

Assert the reference gains, speed filtering, four-channel stop, nonblocking brake API, yaw-tagged odometry interface, and preservation of current subject-2 compatibility functions.

- [ ] **Step 2: Run the tests and verify RED**

Run: `python -m unittest tests.test_rear_global_brake tests.test_kmy_rear_interface -v`

Expected: FAIL because braking, yaw-tagged samples, and reference gains are absent.

- [ ] **Step 3: Implement the rear controller**

Port the 10 ms accumulation, 100 ms filtered speed loop, high-speed feed-forward, PWM slew limiting, reverse minimum output, and nonblocking brake state machine. Preserve `rear_motor_set_full_power`, speed limit, total pulse/distance, and odometer reset APIs used by subject 2.

- [ ] **Step 4: Run the tests and verify GREEN**

Run: `python -m unittest tests.test_rear_global_brake tests.test_kmy_rear_interface -v`

Expected: PASS.

### Task 3: Port Front Steering PID

**Files:**
- Create: `tests/test_motor_pid_migration.py`
- Modify: `code/PID.h`
- Modify: `code/PID.c`
- Modify: `code/angle_control.h`
- Modify: `code/angle_control.c`

- [ ] **Step 1: Write a failing PID contract test**

Assert `PID_Init(..., max_output, integral_max)`, steering gains `500/18/27`, feed-forward `65`, and integral limit `1000`.

- [ ] **Step 2: Run the test and verify RED**

Run: `python -m unittest tests.test_motor_pid_migration -v`

Expected: FAIL on the five-argument initializer and old steering constants.

- [ ] **Step 3: Implement the reference PID contract**

Add `integral_max` to the initializer and update the sole steering caller and constants.

- [ ] **Step 4: Run the test and verify GREEN**

Run: `python -m unittest tests.test_motor_pid_migration -v`

Expected: PASS.

### Task 4: Integrate ISR, Route Odometry, And Braking

**Files:**
- Create: `tests/test_subject_global_brake.py`
- Modify: `user/isr.c`
- Modify: `user/cpu0_main.c`
- Modify: `code/guandao.c`

- [ ] **Step 1: Port failing integration tests**

Assert ISR passes `Yaw_1`, route odometry consumes `(pulses, yaw)` samples, active braking is serviced ahead of mode dispatch, endpoint stops are one-shot brake requests, and emergency/mode-change stops remain immediate.

- [ ] **Step 2: Run the tests and verify RED**

Run: `python -m unittest tests.test_subject_global_brake tests.test_kmy_rear_interface -v`

Expected: FAIL because the current dispatcher and route endpoints only use immediate stop.

- [ ] **Step 3: Adapt subject-2 integration points**

Pass yaw from the 10 ms ISR, consume buffered odometry samples in `update_state`, update active braking before regular commands, and add latches so endpoint braking starts once.

- [ ] **Step 4: Run focused and full regression tests**

Run: `python -m unittest discover -s tests -p "test_*.py" -v`

Run: `Get-ChildItem tests/check_*.ps1 | ForEach-Object { & $_.FullName }`

Expected: all tests PASS.

### Task 5: Verify Source And Buildability

**Files:**
- Verify only: all changed files

- [ ] **Step 1: Check formatting and unintended changes**

Run: `git diff --check`

Run: `git status --short`

Expected: no whitespace errors; `tmp/` remains untouched and untracked.

- [ ] **Step 2: Attempt the TASKING build when available**

Run: `Get-Command amk -ErrorAction SilentlyContinue`

If found, run: `amk -j20 all`

Expected: link completes without unresolved symbols. If `amk` is unavailable, report that limitation and rely on source/static verification.
