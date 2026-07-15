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
$headerPath = Join-Path $root 'code\angle_control.h'
$sourcePath = Join-Path $root 'code\angle_control.c'

$header = Get-Content $headerPath -Raw
$source = Get-Content $sourcePath -Raw

Assert-Contains $header '#define\s+ANGLE_DEFAULT_KP\s+500\.0f' 'Angle Kp must be 500.0f.'
Assert-Contains $header '#define\s+ANGLE_DEFAULT_KI\s+18\.0f' 'Angle Ki must be 18.0f.'
Assert-Contains $header '#define\s+ANGLE_DEFAULT_KD\s+27\.0f' 'Angle Kd must be 27.0f.'
Assert-Contains $header '#define\s+ANGLE_OUTPUT_MAX\s+10000' 'Angle output limit must remain +/-10000.'
Assert-Contains $header '#define\s+ANGLE_DEAD_BAND\s+0\.1f' 'Angle dead band must be 0.1 degree.'
Assert-Contains $header '#define\s+ANGLE_FEED_FORWARD\s+65\.0f' 'Angle feed-forward must be 65.'
Assert-Contains $header '#define\s+ANGLE_INTEGRAL_MAX\s+1000\.0f' 'Angle integral maximum must remain 1000.0f.'

Assert-Contains $source 'angle_ctrl\.pid\.IntegralMax\s*=\s*ANGLE_INTEGRAL_MAX' 'Angle control must set IntegralMax to the angle-specific limit after PID init.'
Assert-Contains $source 'Value_Limit_float\(&pid_output,\s*-ANGLE_OUTPUT_MAX,\s*ANGLE_OUTPUT_MAX\)' 'Angle output must be limited again after feed-forward is added.'
Assert-Contains $source 'float\s+angle_error\s*=\s*angle_ctrl\.target_angle\s*-\s*angle_ctrl\.current_angle' 'Angle control must calculate the steering error before PID.'
Assert-Contains $source 'angle_error\s*<\s*ANGLE_DEAD_BAND[\s\S]*?angle_error\s*>\s*-ANGLE_DEAD_BAND[\s\S]*?pid_output\s*=\s*0[\s\S]*?angle_ctrl\.pid\.Integral\s*=\s*0' 'Angle dead band must stop the motor and clear integral.'
Assert-Contains $source 'pid_output\s*\+=\s*\(angle_error\s*>\s*0\.0f\)\s*\?\s*ANGLE_FEED_FORWARD\s*:\s*-ANGLE_FEED_FORWARD' 'Angle control must add signed feed-forward outside the dead band.'

Write-Host 'Angle control PID checks passed.'
