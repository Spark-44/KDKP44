# Run Mode Left-Turn Voice Aliases Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add voice command IDs `0x2D`, `0x2E`, and `0x2F` as aliases of the existing Run-mode left-turn drive command `0x25`.

**Architecture:** Extend the existing offline-voice command constants and keep command decoding unchanged. Route all four left-turn drive command IDs through the existing `Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_TURN_LEFT)` branch so action parameters and safety behavior remain centralized.

**Tech Stack:** Embedded C for AURIX TC264, PowerShell source-level regression tests.

---

## File structure

- Create `tests/check_offline_voice_left_turn_aliases.ps1`: verifies command values, maximum command ID, and shared Run-mode dispatch.
- Modify `code/offline_voice.h`: declares the three new command aliases and raises `OFFLINE_VOICE_CMD_MAX`.
- Modify `user/cpu0_main.c`: groups the three aliases with the existing left-turn drive case.

### Task 1: Add the failing regression test

**Files:**
- Create: `tests/check_offline_voice_left_turn_aliases.ps1`

- [ ] **Step 1: Write the source-level regression test**

```powershell
$ErrorActionPreference = 'Stop'

$root = Join-Path $PSScriptRoot '..'
$header = Get-Content -Raw (Join-Path $root 'code\offline_voice.h')
$main = Get-Content -Raw (Join-Path $root 'user\cpu0_main.c')

foreach($pattern in @(
    '#define\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_1\s+\(0x2D\)',
    '#define\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_2\s+\(0x2E\)',
    '#define\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_3\s+\(0x2F\)',
    '#define\s+OFFLINE_VOICE_CMD_MAX\s+\(0x2F\)'
)) {
    if($header -notmatch $pattern) {
        throw "Missing left-turn voice alias definition: $pattern"
    }
}

$sharedBranch = 'case\s+OFFLINE_VOICE_CMD_TURN_LEFT_DRIVE\s*:\s*case\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_1\s*:\s*case\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_2\s*:\s*case\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_3\s*:\s*Portion2_Fixed_Action_Start\s*\(\s*VOICE_DRIVE_ACTION_TURN_LEFT\s*\)\s*;'
if($main -notmatch $sharedBranch) {
    throw 'All four left-turn drive command IDs must share the existing left-turn action branch.'
}

Write-Output 'Run-mode left-turn voice aliases are mapped to the existing left-turn action.'
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests\check_offline_voice_left_turn_aliases.ps1
```

Expected: FAIL with `Missing left-turn voice alias definition` because `0x2D`–`0x2F` are not yet defined.

### Task 2: Implement the alias definitions and shared dispatch

**Files:**
- Modify: `code/offline_voice.h:54`
- Modify: `user/cpu0_main.c:995`
- Test: `tests/check_offline_voice_left_turn_aliases.ps1`

- [ ] **Step 1: Add command constants and update the maximum command ID**

Insert after `OFFLINE_VOICE_CMD_BACK_SNAKE`:

```c
#define OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_1    (0x2D)
#define OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_2    (0x2E)
#define OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_3    (0x2F)

#define OFFLINE_VOICE_CMD_MAX                  (0x2F)
```

Replace the existing `OFFLINE_VOICE_CMD_MAX (0x2C)` definition rather than retaining both definitions.

- [ ] **Step 2: Group the aliases with the existing Run-mode left-turn command**

Replace the current single case with:

```c
        case OFFLINE_VOICE_CMD_TURN_LEFT_DRIVE:
        case OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_1:
        case OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_2:
        case OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_3:
            Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_TURN_LEFT);
            break;
```

- [ ] **Step 3: Run the focused test and verify GREEN**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests\check_offline_voice_left_turn_aliases.ps1
```

Expected: PASS and output `Run-mode left-turn voice aliases are mapped to the existing left-turn action.`

### Task 3: Run regression verification

**Files:**
- Test: `tests/check_offline_voice_left_turn_aliases.ps1`
- Test: `tests/check_offline_voice_uart.ps1`
- Test: `tests/check_offline_voice_keepalive.ps1`

- [ ] **Step 1: Run the related voice tests**

```powershell
$tests = 'check_offline_voice_left_turn_aliases.ps1','check_offline_voice_uart.ps1','check_offline_voice_keepalive.ps1'
foreach($name in $tests) {
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path tests $name)
    if($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: all three commands exit with code 0.

- [ ] **Step 2: Run all PowerShell checks and report exact totals**

```powershell
$failed = @()
$passed = 0
Get-ChildItem tests -File -Filter '*.ps1' | Sort-Object Name | ForEach-Object {
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File $_.FullName *> $null
    if($LASTEXITCODE -eq 0) { $passed++ } else { $failed += $_.Name }
}
Write-Output "PowerShell tests: passed=$passed failed=$($failed.Count)"
$failed | ForEach-Object { Write-Output "FAIL $_" }
if($failed.Count -ne 0) { exit 1 }
```

Expected for the new feature: the new alias test passes. Existing unrelated failures must be listed rather than hidden or attributed to this change.

- [ ] **Step 3: Inspect the scoped diff**

```powershell
git diff -- code/offline_voice.h user/cpu0_main.c tests/check_offline_voice_left_turn_aliases.ps1
```

Expected: only three alias definitions, the maximum-command update, the grouped cases, and the regression test.

- [ ] **Step 4: Commit only if explicitly requested by the user**

Do not stage or commit automatically because the current worktree contains pre-existing user changes. If requested, stage only the three scoped files and create a focused commit.
