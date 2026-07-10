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

function Assert-NotContains {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if($Text -match $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$rearHeader = Get-Content (Join-Path $root 'code\rear_motor\rear_motor.h') -Raw
$rearSource = Get-Content (Join-Path $root 'code\rear_motor\rear_motor.c') -Raw
$remoteSource = Get-Content (Join-Path $root 'code\remote_control.c') -Raw

Assert-Contains $rearHeader 'void\s+rear_motor_set_speed_limit_mps\s*\(\s*float\s+limit_mps\s*\)\s*;' 'Rear motor must expose a speed-limit enable API.'
Assert-Contains $rearHeader 'void\s+rear_motor_clear_speed_limit\s*\(\s*void\s*\)\s*;' 'Rear motor must expose a speed-limit disable API.'

Assert-Contains $rearSource 'static\s+float\s+speed_limit_mps' 'Rear motor must store an active speed limit.'
Assert-Contains $rearSource 'void\s+rear_motor_set_speed_limit_mps\s*\(\s*float\s+limit_mps\s*\)' 'Rear motor speed-limit enable API must be implemented.'
Assert-Contains $rearSource 'void\s+rear_motor_clear_speed_limit\s*\(\s*void\s*\)' 'Rear motor speed-limit disable API must be implemented.'
Assert-Contains $rearHeader '#define\s+REAR_KP\s+4\.0f' 'Rear motor Kp must be reduced for smoother remote speed control.'
Assert-Contains $rearHeader '#define\s+REAR_KI\s+0\.1f' 'Rear motor Ki must be reduced for smoother remote speed control.'
Assert-Contains $rearHeader '#define\s+REAR_KD\s+0\.2f' 'Rear motor Kd must be reduced for smoother remote speed control.'
Assert-Contains $rearHeader '#define\s+REAR_FF_GAIN\s+6\.0f' 'Rear motor feed-forward must be reduced to avoid overshoot near 1m/s.'
Assert-Contains $rearHeader '#define\s+REAR_PWM_RATE_LIMIT\s+400' 'Rear motor PWM rate limit must be reduced to avoid jerky throttle steps.'
Assert-Contains $rearHeader '#define\s+REAR_SPEED_LIMIT_SOFT_ZONE_MPS\s+0\.20f' 'Rear motor speed limit must have a soft zone.'
Assert-Contains $rearSource 'speed_limit_scale' 'Rear motor governor must scale same-direction PWM near the speed limit.'
Assert-Contains $rearSource 'speed_limit_mps\s*-\s*speed_abs' 'Rear motor soft speed limit must reduce output as actual speed approaches the limit.'
Assert-Contains $rearSource 'if\s*\(\s*target_mps\s*>\s*0\.0f[\s\S]*?actual_mps\s*>=\s*speed_limit_mps[\s\S]*?ff\s*=\s*0\.0f' 'Forward overspeed must remove same-direction feed-forward.'
Assert-Contains $rearSource 'if\s*\(\s*target_mps\s*<\s*0\.0f[\s\S]*?actual_mps\s*<=\s*-speed_limit_mps[\s\S]*?ff\s*=\s*0\.0f' 'Reverse overspeed must remove same-direction feed-forward.'
Assert-Contains $rearSource 'if\s*\(\s*target_mps\s*>\s*0\.0f[\s\S]*?actual_mps\s*>=\s*speed_limit_mps[\s\S]*?pwm_f\s*>\s*0\.0f[\s\S]*?pwm_f\s*=\s*0\.0f' 'Forward overspeed must clamp positive PWM to zero.'
Assert-Contains $rearSource 'if\s*\(\s*target_mps\s*<\s*0\.0f[\s\S]*?actual_mps\s*<=\s*-speed_limit_mps[\s\S]*?pwm_f\s*<\s*0\.0f[\s\S]*?pwm_f\s*=\s*0\.0f' 'Reverse overspeed must clamp negative PWM to zero.'

Assert-Contains $remoteSource 'rear_motor_set_speed_limit_mps\s*\(\s*REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\)' 'Remote record control must enable rear speed governor at 2m/s.'
Assert-Contains $remoteSource 'rear_motor_clear_speed_limit\s*\(\s*\)' 'Remote failsafe/stop must clear rear speed governor.'
Assert-NotContains $remoteSource 'actualSpeed=%.2f[\s\S]*?pwm=%d' 'Remote debug output must stay disabled by default.'
Assert-Contains $remoteSource '#define\s+REMOTE_CONTROL_CHANNEL_DEAD_ZONE\s+\(50\)' 'Remote speed control must use a larger dead zone around CH2 center.'
Assert-Contains $remoteSource '#define\s+REMOTE_CONTROL_SPEED_FULL_SCALE\s+\(776\.0f\)' 'Remote CH2 full-scale must match the measured bottom throw so full stick maps to the record speed limit.'
Assert-Contains $remoteSource '#define\s+REMOTE_CONTROL_SPEED_RAMP_STEP_MPS\s+\(0\.03f\)' 'Remote speed control must ramp target speed for smoothness.'
Assert-Contains $remoteSource 'static\s+float\s+remote_command_speed_mps' 'Remote control must store the ramped speed command.'
Assert-Contains $remoteSource 'remote_command_speed_mps\s*=\s*remote_control_ramp_speed_command\s*\(\s*remote_control_limit_speed_command\s*\(\s*remote_target_speed_mps\s*\)\s*\)' 'Remote periodic update must apply overspeed limiting and ramping before commanding the rear motor.'

Write-Host 'Remote speed governor checks passed.'
