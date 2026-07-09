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

Assert-Contains $header '#define\s+ANGLE_DEFAULT_KP\s+200\.0f' 'Angle Kp must be restored to 200.0f.'
Assert-Contains $header '#define\s+ANGLE_DEFAULT_KI\s+4\.0f' 'Angle Ki must be restored to 4.0f.'
Assert-Contains $header '#define\s+ANGLE_DEFAULT_KD\s+10\.0f' 'Angle Kd must be restored to 10.0f.'
Assert-Contains $header '#define\s+ANGLE_OUTPUT_MAX\s+10000' 'Angle output limit must remain +/-10000.'
Assert-Contains $header '#define\s+ANGLE_INTEGRAL_MAX\s+1000\.0f' 'Angle integral maximum must remain 1000.0f.'

Assert-Contains $source 'angle_ctrl\.pid\.IntegralMax\s*=\s*ANGLE_INTEGRAL_MAX' 'Angle control must set IntegralMax to the angle-specific limit after PID init.'
Assert-Contains $source 'Value_Limit_float\(&pid_output,\s*-ANGLE_OUTPUT_MAX,\s*ANGLE_OUTPUT_MAX\)' 'Angle output must be limited again after feed-forward is added.'
if($header -match 'ANGLE_GRAVITY_FF') {
    throw 'Angle feed-forward define must be removed.'
}
if($source -match 'ANGLE_GRAVITY_FF|pid_output\s*\+=') {
    throw 'Angle feed-forward logic must be removed.'
}

Write-Host 'Angle control PID checks passed.'
