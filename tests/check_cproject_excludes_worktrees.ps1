$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$cproject = Get-Content -Raw -LiteralPath (Join-Path $root '.cproject')
$project = Get-Content -Raw -LiteralPath (Join-Path $root '.project')

if($project -notmatch '<id>org\.eclipse\.core\.resources\.regexFilterMatcher</id>\s*<arguments>\\?\.worktrees</arguments>') {
    throw '.project must hide .worktrees from Eclipse resource discovery.'
}

if($cproject -match '<listOptionValue[^>]+value="[^"]*\/\.worktrees(?:\/|&quot;)') {
    throw '.cproject include paths must not reference sources inside .worktrees.'
}

Write-Host '.cproject worktree exclusion checks passed.'
