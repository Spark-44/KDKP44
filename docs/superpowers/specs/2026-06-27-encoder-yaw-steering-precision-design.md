# Encoder-Yaw Steering Precision Design

## Goal

Make the `Q` and `R` 10 m actions visibly and continuously correct small heading errors using the steering motor, without changing the recorded-route pursuit parameters.

## Root Cause

The straight controller calculates fractional steering commands, but `Steer_Moter_Contral()` casts them to `int32` before passing them to the rack controller. A command such as `-0.8` degrees therefore becomes `0`. The Q/R controller is also PD-only, so a persistent vehicle bias requires a persistent heading error before it can produce the steering angle that balances the bias.

## Design

- Change the rack target API to accept `float`, preserving fractional steering commands through the actuator boundary.
- Add a Q/R-only yaw integral with a fixed limit. Reset it on every start and stop, and stop integrating while the steering output is saturated in the same direction.
- Keep the existing proportional, derivative, steering-angle and steering-rate limits.
- Add read-only rack telemetry getters and include target angle, actual angle and output PWM in each `[STRAIGHT]` line.
- Leave route 1-9 pursuit gains and limits unchanged.

## Verification

- Static regression checks must prove fractional targets are not cast to integers, Q/R has bounded integral control, and telemetry crosses the actuator boundary.
- Compile the changed C modules with the host C99 syntax harness.
- Run the complete portion 2 serial verification script and `git diff --check` before commit and push.
