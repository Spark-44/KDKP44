# Portion 2 Progress Display Design

## Goal

Make route recording saturation and route-following loss easy to locate from both the IPS screen and the debug serial port.

## Record Mode

- Show the selected route and record state as before.
- Show inertial record progress as `RAW: current/39`.
- Show GPS record progress as `GPS: current/5`.
- Show the active `recode_threshold` and accumulated inertial route distance so a threshold or odometry calibration error is visible during recording.
- Emit a compact periodic `[P2-REC-STATUS]` serial line while recording, in addition to the existing per-point and per-GPS event lines.

## Run Mode

- Show the original recorded-point position as `RAW: current/total` using human-readable one-based numbering.
- Show the smoothed plan position as `PLAN: current/total`.
- Show passed GPS progress as `GPS: current/total`.
- Add the same human-readable progress fields to the existing periodic `[P2-RUN]` serial line while retaining existing diagnostic fields for compatibility.

## Numbering And Boundaries

- Point numbers shown to the user start at 1.
- Empty routes show point 0.
- Completed routes clamp progress to their total rather than displaying a value beyond the end.
- Existing storage limits and route-following behavior are unchanged.

## Verification

- Extend the source verification script first so it fails until all record and run progress fields exist.
- Run the verification script after implementation.
- Attempt the local TASKING build when available and report toolchain availability honestly.
