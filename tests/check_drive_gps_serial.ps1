$ErrorActionPreference = 'Stop'

function Assert-Contains {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$main = Get-Content (Join-Path $root 'user\cpu0_main.c') -Raw

Assert-Contains $main '#define\s+PORTION2_DRIVE_GPS_SERIAL_PERIOD_MS\s+\(500U\)' 'Drive GPS serial output must be rate-limited to 500ms.'
Assert-Contains $main 'static\s+void\s+Portion2_Drive_GPS_Serial_Update\s*\(\s*void\s*\)' 'Drive mode must have a dedicated GPS serial update helper.'
Assert-Contains $main 'Portion2_Drive_GPS_Serial_Update\s*\(\s*void\s*\)[\s\S]*?main_mode\s*!=\s*Guandao_Drive[\s\S]*?return\s*;' 'Drive GPS serial helper must only print in drive mode.'
Assert-Contains $main 'Portion2_Drive_GPS_Serial_Update\s*\(\s*void\s*\)[\s\S]*?now_ms\s*-\s*last_gps_ms[\s\S]*?PORTION2_DRIVE_GPS_SERIAL_PERIOD_MS' 'Drive GPS serial helper must gate output by the configured period.'
Assert-Contains $main '\[DRIVE-GPS\]\s+state=%u\s+sats=%u\s+hdop100=%u\s+lat7=%ld\s+lon7=%ld' 'Drive GPS serial output must include state, satellite count, HDOP, and coordinates.'
Assert-Contains $main 'gnss\.state[\s\S]*?gnss\.satellite_used[\s\S]*?gnss\.hdop[\s\S]*?gnss\.latitude[\s\S]*?gnss\.longitude' 'Drive GPS serial output must read the current GNSS fields.'
Assert-Contains $main 'static\s+void\s+Portion2_Drive_Mode_Task\s*\(\s*void\s*\)[\s\S]*?Portion2_Drive_GPS_Serial_Update\s*\(\s*\)' 'Drive task must call the GPS serial helper.'

Write-Host 'Drive GPS serial checks passed.'
