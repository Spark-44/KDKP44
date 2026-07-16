Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Match {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )
    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

$repo = Split-Path -Parent $PSScriptRoot
$source = Get-Content -Raw -Path (Join-Path $repo 'code/guandao.c')

Assert-Match $source '#define\s+PORTION2_ROUTE11_TERMINAL_BLEND_START_M\s+2\.5f' `
    'route 11 reverse must start terminal yaw correction before the final 2m.'
Assert-Match $source '#define\s+PORTION2_ROUTE11_TERMINAL_BLEND_FULL_M\s+0\.8f' `
    'route 11 reverse must prioritize terminal yaw near the final pose.'
Assert-Match $source '#define\s+PORTION2_ROUTE11_FINAL_YAW_GAIN\s+1\.0f' `
    'route 11 reverse must have terminal yaw steering gain.'
Assert-Match $source '#define\s+PORTION2_ROUTE11_FINAL_YAW_STEER_LIMIT\s+15\.0f' `
    'route 11 reverse must have enough terminal yaw steering authority.'
Assert-Match $source '#define\s+PORTION2_ROUTE11_FINAL_CRAWL_DIST\s+2\.0f' `
    'route 11 reverse must slow down through the final 2m.'
Assert-Match $source '#define\s+PORTION2_ROUTE11_FINAL_CRAWL_SPEED\s+6\.0f' `
    'route 11 reverse final crawl speed must leave time for heading correction.'
Assert-Match $source 'static\s+uint8\s+portion2_route11_reverse_active\s*\(\s*void\s*\)' `
    'route 11 reverse state must be centralized in a helper.'
Assert-Match $source 'portion2_selected_route\s*==\s*PORTION2_ROUTE_STRAIGHT[\s\S]*?portion2_run_drive_reverse' `
    'route 11 reverse helper must depend on the straight route and reverse-drive flag.'
Assert-Match $source 'if\s*\(\s*portion2_route11_reverse_active\s*\(\s*\)\s*\)[\s\S]*?recorded_terminal_yaw\s*=\s*guandao_normalize_angle\s*\(\s*recorded_terminal_yaw\s*\+\s*180\.0f\s*\)' `
    'route 11 reverse final yaw must be shifted 180 degrees to represent vehicle body heading.'
Assert-Match $source 'portion2_route_uses_terminal_yaw_blend[\s\S]*?portion2_route11_reverse_active\s*\(\s*\)' `
    'route 11 reverse must participate in terminal yaw blending.'
Assert-Match $source 'portion2_terminal_yaw_blend_start[\s\S]*?portion2_route11_reverse_active\s*\(\s*\)[\s\S]*?PORTION2_ROUTE11_TERMINAL_BLEND_START_M' `
    'route 11 reverse must use its own terminal blend start distance.'
Assert-Match $source 'portion2_terminal_yaw_blend_full[\s\S]*?portion2_route11_reverse_active\s*\(\s*\)[\s\S]*?PORTION2_ROUTE11_TERMINAL_BLEND_FULL_M' `
    'route 11 reverse must use its own full terminal blend distance.'
Assert-Match $source 'if\s*\(\s*state\s*==\s*&portion_2\s*&&\s*portion2_route11_reverse_active\s*\(\s*\)[\s\S]*?dist_to_final\s*<=\s*PORTION2_ROUTE11_FINAL_CRAWL_DIST[\s\S]*?PORTION2_ROUTE11_FINAL_CRAWL_SPEED' `
    'route 11 reverse must apply final crawl speed.'

Write-Host 'route 11 reverse terminal yaw checks passed.'
