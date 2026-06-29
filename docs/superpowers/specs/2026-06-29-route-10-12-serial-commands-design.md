# Route 10-12 Serial Commands Design

## Goal

Allow routes 10, 11, and 12 to be started from the serial assistant while the vehicle is in RUN mode.

## Command Mapping

- `U` or `u`: run route 10 forward.
- `V` or `v`: run route 11 forward.
- `W` or `w`: run route 12 forward.

These letters are currently unused. Existing commands for routes 1-9, auxiliary actions, fixed actions, stopping, tracing, and route dumps remain unchanged.

## Implementation

Extend `Portion2_Ascii_Command_Execute()` with one branch for `U` through `W` and their lowercase equivalents. Normalize the letter to a zero-based route ID and call `portion2_run_select_route()`.

The implementation remains byte-oriented. Sending the two characters `12` continues to execute route 1 followed by route 2; route 12 must be selected with `W` or `w`.

## Verification

Add static regression checks that verify:

- Both uppercase and lowercase command ranges are accepted.
- `U/u`, `V/v`, and `W/w` map to zero-based route IDs 9, 10, and 11.
- The selected route is passed to `portion2_run_select_route()`.
- Existing serial-command checks continue to pass.
