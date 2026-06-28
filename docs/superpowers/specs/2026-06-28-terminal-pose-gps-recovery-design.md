# Terminal Pose and GPS Recovery Design

## Goal

Stop each portion-2 route near its recorded endpoint with the recorded vehicle heading, while preventing unstable GPS data from corrupting tracking and allowing fusion to recover after temporary outliers.

## Terminal Pose

- Store start yaw and final yaw independently for all nine routes.
- Capture start yaw when K3 begins recording and final yaw when K3 ends recording.
- Persist both yaw arrays in the inertial Flash page under a new route-layout magic value; old routes are invalid and must be recorded again.
- Rotate the saved terminal yaw by the same alignment delta used for route coordinates.
- Shape the final 1.5 m of the route as a cubic Bezier segment ending at the saved position and saved yaw.
- Stop only within 0.20 m and 5 degrees. A low-speed steering correction that moves farther than 0.25 m from the endpoint returns to positional pursuit.

## GPS Quality and Recovery

- Parse GGA HDOP and expose it in serial diagnostics.
- Reject recording and fusion fixes with HDOP above 2.5 or zero HDOP.
- Require five startup fixes stable within 0.6 m and reject startup translation greater than 3 m.
- After three errors above 3 m, suspend corrections instead of permanently abandoning the transform.
- Re-enable fusion after three consecutive fixes within 2 m, then resume the existing bounded correction.

## Diagnostics

Record logs include captured start/final yaw. Runtime GPS logs include HDOP and recovery progress. Runtime end logs continue to expose target and actual yaw.

## Compatibility

Route count, 49 inertial points, 20 GPS points, 0.4 m inertial spacing, 1 m GPS spacing, UART wiring, and normal pursuit behavior remain unchanged.
