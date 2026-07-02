# GPS Quality Filter Design

## Goal

Make recorded GPS anchors and runtime GPS corrections trustworthy enough for route tracking, while refusing data that software cannot safely repair.

## Recording Pipeline

Only a newly parsed RMC sentence represents a new position sample. GGA updates satellite count and HDOP but must not advance the position sequence.

Before the first GPS anchor is stored, collect five valid RMC positions while the vehicle remains at the route origin. Accept the window only when every sample passes the existing fix, satellite, HDOP, and coordinate checks. Use the median latitude and median longitude as the first anchor. Reset the window whenever the spread exceeds the stability limit.

For later anchors, maintain the latest three valid RMC positions and store their coordinate-wise median. Bind the filtered coordinate to the current inertial route point. Do not reuse a coordinate when no new RMC sentence has arrived.

Reduce the GPS-versus-inertial jump allowance from `+2.0m` to `+0.8m`. A rejected candidate remains available for later replacement; it must not consume route GPS capacity.

## Route Fitting

Fit the GPS-to-inertial similarity transform, calculate each anchor residual, and remove anchors whose residual exceeds `1.5m`. Refit once using the remaining anchors. A route is runnable only when at least eight inlier anchors remain and their RMS error is at most `1.2m`.

The serial log must report total anchors, inlier anchors, removed anchors, RMS error, and the rejection reason. Existing start-position rejection above `3m` remains unchanged.

## Runtime Behavior

Record mode waits at the origin until five stable RMC samples are available; motor recording may continue, but no unreliable first GPS anchor is stored. Run mode continues to require stable startup samples and never silently falls back after a start-position mismatch.

## Constraints

Use fixed-size buffers no larger than five coordinate pairs because TC264 RAM is constrained. Do not allocate route-sized temporary arrays. Preserve the existing Flash layout and the stored route format.

## Hardware Conditions

The antenna should be separated from motors, motor drivers, high-current wiring, and metal shielding, with clear sky view. Ten or more satellites are preferred. Standard single-point GNSS cannot guarantee `0.5m`; RTK or differential GNSS is required when that accuracy is mandatory.

## Verification

Source checks must verify RMC-only sequencing, five-sample first-anchor median, three-sample rolling median, `0.8m` jump allowance, robust residual clipping, eight-inlier minimum, and unchanged `3m` startup rejection. Existing route, final-lock, and RAM-oriented checks must continue to pass.
