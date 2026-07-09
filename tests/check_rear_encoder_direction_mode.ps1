$ErrorActionPreference = 'Stop'

function Assert-Contains {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$peripheral = Get-Content (Join-Path $root 'code\peripheral.c') -Raw
$rearHeader = Get-Content (Join-Path $root 'code\rear_motor\rear_motor.h') -Raw

Assert-Contains $peripheral 'void\s+Encoder_Init\s*\(\s*void\s*\)[\s\S]*?encoder_dir_init\s*\(\s*ENCODER_QUADDEC\s*,\s*ENCODER_QUADDEC_A\s*,\s*ENCODER_QUADDEC_B\s*\)' 'Rear encoder must use direction mode: P33.7=pulse, P33.6=direction.'
if($peripheral -match 'void\s+Encoder_Init\s*\(\s*void\s*\)[\s\S]*?encoder_quad_init\s*\(\s*ENCODER_QUADDEC') {
    throw 'Rear encoder init must not use quadrature mode for pulse+direction encoder.'
}
Assert-Contains $rearHeader '#define\s+REAR_ENCODER_PPR\s+1024' 'Rear speed conversion should keep 1024 pulses/rev for the direction encoder.'

Write-Host 'Rear direction encoder checks passed.'
