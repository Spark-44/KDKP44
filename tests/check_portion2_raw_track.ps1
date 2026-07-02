$ErrorActionPreference = 'Stop'

$guandao = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\guandao.c')

if($guandao -match 'static\s+void\s+portion2_shape_terminal_pose\s*\(') {
    throw 'The final 1.5m route is still forcibly reshaped.'
}

foreach($pattern in @(
    'static\s+state_t\s+portion2_raw_route_buffer\s*\[\s*MAX_LENGTH_INDEX\s*\]',
    'static\s+void\s+portion2_capture_raw_route\s*\(\s*void\s*\)',
    'static\s+portion2_track_sample_t\s+portion2_raw_track_sample\s*\(\s*int16\s+run_index\s*\)',
    'portion2_capture_raw_route\s*\(\s*\)[\s\S]*?portion2_gps_fusion_prepare\s*\(\s*&portion_2\s*\)[\s\S]*?portion2_smooth_reference_route\s*\(\s*\)',
    'off_plan=',
    'off_raw=',
    'raw_sample\.abs_error\s*>=\s*PORTION2_TRACK_BAD_THRESHOLD_M'
)) {
    if($guandao -notmatch $pattern) {
        throw "Missing raw-route tracking behavior: $pattern"
    }
}

Write-Output 'Portion-2 raw-route tracking checks passed.'
