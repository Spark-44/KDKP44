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

function Get-CFunctionBody {
    param(
        [string]$Text,
        [string]$SignaturePattern
    )

    $clean = [regex]::Replace($Text, '//[^\r\n]*|/\*[\s\S]*?\*/', '')
    $searchFrom = 0

    while($searchFrom -lt $clean.Length) {
        $match = [regex]::Match($clean.Substring($searchFrom), $SignaturePattern)
        if(-not $match.Success) {
            throw "Missing C function definition: $SignaturePattern"
        }

        $start = $searchFrom + $match.Index
        $afterSignature = $start + $match.Length
        $brace = $clean.IndexOf('{', $afterSignature)
        $semicolon = $clean.IndexOf(';', $afterSignature)
        if($brace -ge 0 -and ($semicolon -lt 0 -or $brace -lt $semicolon)) {
            break
        }
        $searchFrom = $afterSignature
    }

    $depth = 0
    for($index = $brace; $index -lt $clean.Length; $index++) {
        if($clean[$index] -eq '{') {
            $depth++
        }
        elseif($clean[$index] -eq '}') {
            $depth--
            if($depth -eq 0) {
                return $clean.Substring($start, $index - $start + 1)
            }
        }
    }

    throw "Unterminated C function: $SignaturePattern"
}

$root = Split-Path -Parent $PSScriptRoot
$rearHeader = Get-Content (Join-Path $root 'code\rear_motor\rear_motor.h') -Raw
$rearSource = Get-Content (Join-Path $root 'code\rear_motor\rear_motor.c') -Raw
$guandao = Get-Content (Join-Path $root 'code\guandao.c') -Raw
$recordBeginBody = Get-CFunctionBody $guandao 'static\s+void\s+portion2_record_begin_route\s*\(\s*void\s*\)'
$runTaskBody = Get-CFunctionBody $guandao 'void\s+portion2_run_task\s*\(\s*void\s*\)'

Assert-Contains $rearHeader `
    'void\s+rear_motor_discard_odometry_samples\s*\(\s*void\s*\)\s*;' `
    'Rear motor must declare an API for discarding pending odometry samples.'

Assert-Contains $rearSource `
    'void\s+rear_motor_discard_odometry_samples\s*\(\s*void\s*\)\s*\{\s*uint32\s+interrupt_state\s*=\s*interrupt_global_disable\s*\(\s*\)\s*;\s*rear_odometry_pose_buffer_init\s*\(\s*&odometry_pose_buffer\s*\)\s*;\s*interrupt_global_enable\s*\(\s*interrupt_state\s*\)\s*;\s*\}' `
    'Discarding an odometry epoch must atomically initialize only the pose sample queue.'

Assert-Contains $recordBeginBody `
    'rear_motor_discard_odometry_samples\s*\(\s*\)\s*;[\s\S]*?portion2_record_point\s*\(\s*\)\s*;' `
    'Recording must discard pending odometry samples before storing its first point.'

Assert-Contains $runTaskBody `
    'case\s+1\s*:[\s\S]*?portion2_clear_route\s*\(\s*\)\s*;\s*rear_motor_discard_odometry_samples\s*\(\s*\)\s*;\s*Encoder_Get\s*\(\s*&guandao_ecd\s*\)\s*;' `
    'Run initialization must discard pending odometry samples after clearing the route and before reading the encoder.'

Write-Host 'Rear odometry epoch reset checks passed.'
