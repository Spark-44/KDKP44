# GPS Quality Filter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Filter unstable GNSS samples before recording and reject bad GPS anchors without weakening route-start safety.

**Architecture:** Keep a five-position RMC-only ring in `guandao.c`; idle RECORD mode pre-fills it, the first anchor uses a five-sample median, and later anchors use the latest-three median. Refactor GPS fitting in `gps.c` into a fixed-array helper, clip residuals above 1.5m, refit once, and require eight inliers.

**Tech Stack:** TASKING C99 for AURIX TC264, PowerShell source regression checks, existing UART diagnostics.

---

### Task 1: Lock behavior with failing checks

**Files:**
- Create: `tests/check_portion2_gps_quality.ps1`

- [ ] Assert that RMC and generic parsed sequences are separate and position users read the RMC sequence.
- [ ] Assert five-sample origin median, latest-three median, 1.0m origin stability, and 0.8m jump margin.
- [ ] Assert 1.5m residual clipping, eight-inlier minimum, refit, and fit diagnostics.
- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File tests/check_portion2_gps_quality.ps1`; expect failure on missing RMC sequence.

### Task 2: Add RMC-only position sequencing

**Files:**
- Modify: `code/gps.h`
- Modify: `code/gps.c`
- Modify: `user/isr.c`

- [ ] Add `portion2_gps_rmc_sequence` and `portion2_gps_get_rmc_sequence()`.
- [ ] Pass `parse_result` to `portion2_gps_note_parsed_update(uint8 parse_result)`; increment the generic sequence for RMC/GGA and the RMC sequence only for `GNSS_PARSE_RMC_OK`.
- [ ] Use the RMC sequence in GPS fusion startup and runtime position updates.
- [ ] Run the new check and confirm the sequencing assertions pass while filter assertions still fail.

### Task 3: Filter recorded GPS anchors

**Files:**
- Modify: `code/guandao.c`

- [ ] Add one fixed five-coordinate ring, sample timestamp, and RMC sequence marker.
- [ ] Feed the ring only from fresh, valid RMC fixes; reset unstable origin windows whose spread exceeds 1.0m.
- [ ] Use coordinate-wise median of five for the first point and latest three for later points.
- [ ] Change jump margin to `0.8f`, require eight GPS points per route, and add `NO_RMC`, `STABILIZE`, and `STALE` serial reasons.
- [ ] Make K3 set a pending start when the origin window is not ready; begin recording automatically when it becomes ready.
- [ ] Run the new check and existing route-layout checks.

### Task 4: Add robust route fitting

**Files:**
- Modify: `code/gps.c`

- [ ] Extract similarity fitting into a helper that accepts a 30-byte inclusion mask.
- [ ] Fit all anchors, mark residuals at or below `1.5f` as inliers, require at least eight, then refit once.
- [ ] Store and log total, inlier, and removed counts; reject if refit RMS exceeds `1.20f`.
- [ ] Run GPS quality, startup guard, final lock, raw tracking, and RAM-oriented source checks.

### Task 5: Verify and publish

**Files:**
- Modify: `progress.md`

- [ ] Run `git diff --check` and all relevant PowerShell checks.
- [ ] Inspect the staged diff to exclude the user's route-11 speed and `cpu0_main.c` edits.
- [ ] Commit implementation and push to `master` as explicitly requested by the user.
