# Terminal Pose and GPS Recovery Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Preserve the recorded terminal pose and make GPS fusion quality-aware and recoverable.

**Architecture:** Route pose metadata is stored beside route lengths in the existing inertial Flash page. The route planner applies a terminal Bezier segment before smooth-plan generation. GNSS parsing exposes HDOP, while the fusion state machine adds startup rejection and runtime recovery states.

**Tech Stack:** TASKING C99 for AURIX TC26x, Seekfree GNSS driver, Flash page serialization, PowerShell regression checks.

---

### Task 1: Add failing regression checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [x] Require per-route start/final yaw metadata, capture points, Flash magic, and capacity guards.
- [x] Require terminal Bezier shaping, 0.20 m stop distance, 5 degree tolerance, and position reacquisition.
- [x] Require HDOP parsing, quality gates, five-sample startup stability, maximum startup shift, and three-fix recovery.
- [x] Run the verifier and confirm the new checks fail.

### Task 2: Preserve terminal pose

**Files:**
- Modify: `code/guandao.h`
- Modify: `code/guandao.c`
- Modify: `code/flash.c`

- [x] Capture and expose route start/final yaw.
- [x] Store and load yaw metadata with a new route-layout magic value.
- [x] Rotate the saved yaw with route alignment and build a 1.5 m terminal Bezier segment.
- [x] Reacquire endpoint position if yaw correction moves outside 0.25 m.

### Task 3: Gate and recover GPS fusion

**Files:**
- Modify: `libraries/zf_device/zf_device_gnss.h`
- Modify: `libraries/zf_device/zf_device_gnss.c`
- Modify: `code/gps.c`
- Modify: `code/guandao.c`

- [x] Parse HDOP from GGA and include it in diagnostics.
- [x] Reject poor-HDOP route points and fusion updates.
- [x] Reject unstable or excessively shifted startup calibration.
- [x] Suspend after three large errors and recover after three consecutive good fixes.

### Task 4: Verify and publish

**Files:**
- Test: `tools/verify_portion2_serial_debug.ps1`

- [x] Run the full verifier, source consistency checks, and `git diff --check`.
- [ ] Commit the isolated changes, selectively apply them to `master`, verify again, and push.
