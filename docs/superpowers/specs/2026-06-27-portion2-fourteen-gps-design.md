# Portion 2 Fourteen GPS Points Design

## Goal

Increase each of the nine portion-2 routes from 8 to 14 GPS anchors without reducing the existing 39 inertial points per route.

## Storage

Keep route coordinates and route metadata on `RECODE_PASSAGE` (page 9). Store all 126 GPS records on `RECODE_PASSAGE_TWO` (page 8), with a versioned magic value and the total GPS count in that page. A missing or old GPS-page marker invalidates only GPS metadata; route coordinates remain readable.

Each GPS record keeps latitude, longitude, recorded yaw, and bound raw-point index. Saving writes the route page first and the GPS page second. Loading accepts route GPS counts only when the second page marker and total count are valid.

## Recording

Record at most 14 GPS points per route. A candidate must have a valid GNSS fix, enough satellites, nonzero coordinates, and be at least 0.20 m from the previous accepted GPS coordinate. Rejected stale fixes do not consume capacity.

When recording stops, attempt to append the current valid fix if it is distinct, so the final route section has a GPS anchor. Automatic recording keeps the existing 1.0 m inertial spacing.

## Run Validation

Keep the existing minimum of five GPS anchors so short routes remain usable. Additionally require the last GPS anchor to be bound within 4 raw points of the route end. This ensures the final 1.6 m of a route has recorded GPS support at the 0.4 m inertial step.

## Compatibility

The storage layout change invalidates existing portion-2 GPS records. All nine routes must be recorded and saved again after flashing the new firmware.

## Verification

Static verification will check capacities, the second-page read/write path, duplicate filtering, final-point capture, endpoint coverage validation, and absence of hard-coded eight-point assumptions. The project verification scripts and available compiler checks will be run before commit and push.
