# Drive Mode Design

## Goal

Add a third main mode for manually driving straight with stepped speed control, while preserving the existing RECORD and RUN workflows.

## Mode Transitions

- Power-on remains in RECORD mode.
- RECORD + short K4: enter RUN mode.
- RECORD + K4 held longer than 1.5 seconds: enter DRIVE mode.
- RUN + short K4: stop and return to RECORD mode.
- RUN + long K4: no mode change.
- DRIVE + long K4: stop immediately and return to RECORD mode.
- DRIVE + short K4: no mode change.
- RUN and DRIVE cannot transition directly to each other.

K4 produces explicit `NONE`, `SHORT`, and `LONG` events. A long event fires once after the threshold and remains locked until the key is released, preventing the release from also becoming a short press.

## Drive Behavior

- Entering DRIVE always resets target speed to `0.0 m/s`.
- Short K2 increases target speed by `1.0 m/s`.
- Short K1 decreases target speed by `1.0 m/s`.
- Target speed is clamped to `0.0-5.0 m/s`; reverse is not allowed.
- Both rear wheels receive the same target speed.
- Steering target remains at `0 degrees`.
- Entering or leaving DRIVE clears stale key events and stops any route, fixed action, or auxiliary action.
- Leaving DRIVE writes zero wheel and steering outputs and calls the rear-motor stop API before entering RECORD.

The existing rear-motor conversion remains authoritative: meters per second are converted through `GUANDAO_SPEED_TO_MPS` before being written to `out_v_l` and `out_v_r`.

## User Interface

The DRIVE screen displays the mode name and current target speed. It does not reuse RUN route-progress content.

## Files

- `code/display.h`: add the DRIVE main-mode enum value.
- `code/guandao.h` and `code/guandao.c`: expose explicit K4 mode events and preserve the existing mode-key lock.
- `user/cpu0_main.c`: add DRIVE entry, task, key handling, display, and safe exit; remove the accidental `+++++++++++++` suffix from the first include.
- `tests/check_portion2_route_layout.ps1` or a focused new PowerShell check: protect mode transitions, speed limits, steering centering, and safe stop behavior.

## Verification

- A regression check must fail before implementation because DRIVE does not exist.
- Static checks must confirm all legal transitions and reject direct RUN/DRIVE transitions.
- Static checks must confirm initial speed zero, one-meter-per-second steps, zero/five-meter-per-second clamps, and zero steering.
- Existing route-layout checks must continue to pass against the staged source snapshot.
- The project build should be run when the TASKING toolchain is available.
