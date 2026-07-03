$ErrorActionPreference = 'Stop'

$guandao = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\guandao.c')

foreach($pattern in @(
    '#define\s+PORTION2_GUIDED_ROUTE_COUNT\s+10U',
    '#define\s+PORTION2_GUIDED_TERMINAL_BLEND_START_M\s+2\.0f',
    '#define\s+PORTION2_GUIDED_TERMINAL_BLEND_FULL_M\s+1\.0f',
    '#define\s+PORTION2_GUIDED_FINAL_STOP_DIST\s+0\.35f',
    '#define\s+PORTION2_GUIDED_OVERSHOOT_ARM_DIST\s+1\.50f',
    '#define\s+PORTION2_GUIDED_YAW_SLOW_DIST_M\s+3\.0f',
    '#define\s+PORTION2_GUIDED_YAW_SLOW_TRIGGER_DEG\s+20\.0f',
    '#define\s+PORTION2_GUIDED_YAW_SLOW_SPEED\s+8\.0f',
    'static\s+float\s+portion2_guided_terminal_steering\s*\(',
    'portion2_selected_route\s*>=\s*PORTION2_GUIDED_ROUTE_COUNT[\s\S]*?return\s+path_steering',
    'yaw_error\s*=\s*guandao_normalize_angle\s*\(\s*final_yaw\s*-\s*current_yaw\s*\)',
    'target_steering\s*=\s*portion2_guided_terminal_steering\s*\(',
    'portion2_selected_route\s*<\s*PORTION2_GUIDED_ROUTE_COUNT[\s\S]*?PORTION2_GUIDED_FINAL_STOP_DIST',
    'portion2_selected_route\s*<\s*PORTION2_GUIDED_ROUTE_COUNT[\s\S]*?dist_to_final\s*<=\s*PORTION2_GUIDED_YAW_SLOW_DIST_M[\s\S]*?fabsf\s*\(\s*angle_diff\s*\)\s*>=\s*PORTION2_GUIDED_YAW_SLOW_TRIGGER_DEG[\s\S]*?v_center\s*=\s*PORTION2_GUIDED_YAW_SLOW_SPEED',
    'arm_dist\s*=\s*\(\s*portion2_selected_route\s*<\s*PORTION2_GUIDED_ROUTE_COUNT\s*\)[\s\S]*?PORTION2_GUIDED_OVERSHOOT_ARM_DIST'
)) {
    if($guandao -notmatch $pattern) {
        throw "Missing route-2 terminal pose behavior: $pattern"
    }
}

Write-Output 'Routes 1-10 terminal pose checks passed.'
