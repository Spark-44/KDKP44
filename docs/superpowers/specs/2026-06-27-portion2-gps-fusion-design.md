# Portion 2 GPS Fusion Design

## Goal

Use recorded GPS anchors as a low-frequency absolute position reference for portion 2 while preserving smooth inertial control between GNSS updates.

## Route Frame

- Recording and running occur during the same power session, so recorded and live IMU yaw share one reference.
- Translate each selected route so its first raw point is the local origin.
- When a valid GPS transform is available, keep the recorded route orientation and do not rotate the route to the current start yaw.
- When GPS preparation fails, retain the existing yaw-aligned inertial fallback.

## GPS Transform

- Store GPS bind indices as zero-based raw point indices.
- At run start, pair each distinct recorded GPS coordinate with its bound raw route point.
- Convert latitude/longitude deltas to local east/north metres.
- Fit a two-dimensional similarity transform from GPS metres to route-local coordinates using all valid anchors.
- Require at least four distinct anchors, a useful baseline, a reasonable scale, and bounded fit residual.

## Runtime Fusion

- Process each newly parsed GNSS fix at most once.
- Require a valid GNSS state, at least eight satellites, nonzero coordinates, and a prepared transform.
- Reject repeated coordinates and position disagreements greater than 3 metres.
- Blend 10 percent of accepted GPS position error into inertial `x/y`, capped at 0.1 metre per GNSS update.
- Keep IMU yaw unchanged.
- Fall back automatically to encoder and IMU navigation whenever a fix is rejected.

## Diagnostics

- Emit `[P2-GPS-FUSION]` for accepted and rejected fixes with reason, satellites, GPS-derived position, inertial position, error, and applied correction.
- Show `GF ON/OFF`, fix validity, and current GPS error on the run screen.
- Preserve existing `[P2-RUN]`, RAW, PLAN, and GPS progress output.

## Storage Compatibility

- Change the passage GPS layout marker because bind indices change from count-based to zero-based.
- Invalidate old GPS counts on first boot with the new firmware, retaining raw inertial points but requiring routes to be recorded and saved again.
