# Route 12 Reverse W Command Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Map uppercase `W` to backward route 12 while retaining lowercase `w` for forward route 12.

**Architecture:** Keep the existing byte-oriented UART dispatcher. Split uppercase route commands so `U` and `V` retain their forward mappings while `W` calls the existing backward-route entry point; leave lowercase `u` through `w` unchanged.

**Tech Stack:** TASKING C99, PowerShell source-contract tests, Git

---

### Task 1: Add and implement the case-sensitive route command mapping

**Files:**
- Modify: `tests/check_portion2_route_layout.ps1:34-35`
- Modify: `user/cpu0_main.c:454-473`
- Modify: `docs/superpowers/specs/2026-06-29-route-10-12-serial-commands-design.md:9-19`

- [x] **Step 1: Write the failing test**

Replace the uppercase range assertion with separate checks:

```powershell
Assert-Contains $main "data\s*>=\s*'U'\s*&&\s*data\s*<=\s*'V'[\s\S]*?portion2_run_select_route\(data\s*-\s*'U'\s*\+\s*PORTION2_ROUTE_RETURN_5\)" 'serial U-V must run routes 10-11 forward'
Assert-Contains $main "data\s*==\s*'W'[\s\S]*?portion2_run_select_back_route\(PORTION2_ROUTE_SNAKE\)" 'serial W must run route 12 backward'
Assert-Contains $main "data\s*>=\s*'u'\s*&&\s*data\s*<=\s*'w'[\s\S]*?portion2_run_select_route\(data\s*-\s*'u'\s*\+\s*PORTION2_ROUTE_RETURN_5\)" 'serial u-w must run routes 10-12 forward'
```

- [x] **Step 2: Run the test to verify it fails**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/check_portion2_route_layout.ps1
```

Expected: FAIL with `serial U-V must run routes 10-11 forward` or `serial W must run route 12 backward` because uppercase `W` is still included in the forward range.

- [x] **Step 3: Implement the minimal command dispatch change**

Change the uppercase range to `U` through `V`, then add a `W` branch that performs the same command-state cleanup and invokes:

```c
portion2_run_select_back_route(PORTION2_ROUTE_SNAKE);
```

Keep the lowercase `u` through `w` branch unchanged.

- [x] **Step 4: Update the existing command documentation**

Document `U`/`V` as forward routes 10/11, `W` as backward route 12, and lowercase `w` as forward route 12.

- [x] **Step 5: Run verification**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tests/check_portion2_route_layout.ps1
powershell -ExecutionPolicy Bypass -File tools/verify_portion2_serial_debug.ps1
git diff --check
```

Expected: both scripts pass and `git diff --check` reports no whitespace errors.

- [x] **Step 6: Commit and push**

```powershell
git add tests/check_portion2_route_layout.ps1 user/cpu0_main.c docs/superpowers/specs/2026-06-29-route-10-12-serial-commands-design.md docs/superpowers/plans/2026-07-01-route-12-reverse-W.md
git commit -m "Map W to reverse route 12"
git push
```
