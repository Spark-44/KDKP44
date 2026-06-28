# Six-Satellite GPS Threshold Design

## Goal

Allow route recording and runtime GPS fusion when the receiver reports at least six satellites.

## Changes

- Set `PORTION2_GPS_RECORD_MIN_SATELLITES` to `6U`.
- Set `PORTION2_GPS_FUSION_MIN_SATELLITES` to `6U`.
- Keep the existing `HDOP <= 2.5` quality gate for recording, startup calibration, and runtime fusion.
- Add regression checks requiring both satellite thresholds to remain synchronized at six.

## Expected Behavior

Fixes reporting six or seven satellites may be recorded and used by the control loop when all other coordinate, movement, repeat-point, and HDOP checks pass. Fixes below six satellites remain rejected.
