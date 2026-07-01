# Route 10-12 Serial Commands Design

## Goal

Allow routes 10, 11, and 12 to be started from the serial assistant while the vehicle is in RUN mode.

## Command Mapping

- `U` or `u`: run route 10 forward.
- `V` or `v`: run route 11 forward.
- `w`: run route 12 forward.
- `W`: run route 12 backward.

These letters are currently unused. Existing commands for routes 1-9, auxiliary actions, fixed actions, stopping, tracing, and route dumps remain unchanged.

## Implementation

Extend `Portion2_Ascii_Command_Execute()` with forward branches for `U` through `V` and lowercase `u` through `w`. Uppercase `W` calls `portion2_run_select_back_route()` for route 12.

The implementation remains byte-oriented. Sending the two characters `12` continues to execute route 1 followed by route 2; forward route 12 must be selected with `w`, and backward route 12 with `W`.

## Verification

Add static regression checks that verify:

- `U/u` and `V/v` map to forward route IDs 9 and 10.
- Lowercase `w` maps to forward route ID 11.
- Uppercase `W` passes route ID 11 to `portion2_run_select_back_route()`.
- Existing serial-command checks continue to pass.
