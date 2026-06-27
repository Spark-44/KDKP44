# Encoder-Yaw Steering Precision Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve small Q/R steering commands, remove steady heading bias with bounded integral control, and expose actuator telemetry.

**Architecture:** `subject_2_fixed_action.c` remains the owner of Q/R heading control. `angle_control` accepts float targets and exposes read-only telemetry, while `control.c` passes the float command without quantization. Existing route pursuit behavior and rack PID gains remain unchanged.

**Tech Stack:** TASKING C99 for AURIX TC26x, PowerShell static regression script, GCC host syntax harness.

---

### Task 1: Add regression checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Add checks requiring a float rack target API, no `int32` cast in `Steer_Moter_Contral`, bounded Q/R integral control, and actuator telemetry fields.
- [ ] Run `powershell -ExecutionPolicy Bypass -File tools/verify_portion2_serial_debug.ps1` and confirm the new checks fail for the missing behavior.

### Task 2: Preserve fractional rack targets

**Files:**
- Modify: `code/angle_control.h`
- Modify: `code/angle_control.c`
- Modify: `code/control.c`

- [ ] Change `angle_control_set_target` to accept `float` and pass `servo_out` directly from `Steer_Moter_Contral`.
- [ ] Add getters for target angle, current angle, and PID output PWM.

### Task 3: Add bounded Q/R integral correction and telemetry

**Files:**
- Modify: `code/subject_2_fixed_action.c`

- [ ] Add `KI`, integral limit, and state storage; reset state on action start and stop.
- [ ] Integrate heading error using elapsed seconds with anti-windup and include the integral term in the existing bounded steering command.
- [ ] Extend `[STRAIGHT]` with `rack_target`, `rack_actual`, and `rack_pwm`.

### Task 4: Verify, commit, and publish

**Files:**
- Test: `tools/verify_portion2_serial_debug.ps1`

- [ ] Run the complete PowerShell verification script and changed-module C99 syntax checks.
- [ ] Run `git diff --check`, inspect the staged diff, and commit only the files in this plan.
- [ ] Apply the isolated commit to `master` without staging existing user changes, verify again, and push `master`.
