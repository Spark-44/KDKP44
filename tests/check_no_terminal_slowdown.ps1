Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Match {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if($Text -notmatch $Pattern) { throw $Message }
}

function Assert-NotMatch {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if($Text -match $Pattern) { throw $Message }
}

$repo = Split-Path -Parent $PSScriptRoot
$guandao = Get-Content -Raw -Path (Join-Path $repo 'code/guandao.c')

Assert-NotMatch $guandao 'float\s+terminal_speed\s*=\s*base_speed\s*\*\s*\(dist_to_final\s*/\s*final_dsts\)' `
    'Run mode must not linearly slow down near the terminal distance.'

Assert-NotMatch $guandao 'if\s*\(\s*!portion1_parking_zone\s*&&\s*dist_to_final\s*<\s*final_dsts' `
    'Run mode terminal-distance slowdown block must be removed.'

Assert-Match $guandao 'target_steering\s*=\s*portion2_guided_terminal_steering\s*\(' `
    'Guided terminal steering must remain so yaw alignment happens during normal line following.'

Assert-NotMatch $guandao 'uint8\s+align_status\s*=\s*portion2_final_yaw_align\s*\(' `
    'Normal portion2 terminal stop must not enter a separate low-speed yaw alignment phase.'

Assert-NotMatch $guandao 'if\s*\(\s*state\s*==\s*&portion_2\s*&&\s*!\s*portion2_final_yaw_align\s*\(' `
    'Standard final stop must not call separate low-speed yaw alignment.'


Assert-NotMatch $guandao 'v_center\s*=\s*PORTION2_TERMINAL_APPROACH_SPEED' `
    'Run mode must not apply the separate terminal approach speed cap.'
Assert-Match $guandao 'raw_length\s*>\s*0\s*&&\s*raw_point\s*>=\s*raw_length\s*-\s*1[\s\S]*?dist_to_final\s*<=\s*final_stop_distance[\s\S]*?state->current_point_index\s*=\s*route_length' `
    'Final distance stop must directly finish the route once the terminal point is reached.'

Write-Host 'No terminal slowdown checks passed.'

