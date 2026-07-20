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
$guandao = Get-Content (Join-Path $root 'code\guandao.c') -Raw

Assert-Contains $guandao 'state->current_state\.x\s*\+=\s*sample_distance\s*\*\s*sinf\s*\(\s*sample_theta\s*/\s*180\.0f\s*\*\s*M_PI\s*\)' 'Guandao X displacement must use signed yaw-tagged encoder samples.'
Assert-Contains $guandao 'state->current_state\.y\s*\+=\s*sample_distance\s*\*\s*cosf\s*\(\s*sample_theta\s*/\s*180\.0f\s*\*\s*M_PI\s*\)' 'Guandao Y displacement must use signed yaw-tagged encoder samples.'

Write-Host 'Guandao inertial XY direction checks passed.'
