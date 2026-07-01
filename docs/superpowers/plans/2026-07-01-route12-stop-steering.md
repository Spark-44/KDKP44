# Route 12 Stop and Steering Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make route 12 turn more strongly and stop reliably after reaching or passing its endpoint.

**Architecture:** Select route-specific steering and stopping constants inside the existing portion-2 pursuit controller. Add a small resettable overshoot state machine that observes final distance only on route 12's last raw point.

**Tech Stack:** TASKING C99, PowerShell source-contract tests

---

### Task 1: Add failing route-12 behavior checks

- [x] Assert the 20/30 degree route-12 steering limits.
- [x] Assert the 0.40 m stop and yaw-alignment radius.
- [x] Assert the 0.80 m arm distance, 0.15 m rise, and three-cycle filter.
- [x] Run `tests/check_portion2_route_layout.ps1` and verify failure.

### Task 2: Implement and verify

- [x] Add route-12 constants and overshoot state.
- [x] Apply route-specific steering and final-distance selection.
- [x] Stop with reason 12 after confirmed overshoot.
- [x] Reset the monitor on start, stop, and completion.
- [x] Run the focused checks and `git diff --check`.
- [x] Commit and push to `master`.
