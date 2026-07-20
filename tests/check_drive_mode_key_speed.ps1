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
$main = Get-Content (Join-Path $root 'user\cpu0_main.c') -Raw

Assert-Contains $main '#define\s+PORTION2_DRIVE_SPEED_MIN_MPS\s+\(1\.0f\)' 'Drive mode minimum non-zero speed must be 1.0m/s.'
Assert-Contains $main '#define\s+PORTION2_DRIVE_SPEED_STEP_MPS\s+\(1\.0f\)' 'Drive mode speed step must be 1.0m/s.'
Assert-Contains $main '#define\s+PORTION2_DRIVE_SPEED_MAX_MPS\s+\(5\.0f\)' 'Drive mode speed max must remain 5.0m/s.'

Assert-Contains $main 'if\s*\(\s*key1_flag\s*\)[\s\S]*?portion2_drive_target_mps\s*-=\s*PORTION2_DRIVE_SPEED_STEP_MPS[\s\S]*?if\s*\(\s*portion2_drive_target_mps\s*<\s*0\.0f\s*\)[\s\S]*?portion2_drive_target_mps\s*=\s*0\.0f' 'K1 must decrease drive speed by one step and clamp at 0.'
Assert-Contains $main 'if\s*\(\s*key2_flag\s*\)[\s\S]*?portion2_drive_target_mps\s*\+=\s*PORTION2_DRIVE_SPEED_STEP_MPS[\s\S]*?if\s*\(\s*portion2_drive_target_mps\s*>\s*PORTION2_DRIVE_SPEED_MAX_MPS\s*\)[\s\S]*?portion2_drive_target_mps\s*=\s*PORTION2_DRIVE_SPEED_MAX_MPS' 'K2 must increase drive speed by one step and clamp at max.'
Assert-Contains $main 'if\s*\(\s*portion2_drive_full_power\s*\)[\s\S]*?portion2_drive_full_power\s*=\s*0[\s\S]*?portion2_drive_target_mps\s*=\s*PORTION2_DRIVE_SPEED_MAX_MPS' 'K1 from full power must return to max speed.'
Assert-Contains $main 'if\s*\(\s*portion2_drive_target_mps\s*>=\s*PORTION2_DRIVE_SPEED_MAX_MPS\s*\)[\s\S]*?portion2_drive_full_power\s*=\s*1' 'K2 at max speed must enter full power.'

Assert-Contains $main 'static\s+void\s+Portion2_Drive_Encoder_Update_10ms\s*\(\s*void\s*\)' 'Drive mode must have a 10ms-gated rear encoder update helper.'
Assert-Contains $main 'Portion2_Drive_Encoder_Update_10ms\s*\(\s*void\s*\)[\s\S]*?rear_motor_encoder_update_10ms\s*\(\s*Yaw_1\s*\)' 'Drive encoder helper must sample encoder and yaw together.'
Assert-Contains $main 'static\s+void\s+Portion2_Drive_Mode_Task\s*\(\s*void\s*\)[\s\S]*?Portion2_Drive_Encoder_Update_10ms\s*\(\s*\)[\s\S]*?rear_motor_pid_update_100ms\s*\(\s*\)' 'Drive mode must sample rear encoder through the 10ms gate before running rear motor PID.'
Assert-Contains $main 'Portion2_Drive_Key_Log\s*\(\s*"K1"\s*\)' 'Drive mode must log K1 speed events.'
Assert-Contains $main 'Portion2_Drive_Key_Log\s*\(\s*"K2"\s*\)' 'Drive mode must log K2 speed events.'
Assert-Contains $main 'full=%u\s+target=%\.2f' 'Drive key logs must include full-power state and target speed.'

Assert-NotContains $main 'portion2_drive_target_mps\s*<=\s*2\.0f' 'Drive speed keys must not keep the old special 2.0m/s case.'
Assert-NotContains $main 'portion2_drive_target_mps\s*=\s*PORTION2_DRIVE_SPEED_MIN_MPS\s*;' 'Drive speed keys must not snap down to minimum instead of stepping.'
Assert-NotContains $main 'Portion2_Drive_Mode_Task[\s\S]*?Portion2_Drive_Mode_Key_Handle\s*\(\s*\)\s*;\s*rear_motor_encoder_update_10ms\s*\(' 'Drive mode must not sample rear encoder every main-loop iteration.'

Write-Host 'Drive mode key speed checks passed.'
