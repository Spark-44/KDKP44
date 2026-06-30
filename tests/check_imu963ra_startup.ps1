$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$driver = Get-Content -Raw (Join-Path $repo 'libraries/zf_device/zf_device_imu963ra.c')
$driverHeader = Get-Content -Raw (Join-Path $repo 'libraries/zf_device/zf_device_imu963ra.h')
$imu = Get-Content -Raw (Join-Path $repo 'code/IMU.c')
$isr = Get-Content -Raw (Join-Path $repo 'user/isr.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $driver 'gpio_init\(IMU963RA_CS_PIN,\s*GPO,\s*GPIO_HIGH,\s*GPO_PUSH_PULL\)' 'IMU963RA SPI CS must idle high'
Assert-Contains $driver 'system_delay_ms\(50\)' 'IMU963RA startup must allow enough power-on settling time'
Assert-Contains $driverHeader '#define\s+IMU963RA_SPI_SPEED\s+\(1\s*\*\s*1000\s*\*\s*1000\)' 'IMU963RA SPI speed must be reduced for reliable startup WHO_AM_I reads'
Assert-Contains $driver 'gpio_init\(IMU963RA_CS_PIN,\s*GPO,\s*GPIO_HIGH,\s*GPO_PUSH_PULL\);\s*imu963ra_read_acc_gyro_register\(IMU963RA_WHO_AM_I\);' 'IMU963RA SPI init must perform a WHO_AM_I read after CS idles high'
Assert-Contains $driver 'imu963ra_write_acc_gyro_register\(IMU963RA_CTRL3_C,\s*0x01\);\s*//[^\r\n]*\s*system_delay_ms\(20\);\s*imu963ra_write_acc_gyro_register\(IMU963RA_FUNC_CFG_ACCESS,\s*0x00\);' 'IMU963RA reset must allow enough time before WHO_AM_I self check'
Assert-Contains $driver '#define\s+IMU963RA_INIT_RETRY_COUNT\s+\(3U\)' 'IMU963RA driver must retry transient startup failures before logging'
Assert-Contains $driver 'for\(uint8 retry_index = 0; retry_index < IMU963RA_INIT_RETRY_COUNT; retry_index\+\+\)' 'IMU963RA driver self check must retry before reporting failure'
if ($driver -match 'zf_log\(0,\s*"IMU963RA acc and gyro self check error\."\)') {
    throw 'IMU963RA acc/gyro self check must not overwrite the boot screen with zf_log'
}
if ($driver -match 'zf_log\(0,\s*"IMU963RA mag self check error\."\)') {
    throw 'IMU963RA magnetometer self check must be optional for gyro-only navigation'
}
Assert-Contains $driver 'imu963ra_mag_available\s*=\s*0' 'IMU963RA driver must track optional magnetometer availability'
Assert-Contains $driver 'if\(!imu963ra_mag_available\)[\s\S]*?return;' 'IMU963RA mag reads must be skipped when optional mag is unavailable'
Assert-Contains $imu 'if\(0\s*==\s*imu963ra_init\(\)\)' 'IMU init must check the driver return value'
Assert-Contains $imu 'IMU_1_Open_flag\s*=\s*1' 'IMU init must mark the sensor ready only after successful init'
Assert-Contains $imu 'if\(IMU_1_Open_flag\s*!=\s*1\)[\s\S]*?return;' 'IMU reads must be skipped when init failed'
Assert-Contains $isr 'IMU_data_get\(\);' 'PIT ISR must use the guarded IMU read helper'

Write-Host 'IMU963RA startup checks passed'
