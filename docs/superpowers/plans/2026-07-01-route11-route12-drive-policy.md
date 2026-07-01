# Route 11 and Route 12 Drive Policy Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make route 11 a same-order reverse-driving recorded route, keep route 12 same-order forward, and align serial and voice commands with the new behavior.

**Architecture:** Centralize route-specific drive direction in `guandao.c`; route point order remains controlled only by `portion2_run_reverse`. Route 11 sets only `portion2_run_drive_reverse`, while command handlers call the ordinary route-selection API.

**Tech Stack:** TASKING C99 for Infineon AURIX TC26B, PowerShell source-policy regression check.

---

### Task 1: Add the route drive policy

**Files:**
- Modify: `code/guandao.c`

- [ ] Add a route-11 reverse-drive policy helper.
- [ ] Apply it when route 11 recording starts and reset it when recording stops.
- [ ] Apply it in `portion2_run_select_route` without setting `portion2_run_reverse`.
- [ ] Use reverse motion heading for route 11 terminal path shaping.

### Task 2: Align serial and voice mappings

**Files:**
- Modify: `user/cpu0_main.c`

- [ ] Map `V/v` to ordinary route 11 selection and `W/w` to ordinary route 12 selection.
- [ ] Keep `Q/q` and `R/r` mapped to encoder-and-yaw forward/reverse actions.
- [ ] Map both voice command groups consistently to fixed straight actions and routes 11/12.

### Task 3: Verify and publish

**Files:**
- Test: temporary PowerShell source-policy check

- [ ] Verify the policy check fails before implementation.
- [ ] Verify it passes after implementation.
- [ ] Run `git diff --check` and inspect the focused diff.
- [ ] Commit and push the implementation.
