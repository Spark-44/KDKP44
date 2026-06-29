# Route 10-12 Serial Commands Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add collision-free single-letter serial commands for running routes 10, 11, and 12.

**Architecture:** Extend the existing byte-oriented ASCII command dispatcher without changing its state model. Add static regression assertions to the existing route-layout test so the mapping remains documented and protected.

**Tech Stack:** TASKING C99, PowerShell static regression checks, Git.

---

### Task 1: Protect the new serial mapping

**Files:**
- Modify: `tests/check_portion2_route_layout.ps1`

- [ ] **Step 1: Write the failing checks**

Add assertions requiring uppercase `U-W` and lowercase `u-w` branches to normalize their input and call `portion2_run_select_route()` with route IDs 9 through 11.

```powershell
Assert-Contains $main "data\s*>=\s*'U'\s*&&\s*data\s*<=\s*'W'[\s\S]*?portion2_run_select_route\(data\s*-\s*'U'\s*\+\s*PORTION2_ROUTE_RETURN_5\)" 'serial U-W must run routes 10-12'
Assert-Contains $main "data\s*>=\s*'u'\s*&&\s*data\s*<=\s*'w'[\s\S]*?portion2_run_select_route\(data\s*-\s*'u'\s*\+\s*PORTION2_ROUTE_RETURN_5\)" 'serial u-w must run routes 10-12'
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/check_portion2_route_layout.ps1
```

Expected: FAIL with `serial U-W must run routes 10-12` because the mapping does not exist yet.

### Task 2: Implement the byte mappings

**Files:**
- Modify: `user/cpu0_main.c:306`

- [ ] **Step 1: Add the minimal uppercase and lowercase branches**

Insert before the existing `A-H` auxiliary-action branch:

```c
    else if(data >= 'U' && data <= 'W')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        voice_drive_action_stop();
        portion2_run_select_route(data - 'U' + PORTION2_ROUTE_RETURN_5);
    }
    else if(data >= 'u' && data <= 'w')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        voice_drive_action_stop();
        portion2_run_select_route(data - 'u' + PORTION2_ROUTE_RETURN_5);
    }
```

`PORTION2_ROUTE_RETURN_5` is zero-based route ID 9, so the expressions map `U/u`, `V/v`, and `W/w` to IDs 9, 10, and 11.

- [ ] **Step 2: Run the focused test and verify GREEN**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/check_portion2_route_layout.ps1
```

Expected: `portion-2 12-route layout checks passed`.

- [ ] **Step 3: Run the broader serial diagnostics checks**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify_portion2_serial_debug.ps1
```

Expected: all checks pass with exit code 0.

- [ ] **Step 4: Inspect and commit only feature hunks**

Run `git diff --check`, inspect `git diff`, stage the plan, test file, and only the new command-dispatch hunk from `user/cpu0_main.c`, then commit with:

```powershell
git commit -m "Add serial commands for routes 10-12"
git push origin master
```
