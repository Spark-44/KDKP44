# Portion 2 Fourteen GPS Points Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Store and use up to 14 valid GPS anchors for each of nine portion-2 routes.

**Architecture:** Keep inertial route data on Flash page 9 and move the expanded GPS table to page 8. Filter stale GPS fixes during recording, capture an endpoint fix when possible, and reject routes whose GPS coverage ends too far before the route endpoint.

**Tech Stack:** TASKING C99 for AURIX TC26x, Infineon data-flash API, PowerShell static verification.

---

### Task 1: Add failing capacity and storage tests

**Files:**
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Assert 14 GPS points per route, 126 total records, and a 126-entry backing array.
- [ ] Assert GPS persistence reads and writes `RECODE_PASSAGE_TWO` with a version marker.
- [ ] Assert recording filters duplicate fixes and attempts endpoint capture.
- [ ] Run the script and confirm it fails because the implementation still supports eight points on one page.

### Task 2: Expand capacities and split Flash storage

**Files:**
- Modify: `code/guandao.h`
- Modify: `code/flash.c`
- Modify: `code/flash.h`

- [ ] Set per-route capacity to 14 and total capacity to 126.
- [ ] Keep route coordinates and counts on page 9.
- [ ] Serialize the GPS table, total count, and new magic marker on page 8.
- [ ] Load GPS records only when page 8 has a valid marker and bounded count.
- [ ] Run the verification script and confirm storage assertions pass.

### Task 3: Make GPS recording reject stale fixes

**Files:**
- Modify: `code/guandao.c`
- Modify: `tools/verify_portion2_serial_debug.ps1`

- [ ] Add a helper that validates GNSS state, satellite count, nonzero coordinates, and 0.20 m movement from the previous accepted fix.
- [ ] Make automatic recording consume capacity only for accepted fixes.
- [ ] Attempt one final GPS append when K3 stops recording.
- [ ] Require the final accepted anchor to be within four raw points of the route endpoint.
- [ ] Run the verification script and confirm all recording assertions pass.

### Task 4: Regression verification and delivery

**Files:**
- Verify: `code/guandao.c`, `code/guandao.h`, `code/flash.c`, `code/flash.h`, `code/gps.c`

- [ ] Run all repository PowerShell verification scripts.
- [ ] Run the available project build or report that TASKING tooling is unavailable.
- [ ] Inspect the final diff and confirm unrelated `WHEEL_BASE` and `README.md` changes are not committed.
- [ ] Commit the implementation and push `master` to `origin`.
