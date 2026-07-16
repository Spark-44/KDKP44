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

function Assert-NotMatch {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )
    if($Text -match $Pattern) {
        throw $Message
    }
}

$repo = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw -Path (Join-Path $repo 'user/cpu0_main.c')
$voiceHeader = Get-Content -Raw -Path (Join-Path $repo 'code/offline_voice.h')
$isr = Get-Content -Raw -Path (Join-Path $repo 'user/isr.c')

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

Assert-Match $main '(?m)^\s*dot_matrix_screen_init\s*\(\s*\)\s*;' `
    'dot matrix screen must be initialized at startup.'
Assert-Match $main '(?m)^\s*Portion2_Dot_Matrix_Scan_Update\s*\(\s*\)\s*;' `
    'RUN mode must continuously scan the dot matrix screen.'
Assert-Match $main 'Portion2_Aux_Start\s*\(\s*data\s*-\s*''A''\s*\+\s*1\s*\)' `
    'A-F serial commands must still select the six dot matrix light patterns.'
Assert-NotMatch $main '(?m)^\s*offline_voice_init\s*\(' `
    'offline voice must not initialize while UART2 is assigned to the remote receiver.'
Assert-NotMatch $main '(?m)^\s*offline_voice_poll\s*\(\s*\)\s*;' `
    'offline voice polling must not run while UART2 is assigned to the remote receiver.'
Assert-Match $main '(?m)^\s*remote_control_init\s*\(\s*\)\s*;' `
    'remote receiver must initialize while connected to P10.5/P10.6.'
Assert-Match $main '(?m)^\s*remote_control_task\s*\(\s*\)\s*;' `
    'remote receiver task must run in the main loop.'
Assert-Match $voiceHeader '#define\s+OFFLINE_VOICE_UART_INDEX\s+\(UART_2\)' `
    'offline voice low-level config must remain unchanged even while disabled.'
Assert-Match $voiceHeader '#define\s+OFFLINE_VOICE_UART_TX_PIN\s+\(UART2_TX_P10_5\)' `
    'offline voice low-level TX pin must remain unchanged.'
Assert-Match $voiceHeader '#define\s+OFFLINE_VOICE_UART_RX_PIN\s+\(UART2_RX_P10_6\)' `
    'offline voice low-level RX pin must remain unchanged.'
Assert-NotMatch $uart1RxIsr 'offline_voice_uart_rx_handler\s*\(\s*\)' `
    'UART1 RX must stay free of offline voice handling for dot matrix coexistence.'
Assert-NotMatch $uart2RxIsr 'offline_voice_uart_rx_handler\s*\(\s*\)' `
    'UART2 RX must not handle offline voice while the remote receiver is active.'
Assert-Match $uart2RxIsr 'uart_receiver_handler\s*\(\s*\)' `
    'UART2 RX must handle the remote receiver.'

Write-Host 'portion2 dot matrix RUN checks passed.'
