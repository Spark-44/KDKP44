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

foreach($pattern in @(
    '#define\s+OFFLINE_VOICE_UART_INDEX\s+\(UART_1\)',
    '#define\s+OFFLINE_VOICE_UART_TX_PIN\s+\(UART1_TX_P11_12\)',
    '#define\s+OFFLINE_VOICE_UART_RX_PIN\s+\(UART1_RX_P11_10\)'
)) {
    if($header -notmatch $pattern) {
        throw "Offline voice UART pin setting is missing: $pattern"
    }
}

foreach($pattern in @(
    '\[VOICE-UART1\]\s+CMD_ID=0x'
)) {
    if($source -notmatch $pattern) {
        throw "Offline voice command event label is missing: $pattern"
    }
}

foreach($pattern in @(
    '\[VOICE-UART1\]\s+init\s+9600\s+TX=P11\.12\s+RX=P11\.10',
    '\[VOICE-UART1\]\s+bytes='
)) {
    if($source -match $pattern) {
        throw "Offline voice automatic debug output must stay disabled: $pattern"
    }
}

if($source -match 'VOICE-UART2|UART2_TX_P10_5|UART2_RX_P10_6') {
    throw 'Offline voice code still contains old UART2/P10.5/P10.6 identifiers.'
}

if($isr -notmatch 'IFX_INTERRUPT\s*\(\s*uart1_rx_isr[\s\S]*?offline_voice_uart_rx_handler\s*\(\s*\)') {
    throw 'UART1 RX interrupt must feed offline_voice_uart_rx_handler().'
}
if($isr -match 'IFX_INTERRUPT\s*\(\s*uart2_rx_isr[\s\S]*?offline_voice_uart_rx_handler\s*\(\s*\)') {
    throw 'UART2 RX interrupt must no longer feed offline voice.'
}
if($isr -match 'IFX_INTERRUPT\s*\(\s*uart1_rx_isr[\s\S]*?tld7002_callback\s*\(\s*\)') {
    throw 'UART1 RX interrupt must no longer be consumed by TLD7002 when UART1 is assigned to offline voice.'
}
if($main -match '(?m)^\s*dot_matrix_screen_init\s*\(\s*\)\s*;') {
    throw 'Dot-matrix/TLD7002 init must be disabled when UART1 is assigned to offline voice.'
}
if($main -match '(?m)^\s*Portion2_Dot_Matrix_Scan_Update\s*\(\s*\)\s*;') {
    throw 'Voice mode must not scan dot-matrix/TLD7002 while UART1 is assigned to offline voice.'
}
if($isr -match '(?m)^\s*dot_matrix_screen_scan\s*\(\s*\)\s*;') {
    throw 'Dot-matrix EXTI scan must be disabled when UART1 is assigned to offline voice.'
}

Write-Output 'Offline voice UART1/P11.10/P11.12 checks passed.'
