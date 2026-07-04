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

Write-Output 'Gyroscope route 13/14 checks passed.'
