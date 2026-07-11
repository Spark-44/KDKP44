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

function Assert-NotMatch {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )
    if($Text -match $Pattern) {
        throw $Message
    }
}

$repo = Split-Path -Parent $PSScriptRoot
$gpsC = Get-Content -Raw -Path (Join-Path $repo 'code/gps.c')

$startup = [regex]::Match($gpsC, 'uint8\s+portion2_gps_fusion_startup_update\s*\([^)]*\)[\s\S]*?\n}\r?\n\r?\nvoid\s+portion2_gps_fusion_update')
if(!$startup.Success) {
    throw 'Could not locate portion2_gps_fusion_startup_update.'
}

Assert-Match $startup.Value 'PORTION2_GPS_STARTUP_REQUIRED_SAMPLES\s+\(3U\)|PORTION2_GPS_STARTUP_REQUIRED_SAMPLES' `
    'Startup must still use the configured sample count.'

Assert-NotMatch $startup.Value 'hypotf\(last_east,\s*last_north\)\s*<\s*PORTION2_GPS_FUSION_REPEAT_DISTANCE[\s\S]*?return\s+PORTION2_GPS_STARTUP_WAIT' `
    'Startup GPS calibration must allow repeated coordinates to count as stable samples.'

$runtime = [regex]::Match($gpsC, 'void\s+portion2_gps_fusion_update\s*\([^)]*\)[\s\S]*?\n}\r?\n\r?\nuint8\s+portion2_gps_fusion_is_ready')
if(!$runtime.Success) {
    throw 'Could not locate portion2_gps_fusion_update.'
}

Assert-Match $runtime.Value 'hypotf\(last_east,\s*last_north\)\s*<\s*PORTION2_GPS_FUSION_REPEAT_DISTANCE[\s\S]*?last_reason\s*=\s*5[\s\S]*?return;' `
    'Runtime GPS fusion must keep repeat filtering after startup.'

Write-Host 'GPS startup repeat-coordinate checks passed.'
