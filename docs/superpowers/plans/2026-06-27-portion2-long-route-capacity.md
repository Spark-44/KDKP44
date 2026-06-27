# Portion 2 Long Route Capacity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand each portion 2 route to 49 inertial points and 20 GPS points without changing record spacing.

**Architecture:** Capacity constants remain centralized in `guandao.h`. Flash serialization continues using one inertial page and one GPS page, with compile-time size checks and a new GPS layout marker that invalidates old records safely.

**Tech Stack:** TASKING C99 for AURIX TC26x, PowerShell regression script, generated linker map for memory review.

---

### Task 1: Add failing capacity tests

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Require 49 points per route, 441 inertial slots, 20 GPS points per route, 180 GPS records, unchanged 0.4 m and 1.0 m spacing, and compile-time storage guards.
- [ ] Run the verifier and confirm the new capacity checks fail against the old 39/14 layout.

### Task 2: Expand route and GPS storage

**Files:**
- Modify: `code/guandao.h`

- [ ] Set route capacity to 49, total inertial capacity to 441, GPS capacity to 20 per route and 180 total.
- [ ] Add preprocessor checks that total inertial and GPS counts match their backing arrays.

### Task 3: Version and protect Flash storage

**Files:**
- Modify: `code/flash.c`

- [ ] Change the GPS layout marker so old 14-point GPS data is rejected.
- [ ] Add a compile-time check proving 441 coordinate pairs plus route metadata fit the inertial page.
- [ ] Retain the existing GPS-page check and unchanged serialization loops.

### Task 4: Verify and publish

**Files:**
- Test: `tools/verify_portion2_serial_debug.ps1`

- [ ] Run the complete verifier and `git diff --check`.
- [ ] Commit only planned files, cherry-pick to `master` without staging user changes, verify again, and push.
