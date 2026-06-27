# Reset-Safe Route Start Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild route orientation after reset and either calibrate stable GPS to the local origin or fall back to pure inertial navigation before movement.

**Architecture:** Route initialization always aligns local route geometry to current yaw before creating the GPS transform. A stopped startup state calls a GPS calibration API until three stable fixes succeed or a six-second timeout disables fusion.

**Tech Stack:** TASKING C99 for AURIX TC26x and PowerShell static verification.

---

### Task 1: Add failing reset and fallback checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Require unconditional route yaw alignment before GPS preparation.
- [ ] Require a stopped GPS startup state with three samples and a six-second timeout.
- [ ] Require transform translation calibration and three-error runtime disable.
- [ ] Run verification and confirm the new checks fail on current code.

### Task 2: Implement GPS startup calibration

**Files:**
- Modify: `code/gps.h`
- Modify: `code/gps.c`

- [ ] Add startup calibration fields to the private fusion state.
- [ ] Add `portion2_gps_fusion_startup_update()` returning wait, ready, or inertial fallback.
- [ ] Collect three distinct fixes with eight satellites and one-metre stability.
- [ ] Shift transform translation to map the sample mean to the current local origin.
- [ ] Disable fusion after six seconds without stable calibration.

### Task 3: Align route first and add runtime fallback

**Files:**
- Modify: `code/guandao.c`
- Modify: `code/gps.c`

- [ ] Align every route to current yaw before smoothing and GPS preparation.
- [ ] Enter a stopped calibration state when GPS preparation succeeds.
- [ ] Start inertial tracking immediately when preparation fails or calibration falls back.
- [ ] Disable fusion after three consecutive runtime errors above three metres.
- [ ] Run verification, attempt TASKING build, commit task files, and push `master`.
