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

Assert-Match $guandao '#define\s+PORTION2_CURVE_MAX_RAW_DEVIATION_M\s+0\.30f' `
    'portion2 smooth curve must be bounded within 0.30m of the raw recorded segment.'
Assert-Match $guandao 'static\s+state_t\s+portion2_limit_curve_point_to_segment\s*\(' `
    'portion2 curve planner must clamp smoothed points back toward the raw recorded segment.'
Assert-Match $guandao 'portion2_limit_curve_point_to_segment\s*\(\s*point,\s*p1,\s*p2\s*\)' `
    'portion2 curve planner must apply raw-segment bounding to each Catmull-Rom sample.'

Write-Host 'portion2 curve raw-bound checks passed.'
