$ErrorActionPreference = 'Stop'

$source = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\subject_2_fixed_action.c')

$required = @(
    'SUBJECT_2_FIXED_CIRCLE_SLOWDOWN_DEG\s+\(30\.0f\)',
    'SUBJECT_2_FIXED_CIRCLE_MIN_SPEED_MPS\s+\(0\.30f\)',
    'SUBJECT_2_FIXED_CIRCLE_STOP_LEAD_DEG\s+\(1\.0f\)',
    'SUBJECT_2_FIXED_CIRCLE_SETTLE_MS\s+\(300U\)',
    'subject_2_fixed_is_circle_action',
    'circle_settling',
    'subject_2_fixed_log_state\s*\(\s*"SETTLE"\s*,\s*1U\s*\)'
)

foreach($pattern in $required) {
    if($source -notmatch $pattern) {
        throw "Missing circle stop behavior: $pattern"
    }
}

$stopBeforeLog = 'rear_motor_stop\s*\(\s*\)\s*;[\s\S]{0,300}subject_2_fixed_log_state\s*\(\s*"STOP"\s*,\s*1U\s*\)'
if($source -notmatch $stopBeforeLog) {
    throw 'Circle stop must cut motor PWM before writing the STOP log.'
}

Write-Output 'Circle stop control checks passed.'
