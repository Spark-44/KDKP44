# Dynamic Steering And GPS Jump Filter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Use a 12-degree steering limit on ordinary route geometry, 20 degrees on sharp reference bends, and reject recording-time GPS jumps inconsistent with inertial motion.

**Architecture:** Add one helper in `guandao.c` that measures upcoming turn from smoothed raw route points. Extend the existing GPS candidate validator with a displacement-consistency check using the already stored state of the previous accepted anchor.

**Tech Stack:** TASKING C99 for AURIX TC26x and PowerShell static verification.

---

### Task 1: Add failing behavior checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Check for 12-degree normal and 20-degree sharp limits.
- [ ] Check for a 4-degree raw-reference trigger over six points.
- [ ] Check that GPS displacement is rejected above inertial displacement plus 2.0 m.
- [ ] Run the verification script and confirm these new checks fail on the current implementation.

### Task 2: Implement curvature-based steering limit

**Files:**
- Modify: `code/guandao.c`

- [ ] Map planned progress to the current raw point.
- [ ] Measure maximum local turn over the next six smoothed raw points.
- [ ] Select 20 degrees only when the measured turn reaches 4 degrees; otherwise retain 12 degrees.
- [ ] Keep the existing steering rate limiter and run verification.

### Task 3: Implement GPS jump rejection and deliver

**Files:**
- Modify: `code/guandao.c`
- Verify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Compute GPS and inertial displacement from the previous accepted anchor.
- [ ] Reject GPS displacement greater than inertial displacement plus 2.0 m.
- [ ] Run repository verification and `git diff --check`.
- [ ] Attempt the TASKING build, commit only task changes, and push to `master`.
