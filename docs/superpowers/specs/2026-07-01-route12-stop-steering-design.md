# Route 12 Stop and Steering Design

## Scope

Apply the following behavior only when `portion2_selected_route` is route 12.
Routes 1 through 11 retain their existing limits and stop behavior.

## Steering

- Normal route-12 steering limit: 20 degrees.
- Sharp-turn route-12 steering limit: 30 degrees.
- Existing steering rate limiting remains enabled.

## Final Stop

- Route-12 final-point stop radius: 0.40 m.
- Route-12 final yaw alignment may operate inside the same 0.40 m radius.
- Other routes retain the 0.20 m final-point stop radius and 0.25 m yaw
  reacquisition radius.

## Overshoot Protection

When route 12 is on its final raw point and within 0.80 m of the endpoint, arm
an overshoot monitor and track the minimum endpoint distance. If the distance
then exceeds that minimum by at least 0.15 m for three consecutive control
cycles, stop immediately and report stop reason 12.

Reset this monitor when a run starts, is stopped, completes, or another route is
selected. The 0.15 m margin is larger than the configured single GPS correction
cap of 0.10 m, reducing false triggers from one GPS update.

## Verification

Source-contract tests verify route-specific steering limits, stop radius,
overshoot thresholds, consecutive-cycle filtering, and state reset paths.
