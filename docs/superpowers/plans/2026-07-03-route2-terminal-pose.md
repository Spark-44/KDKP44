# Routes 1-10 Terminal Pose Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make routes 1-10 converge to their recorded endpoint heading without changing routes 11-12.

**Architecture:** Use one shared terminal steering helper for routes 1-10. It blends path steering with final-yaw feedback inside the final 2.0 m, applies a 0.35 m yaw-alignment entry distance, arms overshoot detection at 1.50 m, and slows large terminal heading errors to 0.8 m/s.

**Tech Stack:** TASKING C99, PowerShell static regression tests.

---

### Task 1: Route 2 terminal pose control

**Files:**
- Modify: `code/guandao.c`
- Create: `tests/check_route2_terminal_pose.ps1`

- [ ] **Step 1: Write the failing test**

Check for route-2 terminal start/full distances, the final-yaw steering blend helper, its call before final steering limiting, and a route-2 stop distance of 0.35 m.

- [ ] **Step 2: Run test to verify it fails**

Run: `powershell -NoProfile -ExecutionPolicy Bypass -File tests/check_route2_terminal_pose.ps1`

Expected: FAIL because route-2 terminal pose symbols do not exist.

- [ ] **Step 3: Write minimal implementation**

Add a helper that clamps a final-yaw steering command, blends it with path steering from 2.0 m to 1.0 m, and returns the original command for every route except route 2. Select 0.35 m as route 2's final alignment distance.

- [ ] **Step 4: Run tests**

Run the new check and all existing `tests/*.ps1` checks. The new check and previously passing checks must pass.

- [ ] **Step 5: Commit**

Commit only the route-2 implementation, test, design, and plan without staging unrelated working-tree changes.
