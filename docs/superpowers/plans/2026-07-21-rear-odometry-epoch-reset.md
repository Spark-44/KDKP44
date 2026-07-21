# Rear Odometry Epoch Reset Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prevent odometry sampled before a record or Run origin is established from shifting the new local route coordinates.

**Architecture:** Add one rear-motor API that atomically discards only pending pose samples. Call it at the recording and Run coordinate-epoch boundaries while preserving cumulative pulses, speed state, encoder baseline, calibration, and motor output.

**Tech Stack:** Infineon AURIX C, PowerShell structural regression checks, Python unittest regression suite.

---

### Task 1: Lock the coordinate-epoch behavior with a failing check

**Files:**
- Create: `tests/check_rear_odometry_epoch_reset.ps1`
- Inspect: `code/rear_motor/rear_motor.h`
- Inspect: `code/rear_motor/rear_motor.c`
- Inspect: `code/guandao.c`

- [ ] **Step 1: Write the failing test**

Create a PowerShell check that asserts:

```powershell
Assert-Contains $rearHeader 'void\s+rear_motor_discard_odometry_samples\s*\(\s*void\s*\)\s*;' 'Rear motor must expose an API for starting a new odometry coordinate epoch.'
Assert-Contains $rearSource 'void\s+rear_motor_discard_odometry_samples\s*\(\s*void\s*\)[\s\S]*?interrupt_global_disable\s*\(\s*\)[\s\S]*?rear_odometry_pose_buffer_init\s*\(\s*&odometry_pose_buffer\s*\)[\s\S]*?interrupt_global_enable\s*\(' 'Discarding pending odometry must be atomic.'
Assert-Contains $recordBegin 'rear_motor_discard_odometry_samples\s*\(\s*\)\s*;[\s\S]*?portion2_record_point\s*\(\s*\)' 'Recording must discard pre-origin samples before writing its first point.'
Assert-Contains $runInit 'portion2_clear_route\s*\(\s*\)\s*;[\s\S]*?rear_motor_discard_odometry_samples\s*\(\s*\)\s*;[\s\S]*?Encoder_Get\s*\(' 'Run initialization must discard pre-origin samples before tracking starts.'
```

- [ ] **Step 2: Run the check and verify RED**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_rear_odometry_epoch_reset.ps1`

Expected: FAIL because `rear_motor_discard_odometry_samples` does not exist.

### Task 2: Implement the minimal odometry epoch reset

**Files:**
- Modify: `code/rear_motor/rear_motor.h`
- Modify: `code/rear_motor/rear_motor.c`
- Modify: `code/guandao.c`
- Test: `tests/check_rear_odometry_epoch_reset.ps1`

- [ ] **Step 1: Declare and implement the queue-only discard API**

Add this public declaration:

```c
void rear_motor_discard_odometry_samples(void);
```

Implement it without modifying `odometry_total_pulses`, encoder baseline, speed windows, PID state, or output:

```c
void rear_motor_discard_odometry_samples(void)
{
    uint32 interrupt_state = interrupt_global_disable();
    rear_odometry_pose_buffer_init(&odometry_pose_buffer);
    interrupt_global_enable(interrupt_state);
}
```

- [ ] **Step 2: Start fresh epochs at the two route origins**

Call `rear_motor_discard_odometry_samples()` in `portion2_record_begin_route()` after resetting `passage` and before the first `portion2_record_point()`. Call it in Run state 1 immediately after `portion2_clear_route()` and before `Encoder_Get()`.

- [ ] **Step 3: Run the focused check and verify GREEN**

Run: `powershell -ExecutionPolicy Bypass -File tests/check_rear_odometry_epoch_reset.ps1`

Expected: PASS with `Rear odometry epoch reset checks passed.`

### Task 3: Regression verification

**Files:**
- Verify: `tests/check_record_idle_encoder_diag.ps1`
- Verify: `tests/check_rear_encoder_odometer.ps1`
- Verify: `tests/test_*.py`

- [ ] **Step 1: Run related PowerShell checks**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/check_record_idle_encoder_diag.ps1
powershell -ExecutionPolicy Bypass -File tests/check_rear_encoder_odometer.ps1
powershell -ExecutionPolicy Bypass -File tests/check_rear_odometry_epoch_reset.ps1
```

Expected: all three checks pass.

- [ ] **Step 2: Run the rear-driver Python suite**

Run: `python -m unittest discover -s tests -p 'test_*.py' -q`

Expected: all discovered tests pass.

- [ ] **Step 3: Check the scoped diff**

Run: `git diff --check -- code/rear_motor/rear_motor.h code/rear_motor/rear_motor.c code/guandao.c tests/check_rear_odometry_epoch_reset.ps1`

Expected: no output.

- [ ] **Step 4: Hardware acceptance after handoff**

Delete the malformed saved route and record it again. Verify that record point 1 is about one configured step from `(0,0)` rather than jumping approximately 2.98 m, then verify Run begins along the recorded straight segment.
