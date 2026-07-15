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
$guandao = Get-Content -Raw -Path (Join-Path $repo 'code/guandao.c')

Assert-Match $guandao '#define\s+PORTION2_STEERING_CMD_LIMIT\s+22\.0f' `
    'normal portion2 steering limit must be raised to 22 degrees.'
Assert-Match $guandao '#define\s+PORTION2_SHARP_STEERING_CMD_LIMIT\s+34\.0f' `
    'sharp portion2 steering limit must be raised to 34 degrees.'
Assert-Match $guandao '#define\s+PORTION2_SMOOTH_STEER_RATE_LIMIT\s+\(0\.75f\)' `
    'normal portion2 steering slew rate must be raised to 0.75 deg per 20ms.'

Assert-Match $guandao 'static\s+state_t\s+portion2_catmull_rom_point\s*\(' `
    'portion2 route planning must use Catmull-Rom interpolation.'
Assert-Match $guandao 'PORTION2_CURVE_SAMPLE_STEP_M' `
    'portion2 route planning must resample the smooth curve at a fixed distance step.'
Assert-Match $guandao 'portion2_build_curve_plan\s*\(\s*state\s*\)' `
    'guandao_build_smooth_plan must delegate portion2 routes to the curve planner.'
Assert-Match $guandao 'portion2_curve_append_point\s*\(' `
    'curve planner must append distance-gated points instead of following every raw point.'

Assert-Match $guandao 'steer_limit=' `
    'P2-TRACK diagnostics must include the active steering limit.'
Assert-Match $guandao 'steer_cmd=' `
    'P2-TRACK diagnostics must include the unsigned steering command before servo sign inversion.'
Assert-Match $guandao 'rate=' `
    'P2-TRACK diagnostics must include the active steering rate limit.'
Assert-Match $guandao 'ips200_show_string\s*\(\s*X\(1\),\s*Y\(15\),\s*"ST"\s*\)' `
    'portion2 run screen must show the live steering output.'
Assert-Match $guandao 'ips200_show_float\s*\(\s*X\(13\),\s*Y\(15\),\s*guandao_debug_steer_limit' `
    'portion2 run screen must show the active steering limit.'

Write-Host 'portion2 curve smoothing checks passed.'
