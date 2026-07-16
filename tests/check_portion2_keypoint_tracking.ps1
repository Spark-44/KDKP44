$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$source = Get-Content -Raw (Join-Path $repo 'code/guandao.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $source '#define\s+PORTION2_KEYPOINT_TRACE_SEARCH_POINTS\s+3' `
    'routes 1-10 must use a shorter closest-point search window so key points are not skipped.'
Assert-Contains $source '#define\s+PORTION2_KEYPOINT_MIN_PREVIEW_STEPS\s+7' `
    'routes 1-10 must use a shorter steering preview to stay near key points.'
Assert-Contains $source '#define\s+PORTION2_KEYPOINT_CURVE_PREVIEW_STEPS\s+12' `
    'routes 1-10 must use a shorter curve preview to avoid cutting across key turns.'
Assert-Contains $source '#define\s+PORTION2_CURVE_MAX_RAW_DEVIATION_M\s+0\.12f' `
    'portion2 smoothed curve must stay within 0.12m of each raw segment.'
Assert-Contains $source 'static\s+uint8\s+portion2_guided_route_active\s*\(\s*void\s*\)' `
    'route 1-10 specific tracking must use a helper selector.'
Assert-Contains $source 'portion2_guided_route_active\s*\(\s*\)\s*\?\s*PORTION2_KEYPOINT_TRACE_SEARCH_POINTS\s*:\s*GUANDAO_TRACE_SEARCH_POINTS' `
    'routes 1-10 must reduce the closest-point search window.'
Assert-Contains $source 'if\(portion2_guided_route_active\s*\(\s*\)\)[\s\S]*?steer_preview_steps\s*<\s*PORTION2_KEYPOINT_MIN_PREVIEW_STEPS' `
    'routes 1-10 must apply the shorter steering preview.'
Assert-Contains $source 'if\(portion2_guided_route_active\s*\(\s*\)\)[\s\S]*?curve_preview_steps\s*<\s*PORTION2_KEYPOINT_CURVE_PREVIEW_STEPS' `
    'routes 1-10 must apply the shorter curve preview.'
Assert-Contains $source 'uint8\s+force_key_point\s*=\s*\(\s*j\s*==\s*samples\s*\)\s*\?\s*1U\s*:\s*0U' `
    'curve planner must force every raw segment endpoint into the planned route.'
Assert-Contains $source 'portion2_curve_append_point\s*\(\s*state,\s*point,\s*force_key_point\s*\)' `
    'curve planner must not drop raw key points through distance gating.'

Write-Host 'portion2 keypoint tracking checks passed.'
