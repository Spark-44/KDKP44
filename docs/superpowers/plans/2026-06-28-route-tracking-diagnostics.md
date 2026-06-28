# Route Tracking Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Emit one useful cross-track diagnostic per raw route point and a run summary without changing vehicle control behavior.

**Architecture:** Add a diagnostics-only projection helper and run statistics inside `code/guandao.c`. Reuse existing GPS fusion getters, route-index mapping, fixed-point serial formatting, and the current run lifecycle. Replace per-planned-point serial events with per-raw-point `[P2-TRACK]` events and reduce detailed run status to one hertz.

**Tech Stack:** TASKING C99 for AURIX TC26x and PowerShell source regression checks.

---

### Task 1: Specify route tracking output

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [x] **Step 1: Add failing checks**

Add checks requiring:

```text
PORTION2_TRACK_BAD_THRESHOLD_M 0.30f
[P2-TRACK]
side=%s off=
head_err=
steer=
gps=%s gps_err=
[P2-TRACK-END]
first_bad=
max_raw=
max_off=
```

Also require raw-point change gating, segment projection with a clamped interpolation factor, a 1000ms `[P2-RUN]` period, and removal of the `[P2-RUN-PT]` output token.

- [x] **Step 2: Verify the checks fail**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools\verify_portion2_serial_debug.ps1
```

Expected: only the new route-tracking checks fail.

### Task 2: Implement cross-track reporting

**Files:**
- Modify: `code/guandao.c`

- [x] **Step 1: Add diagnostic state and projection helper**

Add a `portion2_track_sample_t` containing signed/absolute cross-track error and heading error. Project `portion_2.current_state` onto the segment immediately behind the current planned point, clamp projection `t` to `[0, 1]`, and derive side from the segment cross product.

- [x] **Step 2: Replace planned-point output with raw-point output**

Change the existing point-event logger so it returns unless the mapped raw point changed. Emit `[P2-TRACK]` with route/raw/plan progress, side, offset, heading error, steering, final distance, GPS readiness/error, and `OK`/`BAD` at 0.30m.

- [x] **Step 3: Track and emit run summary**

Reset `first_bad`, `max_raw`, and `max_off` when a route starts. Update them for every emitted raw point. Emit `[P2-TRACK-END]` once at normal completion and once before an explicit active-run stop; use zero for `first_bad` when no point crossed the threshold.

- [x] **Step 4: Reduce detailed status frequency**

Change the `[P2-RUN]` interval from `200U` to `1000U`. Keep GPS progress event behavior unchanged.

- [x] **Step 5: Run the full verifier**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools\verify_portion2_serial_debug.ps1
git diff --check
```

Expected: all checks pass and no whitespace errors are reported.

### Task 3: Publish

**Files:**
- Commit the plan, verifier, and `code/guandao.c`.

- [x] **Step 1: Review the scoped diff**

Confirm no control gains, route data, Flash layout, or GPS fusion calculations changed.

- [ ] **Step 2: Commit and integrate**

Commit with `Add per-point route tracking diagnostics`, apply the commit to `master` without staging the user's existing changes, rerun verification, and push `master`.
