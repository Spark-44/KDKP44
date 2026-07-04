$ErrorActionPreference = 'Stop'

$root = Join-Path $PSScriptRoot '..'
$headerPath = Join-Path $root 'code\subject_2_gyro_route.h'
$sourcePath = Join-Path $root 'code\subject_2_gyro_route.c'

if(-not (Test-Path $headerPath)) {
    throw 'Missing code\subject_2_gyro_route.h'
}
if(-not (Test-Path $sourcePath)) {
    throw 'Missing code\subject_2_gyro_route.c'
}

$header = Get-Content -Raw $headerPath
$source = Get-Content -Raw $sourcePath

foreach($pattern in @(
    '#define\s+SUBJECT_2_GYRO_ROUTE_13\s+13U',
    '#define\s+SUBJECT_2_GYRO_ROUTE_14\s+14U',
    'void\s+subject_2_gyro_route_start\s*\(\s*uint8\s+route_number\s*,\s*uint8\s+reverse\s*\)',
    'void\s+subject_2_gyro_route_stop\s*\(\s*const\s+char\s*\*\s*reason\s*\)',
    'void\s+subject_2_gyro_route_task\s*\(\s*void\s*\)',
    'uint8\s+subject_2_gyro_route_is_active\s*\(\s*void\s*\)'
)) {
    if($header -notmatch $pattern) {
        throw "Missing gyro-route API: $pattern"
    }
}

function Read-Profile([string]$macroName) {
    $matches = [regex]::Matches(
        $source,
        "(?m)^\s+$macroName\(\s*(-?\d+\.\d+)f\s*,\s*(-?\d+\.\d+)f\s*\)"
    )
    $profile = @()
    foreach($match in $matches) {
        $profile += [pscustomobject]@{
            Distance = [double]::Parse($match.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
            Yaw = [double]::Parse($match.Groups[2].Value, [Globalization.CultureInfo]::InvariantCulture)
        }
    }
    return $profile
}

$route13 = @(Read-Profile 'GYRO_ROUTE_13_SAMPLE')
$route14 = @(Read-Profile 'GYRO_ROUTE_14_SAMPLE')

if($route13.Count -ne 42) { throw "Route 13 sample count is $($route13.Count), expected 42." }
if($route14.Count -ne 44) { throw "Route 14 sample count is $($route14.Count), expected 44." }

$expectedRoute13Yaw = @(
    0.00, -7.67, -15.79, -18.34, -16.80, -13.78, -10.14, -5.68, 2.18, 12.35,
    20.13, 28.09, 34.53, 33.14, 26.88, 18.57, 7.50, -3.64, -14.26, -23.22,
    -28.39, -30.44, -29.80, -25.53, -17.70, -8.11, 2.34, 12.20, 20.46, 24.20,
    27.10, 23.09, 13.99, 2.93, -8.19, -18.44, -19.28, -9.73, 4.00, 9.08,
    0.87, -2.19
)
$expectedRoute14Yaw = @(
    0.00, 3.69, 9.91, 15.63, 22.61, 25.30, 22.85, 19.16, 15.32, 11.19,
    6.82, 1.65, -4.54, -10.75, -17.70, -25.10, -31.33, -35.09, -35.10, -30.12,
    -23.16, -15.92, -8.50, -1.33, 6.06, 13.13, 22.75, 29.26, 32.13, 30.06,
    27.34, 22.45, 14.35, 8.65, 2.90, -4.98, -11.40, -16.90, -22.77, -23.51,
    -17.71, -10.98, -4.45, 1.23
)

foreach($yawCheck in @(@($route13, $expectedRoute13Yaw, 'Route 13'), @($route14, $expectedRoute14Yaw, 'Route 14'))) {
    $profile = @($yawCheck[0])
    $expectedYaw = @($yawCheck[1])
    $name = [string]$yawCheck[2]
    for($i = 0; $i -lt $expectedYaw.Count; $i++) {
        if([math]::Abs($profile[$i].Yaw - $expectedYaw[$i]) -gt 0.001) {
            throw "$name yaw mismatch at sample ${i}: $($profile[$i].Yaw), expected $($expectedYaw[$i])."
        }
    }
}

foreach($entry in @(@($route13, 16.75, 'Route 13'), @($route14, 17.71, 'Route 14'))) {
    $profile = @($entry[0])
    $expectedEnd = [double]$entry[1]
    $name = [string]$entry[2]
    if([math]::Abs($profile[0].Distance) -gt 0.001 -or [math]::Abs($profile[0].Yaw) -gt 0.001) {
        throw "$name must start at zero distance and zero relative yaw."
    }
    for($i = 1; $i -lt $profile.Count; $i++) {
        if($profile[$i].Distance -le $profile[$i - 1].Distance) {
            throw "$name distances are not strictly increasing at sample $i."
        }
    }
    if([math]::Abs($profile[-1].Distance - $expectedEnd) -gt 0.02) {
        throw "$name endpoint $($profile[-1].Distance) does not match $expectedEnd m."
    }
}

if($source -match 'typedef\s+struct\s*\{[^}]*(?:\bfloat\s+x\b|\bfloat\s+y\b)[^}]*\}\s*subject_2_gyro_route_sample_t') {
    throw 'Runtime profile must not store x/y.'
}

foreach($pattern in @(
    '#define\s+SUBJECT_2_GYRO_ROUTE_MAX_ENCODER_DELTA\s+1000',
    '#define\s+SUBJECT_2_GYRO_ROUTE_STALL_MS\s+\(3000U\)',
    '#define\s+SUBJECT_2_GYRO_ROUTE_SLOWDOWN_M\s+\(1\.0f\)',
    '#define\s+SUBJECT_2_GYRO_ROUTE_STEER_LIMIT_DEG\s+\(25\.0f\)',
    'static\s+float\s+subject_2_gyro_route_interpolate_yaw\s*\(',
    'calculate_delta\s*\(',
    'ONE_TICK_DISTANCE',
    'subject_2_gyro_route_state\.distance_m\s*\+=\s*distance_step',
    'subject_2_gyro_route_state\.target_yaw\s*=\s*subject_2_gyro_route_normalize_angle',
    'if\s*\(\s*subject_2_gyro_route_state\.reverse\s*\)[\s\S]*?steer_command\s*=\s*-steer_command',
    'subject_2_gyro_route_finish\s*\(\s*"DISTANCE"\s*\)',
    'subject_2_gyro_route_finish\s*\(\s*"STALL"\s*\)',
    'rear_motor_set_target_mps\s*\(\s*0\.0f\s*\)',
    'uart_write_string\s*\(\s*DEBUG_UART_INDEX\s*,\s*line\s*\)'
)) {
    if($source -notmatch $pattern) {
        throw "Missing gyro-route control behavior: $pattern"
    }
}

if($source -notmatch 'if\s*\(\s*route_number\s*==\s*SUBJECT_2_GYRO_ROUTE_13\s*&&\s*!reverse\s*\)') {
    throw 'Route 13 must reject forward drive.'
}
if($source -notmatch 'speed_mps\s*=\s*subject_2_gyro_route_state\.reverse\s*\?\s*-speed_mps\s*:\s*speed_mps') {
    throw 'Runtime speed sign must follow the reverse flag.'
}

$main = Get-Content -Raw (Join-Path $root 'user\cpu0_main.c')
$guandao = Get-Content -Raw (Join-Path $root 'code\guandao.c')
$guandaoHeader = Get-Content -Raw (Join-Path $root 'code\guandao.h')
$fixed = Get-Content -Raw (Join-Path $root 'code\subject_2_fixed_action.c')

foreach($pattern in @(
    '#include\s+"subject_2_gyro_route\.h"',
    "data\s*==\s*'X'\s*\|\|\s*data\s*==\s*'x'[\s\S]*?subject_2_gyro_route_start\s*\(\s*SUBJECT_2_GYRO_ROUTE_13\s*,\s*1U\s*\)",
    "data\s*==\s*'y'[\s\S]*?subject_2_gyro_route_start\s*\(\s*SUBJECT_2_GYRO_ROUTE_14\s*,\s*0U\s*\)",
    "data\s*==\s*'Y'[\s\S]*?subject_2_gyro_route_start\s*\(\s*SUBJECT_2_GYRO_ROUTE_14\s*,\s*1U\s*\)",
    "data\s*==\s*'S'[\s\S]*?subject_2_gyro_route_stop\s*\(\s*`"COMMAND`"\s*\)",
    'subject_2_gyro_route_is_active\s*\(\s*\)[\s\S]*?subject_2_gyro_route_task\s*\(\s*\)',
    'void\s+portion2_run_select_route[\s\S]*?subject_2_gyro_route_stop\s*\(\s*"RECORDED_ROUTE"\s*\)',
    'void\s+voice_drive_action_start[\s\S]*?subject_2_gyro_route_stop\s*\(\s*"FIXED_ACTION"\s*\)'
)) {
    if(($main + "`n" + $guandao + "`n" + $fixed) -notmatch $pattern) {
        throw "Missing gyro-route integration: $pattern"
    }
}

if($main -notmatch "data\s*>=\s*'U'\s*&&\s*data\s*<=\s*'V'[\s\S]*?portion2_run_select_route\s*\(\s*data\s*-\s*'U'\s*\+\s*PORTION2_ROUTE_RETURN_5\s*\)") {
    throw 'Existing uppercase route 10/11 mapping changed.'
}
if($main -notmatch "data\s*>=\s*'u'\s*&&\s*data\s*<=\s*'w'[\s\S]*?portion2_run_select_route\s*\(\s*data\s*-\s*'u'\s*\+\s*PORTION2_ROUTE_RETURN_5\s*\)") {
    throw 'Existing lowercase route 10/11/12 mapping changed.'
}
if($main -notmatch "data\s*==\s*'W'[\s\S]*?portion2_run_select_route\s*\(\s*PORTION2_ROUTE_SNAKE\s*\)") {
    throw 'Existing uppercase route 12 mapping changed.'
}
if($guandaoHeader -notmatch '#define\s+PORTION2_ROUTE_COUNT\s+12') {
    throw 'Compiled gyro routes must not alter recorded-route Flash layout.'
}

Write-Output 'Gyroscope route 13/14 checks passed.'
