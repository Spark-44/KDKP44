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

Assert-Contains $guandao 'state->current_state\.x\s*-=\s*delta_real_center\s*\*\s*sinf\s*\(\s*state->current_state\.theta\s*/\s*180\.0f\s*\*\s*M_PI\s*\)' 'Guandao inertial X displacement must be inverted.'
Assert-Contains $guandao 'state->current_state\.y\s*-=\s*delta_real_center\s*\*\s*cosf\s*\(\s*state->current_state\.theta\s*/\s*180\.0f\s*\*\s*M_PI\s*\)' 'Guandao inertial Y displacement must be inverted.'

if($guandao -match 'state->current_state\.x\s*\+=\s*delta_real_center\s*\*\s*sinf\s*\(\s*state->current_state\.theta\s*/\s*180\.0f\s*\*\s*M_PI\s*\)') {
    throw 'Guandao inertial X displacement is still using the original non-inverted direction.'
}

if($guandao -match 'state->current_state\.y\s*\+=\s*delta_real_center\s*\*\s*cosf\s*\(\s*state->current_state\.theta\s*/\s*180\.0f\s*\*\s*M_PI\s*\)') {
    throw 'Guandao inertial Y displacement is still using the original non-inverted direction.'
}

Write-Host 'Guandao inertial XY direction checks passed.'
