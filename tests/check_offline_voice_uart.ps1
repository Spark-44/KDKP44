$ErrorActionPreference = 'Stop'

$root = Join-Path $PSScriptRoot '..'
$headerPath = Join-Path $root 'code\offline_voice.h'
$sourcePath = Join-Path $root 'code\offline_voice.c'
$isrPath = Join-Path $root 'user\isr.c'
$mainPath = Join-Path $root 'user\cpu0_main.c'

if(-not (Test-Path $headerPath)) {
    throw 'Missing code\offline_voice.h'
}
if(-not (Test-Path $sourcePath)) {
    throw 'Missing code\offline_voice.c'
}
if(-not (Test-Path $isrPath)) {
    throw 'Missing user\isr.c'
}
if(-not (Test-Path $mainPath)) {
    throw 'Missing user\cpu0_main.c'
}

$header = Get-Content -Raw $headerPath
$source = Get-Content -Raw $sourcePath
$isr = Get-Content -Raw $isrPath
$main = Get-Content -Raw $mainPath

function Get-IsrBlock {
    param(
        [string]$Text,
        [string]$Name
    )
    $pattern = "IFX_INTERRUPT\s*\(\s*$Name[\s\S]*?\n\}"
    $match = [regex]::Match($Text, $pattern)
    if(-not $match.Success) {
        throw "Missing ISR block: $Name"
    }
    return $match.Value
}

$uart1RxIsr = Get-IsrBlock $isr 'uart1_rx_isr'
$uart2RxIsr = Get-IsrBlock $isr 'uart2_rx_isr'

foreach($pattern in @(
    '#define\s+OFFLINE_VOICE_UART_INDEX\s+\(UART_2\)',
    '#define\s+OFFLINE_VOICE_UART_TX_PIN\s+\(UART2_TX_P10_5\)',
    '#define\s+OFFLINE_VOICE_UART_RX_PIN\s+\(UART2_RX_P10_6\)'
)) {
    if($header -notmatch $pattern) {
        throw "Offline voice UART pin setting is missing: $pattern"
    }
}

foreach($pattern in @(
    '\[VOICE-UART2\]\s+CMD_ID=0x'
)) {
    if($source -notmatch $pattern) {
        throw "Offline voice command event label is missing: $pattern"
    }
}

foreach($pattern in @(
    '\[VOICE-UART2\]\s+init\s+9600\s+TX=P10\.5\s+RX=P10\.6',
    '\[VOICE-UART2\]\s+bytes='
)) {
    if($source -match $pattern) {
        throw "Offline voice automatic debug output must stay disabled: $pattern"
    }
}

if($source -match 'VOICE-UART1|UART1_TX_P11_12|UART1_RX_P11_10') {
    throw 'Offline voice code still contains old UART1/P11.12/P11.10 identifiers.'
}

if($uart2RxIsr -notmatch 'offline_voice_uart_rx_handler\s*\(\s*\)') {
    throw 'UART2 RX interrupt must feed offline voice.'
}
if($uart1RxIsr -match 'offline_voice_uart_rx_handler\s*\(\s*\)') {
    throw 'UART1 RX interrupt must no longer feed offline voice.'
}
if($uart2RxIsr -match 'uart_receiver_handler\s*\(\s*\)') {
    throw 'UART2 RX interrupt must not be consumed by the remote receiver.'
}
if($main -notmatch 'portion2_uart1_update_for_mode\s*\(\s*main_mode\s*\)') {
    throw 'UART1 receiver/dot-matrix ownership must follow the operating mode.'
}
if($main -notmatch '(?m)^\s*Portion2_Dot_Matrix_Scan_Update\s*\(\s*\)\s*;') {
    throw 'Voice mode must keep scanning dot-matrix/TLD7002 on UART1.'
}
if($main -notmatch '(?m)^\s*offline_voice_init\s*\(') {
    throw 'Offline voice must initialize on UART2.'
}
if($main -notmatch '(?m)^\s*offline_voice_poll\s*\(\s*\)\s*;') {
    throw 'Offline voice must be polled.'
}
if($main -notmatch '(?m)^\s*remote_control_task\s*\(\s*\)\s*;') {
    throw 'Remote receiver task must continue running in record mode.'
}

Write-Output 'Offline voice enabled on UART2 while UART1 is mode-multiplexed.'
