# Portion 2 Route Capacity Expansion Design

## Goal

Increase every portion-2 route from 49 to 75 inertial points and from 20 to 30
GPS points while preserving all 12 routes.

## Capacity

- Route count: 12.
- Inertial capacity: 75 points per route, 900 points total.
- GPS capacity: 30 points per route, 360 points total.
- The 0.4 m inertial recording interval and 1.0 m GPS interval remain unchanged.

At 0.4 m spacing, 75 inertial points cover approximately 29.6 m between the
first and last point.

## Flash Layout

Inertial coordinates continue to use two data-flash pages:

- Page 9 stores the first 509 points after its two-word header.
- Page 7 stores the remaining 391 points plus the 12-route metadata.

GPS records use two pages because 360 four-word records cannot fit in one page:

- Page 8 stores the first 254 GPS records after its two-word header.
- Page 6 stores the remaining 106 GPS records after a continuation header.

Both GPS pages receive layout markers and record counts. Compile-time checks
must reject any future capacity that exceeds these page layouts.

## Compatibility

The route and GPS layout markers change. Existing 49-point/20-GPS records are
invalidated on first boot with the new firmware; all routes must be recorded and
saved again. Runtime code must not attempt to migrate or reinterpret old data.

## RAM Layout

The existing linker map leaves about 15 KB free in DSRAM1, so expanding every
embedded `guandao_state` array would not link. `MAX_LENGTH_INDEX` therefore
remains 588 and `MAX_GPS_RECODE` remains 240.

The route repository uses the existing arrays for the first 588 inertial and 240
GPS records, then uses dedicated extension arrays for the remaining 312 inertial
and 120 GPS records. Accessor functions hide this split from recording, runtime
loading, route dumps, and Flash persistence. This adds approximately 6.6 KB of
static RAM instead of approximately 40 KB.

## Verification

- Source-contract tests verify 75/30 per-route capacity and 900/360 totals.
- Tests verify inertial data still spans pages 9 and 7.
- Tests verify GPS data spans pages 8 and 6 with independent markers.
- Tests verify segmented RAM access keeps the embedded runtime arrays at 588/240.
- Tests verify compile-time page-capacity guards remain present.
- Existing serial route diagnostics display the new limits automatically.
