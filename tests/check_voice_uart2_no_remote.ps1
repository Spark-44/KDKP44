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

$repo = Split-Path -Parent $PSScriptRoot
$voiceHeader = Get-Content -Raw -Path (Join-Path $repo 'code/offline_voice.h')
$voiceSource = Get-Content -Raw -Path (Join-Path $repo 'code/offline_voice.c')
$main = Get-Content -Raw -Path (Join-Path $repo 'user/cpu0_main.c')
$isr = Get-Content -Raw -Path (Join-Path $repo 'user/isr.c')
$uart1RxIsr = Get-IsrBlock $isr 'uart1_rx_isr'
$uart2RxIsr = Get-IsrBlock $isr 'uart2_rx_isr'
$remoteSourcePath = Join-Path $repo 'code/remote_control.c'
$remoteReceiverHeaderPath = Join-Path $repo 'libraries/zf_device/zf_device_uart_receiver.h'

if(-not (Test-Path $remoteSourcePath)) {
    throw 'remote control driver source must remain in the repository.'
}
if(-not (Test-Path $remoteReceiverHeaderPath)) {
    throw 'remote receiver UART driver header must remain in the repository.'
}
$remoteReceiverHeader = Get-Content -Raw -Path $remoteReceiverHeaderPath

Assert-Match $voiceHeader '#define\s+OFFLINE_VOICE_UART_INDEX\s+\(UART_2\)' `
    'offline voice low-level UART setting must stay unchanged for future voice use.'
Assert-Match $voiceHeader '#define\s+OFFLINE_VOICE_UART_TX_PIN\s+\(UART2_TX_P10_5\)' `
    'offline voice low-level TX pin must stay unchanged.'
Assert-Match $voiceHeader '#define\s+OFFLINE_VOICE_UART_RX_PIN\s+\(UART2_RX_P10_6\)' `
    'offline voice low-level RX pin must stay unchanged.'
Assert-Match $voiceSource '\[VOICE-UART2\]\s+CMD_ID=0x' `
    'voice command diagnostics must identify UART2.'
Assert-Match $remoteReceiverHeader '#define\s+UART_RECEVIER_UART_INDEX\s+\(UART_1\)' `
    'remote receiver must use UART1.'
Assert-Match $remoteReceiverHeader '#define\s+UART_RECEVIER_TX_PIN\s+\(UART1_TX_P02_2\)' `
    'remote receiver TX placeholder must use P02.2.'
Assert-Match $remoteReceiverHeader '#define\s+UART_RECEVIER_RX_PIN\s+\(UART1_RX_P02_3\)' `
    'remote receiver RX must use P02.3.'

Assert-Match $main '(?m)^\s*offline_voice_init\s*\(' `
    'offline voice must be initialized on UART2.'
Assert-Match $main '(?m)^\s*offline_voice_poll\s*\(\s*\)\s*;' `
    'offline voice polling must run.'
Assert-Match $main 'portion2_uart1_update_for_mode\s*\(\s*main_mode\s*\)' `
    'remote receiver must be selected through the UART1 mode mux.'
Assert-Match $main '(?m)^\s*remote_control_task\s*\(\s*\)\s*;' `
    'remote receiver task must run in the main loop.'

Assert-Match $uart2RxIsr 'offline_voice_uart_rx_handler\s*\(\s*\)' `
    'UART2 RX interrupt must feed offline voice.'
Assert-NotMatch $uart2RxIsr 'uart_receiver_handler\s*\(\s*\)' `
    'UART2 RX interrupt must not feed the remote receiver.'
Assert-Match $uart1RxIsr 'portion2_uart1_rx_isr_handler\s*\(\s*\)' `
    'UART1 RX interrupt must dispatch the receiver or dot matrix owner.'
Assert-NotMatch $uart1RxIsr 'offline_voice_uart_rx_handler\s*\(\s*\)' `
    'UART1 RX interrupt must no longer feed offline voice.'

Write-Host 'voice UART2 with receiver UART1 mux checks passed.'
