# Route 11 and Route 12 Drive Policy Design

## Goal

Route 11 records and follows points in their original order while the vehicle drives backward. Route 12 records and follows points in their original order while the vehicle drives forward.

## Route Policy

- Route 11: keep inertial and GPS point order, use reverse motor drive, and use `Yaw + 180 degrees` as the motion heading.
- Route 12: keep inertial and GPS point order and use forward motor drive.
- Neither route uses the old array-reversal behavior.
- Route 11 record mode expects the operator to move the vehicle backward. Its recorded point heading represents backward motion, while stored start/final yaw remains the physical body yaw.

## Command Mapping

- `V` or `v`: follow route 11 in same-order reverse-drive mode.
- `W` or `w`: follow route 12 in same-order forward-drive mode.
- `Q` or `q`: encoder-and-yaw forward 10 m action.
- `R` or `r`: encoder-and-yaw reverse 10 m action.
- Voice snake backward 10 m: route 11.
- Voice snake forward 10 m: route 12.
- Voice forward 10 m: encoder-and-yaw forward action.
- Voice backward 10 m: encoder-and-yaw reverse action.

Both legacy voice command groups use the same semantic mapping so commands `0x1F`-`0x22` and `0x29`-`0x2C` cannot disagree.

## Terminal Pose

The route geometry uses the motion heading. Therefore route 11 terminal path shaping uses physical final yaw plus 180 degrees, while final body-yaw alignment still targets the recorded physical final yaw.

## Verification

- Confirm route 11 starts with `portion2_run_reverse=0` and `portion2_run_drive_reverse=1`.
- Confirm route 12 starts with both reverse flags cleared.
- Confirm GPS and inertial copies use their original indices for both routes.
- Confirm serial and voice mappings call the same route/fixed-action entry points.
