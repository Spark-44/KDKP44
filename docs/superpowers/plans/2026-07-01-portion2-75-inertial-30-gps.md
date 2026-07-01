# Portion 2 75 Inertial and 30 GPS Capacity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand each of the 12 portion-2 routes to 75 inertial points and 30 GPS points with persistent two-page storage for each data type.

**Architecture:** Keep the embedded runtime arrays at 588 inertial and 240 GPS records, then add dedicated tails of 312 inertial and 120 GPS records behind accessor functions. Keep inertial persistence on pages 9 and 7, split GPS persistence between pages 8 and 6, and validate both GPS page headers before accepting saved data.

**Tech Stack:** TASKING C99, TC264 data-flash API, PowerShell source-contract tests, Git

---

### Task 1: Define and test the expanded capacities

**Files:**
- Modify: `tests/check_portion2_route_layout.ps1:16-24`
- Modify: `code/guandao.h:6-13`

- [x] **Step 1: Add failing capacity assertions**

Require `PORTION2_ROUTE_MAX_POINTS` 75 and 900 total route points while retaining `MAX_LENGTH_INDEX` 588 with a 312-point extension. Require `PORTION2_GPS_PER_ROUTE` 30 and 360 total GPS records while retaining `MAX_GPS_RECODE` 240 with a 120-record extension.

- [x] **Step 2: Run the layout test and verify it fails**

Run `powershell -ExecutionPolicy Bypass -File tests/check_portion2_route_layout.ps1` and expect failure on the new capacity assertions.

- [x] **Step 3: Update the capacity macros**

Set the logical capacity and extension macros in `code/guandao.h` without changing route count or recording intervals.

### Task 2: Split GPS persistence across two flash pages

**Files:**
- Modify: `tests/check_portion2_route_layout.ps1:23-24`
- Modify: `code/flash.c:20-44,164-320`
- Modify: `code/flash.h:14-19`

- [x] **Step 1: Add failing flash-layout assertions**

Require a GPS continuation marker, primary/continuation record counts, page 6 read/write calls, and compile-time checks for both GPS pages.

- [x] **Step 2: Run the test and verify it fails**

Run the layout test and expect failure because GPS currently assumes one page.

- [x] **Step 3: Add two-page GPS constants and guards**

Use 254 records on page 8 and 106 records on page 6. Define separate continuation header indexes and reject layouts larger than either 1020-word page.

- [x] **Step 4: Write both GPS pages**

Write records `0..253` to page 8 and records `254..359` to page 6. Store the total high-water mark on page 8 and the continuation count on page 6.

- [x] **Step 5: Read and validate both GPS pages**

Accept GPS data only when the route layout, primary GPS marker, total count, continuation marker, and continuation count all match. Read each page into its corresponding record range.

- [x] **Step 6: Verify, commit, and push**

Run the layout test and `git diff --check`. Inspect the final diff, commit as `Expand route point and GPS capacity`, and push `master` as previously requested.
