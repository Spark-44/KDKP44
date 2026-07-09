$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$header = Get-Content -Raw (Join-Path $repo 'code/guandao.h')
$source = Get-Content -Raw (Join-Path $repo 'code/guandao.c')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $header 'void\s+portion2_serial_toggle_route_status_live\s*\(\s*void\s*\)' 'route status live toggle must be declared'
Assert-Contains $header 'void\s+portion2_serial_route_status_live_task\s*\(\s*void\s*\)' 'route status live task must be declared'
Assert-Contains $source '#define\s+PORTION2_ROUTE_STATUS_LIVE_PERIOD_MS\s+\(1000U\)' 'route status live output must use a 1 second period'
Assert-Contains $source 'static\s+uint8\s+portion2_route_status_live_enabled\s*=\s*0' 'route status live must default off'
Assert-Contains $source 'void\s+portion2_serial_toggle_route_status_live\s*\(\s*void\s*\)[\s\S]*?portion2_route_status_live_enabled\s*=\s*!portion2_route_status_live_enabled' 'L command must toggle live route status'
Assert-Contains $source '\[P2-ROUTE-LIVE\]\s+enabled' 'route status live must announce enabled state'
Assert-Contains $source '\[P2-ROUTE-LIVE\]\s+disabled' 'route status live must announce disabled state'
Assert-Contains $source 'for\s*\(\s*uint8\s+i\s*=\s*0\s*;\s*i\s*<\s*PORTION2_ROUTE_COUNT\s*;\s*i\+\+\s*\)' 'route status live must iterate all configured routes'
Assert-Contains $source 'portion2_route_length\[i\]\s*>\s*0[\s\S]*?"YES"\s*:\s*"NO"' 'route status live marked field must reflect whether points exist'
Assert-Contains $source '\[P2-ROUTE-LIVE\]\s+route=%02u\s+pts=%03u\s+gps=%02u\s+marked=%s' 'route status live output must include route, points, GPS and marked fields'
Assert-Contains $main "data\s*==\s*'L'[\s\S]*?portion2_serial_toggle_route_status_live\s*\(\s*\)" 'serial L must toggle live route status'
Assert-Contains $main 'portion2_serial_route_status_live_task\s*\(\s*\);[\s\S]*?switch\s*\(\s*main_mode\s*\)' 'live route status task must run before mode-specific work'

Write-Output 'Route status live checks passed.'
