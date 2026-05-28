# Smooth Guandao Route Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a planned smooth route for subject-one inertial navigation so auto driving follows a continuous reference path instead of hard-tracking every recorded point.

**Architecture:** Preserve `recode_map[]` as the original recorded route and add `planned_map[]` as the generated tracking route. Build the plan once when entering `portion_1()`, then make pure pursuit read from planned points when available.

**Tech Stack:** Embedded C for TC264, existing `guandao.h/.c`, existing math functions `hypotf`, `atan2f`, `sinf`, `tanf`.

---

### Task 1: Add smooth route storage

**Files:**
- Modify: `code/guandao.h`
- Modify: `code/guandao.c`

- [ ] Add `planned_map`, `planned_length`, and `plan_ready` to `guandao_state`.
- [ ] Initialize the new fields in `guandao_state_init()`.
- [ ] Add `guandao_build_smooth_plan()` declaration.

### Task 2: Generate smooth reference route

**Files:**
- Modify: `code/guandao.c`

- [ ] Add helpers to select planned route length and points.
- [ ] Implement Catmull-Rom interpolation from `recode_map[]` into `planned_map[]`.
- [ ] Keep first and final target points stable.
- [ ] Fall back to original recorded points if the route has fewer than three points.

### Task 3: Track planned route in auto mode

**Files:**
- Modify: `code/guandao.c`

- [ ] Build the smooth plan once in `portion_1()` after resolving the stop length.
- [ ] Make closest-point search, target point selection, preview point selection, final distance, and stop checks use the planned route when ready.
- [ ] Preserve existing `out_v_l`, `out_v_r`, `out_servo`, `azimuth_adjust()`, and final deceleration behavior.

### Task 4: Verify build-level consistency

**Files:**
- Read-only check: `code/guandao.h`, `code/guandao.c`

- [ ] Search for all route point accesses and confirm planned/original selection is intentional.
- [ ] Run a compiler/build command if available in the IDE project; otherwise inspect for C syntax issues.
