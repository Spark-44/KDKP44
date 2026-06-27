# Reset-Safe Route Start Design

## Goal

Make recorded routes start safely after an MCU reset by rebuilding the route yaw frame from the current vehicle yaw and calibrating GPS translation before motion.

## Route Frame

Every forward or reversed route is translated to its local origin and then rotated so its initial path tangent matches the current `Yaw_1`. GPS transform preparation runs after this alignment and after reference smoothing. GPS availability no longer decides whether yaw alignment occurs.

For a reversed route this assumes the vehicle is physically facing the desired return direction before the command. Route reversal remains forward driving; it does not perform an automatic U-turn.

## GPS Startup Calibration

When the stored GPS anchors can build a transform, hold motor and steering output at zero for at most 6 seconds. Collect three distinct fixes with at least eight satellites. Consecutive startup samples must remain within 1 metre of the running mean.

After three stable samples, shift the GPS transform translation so their average maps to the route origin. If calibration times out or remains unstable, disable GPS fusion for this run and continue using encoder and gyro navigation.

## Runtime Fallback

During route execution, count consecutive transformed GPS errors above 3 metres. Three consecutive large errors disable GPS fusion for the remainder of the run. Repeated coordinates and temporary low satellite counts do not move the vehicle state and do not independently disable fusion.

## Diagnostics

Serial output reports startup calibration progress, success, timeout fallback, and runtime disable. The existing fusion status API reports `ready=0` after fallback.

## Verification

Static checks verify unconditional yaw alignment before GPS preparation, the stopped calibration state, three-sample/6-second calibration, origin translation adjustment, and three-error runtime fallback.
