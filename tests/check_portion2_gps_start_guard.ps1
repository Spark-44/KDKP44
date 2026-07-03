$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$gpsC = Get-Content -Raw (Join-Path $root 'code/gps.c')
$gpsH = Get-Content -Raw (Join-Path $root 'code/gps.h')
$guandao = Get-Content -Raw (Join-Path $root 'code/guandao.c')

function Assert-Match([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Match $gpsC '#define\s+PORTION2_GPS_FUSION_MAX_RMS_ERROR\s+\(1\.20f\)' 'GPS route-fit RMS limit must be 1.20 m.'
Assert-Match $gpsC '#define\s+PORTION2_GPS_STARTUP_REQUIRED_SAMPLES\s+\(3U\)' 'GPS startup must require three stable samples.'
Assert-Match $gpsC '#define\s+PORTION2_GPS_STARTUP_STABILITY_M\s+\(1\.2f\)' 'GPS startup stability radius must be 1.2 m.'
Assert-Match $gpsC '#define\s+PORTION2_GPS_FUSION_MAX_HDOP\s+\(3\.0f\)' 'GPS startup must accept HDOP up to 3.0.'
Assert-Match $gpsH '#define\s+PORTION2_GPS_STARTUP_REJECT\s+3U' 'GPS startup result compatibility must be preserved.'
Assert-Match $gpsC 'startup_shift\s*>\s*PORTION2_GPS_STARTUP_MAX_SHIFT_M[\s\S]*?ready\s*=\s*0[\s\S]*?recovering\s*=\s*0[\s\S]*?last_reason\s*=\s*21[\s\S]*?return\s+PORTION2_GPS_STARTUP_FALLBACK;' 'An untrusted start position must disable GPS and fall back to inertial navigation.'

Assert-Match $guandao 'portion2_route_uses_gps\(portion2_selected_route\)[\s\S]*?if\(!gps_prepare_ready\)[\s\S]*?portion2_run_reject_reason\s*=\s*5;' 'A route with an invalid GPS fit must be rejected before movement.'
if($guandao -match 'gps_startup_result\s*==\s*PORTION2_GPS_STARTUP_REJECT[\s\S]*?portion2_run_reject_reason\s*=\s*6;') {
    throw 'A GPS start mismatch must not block a manually positioned inertial route.'
}

Assert-Match $guandao '#define\s+PORTION2_RAW_TERMINAL_LENGTH_M\s+2\.0f' 'The final 2 m must preserve raw route geometry.'
Assert-Match $guandao 'portion2_terminal_raw_start_index\s*\(' 'A terminal raw-route boundary helper is required.'
Assert-Match $guandao 'if\(i\s*>=\s*terminal_start\)[\s\S]*?portion2_reference_smooth_buffer\[i\]\s*=\s*cur;[\s\S]*?continue;' 'Reference smoothing must skip the terminal raw section.'
Assert-Match $guandao 'keep_terminal_linear[\s\S]*?i\s*>=\s*terminal_start[\s\S]*?keep_corner_linear\s*=\s*keep_terminal_linear' 'The generated plan must use linear interpolation in the terminal section.'

Write-Output 'portion2 GPS startup guard checks passed'
