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

Assert-Match $source '#define\s+PORTION2_SNAKE_TERMINAL_BLEND_START_M\s+2\.5f' `
    'route 12 must start blending final yaw before the last 2m.'
Assert-Match $source '#define\s+PORTION2_SNAKE_TERMINAL_BLEND_FULL_M\s+0\.8f' `
    'route 12 must fully prioritize final yaw near the end.'
Assert-Match $source '#define\s+PORTION2_SNAKE_FINAL_YAW_STEER_LIMIT\s+15\.0f' `
    'route 12 final yaw blend must have enough steering authority.'
Assert-Match $source '#define\s+PORTION2_SNAKE_FINAL_YAW_ALIGN_MAX_DIST\s+0\.80f' `
    'route 12 final yaw alignment must be allowed before it reaches the exact stop point.'
Assert-Match $source '#define\s+PORTION2_SNAKE_FINAL_CRAWL_DIST\s+2\.0f' `
    'route 12 must slow down through the final 2m.'
Assert-Match $source '#define\s+PORTION2_SNAKE_FINAL_CRAWL_SPEED\s+6\.0f' `
    'route 12 final crawl speed must leave time for yaw correction.'
Assert-Match $source 'static\s+uint8\s+portion2_route_uses_terminal_yaw_blend\s*\(\s*void\s*\)' `
    'terminal yaw blend must include a route selector helper.'
Assert-Match $source 'portion2_selected_route\s*==\s*PORTION2_ROUTE_SNAKE' `
    'route 12 must be included in terminal yaw handling.'
Assert-Match $source 'portion2_terminal_yaw_blend_start\s*\(\s*\)' `
    'terminal yaw blend must use per-route blend start distance.'
Assert-Match $source 'if\s*\(\s*state\s*==\s*&portion_2\s*&&\s*portion2_selected_route\s*==\s*PORTION2_ROUTE_SNAKE[\s\S]*?dist_to_final\s*<=\s*PORTION2_SNAKE_FINAL_CRAWL_DIST[\s\S]*?PORTION2_SNAKE_FINAL_CRAWL_SPEED' `
    'route 12 must apply its own final crawl speed window.'
Assert-Match $source 'if\s*\(\s*state\s*==\s*&portion_2[\s\S]*?state->current_point_index\s*>=\s*route_length[\s\S]*?portion2_final_yaw_align\s*\(' `
    'when a portion-2 route reaches the final index, it must try final yaw alignment before stopping.'

Write-Host 'route 12 terminal yaw checks passed.'
