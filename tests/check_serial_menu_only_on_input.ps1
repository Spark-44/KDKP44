$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotContains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) {
        throw $Message
    }
}

$loopStart = $main.IndexOf('while (TRUE)')
if ($loopStart -lt 0) {
    throw 'main loop must contain while (TRUE).'
}
$loop = $main.Substring($loopStart)

Assert-Contains $loop 'Portion2_Serial_Command_Update\s*\(\s*\);' 'serial input polling must stay enabled.'
Assert-Contains $main 'serial_menu_handle_byte\s*\(\s*data\s*\)' 'serial menu must keep handling key-triggered menu refresh.'
Assert-NotContains $loop 'Record_Idle_Encoder_Diag_Update\s*\(\s*\);' 'record idle encoder diagnostic must not refresh serial output periodically.'
Assert-NotContains $loop 'Rear_Motor_Serial_Telemetry_Update\s*\(\s*\);' 'rear motor speed telemetry must not refresh serial output periodically.'
Assert-NotContains $loop 'gps_serial_diagnostic_task\s*\(\s*\);' 'GNSS diagnostics must not refresh serial output periodically.'

Write-Output 'Serial menu only-on-input checks passed.'
