Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$source = Get-Content -Raw -Path (Join-Path $repo 'code/offline_voice.c')
$header = Get-Content -Raw -Path (Join-Path $repo 'code/offline_voice.h')

if($source -match '#define\s+OFFLINE_VOICE_KEEPALIVE_MS\s+\(12000U\)|offline_voice_keepalive\s*\(') {
    throw 'offline voice must not send an automatic 12-second keepalive.'
}
if($source -match 'offline_voice_last_valid_ms|offline_voice_send_wakeup\s*\(') {
    throw 'offline voice must not retain automatic wakeup state or sender.'
}
if($source -notmatch 'offline_voice_send_response\s*\(\s*cmd_id\s*\)') {
    throw 'normal voice command responses must remain enabled.'
}
if($header -notmatch '#define\s+OFFLINE_VOICE_CMD_SLEEP\s+\(0x03\)') {
    throw 'offline voice sleep command id must remain defined.'
}

Write-Host 'offline voice automatic keepalive removal checks passed.'
