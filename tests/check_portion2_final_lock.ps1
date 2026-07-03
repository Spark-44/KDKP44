$ErrorActionPreference = 'Stop'

$guandao = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\guandao.c')

foreach($pattern in @(
    '#define\s+PORTION2_FINAL_YAW_ALIGN_SPEED\s+4\.0f',
    '#define\s+PORTION2_FINAL_YAW_ALIGN_MAX_DIST\s+0\.60f',
    '#define\s+PORTION2_FINAL_OVERSHOOT_ARM_DIST\s+1\.00f',
    'static\s+uint8\s+portion2_final_zone_armed',
    'static\s+void\s+portion2_final_zone_reset\s*\(\s*void\s*\)',
    'static\s+uint8\s+portion2_final_zone_overshoot_detect\s*\(',
    'portion2_selected_route\s*==\s*PORTION2_ROUTE_SNAKE[\s\S]*?portion2_final_zone_reset\s*\(\s*\)',
    'arm_dist\s*=\s*\(\s*portion2_selected_route\s*<\s*PORTION2_GUIDED_ROUTE_COUNT\s*\)[\s\S]*?PORTION2_GUIDED_OVERSHOOT_ARM_DIST\s*:\s*PORTION2_FINAL_OVERSHOOT_ARM_DIST',
    'dist_to_final\s*<=\s*arm_dist[\s\S]*?portion2_final_zone_armed\s*=\s*1',
    'dist_to_final\s*>=\s*portion2_final_zone_min_dist\s*\+\s*PORTION2_FINAL_OVERSHOOT_RISE_DIST',
    'raw_point\s*>=\s*raw_length\s*-\s*1[\s\S]*?dist_to_final\s*<=\s*final_stop_distance',
    'portion2_final_zone_overshoot_detect\s*\(\s*raw_point\s*,\s*raw_length\s*,\s*dist_to_final\s*\)',
    'guandao_debug_stop_reason\s*=\s*13'
)) {
    if($guandao -notmatch $pattern) {
        throw "Missing final-zone lock behavior: $pattern"
    }
}

if($guandao -match 'dist_to_final\s*<=\s*final_stop_distance\s*\|\|\s*portion2_final_zone_armed') {
    throw 'Overshoot arming must not enter final yaw alignment before the precise stop zone.'
}

Write-Output 'Portion-2 final-zone lock checks passed.'
