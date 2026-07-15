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

Assert-Match $gps '#define\s+PORTION2_GPS_STARTUP_MAX_SHIFT_M\s+\(1\.2f\)' `
    'GPS startup alignment must reject route-frame shifts larger than 1.2m.'
Assert-Match $gps '#define\s+PORTION2_GPS_FUSION_HOLD_ERROR\s+\(0\.80f\)' `
    'GPS fusion must stop correcting the inertial position when error exceeds 0.80m.'
Assert-Match $gps '#define\s+PORTION2_GPS_FUSION_DISABLE_ERROR\s+\(1\.00f\)' `
    'GPS fusion must disable itself when error reaches 1.00m.'
Assert-Match $gps '#define\s+PORTION2_GPS_FUSION_DISABLE_COUNT\s+\(2U\)' `
    'GPS fusion must disable after two consecutive large GPS errors.'
Assert-Match $gps 'error\s*>\s*PORTION2_GPS_FUSION_HOLD_ERROR[\s\S]*?last_reason\s*=\s*25[\s\S]*?portion2_gps_fusion_log\s*\(\s*state\s*\)[\s\S]*?return;' `
    'GPS fusion must log and return without modifying current_state when error is above the hold threshold.'
Assert-Match $gps 'error\s*>=\s*PORTION2_GPS_FUSION_DISABLE_ERROR[\s\S]*?large_error_count\+\+[\s\S]*?PORTION2_GPS_FUSION_DISABLE_COUNT[\s\S]*?ready\s*=\s*0[\s\S]*?recovering\s*=\s*1' `
    'GPS fusion must turn off and enter recovery after repeated disable-threshold errors.'

Write-Host 'portion2 strict GPS fusion guard checks passed.'
