# Record Rear-Motor Telemetry Design

## Goal

While the vehicle is driving under remote control in `Guandao_Portion2_Recode` mode, print the rear-wheel actual speed and motor PWM value on the debug UART every 200 ms.

## Behavior

- Telemetry is active only when `main_mode == Guandao_Portion2_Recode` and `remote_control_is_active()` is true.
- Each line uses this stable format:

  ```text
  [REC-REAR] speed=1.234mps pwm=3500
  ```

- `speed` comes from `rear_motor_get_speed_mps()`.
- `pwm` comes from `rear_motor_get_pwm()` and represents the signed PWM command used by the rear-motor driver.
- Output is rate-limited to one line every 200 ms to avoid loading the control loop and 115200-baud debug UART.
- Existing idle-only `[ENC-DIAG]` output remains unchanged.
- Existing serial menus and command handling remain unchanged.

## Implementation

Add a focused update function in `user/cpu0_main.c`. The function owns its last-report timestamp, checks the Record-mode and remote-active guards, formats one line, and writes it through `DEBUG_UART_INDEX`. Call it from the `Guandao_Portion2_Recode` branch after `portion2_record_task()` so it observes the latest motor state.

## Verification

Add a PowerShell static regression check covering:

- the 200 ms interval;
- Record-mode and remote-active guards;
- the `[REC-REAR]` format;
- use of the rear speed and PWM getters;
- scheduling from the Record-mode branch;
- preservation of the existing idle encoder diagnostic call.

Hardware acceptance: enter Record mode, drive with the remote controller, and confirm one `[REC-REAR]` line appears approximately every 200 ms with changing speed and PWM values.
