$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$header = Get-Content -Raw (Join-Path $repo 'code/guandao.h')
$source = Get-Content -Raw (Join-Path $repo 'code/guandao.c')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')
$flash = Get-Content -Raw (Join-Path $repo 'code/flash.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $header '#define\s+PORTION2_ROUTE_COUNT\s+12' 'route count must be 12'
Assert-Contains $header '#define\s+PORTION2_TOTAL_ROUTE_POINTS\s+588' 'inertial capacity must be 12 x 49'
Assert-Contains $header '#define\s+PORTION2_TOTAL_GPS_COUNT\s+240' 'GPS capacity must be 12 x 20'
Assert-Contains $header '#define\s+PORTION2_ROUTE_RETURN_1\s+5' 'route 6 must replace old reverse route 1'
Assert-Contains $header '#define\s+PORTION2_ROUTE_RETURN_5\s+9' 'route 10 must replace old reverse route 5'
Assert-Contains $header '#define\s+PORTION2_ROUTE_STRAIGHT\s+10' 'route 11 must be the straight route'
Assert-Contains $header '#define\s+PORTION2_ROUTE_SNAKE\s+11' 'route 12 must be the snake route'
Assert-Contains $source 'const uint8 portion2_route_required_gps_count\[PORTION2_ROUTE_COUNT\]\s*=\s*\{(?:\s*5,){11}\s*5\s*\}' 'all 12 routes must require GPS points'
Assert-Contains $flash 'PORTION2_ROUTE_CONTINUATION_MAGIC' 'expanded inertial records must use a continuation flash page'

$voiceMappings = @(
    'OFFLINE_VOICE_CMD_GATE1_RIGHT_BACK[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_RETURN_1\)',
    'OFFLINE_VOICE_CMD_GATE3_LEFT_BACK[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_RETURN_5\)',
    'OFFLINE_VOICE_CMD_FORWARD_10M[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_STRAIGHT\)',
    'OFFLINE_VOICE_CMD_BACKWARD_10M[\s\S]*?portion2_run_select_back_route\(PORTION2_ROUTE_STRAIGHT\)',
    'OFFLINE_VOICE_CMD_SNAKE_FORWARD[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_SNAKE\)',
    'OFFLINE_VOICE_CMD_SNAKE_BACKWARD[\s\S]*?portion2_run_select_back_route\(PORTION2_ROUTE_SNAKE\)'
)

foreach ($mapping in $voiceMappings) {
    Assert-Contains $main $mapping "missing voice mapping: $mapping"
}

Write-Host 'portion-2 12-route layout checks passed'
