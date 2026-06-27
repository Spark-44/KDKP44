# GNSS Receive Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make GNSS receive status truthful and expose whether failures occur during UART framing, sentence recognition, or NMEA parsing.

**Architecture:** The GNSS driver owns framing and parse counters. The periodic ISR consumes a structured parse result, updates GPS state only on valid RMC/GGA data, and rate-limits a compact debug-UART report.

**Tech Stack:** TASKING C99 for AURIX TC26x, Seekfree UART/FIFO GNSS driver, PowerShell static regression verifier.

---

### Task 1: Add failing GNSS diagnostic checks

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Require public GNSS diagnostic counters and sentence-type status.
- [ ] Require bounded framing, checksum guards, and explicit string termination.
- [ ] Require success-only `update_gpsinformation()` calls and a one-second `[GNSS-DIAG]` report.
- [ ] Run the verifier and confirm the new checks fail against the old driver.

### Task 2: Harden GNSS framing and parsing

**Files:**
- Modify: `libraries/zf_device/zf_device_gnss.h`
- Modify: `libraries/zf_device/zf_device_gnss.c`

- [ ] Add diagnostic state and counters exposed through a const getter.
- [ ] Classify complete sentences before setting `gnss_flag`.
- [ ] Null-terminate copied frames and reject malformed checksum boundaries.
- [ ] Return parse-result bits for successful RMC, GGA, and THS parsing.

### Task 3: Make sequence updates truthful

**Files:**
- Modify: `user/isr.c`

- [ ] Call `update_gpsinformation()` only when RMC or GGA parsed successfully.
- [ ] Emit `[GNSS-DIAG]` no more than once per second with all diagnostic counters.

### Task 4: Verify and publish

**Files:**
- Test: `tools/verify_portion2_serial_debug.ps1`

- [ ] Run the GNSS checks, full verifier, preprocessor checks, and `git diff --check`.
- [ ] Commit the isolated changes, apply only those changes to `master`, verify again, and push.
