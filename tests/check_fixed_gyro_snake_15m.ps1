$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$header = Get-Content -Raw (Join-Path $repo 'code/voice_drive_action.h')
$fixed = Get-Content -Raw (Join-Path $repo 'code/subject_2_fixed_action.c')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $header 'VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M,\s*VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M,\s*VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M' `
    'gyro snake modes must be appended after existing fixed action modes'

Assert-Contains $fixed '#define\s+SUBJECT_2_GYRO_SNAKE_DISTANCE_M\s+\(15\.0f\)' `
    'gyro snake encoder distance must be 15m'
Assert-Contains $fixed '#define\s+SUBJECT_2_GYRO_SNAKE_SPEED_MPS\s+\(1\.0f\)' `
    'gyro snake speed must be 1.0m/s'
Assert-Contains $fixed '#define\s+SUBJECT_2_GYRO_SNAKE_STEER_DEG\s+\(20\.0f\)' `
    'gyro snake steering command must be 20 degrees'
Assert-Contains $fixed '#define\s+SUBJECT_2_GYRO_SNAKE_YAW_THRESHOLD_DEG\s+\(40\.0f\)' `
    'gyro snake yaw threshold must be 40 degrees'
Assert-Contains $fixed '#define\s+SUBJECT_2_GYRO_SNAKE_CENTER_DISTANCE_M\s+\(0\.8f\)' `
    'gyro snake must run straight for 0.8m before reversing steering'
Assert-Contains $fixed 'SUBJECT_2_GYRO_SNAKE_STALL_MS\s+SUBJECT_2_ENCODER_YAW_STALL_MS' `
    'gyro snake must keep the existing 3s no-progress protection'
Assert-Contains $fixed 'SUBJECT_2_GYRO_SNAKE_PHASE_TURNING' `
    'gyro snake must track a turning phase'
Assert-Contains $fixed 'SUBJECT_2_GYRO_SNAKE_PHASE_CENTER_HOLD' `
    'gyro snake must track a center-hold phase'
Assert-Contains $fixed 'subject_2_fixed_is_gyro_snake_action\s*\(' `
    'gyro snake action classifier must exist'
Assert-Contains $fixed 'subject_2_gyro_snake_task\s*\(' `
    'gyro snake must have an encoder/yaw task'
Assert-Contains $fixed 'subject_2_fixed_yaw_step\s*\(\s*Yaw_Straight_1,\s*subject_2_fixed_action_state\.yaw_target\s*\)' `
    'gyro snake yaw must be measured relative to its start heading with wrap handling'
Assert-Contains $fixed 'subject_2_fixed_action_state\.snake_steer_side\s*=\s*subject_2_fixed_is_gyro_snake_action\(mode\)\s*\?\s*1\s*:\s*0' `
    'gyro snake must start by steering left'
Assert-Contains $fixed 'subject_2_fixed_action_state\.snake_next_steer_side\s*=\s*-subject_2_fixed_action_state\.snake_steer_side' `
    'gyro snake must prepare the opposite physical steering side after each target angle'
Assert-Contains $fixed 'subject_2_fixed_action_state\.snake_steer_side\s*=\s*subject_2_fixed_action_state\.snake_next_steer_side' `
    'gyro snake must resume with the prepared left/right steering side after center hold'
Assert-Contains $fixed 'subject_2_fixed_action_state\.snake_target_side\s*=\s*-subject_2_fixed_action_state\.snake_target_side' `
    'gyro snake must alternate yaw target sides at each threshold crossing'
Assert-Contains $fixed 'subject_2_fixed_action_state\.snake_phase\s*=\s*SUBJECT_2_GYRO_SNAKE_PHASE_CENTER_HOLD' `
    'gyro snake must center steering after reaching a target angle'
Assert-Contains $fixed 'subject_2_fixed_action_state\.snake_center_start_distance_m\s*=\s*subject_2_fixed_action_state\.distance_m' `
    'gyro snake must measure the center hold using encoder distance'
Assert-Contains $fixed 'subject_2_fixed_action_state\.distance_m\s*-\s*subject_2_fixed_action_state\.snake_center_start_distance_m\s*>=\s*SUBJECT_2_GYRO_SNAKE_CENTER_DISTANCE_M' `
    'gyro snake must keep steering centered for 0.8m'
Assert-Contains $fixed 'subject_2_fixed_action_state\.snake_phase\s*==\s*SUBJECT_2_GYRO_SNAKE_PHASE_CENTER_HOLD[\s\S]*?steer_deg\s*=\s*0\.0f' `
    'gyro snake must command zero steering during the 0.8m center hold'
Assert-Contains $fixed 'distance_m\s*>=\s*SUBJECT_2_GYRO_SNAKE_DISTANCE_M' `
    'gyro snake must stop by encoder distance'

Assert-Contains $main "data\s*==\s*'Z'[\s\S]*?Portion2_Fixed_Action_Start\(VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M\)" `
    'serial Z must start forward gyro snake 15m'
Assert-Contains $main "data\s*==\s*'z'[\s\S]*?Portion2_Fixed_Action_Start\(VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M\)" `
    'serial z must start reverse gyro snake 15m'

Write-Host 'fixed gyro snake 15m checks passed'
