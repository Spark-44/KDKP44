$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$header = Get-Content -Raw (Join-Path $repo 'code/guandao.h')
$source = Get-Content -Raw (Join-Path $repo 'code/guandao.c')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')

function Assert-NotContains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) {
        throw $Message
    }
}

Assert-NotContains $header 'portion2_serial_toggle_route_status_live' 'route status live toggle must be removed from the public API.'
Assert-NotContains $header 'portion2_serial_route_status_live_task' 'route status live periodic task must be removed from the public API.'
Assert-NotContains $source 'PORTION2_ROUTE_STATUS_LIVE_PERIOD_MS' 'serial route live period must be removed.'
Assert-NotContains $source 'portion2_route_status_live_enabled' 'serial route live enabled state must be removed.'
Assert-NotContains $source 'portion2_serial_toggle_route_status_live' 'serial route live toggle implementation must be removed.'
Assert-NotContains $source 'portion2_serial_route_status_live_task' 'serial route live periodic implementation must be removed.'
Assert-NotContains $main "data\s*==\s*'L'[\s\S]*?portion2_serial_toggle_route_status_live" 'serial L must no longer toggle live route status.'
Assert-NotContains $main 'portion2_serial_route_status_live_task\s*\(\s*\)' 'main loop must not call a periodic serial route status refresh.'

Write-Output 'Serial route live refresh removal checks passed.'
