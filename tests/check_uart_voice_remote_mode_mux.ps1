Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Match {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if($Text -notmatch $Pattern) { throw $Message }
}

$repo = Split-Path -Parent $PSScriptRoot
$receiver = Get-Content -Raw (Join-Path $repo 'libraries/zf_device/zf_device_uart_receiver.h')
$voice = Get-Content -Raw (Join-Path $repo 'code/offline_voice.h')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')
$isr = Get-Content -Raw (Join-Path $repo 'user/isr.c')
$remote = Get-Content -Raw (Join-Path $repo 'code/remote_control.c')

Assert-Match $voice 'OFFLINE_VOICE_UART_INDEX\s+\(UART_2\)' 'Voice must use UART2.'
Assert-Match $voice 'OFFLINE_VOICE_UART_TX_PIN\s+\(UART2_TX_P10_5\)' 'Voice TX must use P10.5.'
Assert-Match $voice 'OFFLINE_VOICE_UART_RX_PIN\s+\(UART2_RX_P10_6\)' 'Voice RX must use P10.6.'
Assert-Match $receiver 'UART_RECEVIER_UART_INDEX\s+\(UART_1\)' 'Receiver must use UART1.'
Assert-Match $receiver 'UART_RECEVIER_TX_PIN\s+\(UART1_TX_P02_2\)' 'Receiver TX must use P02.2.'
Assert-Match $receiver 'UART_RECEVIER_RX_PIN\s+\(UART1_RX_P02_3\)' 'Receiver RX must use P02.3.'
Assert-Match $main 'offline_voice_init\s*\(\s*Portion2_Voice_Command_Handle\s*,\s*0\s*\)' 'Voice must be initialized at startup.'
Assert-Match $main 'portion2_uart1_update_for_mode\s*\(\s*main_mode\s*\)' 'UART1 owner must track operating mode.'
Assert-Match $isr 'uart2_rx_isr[\s\S]*?offline_voice_uart_rx_handler\s*\(' 'UART2 RX ISR must serve voice.'
Assert-Match $isr 'uart1_rx_isr[\s\S]*?portion2_uart1_rx_isr_handler\s*\(' 'UART1 RX ISR must dispatch by owner.'
Assert-Match $remote 'portion2_uart1_select_record_receiver[\s\S]*?remote_control_init\s*\(' 'Record mode must initialize receiver.'
Assert-Match $remote 'portion2_uart1_select_run_dot_matrix[\s\S]*?dot_matrix_screen_init\s*\(' 'Run mode must initialize dot matrix.'

Write-Host 'Voice/receiver UART mode mux checks passed.'
