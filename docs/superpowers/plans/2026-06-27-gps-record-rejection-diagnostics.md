# GPS Record Rejection Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expose why automatic GPS points are rejected during route recording.

**Architecture:** `gps.c` exposes a read-only parsed-update sequence. `guandao.c` owns record-filter reason classification, rate-limited serial diagnostics, and record-screen presentation; all existing acceptance thresholds remain unchanged.

**Tech Stack:** TASKING C99 for AURIX TC26x, PowerShell regression script, GCC host syntax harness.

---

### Task 1: Add failing diagnostics checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Require named GPS rejection reasons, a one-second skip log, GNSS state fields, screen fields, and an update-sequence getter.
- [ ] Run the verifier and confirm these new checks fail because diagnostics are absent.

### Task 2: Expose GNSS update sequence

**Files:**
- Modify: `code/gps.h`
- Modify: `code/gps.c`

- [ ] Add `portion2_gps_get_fix_sequence()` returning the existing parsed-fix sequence without changing its update behavior.

### Task 3: Classify and report record rejection

**Files:**
- Modify: `code/guandao.c`

- [ ] Assign a specific reason at each return in candidate validation and record-capacity/interval/index checks.
- [ ] Emit `[P2-REC-GPS-SKIP]` at most once per second while recording and immediately retain `NONE` after a successful point.
- [ ] Show fix state, satellites, coordinate validity, sequence, and compact reason on rows 8-11.

### Task 4: Verify and publish

**Files:**
- Test: `tools/verify_portion2_serial_debug.ps1`

- [ ] Run the full verifier, host C99 syntax check, and `git diff --check`.
- [ ] Commit only the planned files, cherry-pick to `master` without staging user changes, verify again, and push.
