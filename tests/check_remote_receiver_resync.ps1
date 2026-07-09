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
$header = Get-Content (Join-Path $root 'libraries\zf_device\zf_device_uart_receiver.h') -Raw
$source = Get-Content (Join-Path $root 'libraries\zf_device\zf_device_uart_receiver.c') -Raw
$isr = Get-Content (Join-Path $root 'user\isr.c') -Raw

Assert-Contains $header 'uint32\s+frame_count' 'SBUS diagnostics must expose accepted frame count.'
Assert-Contains $header 'uint32\s+drop_count' 'SBUS diagnostics must expose dropped byte count.'
Assert-Contains $header 'uint32\s+resync_count' 'SBUS diagnostics must expose resync count.'
Assert-Contains $header 'uint32\s+uart_error_count' 'SBUS diagnostics must expose UART error count.'
Assert-Contains $header 'void\s+uart_receiver_reset_parser\s*\(\s*void\s*\)\s*;' 'SBUS parser reset API must be declared.'
Assert-Contains $header 'void\s+uart_receiver_note_uart_error\s*\(\s*void\s*\)\s*;' 'SBUS UART error API must be declared.'

Assert-Contains $source 'UART_RECEIVER_SYNC_WAIT_HEADER' 'SBUS parser must have a wait-for-header sync state.'
Assert-Contains $source 'UART_RECEIVER_SYNC_IN_FRAME' 'SBUS parser must have an in-frame sync state.'
Assert-Contains $source 'if\s*\(\s*data\s*!=\s*FRAME_STAR\s*\)[\s\S]*?drop_count\+\+' 'SBUS parser must discard bytes until FRAME_STAR is found.'
Assert-Contains $source 'if\s*\(\s*uart_receiver_data\[REV_DATA_LEN\s*-\s*1U\]\s*==\s*FRAME_STAR\s*\)' 'SBUS parser must try to reuse an invalid frame tail byte when it is a new FRAME_STAR.'
Assert-Contains $source 'void\s+uart_receiver_reset_parser\s*\(\s*void\s*\)' 'SBUS parser reset function must be implemented.'
Assert-Contains $source 'void\s+uart_receiver_note_uart_error\s*\(\s*void\s*\)' 'SBUS UART error function must be implemented.'
Assert-Contains $source '(uart_receiver|remote_data)->frame_count\+\+' 'SBUS parser must count accepted frames.'

Assert-Contains $isr 'IFX_INTERRUPT\s*\(\s*uart2_er_isr[\s\S]*?IfxAsclin_Asc_isrError\s*\(&uart2_handle\)\s*;[\s\S]*?uart_receiver_note_uart_error\s*\(\s*\)\s*;' 'UART2 error ISR must reset/resync the SBUS parser.'

Write-Host 'Remote receiver resync/error handling checks passed.'
