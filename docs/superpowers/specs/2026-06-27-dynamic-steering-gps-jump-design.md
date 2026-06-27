# Dynamic Steering And GPS Jump Filter Design

## Goal

Allow portion-2 routes to use enough steering for recorded sharp bends while preserving the smooth 12-degree behavior on straight routes, and reject implausible GPS jumps during recording.

## Steering

The controller will inspect the smoothed raw reference route rather than tracking error. It maps the current planned point back to its raw point and scans the next six raw points. If the largest local reference turn is at least 4 degrees, the steering command limit is 20 degrees; otherwise it remains 12 degrees.

This keeps route 2 and route 3 calm when their reference geometry is straight. Cross-track or heading error alone cannot unlock 20 degrees. Existing steering rate limiting remains active when entering or leaving a sharp bend.

## GPS Recording

The existing minimum 0.20 m movement check remains. For every later GPS anchor, compare GPS displacement from the previous accepted fix with inertial displacement from the vehicle state captured with that fix. Reject the candidate when GPS displacement exceeds inertial displacement plus 2.0 m.

This rejects the observed 4.7 m endpoint jump after less than 1 m of vehicle motion, while allowing a delayed GPS fix to catch up after the vehicle has traveled several meters.

## Verification

Static checks will verify the 12/20-degree limits, route-curvature trigger, six-point lookahead, and GPS-versus-inertial jump test. Existing portion-2 verification remains the regression suite.
