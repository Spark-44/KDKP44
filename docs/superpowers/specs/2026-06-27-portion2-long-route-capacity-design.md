# Portion 2 Long Route Capacity Design

## Goal

Support nine routes of up to 49 inertial points and 20 GPS points each while keeping the inertial spacing at 0.4 m and automatic GPS spacing at 1.0 m.

## Storage Layout

- Increase each route from 39 to 49 inertial points, giving a nominal maximum distance of 19.2 m.
- Increase each route from 14 to 20 GPS points, reserving the final slot for the endpoint as before.
- Increase the shared inertial array to 441 points and GPS array to 180 records.
- Add compile-time guards proving the shared arrays and both Flash pages are large enough.
- Change the GPS layout magic and rely on the new total inertial count to reject the old route layout. Existing saved routes must be recorded again.

## Behavior

Recording, filtering, required minimum GPS count, screen progress, serial progress, and one-metre automatic GPS spacing remain unchanged. Short routes record only the number of points their distance requires; 20 is a capacity, not a forced count.

## Verification

Regression checks require all new capacities, Flash versioning and compile-time guards. The full portion 2 verifier and whitespace checks must pass before commit and push.
