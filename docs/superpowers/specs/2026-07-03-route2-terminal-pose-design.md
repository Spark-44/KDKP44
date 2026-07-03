# Routes 1-10 Terminal Pose Design

## Goal

Prevent routes 1-10 from turning sharply beside the endpoint and stopping with a large heading error.

## Design

- Apply the terminal steering blend to routes 1-10 during the final 2.0 m.
- Blend the normal path steering toward a steering command derived from the recorded final yaw.
- Reach full final-yaw control within the last 1.0 m so a near-end lookahead point cannot keep commanding a hard turn after the vehicle has reached the recorded heading.
- Allow routes 1-10 to enter final-yaw alignment at 0.35 m and arm overshoot detection at 1.50 m.
- Limit routes 1-10 to 0.8 m/s within the final 3.0 m when heading error reaches 20 degrees.
- Keep the existing 0.6 m/s terminal speed, 0.4 m/s yaw-alignment speed, and overshoot safety stop.
- Do not change the dedicated route 11 and route 12 behavior.

## Verification

- A static regression test checks the route-2-only constants, steering blend, and 0.35 m alignment threshold.
- Existing final-zone, GPS, IMU, and route-layout checks remain unchanged.
