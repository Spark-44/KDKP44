# Six-Satellite GPS Threshold Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Allow route recording and runtime GPS fusion with at least six satellites.

**Architecture:** Keep the existing independent recording and fusion quality gates, but synchronize both minimum-satellite constants at `6U`. Preserve the current HDOP, coordinate, repeat-point, and movement validation.

**Tech Stack:** TASKING C99 for AURIX TC26x and PowerShell source regression checks.

---

### Task 1: Synchronize minimum satellite thresholds

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`
- Modify: `code/guandao.c`
- Modify: `code/gps.c`

- [x] **Step 1: Write the failing regression check**

Add a verifier entry requiring `PORTION2_GPS_RECORD_MIN_SATELLITES 6U` and `PORTION2_GPS_FUSION_MIN_SATELLITES (6U)`.

- [x] **Step 2: Verify the new check fails**

Run: `powershell -ExecutionPolicy Bypass -File tools\verify_portion2_serial_debug.ps1`

Expected: FAIL for `gps recording and fusion accept six satellites` because both production constants are currently eight.

- [x] **Step 3: Implement the two threshold changes**

Set the recording constant in `code/guandao.c` and fusion constant in `code/gps.c` to `6U`. Do not modify the `2.5f` HDOP gates.

- [x] **Step 4: Verify all checks pass**

Run: `powershell -ExecutionPolicy Bypass -File tools\verify_portion2_serial_debug.ps1`

Expected: all checks PASS.

- [x] **Step 5: Check and commit the scoped diff**

Run `git diff --check`, confirm only the plan, verifier, and two GPS source files changed, then commit with message `Lower GPS satellite threshold to six`.
