$ErrorActionPreference = 'Stop'

function Assert-Contains {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$main = Get-Content (Join-Path $root 'user\cpu0_main.c') -Raw

Assert-Contains $main 'static\s+void\s+Rear_Motor_Serial_Telemetry_Update\s*\(\s*void\s*\)' 'Main loop must define rear motor serial telemetry task.'
Assert-Contains $main 'REAR_MOTOR_TELEMETRY_PERIOD_MS\s+\(200U\)' 'Rear motor telemetry should be rate-limited to 200ms.'
Assert-Contains $main '\[REAR-SPEED\][\s\S]*?target=%\.2f[\s\S]*?actual=%\.2f[\s\S]*?pwm=%d[\s\S]*?enc10=%d[\s\S]*?enc100=%ld' 'Rear motor telemetry must print target, actual speed, PWM and encoder pulses.'
Assert-Contains $main '\[REAR-SPEED\][\s\S]*?speed=%\.3fmps' 'Rear motor telemetry must print a clearly named speed field in m/s.'
Assert-Contains $main 'rear_motor_get_target_mps\s*\(\s*\)' 'Rear motor telemetry must read target speed.'
Assert-Contains $main 'rear_motor_get_speed_mps\s*\(\s*\)' 'Rear motor telemetry must read actual speed.'
Assert-Contains $main 'rear_motor_get_pwm\s*\(\s*\)' 'Rear motor telemetry must read current PWM.'
Assert-Contains $main 'rear_motor_get_encoder_10ms\s*\(\s*\)' 'Rear motor telemetry must read 10ms encoder delta.'
Assert-Contains $main 'rear_motor_get_encoder_100ms\s*\(\s*\)' 'Rear motor telemetry must read 100ms encoder delta.'
Assert-Contains $main 'remote_control_task\s*\(\s*\)\s*;[\s\S]*?Rear_Motor_Serial_Telemetry_Update\s*\(\s*\)\s*;' 'Rear motor telemetry must run after remote task in the main loop.'

Write-Host 'Rear motor serial telemetry checks passed.'
