# Route Tracking Diagnostics Design

## Goal

Report where each autonomous route begins to deviate and how far the estimated vehicle position is from the route centerline, without saturating the debug serial port.

## Measurement

For the current planned route segment, project `portion_2.current_state` onto the segment and calculate the shortest distance to the clamped projection. Use the segment cross product to report `LEFT` or `RIGHT`; positive is left and negative is right. This is an estimated cross-track error. When GPS fusion is unavailable, it reflects encoder and IMU dead reckoning and may differ from the vehicle's physical displacement.

Calculate heading error as the normalized difference between the segment heading and `Yaw_1`.

## Output Frequency

Emit one `[P2-TRACK]` line when the mapped raw record-point index changes, approximately once per 0.4m. Emit the first point immediately. Do not emit one line for every 0.08m planned point.

Keep `[P2-RUN]` as a low-frequency detailed status line, reduced from 200ms to 1000ms. Keep GPS progress events only when the GPS binding index changes.

## Serial Format

```text
[P2-TRACK] route=5 raw=17/43 plan=97/253 side=RIGHT off=0.42 head_err=6.30 steer=-12.00 final=4.10 gps=OFF gps_err=2.50 status=BAD
```

- `raw` and `plan`: one-based progress and total counts.
- `side`: `LEFT`, `RIGHT`, or `CENTER` near zero.
- `off`: absolute cross-track error in meters.
- `head_err`: signed heading error in degrees.
- `steer`: current steering command in degrees.
- `final`: estimated distance to the final route point in meters.
- `gps`: fusion readiness, `ON` or `OFF`.
- `gps_err`: current GPS-to-inertial error in meters.
- `status`: `OK` below 0.30m and `BAD` at or above 0.30m.

## Run Summary

At run completion or explicit `S` stop, emit:

```text
[P2-TRACK-END] route=5 first_bad=17 max_raw=31 max_off=0.76
```

Use `first_bad=0` when no raw point reaches 0.30m. Reset the summary when a new route starts.

## Verification

Extend `tools/verify_portion2_serial_debug.ps1` to require the cross-track projection, raw-point event gating, 0.30m threshold, GPS and steering fields, run summary, one-second detailed status, and removal of per-planned-point reporting.
