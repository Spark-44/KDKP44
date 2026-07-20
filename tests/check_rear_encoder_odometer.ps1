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
$rearHeader = Get-Content (Join-Path $root 'code\rear_motor\rear_motor.h') -Raw
$rearSource = Get-Content (Join-Path $root 'code\rear_motor\rear_motor.c') -Raw
$calibration = Get-Content (Join-Path $root 'code\rear_motor\rear_encoder_calibration.h') -Raw
$main = Get-Content (Join-Path $root 'user\cpu0_main.c') -Raw

Assert-Contains $rearHeader 'int32\s+rear_motor_get_total_encoder_pulses\s*\(\s*void\s*\)\s*;' 'Rear motor must expose total encoder pulses.'
Assert-Contains $rearHeader 'float\s+rear_motor_get_total_distance_m\s*\(\s*void\s*\)\s*;' 'Rear motor must expose total encoder distance.'
Assert-Contains $rearHeader 'void\s+rear_motor_clear_odometer\s*\(\s*void\s*\)\s*;' 'Rear motor must expose an odometer reset API.'
Assert-Contains $rearHeader 'REAR_DISTANCE_PER_PULSE_M' 'Rear motor must expose the distance-per-pulse conversion constant.'
Assert-Contains $rearHeader '#define\s+REAR_ENCODER_FEEDBACK_DIRECTION\s+\(-1\)' 'Rear motor encoder feedback direction must be inverted.'
Assert-Contains $calibration '#define\s+REAR_ENCODER_METERS_PER_PULSE\s+0\.000378f' 'Rear odometry and speed loop must share the subject-2 measured calibration.'

Assert-Contains $rearSource 'static\s+volatile\s+int32\s+odometry_total_pulses' 'Rear motor must store cumulative encoder pulses.'
Assert-Contains $rearSource 'odometry_total_pulses\s*\+=\s*delta' 'Rear motor must accumulate each accepted encoder sample.'
Assert-Contains $rearSource 'delta\s*=\s*\(int32\)REAR_ENCODER_FEEDBACK_DIRECTION[\s\S]*?calculate_delta' 'Rear motor must apply feedback direction to encoder delta.'
Assert-Contains $rearSource 'rear_motor_get_total_encoder_pulses\s*\(\s*void\s*\)' 'Rear motor total pulse getter must be implemented.'
Assert-Contains $rearSource 'rear_motor_get_total_distance_m\s*\(\s*void\s*\)' 'Rear motor total distance getter must be implemented.'
Assert-Contains $rearSource 'rear_motor_get_total_distance_m[\s\S]*?REAR_ENCODER_METERS_PER_PULSE' 'Distance conversion must use the shared calibration.'
Assert-Contains $rearSource 'actual_mps\s*=\s*filtered_pulses_100ms\s*\*\s*REAR_ENCODER_METERS_PER_PULSE\s*/\s*0\.1f' 'Rear speed must use filtered 100ms pulses and shared calibration.'
Assert-Contains $rearSource 'target_pulses\s*=\s*target_mps\s*/\s*REAR_ENCODER_METERS_PER_PULSE\s*\*\s*0\.1f' 'Rear target pulse conversion must use shared calibration.'

Assert-Contains $main '\[ENC-DIAG\][\s\S]*?totalPulse=%ld[\s\S]*?dist=%\.3f' 'Encoder diagnostic output must print cumulative pulses and distance.'
Assert-Contains $main '\[REAR-SPEED\][\s\S]*?totalPulse=%ld[\s\S]*?dist=%\.3f' 'Rear speed telemetry must print cumulative pulses and distance.'
Assert-Contains $main 'rear_motor_get_total_encoder_pulses\s*\(\s*\)' 'Encoder diagnostic output must read cumulative pulses.'
Assert-Contains $main 'rear_motor_get_total_distance_m\s*\(\s*\)' 'Encoder diagnostic output must read cumulative distance.'

Write-Host 'Rear encoder odometer checks passed.'
