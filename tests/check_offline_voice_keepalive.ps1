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
$source = Get-Content -Raw -Path (Join-Path $repo 'code/offline_voice.c')
$header = Get-Content -Raw -Path (Join-Path $repo 'code/offline_voice.h')

Assert-Match $source '#define\s+OFFLINE_VOICE_KEEPALIVE_MS\s+\(12000U\)' `
    'offline voice must send keepalive before the 15s module sleep timeout.'
Assert-Match $source 'static\s+uint32\s+offline_voice_last_valid_ms\s*=\s*0' `
    'offline voice must track the last valid frame time.'
Assert-Match $source 'static\s+void\s+offline_voice_send_wakeup\s*\(\s*void\s*\)' `
    'offline voice must provide a wakeup sender.'
Assert-Match $source 'offline_voice_send_response\s*\(\s*OFFLINE_VOICE_CMD_WAKEUP\s*\)' `
    'wakeup sender must transmit the wakeup command frame.'
Assert-Match $source 'static\s+void\s+offline_voice_keepalive\s*\(\s*void\s*\)' `
    'offline voice must check for idle keepalive in poll.'
Assert-Match $source 'if\s*\(\s*\(uint32\)\s*\(\s*now\s*-\s*offline_voice_last_valid_ms\s*\)\s*>=\s*OFFLINE_VOICE_KEEPALIVE_MS\s*\)' `
    'offline voice keepalive must use elapsed time from the last valid frame.'
Assert-Match $source 'offline_voice_keepalive\s*\(\s*\)\s*;\s*\r?\n\}' `
    'offline_voice_poll must run the keepalive check.'
Assert-Match $source 'if\s*\(\s*cmd_id\s*==\s*OFFLINE_VOICE_CMD_SLEEP\s*\)[\s\S]*?offline_voice_send_wakeup\s*\(\s*\)' `
    'receiving the module sleep command must immediately send wakeup.'
Assert-Match $source 'offline_voice_last_valid_ms\s*=\s*system_getval_ms\s*\(\s*\)' `
    'valid voice frames and wakeup sends must refresh the keepalive timer.'
Assert-Match $header '#define\s+OFFLINE_VOICE_CMD_SLEEP\s+\(0x03\)' `
    'offline voice sleep command id must remain defined.'

Write-Host 'offline voice keepalive checks passed.'
