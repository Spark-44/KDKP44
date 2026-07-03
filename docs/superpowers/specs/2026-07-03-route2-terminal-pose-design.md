# Route 2 Terminal Pose Design

## Goal

Prevent route 2 from turning sharply beside the endpoint and stopping with a large heading error.

## Design

- Apply a route-2-only terminal steering blend during the final 2.0 m.
- Blend the normal path steering toward a steering command derived from the recorded final yaw.
- Reach full final-yaw control within the last 1.0 m so a near-end lookahead point cannot keep commanding a hard turn after the vehicle has reached the recorded heading.
- Allow route 2 to enter the existing final-yaw alignment at 0.35 m instead of 0.20 m.
- Keep the existing 0.6 m/s terminal speed, 0.4 m/s yaw-alignment speed, and overshoot safety stop.
- Do not change routes 1 or 3-12.

## Verification

- A static regression test checks the route-2-only constants, steering blend, and 0.35 m alignment threshold.
- Existing final-zone, GPS, IMU, and route-layout checks remain unchanged.
