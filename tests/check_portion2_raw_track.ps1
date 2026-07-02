$ErrorActionPreference = 'Stop'

$guandao = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\guandao.c')

if($guandao -match 'static\s+void\s+portion2_shape_terminal_pose\s*\(') {
    throw 'The final 1.5m route is still forcibly reshaped.'
}

foreach($pattern in @(
    'static\s+state_t\s+portion2_raw_route_origin',
    'static\s+float\s+portion2_raw_route_cos',
    'static\s+float\s+portion2_raw_route_sin',
    'static\s+void\s+portion2_capture_raw_transform\s*\(\s*float\s+yaw_delta\s*\)',
    'static\s+state_t\s+portion2_aligned_raw_route_point\s*\(\s*int16\s+raw_index\s*\)',
    'portion2_route_storage_get\s*\(',
    'static\s+portion2_track_sample_t\s+portion2_raw_track_sample\s*\(\s*int16\s+run_index\s*\)',
    'portion2_capture_raw_transform\s*\(\s*yaw_delta\s*\)[\s\S]*?portion2_gps_fusion_prepare\s*\(\s*&portion_2\s*\)[\s\S]*?portion2_smooth_reference_route\s*\(\s*\)',
    'off_plan=',
    'off_raw=',
    'raw_sample\.abs_error\s*>=\s*PORTION2_TRACK_BAD_THRESHOLD_M'
)) {
    if($guandao -notmatch $pattern) {
        throw "Missing raw-route tracking behavior: $pattern"
    }
}

if($guandao -match 'portion2_raw_route_buffer\s*\[\s*MAX_LENGTH_INDEX\s*\]') {
    throw 'Raw-route diagnostics must not allocate another full route buffer.'
}

Write-Output 'Portion-2 raw-route tracking checks passed.'
