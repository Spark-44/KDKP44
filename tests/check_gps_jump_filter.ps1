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
$gpsC = Get-Content -Raw -Path (Join-Path $repo 'code/gps.c')
$guandao = Get-Content -Raw -Path (Join-Path $repo 'code/guandao.c')

Assert-Match $gpsC '#define\s+PORTION2_GPS_FUSION_MAX_JUMP_M\s+\(1\.0f\)' `
    'Run-mode GPS fusion must reject one-frame jumps above 1.0 m.'

Assert-Match $gpsC 'PORTION2_GPS_FUSION_REASON_JUMP\s+\(24U\)' `
    'Run-mode GPS fusion must expose a distinct jump rejection reason.'

Assert-Match $gpsC 'if\(hypotf\(last_east,\s*last_north\)\s*>\s*PORTION2_GPS_FUSION_MAX_JUMP_M\)[\s\S]*?last_reason\s*=\s*PORTION2_GPS_FUSION_REASON_JUMP[\s\S]*?portion2_gps_fusion_log\(state\);[\s\S]*?return;' `
    'Run-mode GPS fusion must drop jump samples before applying correction.'

$jumpReject = [regex]::Match($gpsC, 'if\(hypotf\(last_east,\s*last_north\)\s*>\s*PORTION2_GPS_FUSION_MAX_JUMP_M\)[\s\S]*?return;')
if(!$jumpReject.Success) {
    throw 'Jump rejection block not found.'
}
if($jumpReject.Value -match 'last_lat\s*=|last_lon\s*=') {
    throw 'Rejected GPS jump samples must not replace the last accepted coordinate.'
}

Assert-Match $guandao '#define\s+PORTION2_GPS_RECORD_MAX_JUMP_MARGIN_M\s+0\.8f' `
    'Record-mode GPS jump margin must stay tightened to 0.8 m.'
Assert-Match $guandao 'gps_distance\s*>\s*inertial_distance\s*\+\s*PORTION2_GPS_RECORD_MAX_JUMP_MARGIN_M[\s\S]*?PORTION2_GPS_REJECT_JUMP' `
    'Record-mode GPS points must reject jumps beyond inertial movement plus margin.'

Write-Host 'GPS jump filter checks passed.'
