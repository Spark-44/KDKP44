$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$remote = Get-Content -Raw (Join-Path $repo 'code/remote_control.c')
$guandao = Get-Content -Raw (Join-Path $repo 'code/guandao.c')
$voice = Get-Content -Raw (Join-Path $repo 'code/offline_voice.c')
$screen = Get-Content -Raw (Join-Path $repo 'code/screen.c')

function Assert-NotContains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) {
        throw $Message
    }
}

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-NotContains $remote '\[REMOTE-SBUS\]\s+ch1=' 'remote channel status must not auto-refresh on the debug serial port.'
Assert-NotContains $remote '\[REMOTE-SBUS\]\s+failsafe' 'remote failsafe status must not print automatically.'
Assert-NotContains $remote '\[REMOTE-SBUS\]\s+init' 'remote init status must not print automatically.'
Assert-NotContains $remote 'remote_control_debug_print\s*\(\s*\)\s*;' 'remote task must not call periodic debug printing by default.'

Assert-NotContains $guandao '\[P2-REC-GPS-SKIP\]' 'record GPS skip diagnostics must not auto-refresh on the debug serial port.'

Assert-NotContains $voice '\[VOICE-UART1\]\s+bytes=' 'offline voice stats must not auto-refresh on the debug serial port.'
Assert-NotContains $voice '\[VOICE-UART1\]\s+init' 'offline voice init must not print automatically.'
Assert-Contains $voice '\[VOICE-UART1\]\s+CMD_ID=0x' 'actual offline voice command events may still print.'

Assert-NotContains $screen '===== CI1302 Screen Ready =====' 'screen init banner must not print automatically.'
Assert-NotContains $screen '\[CI1302-IDLE\]' 'screen idle diagnostics must not print automatically.'
Assert-NotContains $screen '\[CI1302-RAW\]' 'screen raw diagnostics must not print automatically.'
Assert-NotContains $screen '\[CI1302-STAT\]' 'screen stats diagnostics must not print automatically.'
Assert-Contains $screen '\[CI1302\]\s+CMD_ID=0x' 'actual CI1302 command events may still print.'

Write-Output 'Serial automatic debug silence checks passed.'
