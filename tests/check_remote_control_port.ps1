$ErrorActionPreference = 'Stop'

$root = Join-Path $PSScriptRoot '..'
$receiverHeaderPath = Join-Path $root 'libraries\zf_device\zf_device_uart_receiver.h'
$receiverSourcePath = Join-Path $root 'libraries\zf_device\zf_device_uart_receiver.c'
$deviceTypeHeaderPath = Join-Path $root 'libraries\zf_device\zf_device_type.h'
$deviceTypeSourcePath = Join-Path $root 'libraries\zf_device\zf_device_type.c'
$headfilePath = Join-Path $root 'libraries\zf_common\zf_common_headfile.h'
$remoteHeaderPath = Join-Path $root 'code\remote_control.h'
$remoteSourcePath = Join-Path $root 'code\remote_control.c'
$isrPath = Join-Path $root 'user\isr.c'
$mainPath = Join-Path $root 'user\cpu0_main.c'
$guandaoHeaderPath = Join-Path $root 'code\guandao.h'
$guandaoSourcePath = Join-Path $root 'code\guandao.c'

foreach($path in @(
    $receiverHeaderPath,
    $receiverSourcePath,
    $remoteHeaderPath,
    $remoteSourcePath,
    $deviceTypeHeaderPath,
    $deviceTypeSourcePath,
    $headfilePath,
    $isrPath,
    $mainPath,
    $guandaoHeaderPath,
    $guandaoSourcePath
)) {
    if(-not (Test-Path $path)) {
        throw "Missing required remote-control port file: $path"
    }
}

$receiverHeader = Get-Content -Raw $receiverHeaderPath
$receiverSource = Get-Content -Raw $receiverSourcePath
$remoteHeader = Get-Content -Raw $remoteHeaderPath
$remoteSource = Get-Content -Raw $remoteSourcePath
$deviceTypeHeader = Get-Content -Raw $deviceTypeHeaderPath
$deviceTypeSource = Get-Content -Raw $deviceTypeSourcePath
$headfile = Get-Content -Raw $headfilePath
$isr = Get-Content -Raw $isrPath
$main = Get-Content -Raw $mainPath
$guandaoHeader = Get-Content -Raw $guandaoHeaderPath
$guandaoSource = Get-Content -Raw $guandaoSourcePath
$uart1RxIsr = [regex]::Match($isr, 'IFX_INTERRUPT\s*\(\s*uart1_rx_isr[\s\S]*?\n\}').Value
$uart2RxIsr = [regex]::Match($isr, 'IFX_INTERRUPT\s*\(\s*uart2_rx_isr[\s\S]*?\n\}').Value

foreach($pattern in @(
    '#define\s+UART_RECEVIER_UART_INDEX\s+\(UART_1\)',
    '#define\s+UART_RECEVIER_TX_PIN\s+\(UART1_TX_P02_2\)',
    '#define\s+UART_RECEVIER_RX_PIN\s+\(UART1_RX_P02_3\)',
    '#define\s+SBUS_UART_BAUDRATE\s+\(100000\)',
    '#define\s+UART_RECEVIER_CHANNEL_NUM\s+\(\s*6\s*\)',
    'extern\s+uart_receiver_struct\s+uart_receiver\s*;',
    'void\s+uart_receiver_init\s*\(\s*void\s*\)\s*;'
)) {
    if($receiverHeader -notmatch $pattern) {
        throw "Receiver header missing expected SBUS/UART1 setting: $pattern"
    }
}

foreach($pattern in @(
    'uart_sbus_init\s*\(\s*UART_RECEVIER_UART_INDEX\s*,\s*SBUS_UART_BAUDRATE\s*,\s*UART_RECEVIER_TX_PIN\s*,\s*UART_RECEVIER_RX_PIN\s*\)',
    'set_wireless_type\s*\(\s*RECEIVER_UART\s*,\s*uart_receiver_callback\s*\)',
    'FRAME_STAR\s*==\s*uart_receiver_data\[0\]',
    'FRAME_END\s*==\s*uart_receiver_data\[24\]',
    'SBUS_ABNORMAL_STATE'
)) {
    if($receiverSource -notmatch $pattern) {
        throw "Receiver source missing expected parser/init behavior: $pattern"
    }
}

foreach($pattern in @(
    'extern\s+callback_function\s+uart_receiver_handler\s*;',
    'RECEIVER_UART'
)) {
    if($deviceTypeHeader -notmatch $pattern) {
        throw "Device type header missing receiver callback support: $pattern"
    }
}
foreach($pattern in @(
    'callback_function\s+uart_receiver_handler\s*=',
    'if\s*\(\s*RECEIVER_UART\s*==\s*wireless_type\s*\)',
    'uart_receiver_handler\s*=\s*\(\(wireless_callback\s*==\s*NULL\)'
)) {
    if($deviceTypeSource -notmatch $pattern) {
        throw "Device type source missing receiver callback routing: $pattern"
    }
}

foreach($pattern in @(
    '#include\s+"zf_device_uart_receiver\.h"',
    '#include\s+"remote_control\.h"'
)) {
    if($headfile -notmatch $pattern) {
        throw "Common headfile missing include: $pattern"
    }
}

foreach($pattern in @(
    'void\s+remote_control_init\s*\(\s*void\s*\)\s*;',
    'uint8\s+remote_control_task\s*\(\s*void\s*\)\s*;',
    'void\s+remote_control_stop\s*\(\s*void\s*\)\s*;',
    'uint8\s+remote_control_is_active\s*\(\s*void\s*\)\s*;'
)) {
    if($remoteHeader -notmatch $pattern) {
        throw "Remote control header missing API: $pattern"
    }
}
foreach($pattern in @(
    '#define\s+REMOTE_CONTROL_FRAME_TIMEOUT_MS\s+\(300U\)',
    '#define\s+REMOTE_CONTROL_CHANNEL_DEAD_ZONE\s+\(50\)',
    '#define\s+REMOTE_CONTROL_SPEED_CENTER\s+\(992U\)',
    '#define\s+REMOTE_CONTROL_SPEED_FULL_SCALE\s+\(776\.0f\)',
    '#define\s+REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s+\(2\.0f\)',
    '#define\s+REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS\s+\(0\.02f\)',
    '#define\s+REMOTE_CONTROL_MODE_LOW_THRESHOLD\s+\(600U\)',
    '#define\s+REMOTE_CONTROL_MODE_HIGH_THRESHOLD\s+\(1400U\)',
    '#define\s+REMOTE_CONTROL_BUTTON_THRESHOLD\s+\(1400U\)',
    'uart_receiver_init\s*\(\s*\)',
    'uart_receiver\.finsh_flag',
    'uart_receiver\.state',
    'remote_control_apply_failsafe\s*\(\s*"DISCONNECT"\s*\)',
    'remote_control_apply_failsafe\s*\(\s*"TIMEOUT"\s*\)',
    'void\s+remote_control_stop\s*\(\s*void\s*\)',
    'void\s+remote_control_stop\s*\(\s*void\s*\)\s*\{[\s\S]*?if\s*\(\s*remote_control_active\s*\)',
    'remote_control_apply_failsafe\s*\(\s*"DISABLED"\s*\)',
    'remote_control_apply_failsafe\s*\(\s*"MODE"\s*\)',
    'static\s+void\s+remote_control_update_mode_from_ch4\s*\(\s*void\s*\)',
    'uart_receiver\.channel\[3\]\s*<\s*REMOTE_CONTROL_MODE_LOW_THRESHOLD[\s\S]*?next_mode\s*=\s*Guandao_Drive',
    'uart_receiver\.channel\[3\]\s*<\s*REMOTE_CONTROL_MODE_HIGH_THRESHOLD[\s\S]*?next_mode\s*=\s*Guandao_Portion2_Recode',
    'next_mode\s*=\s*Guandao_Voice',
    'main_mode\s*==\s*Guandao_Portion2_Recode',
    'static\s+float\s+remote_control_map_speed_from_ch2\s*\(\s*uint16\s+channel_value\s*\)',
    'static\s+float\s+remote_control_limit_speed_command\s*\(\s*float\s+target_mps\s*\)',
    '#define\s+REMOTE_CONTROL_SPEED_RAMP_STEP_MPS\s+\(0\.03f\)',
    'static\s+float\s+remote_control_ramp_speed_command\s*\(\s*float\s+target_mps\s*\)',
    'remote_command_speed_mps\s*\+=\s*REMOTE_CONTROL_SPEED_RAMP_STEP_MPS',
    'remote_command_speed_mps\s*-=\s*REMOTE_CONTROL_SPEED_RAMP_STEP_MPS',
    'rear_motor_get_speed_mps\s*\(\s*\)',
    'actual_mps\s*>=\s*\(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\+\s*REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS\)',
    'actual_mps\s*<=\s*-\(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\+\s*REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS\)',
    'return\s+REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*;',
    'return\s+-REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*;',
    '\(int32\)channel_value\s*-\s*\(int32\)REMOTE_CONTROL_SPEED_CENTER',
    'REMOTE_CONTROL_SPEED_FULL_SCALE\s*\*\s*REMOTE_CONTROL_MAX_TARGET_SPEED_MPS',
    'remote_target_speed_mps\s*=\s*remote_control_map_speed_from_ch2\s*\(\s*uart_receiver\.channel\[1\]\s*\)',
    'static\s+void\s+remote_control_update_record_buttons\s*\(\s*void\s*\)',
    'uart_receiver\.channel\[2\]\s*>=\s*REMOTE_CONTROL_BUTTON_THRESHOLD[\s\S]*?portion2_record_remote_start_stop_request\s*\(\s*\)',
    'uart_receiver\.channel\[4\]\s*>=\s*REMOTE_CONTROL_BUTTON_THRESHOLD[\s\S]*?portion2_record_remote_clear_request\s*\(\s*\)',
    'uart_receiver\.channel\[5\]\s*>=\s*REMOTE_CONTROL_BUTTON_THRESHOLD[\s\S]*?portion2_record_remote_save_request\s*\(\s*\)',
    'route_setting_choice\s*=\s*3[\s\S]*?conrtol_mode\s*=\s*GUANDAO',
    'route_setting_choice\s*=\s*1[\s\S]*?conrtol_mode\s*=\s*IDLE',
    'route_setting_choice\s*=\s*1[\s\S]*?conrtol_mode\s*=\s*GUANDAO',
    'if\s*\(\s*main_mode\s*!=\s*Guandao_Portion2_Recode\s*\)[\s\S]*?remote_control_apply_failsafe\s*\(\s*"MODE"\s*\)',
    'rear_motor_stop\s*\(\s*\)',
    'remote_command_speed_mps\s*=\s*remote_control_ramp_speed_command\s*\(\s*remote_control_limit_speed_command\s*\(\s*remote_target_speed_mps\s*\)\s*\)',
    'rear_motor_set_target_mps\s*\(\s*remote_command_speed_mps\s*\)',
    'rear_motor_encoder_update_10ms\s*\(\s*\)[\s\S]*?rear_motor_set_target_mps\s*\(\s*remote_command_speed_mps\s*\)[\s\S]*?rear_motor_pid_update_100ms\s*\(\s*\)',
    'Steer_Moter_Contral\s*\(\s*remote_target_angle_deg\s*\)'
)) {
    if($remoteSource -notmatch $pattern) {
        throw "Remote control source missing behavior: $pattern"
    }
}
if($remoteSource -match 'remote_last_100ms[\s\S]*?rear_motor_pid_update_100ms\s*\(\s*\)') {
    throw 'Remote motor PID must be fed from the 10ms block; a 100ms caller makes the rear PID update about once per second.'
}
if($remoteSource -match 'actual_mps\s*>=\s*\(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\+\s*REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS\)[\s\S]*?return\s+0\.0f') {
    throw 'Positive overspeed must clamp to +2m/s, not zero.'
}
if($remoteSource -match 'actual_mps\s*<=\s*-\(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\+\s*REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS\)[\s\S]*?return\s+0\.0f') {
    throw 'Negative overspeed must clamp to -2m/s, not zero.'
}

if($isr -notmatch 'IFX_INTERRUPT\s*\(\s*uart1_rx_isr[\s\S]*?portion2_uart1_rx_isr_handler\s*\(\s*\)') {
    throw 'UART1 RX ISR must dispatch the current UART1 owner.'
}
if($isr -match 'IFX_INTERRUPT\s*\(\s*uart3_rx_isr[\s\S]*?uart_receiver_handler\s*\(\s*\)') {
    throw 'UART3 RX ISR must stay free for GNSS and must not dispatch SBUS receiver handler.'
}
if($isr -notmatch 'IFX_INTERRUPT\s*\(\s*uart3_rx_isr[\s\S]*?gnss_uart_callback\s*\(\s*\)') {
    throw 'UART3 RX ISR must keep GNSS callback.'
}
if($uart1RxIsr -match 'offline_voice_uart_rx_handler\s*\(\s*\)') {
    throw 'UART1 RX ISR must not dispatch offline voice.'
}
if($uart2RxIsr -notmatch 'offline_voice_uart_rx_handler\s*\(\s*\)') {
    throw 'UART2 RX ISR must dispatch offline voice.'
}

foreach($pattern in @(
    'portion2_uart1_update_for_mode\s*\(\s*main_mode\s*\)',
    'while\s*\(\s*TRUE\s*\)[\s\S]*?remote_control_task\s*\(\s*\)\s*;',
    'case\s+Guandao_Portion2_Recode\s*:[\s\S]*?portion2_record_task\s*\(\s*\)[\s\S]*?if\s*\(\s*remote_control_is_active\s*\(\s*\)\s*\)[\s\S]*?continue\s*;',
    'case\s+Guandao_Drive\s*:[\s\S]*?Portion2_Drive_Mode_Task\s*\(\s*\)'
)) {
    if($main -notmatch $pattern) {
        throw "Main loop missing remote control integration: $pattern"
    }
}

if($main -notmatch '(?m)^\s*offline_voice_init\s*\(' -or $main -notmatch '(?m)^\s*offline_voice_poll\s*\(\s*\)\s*;') {
    throw 'Offline voice must initialize and poll on UART2.'
}

if($main -match 'case\s+Guandao_Voice\s*:[\s\S]*?remote_control_stop\s*\(\s*\)') {
    throw 'Run mode must not stop SBUS mode switching; CH3 must remain active.'
}
if($main -match 'case\s+Guandao_Drive\s*:[\s\S]*?remote_control_stop\s*\(\s*\)') {
    throw 'Drive mode must not stop SBUS mode switching; CH3 must remain active.'
}
if($main -match '#include\s+"rear_motor/rear_motor\.h"li') {
    throw 'Stray li after rear_motor include must be removed.'
}
if($remoteSource -match 'update_mode_from_ch3|channel\[2\]\s*<\s*REMOTE_CONTROL_MODE_') {
    throw 'Mode switching must use CH4, not CH3.'
}

foreach($pattern in @(
    'void\s+portion2_record_remote_start_stop_request\s*\(\s*void\s*\)\s*;',
    'void\s+portion2_record_remote_clear_request\s*\(\s*void\s*\)\s*;',
    'void\s+portion2_record_remote_save_request\s*\(\s*void\s*\)\s*;'
)) {
    if($guandaoHeader -notmatch $pattern) {
        throw "Guandao header missing remote record API: $pattern"
    }
}

foreach($pattern in @(
    'static\s+uint8\s+portion2_record_remote_start_stop_req',
    'static\s+uint8\s+portion2_record_remote_clear_req',
    'static\s+uint8\s+portion2_record_remote_save_req',
    'void\s+portion2_record_remote_start_stop_request\s*\(\s*void\s*\)[\s\S]*?portion2_record_remote_start_stop_req\s*=\s*1',
    'void\s+portion2_record_remote_clear_request\s*\(\s*void\s*\)[\s\S]*?portion2_record_remote_clear_req\s*=\s*1',
    'void\s+portion2_record_remote_save_request\s*\(\s*void\s*\)[\s\S]*?portion2_record_remote_save_req\s*=\s*1',
    'portion2_record_remote_clear_req[\s\S]*?k1_long\s*=\s*1',
    'portion2_record_remote_save_req[\s\S]*?k2_long\s*=\s*1',
    'portion2_record_remote_start_stop_req[\s\S]*?k3_short\s*=\s*1'
)) {
    if($guandaoSource -notmatch $pattern) {
        throw "Guandao source missing remote record behavior: $pattern"
    }
}

Write-Output 'Remote control UART2/SBUS port checks passed.'
