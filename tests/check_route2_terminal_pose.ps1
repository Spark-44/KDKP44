$ErrorActionPreference = 'Stop'

$guandao = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\guandao.c')

foreach($pattern in @(
    '#define\s+PORTION2_ROUTE2_ID\s+1U',
    '#define\s+PORTION2_ROUTE2_TERMINAL_BLEND_START_M\s+2\.0f',
    '#define\s+PORTION2_ROUTE2_TERMINAL_BLEND_FULL_M\s+1\.0f',
    '#define\s+PORTION2_ROUTE2_FINAL_STOP_DIST\s+0\.35f',
    'static\s+float\s+portion2_route2_terminal_steering\s*\(',
    'portion2_selected_route\s*!=\s*PORTION2_ROUTE2_ID[\s\S]*?return\s+path_steering',
    'yaw_error\s*=\s*guandao_normalize_angle\s*\(\s*final_yaw\s*-\s*current_yaw\s*\)',
    'target_steering\s*=\s*portion2_route2_terminal_steering\s*\(',
    'portion2_selected_route\s*==\s*PORTION2_ROUTE2_ID[\s\S]*?PORTION2_ROUTE2_FINAL_STOP_DIST'
)) {
    if($guandao -notmatch $pattern) {
        throw "Missing route-2 terminal pose behavior: $pattern"
    }
}

Write-Output 'Route-2 terminal pose checks passed.'
