# Portion 2 GPS Fusion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add filtered GPS position feedback to portion 2 route following and eliminate start-yaw route rotation when a valid absolute GPS frame is available.

**Architecture:** Implement transform fitting and runtime fusion in `gps.c`, expose a small status API in `gps.h`, and call it from the existing portion 2 route lifecycle in `guandao.c`. Keep control output in the existing pursuit controller; GPS only corrects the controller's current `x/y` estimate.

**Tech Stack:** AURIX TC264 C99, TASKING compiler, PowerShell structural verification.

---

### Task 1: Add failing GPS fusion checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Require fusion prepare/update/status APIs, zero-based GPS binding, new Flash marker, guarded route alignment, correction gain and cap, GNSS validity checks, serial fusion diagnostics, and run-screen fusion state.
- [ ] Run the verification script and confirm these checks fail before production edits.

### Task 2: Implement transform and filtered fusion

**Files:**
- Modify: `code/gps.c`
- Modify: `code/gps.h`

- [ ] Track a monotonically increasing GNSS fix sequence from `update_gpsinformation()`.
- [ ] Fit a similarity transform from distinct recorded GPS anchors to bound raw route points.
- [ ] Reject invalid fixes, fewer than eight satellites, repeated coordinates, transform failures, and errors greater than 3 metres.
- [ ] Apply 10 percent position correction capped at 0.1 metre per accepted fix.
- [ ] Expose readiness, latest validity, error, correction, satellites, and serial logging.

### Task 3: Integrate fusion into record and run lifecycle

**Files:**
- Modify: `code/guandao.c`
- Modify: `code/flash.c`

- [ ] Save GPS bind indices as zero-based raw indices.
- [ ] Translate selected raw routes to the local origin before GPS preparation.
- [ ] Prepare fusion after route and GPS data are copied; skip start-yaw rotation only when preparation succeeds.
- [ ] Update fusion after encoder integration and before pursuit steering.
- [ ] Add run-screen `GF`, validity, satellites, and error fields.
- [ ] Change the GPS layout marker so old bind data is invalidated.

### Task 4: Verify and publish

**Files:**
- Verify: `code/gps.c`
- Verify: `code/gps.h`
- Verify: `code/guandao.c`
- Verify: `code/flash.c`
- Verify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Run the full PowerShell verification and `git diff --check`.
- [ ] Attempt the TASKING build and report tool availability.
- [ ] Stage only task files, preserving the user's pending `WHEEL_BASE` and `README.md` changes.
- [ ] Commit and push to `master`.
