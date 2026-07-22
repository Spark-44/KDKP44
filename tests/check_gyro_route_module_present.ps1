$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$header = Join-Path $root 'code/subject_2_gyro_route.h'
$source = Join-Path $root 'code/subject_2_gyro_route.c'

if(-not (Test-Path -LiteralPath $header)) {
    throw 'cpu0_main.c references subject_2_gyro_route.h, but the header is missing.'
}

if(-not (Test-Path -LiteralPath $source)) {
    throw 'The subject_2_gyro_route implementation is missing.'
}

$headerText = Get-Content -Raw -LiteralPath $header
$sourceText = Get-Content -Raw -LiteralPath $source

foreach($symbol in @(
    'subject_2_gyro_route_start',
    'subject_2_gyro_route_stop',
    'subject_2_gyro_route_task',
    'subject_2_gyro_route_is_active'
)) {
    if($headerText -notmatch [regex]::Escape($symbol)) {
        throw "Missing public gyro-route declaration: $symbol"
    }
    if($sourceText -notmatch [regex]::Escape($symbol)) {
        throw "Missing gyro-route implementation: $symbol"
    }
}

Write-Host 'gyro route module presence checks passed.'
