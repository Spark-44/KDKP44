# Route 12 Reverse W Command Design

## Goal

Allow route 12 to run backward with a single serial command because the current
`-w` sequence is not recognized by the command parser.

## Command Mapping

- `W`: run route 12 backward with `portion2_run_select_back_route()`.
- `w`: keep running route 12 forward with `portion2_run_select_route()`.
- `U`, `V`, `u`, and `v`: keep their existing route mappings.

## Scope

Only the UART ASCII command dispatch and its regression test change. Route
recording, route data, voice commands, and tracking behavior remain unchanged.

## Verification

The route-layout test must verify the case-sensitive mapping and continue to
verify the unchanged route 10 and route 11 commands.
