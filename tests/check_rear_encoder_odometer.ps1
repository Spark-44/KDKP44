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
$main = Get-Content (Join-Path $root 'user\cpu0_main.c') -Raw

Assert-Contains $rearHeader 'int32\s+rear_motor_get_total_encoder_pulses\s*\(\s*void\s*\)\s*;' 'Rear motor must expose total encoder pulses.'
Assert-Contains $rearHeader 'float\s+rear_motor_get_total_distance_m\s*\(\s*void\s*\)\s*;' 'Rear motor must expose total encoder distance.'
Assert-Contains $rearHeader 'void\s+rear_motor_clear_odometer\s*\(\s*void\s*\)\s*;' 'Rear motor must expose an odometer reset API.'
Assert-Contains $rearHeader 'REAR_ODOMETER_PULSES_PER_WHEEL_REV' 'Rear motor must expose the wheel-revolution pulse calibration constant.'
Assert-Contains $rearHeader 'REAR_DISTANCE_PER_PULSE_M' 'Rear motor must expose the distance-per-pulse conversion constant.'
Assert-Contains $rearHeader '#define\s+REAR_ODOMETER_PULSES_PER_WHEEL_REV\s+2122\.0f' 'Rear motor odometer must use the measured one-wheel-revolution pulse count.'
Assert-Contains $rearHeader '#define\s+REAR_ENCODER_FEEDBACK_DIRECTION\s+\(-1\)' 'Rear motor encoder feedback direction must be inverted.'
Assert-Contains $rearHeader '#define\s+REAR_SPEED_CALIBRATION_FACTOR\s+0\.7557f' 'Rear motor speed must use the measured 1.5m/s calibration factor.'

Assert-Contains $rearSource 'static\s+int32\s+total_encoder_pulses' 'Rear motor must store cumulative encoder pulses.'
Assert-Contains $rearSource 'total_encoder_pulses\s*\+=\s*\(int32\)encoder_10ms' 'Rear motor must accumulate encoder pulses on each sample.'
Assert-Contains $rearSource 'encoder_10ms\s*=\s*\(int16\)\s*\(\s*\(int32\)REAR_ENCODER_FEEDBACK_DIRECTION\s*\*\s*\(int32\)calculate_delta' 'Rear motor must apply the feedback direction to encoder delta.'
Assert-Contains $rearSource 'rear_motor_get_total_encoder_pulses\s*\(\s*void\s*\)' 'Rear motor total pulse getter must be implemented.'
Assert-Contains $rearSource 'rear_motor_get_total_distance_m\s*\(\s*void\s*\)' 'Rear motor total distance getter must be implemented.'
Assert-Contains $rearSource 'REAR_DISTANCE_PER_PULSE_M' 'Distance conversion must use the explicit distance-per-pulse coefficient.'
Assert-Contains $rearSource 'actual_mps\s*=\s*\(float\)encoder_100ms\s*\*\s*REAR_DISTANCE_PER_PULSE_M\s*\*\s*REAR_SPEED_CALIBRATION_FACTOR\s*/\s*0\.1f' 'Rear motor speed calculation must use calibrated 100ms average distance-per-pulse and speed factor.'
Assert-Contains $rearSource 'target_pulses\s*=\s*target_mps\s*\*\s*0\.1f\s*/\s*REAR_DISTANCE_PER_PULSE_M' 'Rear motor PID target pulses must use the calibrated distance-per-pulse.'

Assert-Contains $main '\[ENC-DIAG\][\s\S]*?totalPulse=%ld[\s\S]*?dist=%\.3f' 'Encoder diagnostic output must print cumulative pulses and distance.'
Assert-Contains $main '\[REAR-SPEED\][\s\S]*?totalPulse=%ld[\s\S]*?dist=%\.3f' 'Rear speed telemetry must print cumulative pulses and distance.'
Assert-Contains $main 'rear_motor_get_total_encoder_pulses\s*\(\s*\)' 'Encoder diagnostic output must read cumulative pulses.'
Assert-Contains $main 'rear_motor_get_total_distance_m\s*\(\s*\)' 'Encoder diagnostic output must read cumulative distance.'

Write-Host 'Rear encoder odometer checks passed.'
