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
$gps = Get-Content -Raw -Path (Join-Path $repo 'code/gps.c')

Assert-Match $gps '#define\s+PORTION2_GPS_FUSION_DAMP_START_ERROR\s+\(0\.50f\)' `
    'GPS fusion must start damping corrections when inertial/GPS error exceeds 0.50 m.'
Assert-Match $gps '#define\s+PORTION2_GPS_FUSION_DAMP_FULL_ERROR\s+\(0\.90f\)' `
    'GPS fusion must be strongly damped by 0.90 m error.'
Assert-Match $gps '#define\s+PORTION2_GPS_FUSION_MIN_GAIN_SCALE\s+\(0\.25f\)' `
    'GPS fusion must keep only 25 percent correction gain at high disagreement.'
Assert-Match $gps 'static\s+float\s+portion2_gps_fusion_gain_scale\s*\(\s*float\s+error\s*\)' `
    'GPS fusion must centralize error-to-gain damping in a helper.'
Assert-Match $gps 'correction_x\s*=\s*error_x\s*\*\s*PORTION2_GPS_FUSION_GAIN\s*\*\s*gain_scale\s*;' `
    'GPS correction X must use the damped gain scale.'
Assert-Match $gps 'correction_y\s*=\s*error_y\s*\*\s*PORTION2_GPS_FUSION_GAIN\s*\*\s*gain_scale\s*;' `
    'GPS correction Y must use the damped gain scale.'
Assert-Match $gps 'gain=' `
    'GPS fusion logs must include active gain scale for route-5 drift diagnosis.'

Write-Host 'portion2 GPS error damping checks passed.'
