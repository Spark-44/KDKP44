$ErrorActionPreference = 'Stop'

$imu = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\IMU.c')
$fixed = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\subject_2_fixed_action.c')

if($imu -match 'gyro_z\s*\*\s*0\.00916') {
    throw 'Yaw integration still uses the incorrect fixed 9.16 ms interval.'
}

foreach($pattern in @(
    'IMU_YAW_DEFAULT_DELTA_T\s+\(0\.008f\)',
    'system_getval_ms\s*\(\)',
    'IMU_Update_Straight_Yaw\s*\(\s*float\s+delta_t\s*\)',
    'IMU_Handle_180\s*\(\s*float\s+delta_t\s*\)',
    'gyro_z\s*\*\s*delta_t'
)) {
    if($imu -notmatch $pattern) {
        throw "Missing dynamic yaw timing behavior: $pattern"
    }
}

if($fixed -notmatch 'subject_2_fixed_log_state\s*\(\s*"STOP"\s*,\s*1U\s*\)') {
    throw 'The final fixed-action angle is not force-logged.'
}

Write-Output 'IMU yaw timing checks passed.'
