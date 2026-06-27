# GPS Record Rejection Diagnostics Design

## Goal

Make every failed automatic GPS record attempt explainable from the screen and serial log while preserving the existing route-recording behavior and GPS quality thresholds.

## Design

- Track one explicit rejection reason for each gate: no fix, low satellites, zero coordinate, inertial interval, repeated coordinate, jump, capacity, or index error.
- Rate-limit `[P2-REC-GPS-SKIP]` to once per second and include route, reason, fix state, satellite count, coordinate validity, GNSS update sequence, and GPS count.
- Expose the GNSS update sequence from `gps.c`; it increments whenever parsed GNSS data reaches `update_gpsinformation()`.
- Show `FIX`, `SAT`, coordinate validity, update sequence, and the latest compact rejection reason on record-screen rows 8-11.
- Continue inertial recording when GPS is invalid. Do not lower the eight-satellite minimum or alter distance/jump filters.

## Verification

Static regression checks cover every rejection reason, serial fields, screen fields, update-sequence API, rate limiting, and continued inertial recording. The complete portion 2 regression script and C99 syntax checks must pass before publishing.
