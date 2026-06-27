# Encoder-Yaw Straight Actions Design

## Goal

Add two serial-only fixed actions that travel a measured 10 metres in a straight line without recorded route points or GPS:

- `Q` or `q`: drive forward 10 metres.
- `R` or `r`: keep the vehicle heading unchanged and reverse 10 metres.

Both actions run at a maximum speed of 0.35 m/s. Existing serial commands, including `I/J`, retain their current behavior.

Route 8 remains a recorded-point route. Forward and reverse route 8 execution uses inertial route tracking but does not use or require GPS.

## Command Integration

The ASCII command dispatcher starts the two new actions only while the application is in run mode. Starting either action stops any active recorded route or fixed action first. Existing `S`, K3, and the run-to-record K4 transition stop the action through the existing common stop path.

The fixed-action mode enumeration gains distinct encoder-yaw forward and reverse modes. They reuse the fixed-action task entry point but have separate state and control handling from the existing time-integrated `I/J` actions.

## Distance Measurement

At action start, initialize and sample the same left/right encoder odometry used by inertial route tracking. On each task update:

1. Read the incremental left and right encoder counts.
2. Convert each increment with `ONE_TICK_DISTANCE`.
3. Accumulate the absolute average wheel travel.
4. Treat 10.0 metres of accumulated travel as the stop target.

Distance must come from measured encoder increments. Commanded speed multiplied by elapsed time is not used for these two modes.

## Heading Control

Capture `Yaw_1` when the action starts and use it as the target vehicle heading for the entire action. Each update normalizes `target_yaw - Yaw_1` to `[-180, 180]`.

Use a PD steering controller with:

- a small heading-error deadband to prevent steering chatter;
- proportional correction for steady heading error;
- derivative damping from the change in heading error;
- steering-angle and steering-rate limits;
- zero integral term, avoiding accumulated steering bias while the vehicle is stopped.

Forward motion applies the normal steering correction. Reverse motion applies the opposite physical steering sign because the front steering axle acts behind the direction of travel. Both modes keep the captured vehicle nose heading unchanged.

Initial gains and limits are conservative constants in the fixed-action module and are exposed in serial diagnostics so they can be tuned from test logs.

## Speed Profile And Stop

Use 0.35 m/s through the main part of the action. Reduce speed during the final approach so encoder quantization and motor stopping distance do not cause a large overshoot. At or beyond 10.0 metres:

- set rear motor target to zero;
- set left/right command speed and steering output to zero;
- clear the active fixed-action mode.

A generous safety timeout stops the vehicle if encoder distance does not progress, preventing an endless run after an encoder failure. The normal emergency stop paths remain authoritative.

## Route 8 GPS Policy

Route 8 continues to load, reverse, smooth, and follow its saved inertial points exactly like the other recorded routes. Its special policy is limited to GPS:

- route readiness requires inertial points but not GPS points;
- GPS transform startup calibration is skipped;
- runtime GPS fusion updates are disabled;
- route 8 starts tracking immediately instead of entering the GPS calibration state.

GPS behavior for routes 1-7 and route 9 remains unchanged.

## Diagnostics

Emit a periodic `[STRAIGHT]` serial line containing direction, encoder distance, target yaw, current yaw, normalized yaw error, steering command, and speed command. Emit explicit `START`, `STOP`, `TIMEOUT`, and encoder-stall reasons.

The run screen may show the active fixed-action mode through the existing fixed-action fields; no new page or mode is introduced.

## Verification

Static and host-side checks cover:

- `Q/q` and `R/r` command dispatch without collisions;
- encoder-based distance accumulation for only the new modes;
- captured-yaw PD correction and reverse steering sign;
- slowdown and stop at 10 metres;
- common stop behavior;
- route 8 bypassing GPS requirements and fusion;
- all other routes retaining GPS preparation and runtime fusion.

On-vehicle verification records one forward and one reverse run on level ground. Confirm the serial distance reaches approximately 10 metres, yaw error remains bounded, the reverse action keeps the nose orientation unchanged, and route 8 emits no GPS-fusion corrections.
