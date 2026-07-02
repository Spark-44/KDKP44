$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$gpsC = Get-Content -Raw (Join-Path $root 'code/gps.c')
$gpsH = Get-Content -Raw (Join-Path $root 'code/gps.h')
$guandao = Get-Content -Raw (Join-Path $root 'code/guandao.c')
$isr = Get-Content -Raw (Join-Path $root 'user/isr.c')

function Assert-Match([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { throw $Message }
}

Assert-Match $gpsH 'portion2_gps_get_rmc_sequence\s*\(' 'A dedicated RMC position sequence getter is required.'
Assert-Match $gpsC 'portion2_gps_rmc_sequence' 'A dedicated RMC position sequence counter is required.'
Assert-Match $gpsC 'portion2_gps_note_parsed_update\s*\(uint8\s+parse_result\)' 'Parsed sentence type must be passed to the sequence update.'
Assert-Match $gpsC 'parse_result\s*&\s*GNSS_PARSE_RMC_OK[\s\S]*?portion2_gps_rmc_sequence\+\+' 'Only successful RMC parsing may advance the position sequence.'
Assert-Match $isr 'portion2_gps_note_parsed_update\s*\(parse_result\)' 'ISR must pass the parsed sentence flags to GPS sequencing.'

Assert-Match $guandao '#define\s+PORTION2_GPS_ORIGIN_SAMPLE_COUNT\s+5U' 'The route origin must use five RMC samples.'
Assert-Match $guandao '#define\s+PORTION2_GPS_FILTER_SAMPLE_COUNT\s+3U' 'Later GPS anchors must use a three-sample median.'
Assert-Match $guandao '#define\s+PORTION2_GPS_ORIGIN_STABILITY_M\s+\(1\.0f\)' 'Origin sample spread must be limited to 1.0 m.'
Assert-Match $guandao '#define\s+PORTION2_GPS_RECORD_MAX_JUMP_MARGIN_M\s+0\.8f' 'GPS jump margin must be reduced to 0.8 m.'
Assert-Match $guandao 'portion2_gps_median' 'GPS coordinates must be median filtered.'
Assert-Match $guandao 'portion2_gps_get_rmc_sequence\s*\(' 'Recording must consume the RMC-only sequence.'
Assert-Match $guandao 'portion2_record_start_pending' 'K3 must wait for a stable origin without requiring another press.'
Assert-Match $guandao 'void\s+portion2_record_reset\s*\([^)]*\)[\s\S]*?portion2_record_start_pending\s*=\s*0[\s\S]*?void\s+portion2_record_mark_loaded_routes_saved' 'A full record reset must cancel a pending GPS start.'
Assert-Match $guandao 'portion2_gps_filter_update[\s\S]*?PORTION2_GPS_REJECT_NO_FIX[\s\S]*?PORTION2_GPS_REJECT_LOW_SAT[\s\S]*?PORTION2_GPS_REJECT_BAD_HDOP' 'Invalid RMC samples must expose their quality rejection reason.'
Assert-Match $guandao 'if\(!portion2_record_try_gps_point\(0\)\)[\s\S]*?portion2_record_start_pending\s*=\s*1[\s\S]*?return;' 'Recording must not start until the filtered origin anchor is stored.'
Assert-Match $guandao 'portion2_route_required_gps_count[^;]*\{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8\}' 'Every route must require eight trustworthy GPS anchors.'

Assert-Match $gpsC '#define\s+PORTION2_GPS_FUSION_MAX_ANCHOR_RESIDUAL\s+\(1\.5f\)' 'Fit outliers must be clipped above 1.5 m.'
Assert-Match $gpsC '#define\s+PORTION2_GPS_FUSION_MIN_INLIERS\s+\(8U\)' 'Route fitting must require eight inliers.'
Assert-Match $gpsC 'portion2_gps_fit_transform' 'Similarity fitting must be reusable for the refit.'
Assert-Match $gpsC 'inlier_mask\s*\[PORTION2_GPS_PER_ROUTE\]' 'Robust fitting must use a fixed-size inlier mask.'
Assert-Match $gpsC 'residual\s*<=\s*PORTION2_GPS_FUSION_MAX_ANCHOR_RESIDUAL' 'Residual clipping must mark only acceptable anchors as inliers.'
Assert-Match $gpsC 'anchors=%u inliers=%u removed=%u' 'Fusion diagnostics must report total, inlier, and removed anchors.'

Write-Output 'portion2 GPS quality checks passed'
