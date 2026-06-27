# Portion 2 Progress Display Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show record and run point progress clearly on the IPS screen and debug serial output without changing vehicle control behavior.

**Architecture:** Keep progress calculations inside `guandao.c`, next to the existing route state and serial logging. Extend the existing source verification script with structural checks before changing production code, then reuse small clamped, one-based progress helpers in both screen and serial output.

**Tech Stack:** AURIX TC264 C99, TASKING compiler, PowerShell source verification.

---

### Task 1: Add failing progress-display checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Add checks requiring `[P2-REC-STATUS]`, record `raw/gps/dist/step` fields, run `raw_no/plan_no/gps_no` fields, and IPS labels `RAW`, `PLAN`, `GPS`, `DIST`, and `STEP`.
- [ ] Run `powershell -ExecutionPolicy Bypass -File tools\verify_portion2_serial_debug.ps1` and confirm the new checks fail because the fields are absent.

### Task 2: Implement record diagnostics

**Files:**
- Modify: `code/guandao.c`

- [ ] Add a helper that sums distances between the selected route's stored raw points.
- [ ] Add a 500 ms `[P2-REC-STATUS]` line containing route, state, `raw=current/39`, `gps=current/5`, accumulated distance, active `recode_threshold`, position, and yaw.
- [ ] Call the status logger from `portion2_record_task()` and show `RAW`, `GPS`, `DIST`, and `STEP` on unused record-screen rows.

### Task 3: Implement run diagnostics

**Files:**
- Modify: `code/guandao.c`

- [ ] Clamp original and planned progress to their valid totals and convert displayed point numbers to one-based values, retaining zero for an empty route.
- [ ] Append `raw_no`, `plan_no`, and `gps_no` to the existing periodic `[P2-RUN]` line without removing existing diagnostic fields.
- [ ] Replace the lower run debug rows with prominent `RAW`, `PLAN`, and `GPS` progress displays while retaining route, state, reject, and reverse indicators.

### Task 4: Verify and publish

**Files:**
- Verify: `tools/verify_portion2_serial_debug.ps1`
- Verify: `code/guandao.c`

- [ ] Run the PowerShell verification and require zero failed checks.
- [ ] Run `git diff --check`.
- [ ] Attempt `make -C Debug` and record whether the TASKING build tool is available.
- [ ] Stage only `code/guandao.c` and `tools/verify_portion2_serial_debug.ps1`, commit, and push to `master`.
