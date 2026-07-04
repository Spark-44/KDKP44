# Route 13 and Route 14 Gyroscope/Encoder Design

## Goal

Add two compiled-in routes extracted from
`records-2026-07-04-17-36-23.json`. These routes run from encoder distance and
gyroscope yaw only. They do not require route recording, inertial `x/y` points,
GPS, or Flash route data.

Existing routes 11 and 12, including their recording, storage, GPS, commands,
and run behavior, remain unchanged.

## Extracted Profiles

- Route 13 is derived from recorded route 11. It contains all 42 recorded yaw
  samples, spans approximately 16.75 metres, and runs backward.
- Route 14 is derived from recorded route 12. It contains all 44 recorded yaw
  samples, spans approximately 17.71 metres, and supports forward and backward
  drive.
- Each profile entry contains cumulative encoder distance and yaw relative to
  the first sample. Absolute recorded headings near plus or minus 90 degrees are
  not stored as runtime targets.
- Cumulative distances are calculated from consecutive recorded `x/y` samples.
  The `x/y` values are used once for offline extraction only; runtime control
  does not consume them.
- Relative yaw is unwrapped before interpolation so a transition across
  plus/minus 180 degrees remains continuous.

## Architecture

Add a small, independent gyroscope-route controller beside the existing fixed
action controller. The controller owns:

- immutable route 13 and route 14 distance/yaw profiles;
- current route, direction, start yaw, encoder baseline, distance, and PID
  state;
- profile interpolation, steering control, slowdown, stop, and safety logic;
- serial diagnostics for start, running, stop, and rejection events.

The existing recorded-route arrays and Flash layout are not expanded. Route 13
and route 14 are logical compiled-in route IDs and are not selectable in the
recording menu. This avoids invalidating or overwriting any saved route 11/12
data.

## Runtime Data Flow

1. A serial command stops any active recorded route or fixed action, then starts
   the requested gyroscope route and direction.
2. The controller captures the current yaw and encoder count as the runtime
   origin.
3. Each task update converts the absolute encoder delta to travelled distance.
4. The controller finds the two profile samples surrounding that distance and
   linearly interpolates the unwrapped relative yaw.
5. Runtime target yaw equals start yaw plus interpolated relative yaw, normalized
   for comparison with the live gyroscope yaw.
6. A bounded PID controller converts yaw error to steering. Reverse drive
   negates the steering correction.
7. The controller commands a signed speed, reduces speed near the terminal
   distance, and stops at the profile end.

Route 13 always uses reverse drive. Route 14 uses forward drive for `y` and
reverse drive for `Y`. Reverse route 14 follows the same relative-yaw schedule
while travelling backward; steering correction is inverted for reverse vehicle
dynamics.

## Commands

- `X` or `x`: start route 13 in reverse.
- `y`: start route 14 forward.
- `Y`: start route 14 in reverse.
- `S`: stop the gyroscope route as well as existing route and fixed-action
  controllers.

The new dispatch must not change the mappings of `V/v` (route 11) or `W/w`
(route 12).

## Control and Safety

- Reuse the proven encoder distance conversion and yaw-control structure from
  the existing encoder/yaw fixed actions, without coupling the new profiles to
  recorded-route pursuit.
- Clamp implausible encoder deltas so a counter discontinuity cannot skip a
  section or finish a route.
- Stop with a `STALL` reason if meaningful encoder progress is absent for three
  seconds while a route is active.
- Rate-limit steering changes and clamp the final steering command.
- Reduce speed over the final section and command zero motor speed and zero
  steering on completion, cancellation, or rejection.
- Starting a recorded route or another fixed action must cancel an active
  gyroscope route, so only one drive controller owns the outputs.

## Diagnostics

Serial output uses a dedicated `[GYRO-ROUTE]` prefix. A running diagnostic line
includes route number, direction, travelled/total distance, target yaw, current
yaw, yaw error, steering command, encoder delta, and stop reason. Output is
rate-limited during normal running and forced for start and stop events.

## Testing

Static and host-side checks will verify:

- route 13 has 42 monotonic-distance samples and route 14 has 44;
- the extracted endpoints and relative-yaw values match the supplied serial
  record;
- interpolation, angle unwrapping, and normalization handle plus/minus 180
  degrees correctly;
- route 13 is always reverse, while `y` and `Y` select route 14 forward and
  reverse respectively;
- reverse steering correction has the opposite sign from forward correction;
- slowdown, terminal stop, stall stop, and encoder-jump rejection work;
- `S` stops the new controller;
- existing route 11/12 command mappings and implementation remain intact.

The existing project verification scripts are run as regressions after the new
focused checks pass.
