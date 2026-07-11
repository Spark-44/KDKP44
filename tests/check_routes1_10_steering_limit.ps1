$ErrorActionPreference = 'Stop'

function Assert-Match {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $root 'code\guandao.c'
$source = Get-Content -Raw -Path $sourcePath

Assert-Match $source '#define\s+PORTION2_STEERING_CMD_LIMIT\s+16\.0f' `
    'routes 1-10 normal steering limit must be 16 degrees'

Assert-Match $source '#define\s+PORTION2_SHARP_STEERING_CMD_LIMIT\s+28\.0f' `
    'routes 1-10 sharp steering limit must be 28 degrees'

Assert-Match $source '#define\s+PORTION2_ROUTE11_STEERING_CMD_LIMIT\s+18\.0f' `
    'route 11 reverse steering limit must remain 18 degrees'

Assert-Match $source '#define\s+PORTION2_ROUTE11_SHARP_STEERING_CMD_LIMIT\s+25\.0f' `
    'route 11 reverse sharp steering limit must remain 25 degrees'

Assert-Match $source '#define\s+PORTION2_SNAKE_STEERING_CMD_LIMIT\s+20\.0f' `
    'route 12 normal steering limit must remain 20 degrees'

Assert-Match $source '#define\s+PORTION2_SNAKE_SHARP_STEERING_CMD_LIMIT\s+30\.0f' `
    'route 12 sharp steering limit must remain 30 degrees'

Write-Host 'OK: routes 1-10 steering limit is enlarged while route 11/12 special limits are preserved.'
