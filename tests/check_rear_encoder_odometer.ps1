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
$guandao = Get-Content (Join-Path $root 'code\guandao.c') -Raw
$calibration = Get-Content (Join-Path $root 'code\rear_motor\rear_encoder_calibration.h') -Raw

Assert-Contains $rearHeader 'void\s+rear_motor_encoder_update_10ms\s*\(\s*float\s+yaw_deg\s*\)\s*;' 'Rear encoder samples must receive the current yaw.'
Assert-Contains $rearHeader 'uint8\s+rear_motor_take_odometry_sample\s*\(\s*int32\s*\*pulses\s*,\s*float\s*\*yaw_deg\s*\)\s*;' 'Rear motor must expose the pulse-and-yaw sample queue.'
Assert-Contains $rearHeader 'int32\s+rear_motor_get_odometry_total_pulses\s*\(\s*void\s*\)\s*;' 'Rear motor must expose cumulative odometry pulses.'
Assert-Contains $rearHeader '#define\s+REAR_ENCODER_FEEDBACK_DIRECTION\s+\(-1\)' 'Rear motor encoder feedback direction must be inverted.'
Assert-Contains $calibration '#define\s+REAR_ENCODER_METERS_PER_PULSE\s+0\.000378f' 'Rear speed and route odometry must share the measured pulse distance.'

Assert-Contains $rearSource 'static\s+volatile\s+int32\s+odometry_total_pulses' 'Rear motor must store cumulative odometry pulses.'
Assert-Contains $rearSource 'odometry_total_pulses\s*\+=\s*raw_encoder_delta' 'Rear motor must accumulate every accepted encoder sample.'
Assert-Contains $rearSource 'rear_odometry_pose_buffer_add\s*\(\s*&odometry_pose_buffer\s*,\s*raw_encoder_delta\s*,\s*yaw_deg\s*\)' 'Rear motor must queue each pulse sample with its yaw.'
Assert-Contains $rearSource 'rear_motor_get_odometry_total_pulses\s*\(\s*void\s*\)' 'Rear odometry total pulse getter must be implemented.'
Assert-Contains $rearSource 'raw_actual_mps\s*=\s*measured_pulses\s*\*\s*REAR_ENCODER_METERS_PER_PULSE\s*/\s*0\.1f' 'Rear speed calculation must use the shared pulse calibration.'
Assert-Contains $rearSource 'target_pulses\s*=\s*target_mps\s*/\s*REAR_ENCODER_METERS_PER_PULSE\s*\*\s*0\.1f' 'Rear PID target pulses must use the shared pulse calibration.'
Assert-Contains $guandao 'rear_motor_take_odometry_sample\s*\(\s*&odometry_pulses\s*,\s*&sample_yaw\s*\)' 'Route odometry must consume the pulse-and-yaw queue.'

Assert-Contains $main '\[ENC-DIAG\][\s\S]*?totalPulse=%ld[\s\S]*?dist=%\.3f' 'Encoder diagnostic output must print cumulative pulses and distance.'
Assert-Contains $main '\[REAR-SPEED\][\s\S]*?totalPulse=%ld[\s\S]*?dist=%\.3f' 'Rear speed telemetry must print cumulative pulses and distance.'
Assert-Contains $main 'rear_motor_get_odometry_total_pulses\s*\(\s*\)' 'Encoder diagnostics must read cumulative odometry pulses.'
Assert-Contains $main '\(float\)rear_motor_get_odometry_total_pulses\s*\(\s*\)\s*\*\s*ONE_TICK_DISTANCE' 'Encoder diagnostics must calculate distance from the current route calibration.'

Write-Host 'Rear encoder odometer checks passed.'
