# Encoder-Yaw Straight Actions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add `Q/q` forward and `R/r` reverse encoder-measured 10-metre straight actions with yaw correction, and disable GPS requirements and fusion for recorded route 8.

**Architecture:** Extend the existing fixed-action state machine with two isolated modes. These modes accumulate raw route-encoder count differences and run a bounded yaw PD controller, while existing `I/J` actions remain unchanged. Add one route GPS policy helper so route 8 bypasses GPS validation, preparation, and runtime updates without affecting other routes.

**Tech Stack:** TASKING C99 for Infineon TC264, PowerShell static verification, existing encoder/IMU/rear-motor APIs.

---

### Task 1: Add failing static behavior checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] **Step 1: Add checks for command and action declarations**

Require `Q/q` and `R/r` dispatch to distinct `VOICE_DRIVE_ACTION_ENCODER_YAW_*` values, and require the two enum members in `code/voice_drive_action.h`.

- [ ] **Step 2: Add checks for measured distance and yaw control**

Require use of `l_ecdcounter()`, `calculate_delta`, `ONE_TICK_DISTANCE`, captured `yaw_target`, PD constants, reverse steering sign, 10.0-metre target, final slowdown, timeout, stall protection, and `[STRAIGHT]` diagnostics.

- [ ] **Step 3: Add route 8 GPS policy checks**

Require a route policy helper that returns false for `PORTION2_ROUTE_STRAIGHT`, bypasses GPS completeness, skips prepare, and skips runtime update while preserving GPS for other route IDs.

- [ ] **Step 4: Run the verifier and confirm RED**

Run: `powershell -ExecutionPolicy Bypass -File tools/verify_portion2_serial_debug.ps1`

Expected: the new checks fail because `Q/R`, encoder-yaw action modes, and route 8 GPS policy do not exist.

### Task 2: Implement encoder-yaw fixed actions

**Files:**
- Modify: `code/voice_drive_action.h`
- Modify: `code/subject_2_fixed_action.c`

- [ ] **Step 1: Add two action modes**

Append `VOICE_DRIVE_ACTION_ENCODER_YAW_FORWARD_10M` and `VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M` so existing action numeric values remain stable.

- [ ] **Step 2: Extend fixed-action state**

Store last encoder count, measured distance, target yaw, previous yaw error, last control time, last progress time, and retained steering command. Initialize these fields in start and clear them in stop.

- [ ] **Step 3: Accumulate real encoder distance**

For only the two new modes, read `l_ecdcounter()`, calculate the wrap-safe raw difference with `calculate_delta()`, reject implausible deltas, and add `fabs(delta) * ONE_TICK_DISTANCE`. Do not add commanded-speed distance for these modes.

- [ ] **Step 4: Apply yaw PD and speed profile**

Every 20ms normalize `yaw_target - Yaw_1`, apply a deadband, proportional and derivative damping, clamp steering magnitude and rate, and invert the physical steering command for reverse. Command 0.35m/s, reducing toward 0.15m/s over the final metre.

- [ ] **Step 5: Add stop safety and diagnostics**

Stop at 10.0m, after a 60-second timeout, or after 3 seconds without encoder progress. Emit `[STRAIGHT] START`, periodic `RUN`, and terminal `STOP` with direction, distance, yaw values, steering, speed, and reason.

- [ ] **Step 6: Run verifier for partial GREEN**

Run: `powershell -ExecutionPolicy Bypass -File tools/verify_portion2_serial_debug.ps1`

Expected: fixed-action checks pass; command and route policy checks may still fail.

### Task 3: Dispatch Q/R and preserve common stop behavior

**Files:**
- Modify: `user/cpu0_main.c`

- [ ] **Step 1: Add Q/q and R/r branches**

Dispatch both cases through `Portion2_Fixed_Action_Start()`, using forward mode for Q and reverse mode for R. Clear pending route/dump prefixes, update receive diagnostics, and echo the recognized byte.

- [ ] **Step 2: Verify common stops remain shared**

Confirm `S`, K3, and K4 continue calling `voice_drive_action_stop()` and therefore stop either new mode without a separate stop path.

- [ ] **Step 3: Run verifier**

Run: `powershell -ExecutionPolicy Bypass -File tools/verify_portion2_serial_debug.ps1`

Expected: Q/R command checks pass.

### Task 4: Disable GPS for route 8 only

**Files:**
- Modify: `code/guandao.c`

- [ ] **Step 1: Add route GPS policy helper**

Add `portion2_route_uses_gps(route_id)` returning false only for `PORTION2_ROUTE_STRAIGHT` and true for all other valid routes.

- [ ] **Step 2: Bypass route 8 GPS readiness checks**

Keep the five inertial-point minimum for route 8, but skip GPS count and endpoint-coverage rejection.

- [ ] **Step 3: Bypass preparation and runtime fusion**

Reset fusion and start route tracking immediately for route 8. Guard runtime `portion2_gps_fusion_update()` with the same helper. Leave all other routes on startup calibration and runtime fusion.

- [ ] **Step 4: Run full verifier and diff checks**

Run: `powershell -ExecutionPolicy Bypass -File tools/verify_portion2_serial_debug.ps1`

Run: `git diff --check`

Expected: all checks pass and diff check exits zero.

### Task 5: Build, review, commit, and push

**Files:**
- Verify all modified source, test, and plan files.

- [ ] **Step 1: Attempt ADS-integrated build**

Build the Debug configuration in AURIX Development Studio. If the non-commercial TASKING license blocks standalone/headless compilation, report that limitation and require the user to build in the visible ADS IDE.

- [ ] **Step 2: Review scoped diff**

Confirm `code/guandao.h` and `README.md` remain untouched and unstaged because they are pre-existing user changes.

- [ ] **Step 3: Commit implementation**

Stage only `code/voice_drive_action.h`, `code/subject_2_fixed_action.c`, `code/guandao.c`, `user/cpu0_main.c`, `tools/verify_portion2_serial_debug.ps1`, and this plan.

Commit message: `Add encoder yaw straight commands`

- [ ] **Step 4: Push master**

Push over the configured SSH-over-443 GitHub remote and verify the remote accepts the new commit.
