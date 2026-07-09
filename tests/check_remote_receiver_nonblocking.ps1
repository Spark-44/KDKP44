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

function Assert-NotContains {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if($Text -match $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$headerPath = Join-Path $root 'libraries\zf_device\zf_device_uart_receiver.h'
$sourcePath = Join-Path $root 'libraries\zf_device\zf_device_uart_receiver.c'

$header = Get-Content $headerPath -Raw
$source = Get-Content $sourcePath -Raw

Assert-Contains $header 'volatile\s+uint16\s+channel\[UART_RECEVIER_CHANNEL_NUM\]' 'SBUS channel data shared with ISR must be volatile.'
Assert-Contains $header 'volatile\s+uint8\s+state' 'SBUS state shared with ISR must be volatile.'
Assert-Contains $header 'volatile\s+uint8\s+finsh_flag' 'SBUS frame flag shared with ISR must be volatile.'

$callbackMatch = [regex]::Match($source, 'static\s+void\s+uart_receiver_callback\s*\(\s*void\s*\)\s*\{([\s\S]*?)\n\}')
if(-not $callbackMatch.Success) {
    throw 'Could not locate uart_receiver_callback().'
}

$callbackBody = $callbackMatch.Groups[1].Value
Assert-Contains $callbackBody 'while\s*\(\s*uart_query_byte\s*\(\s*UART_RECEVIER_UART_INDEX\s*,\s*&data\s*\)\s*\)' 'SBUS ISR callback must drain UART RX FIFO with non-blocking uart_query_byte().'
Assert-NotContains $callbackBody 'uart_read_byte\s*\(' 'SBUS ISR callback must not call blocking uart_read_byte().'
Assert-Contains $source 'static\s+void\s+uart_receiver_process_byte\s*\(\s*uint8\s+data\s*\)' 'SBUS byte parser should be split out from the ISR drain loop.'

Write-Host 'Remote receiver non-blocking ISR checks passed.'
