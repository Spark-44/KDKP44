# Drive Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a safe DRIVE main mode with K1/K2 stepped straight-line speed control and long-K4 transitions to and from RECORD.

**Architecture:** Replace the K4 short-only helper with an explicit event API shared by RECORD, RUN, and DRIVE. Keep DRIVE output ownership in `cpu0_main.c`, using existing rear-motor and steering paths while isolating route logic.

**Tech Stack:** TASKING C99, existing GPIO/key APIs, PowerShell static checks, Git.

---

### Task 1: Add failing DRIVE regression checks

**Files:**
- Create: `tests/check_drive_mode.ps1`

- [ ] Assert the DRIVE enum, explicit K4 events, legal transitions, zero initial speed, 1 m/s steps, 0-5 m/s clamps, centered steering, and safe exit.
- [ ] Run `powershell -NoProfile -ExecutionPolicy Bypass -File tests/check_drive_mode.ps1` and confirm it fails because DRIVE is absent.

### Task 2: Add explicit K4 mode events

**Files:**
- Modify: `code/guandao.h`
- Modify: `code/guandao.c`

- [ ] Define `PORTION2_MODE_KEY_NONE`, `PORTION2_MODE_KEY_SHORT`, and `PORTION2_MODE_KEY_LONG`.
- [ ] Return LONG once after 1.5 seconds and lock until release; return SHORT only for a release before the threshold.
- [ ] Update RECORD so SHORT enters RUN and LONG enters DRIVE.

### Task 3: Implement DRIVE mode

**Files:**
- Modify: `code/display.h`
- Modify: `user/cpu0_main.c`

- [ ] Add `Guandao_Drive` to the main-mode enum.
- [ ] Add DRIVE entry/reset, K1/K2 release-event handling, equal wheel outputs, zero steering, UI, and long-K4 safe return to RECORD.
- [ ] Ensure RUN ignores long K4 and cannot enter DRIVE directly.
- [ ] Restore the malformed first include line so the source can compile.

### Task 4: Verify and publish

- [ ] Run the focused DRIVE test and existing staged-snapshot route-layout test.
- [ ] Run `git diff --check` and inspect the staged diff.
- [ ] Commit only DRIVE-related hunks and push `master`.
