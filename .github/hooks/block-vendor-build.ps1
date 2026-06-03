$ErrorActionPreference = 'Stop'

function Get-ToolNameFromPayload {
    param([object]$Payload, [string]$Raw)

    $candidates = @(
        $Payload.toolName,
        $Payload.tool,
        $Payload.name,
        $Payload.tool_name,
        $Payload.toolNameRaw,
        $Payload.hookEventInput.toolName,
        $Payload.hookEventInput.tool,
        $Payload.hookEventInput.name
    ) | Where-Object { $_ -and $_.ToString().Trim().Length -gt 0 }

    if ($candidates.Count -gt 0) {
        return $candidates[0].ToString().ToLowerInvariant()
    }

    if ($Raw -match '(?i)functions\\.([a-z_]+)') {
        return $Matches[1].ToLowerInvariant()
    }

    return ''
}

function Write-Allow {
    $out = @{
        hookSpecificOutput = @{
            hookEventName = 'PreToolUse'
            permissionDecision = 'allow'
            permissionDecisionReason = 'No restricted path detected.'
        }
    }
    $out | ConvertTo-Json -Depth 10 -Compress
}

function Write-Deny {
    param([string]$Reason)

    $out = @{
        hookSpecificOutput = @{
            hookEventName = 'PreToolUse'
            permissionDecision = 'deny'
            permissionDecisionReason = $Reason
        }
        systemMessage = $Reason
    }
    $out | ConvertTo-Json -Depth 10 -Compress
}

$raw = [Console]::In.ReadToEnd()
if ([string]::IsNullOrWhiteSpace($raw)) {
    Write-Allow
    exit 0
}

try {
    $payload = $raw | ConvertFrom-Json -Depth 100
}
catch {
    Write-Allow
    exit 0
}

$toolName = Get-ToolNameFromPayload -Payload $payload -Raw $raw
$editableTools = @(
    'apply_patch',
    'create_file',
    'create_directory',
    'edit_notebook_file',
    'vscode_renamesymbol'
)

$isEditCapable = $false
foreach ($t in $editableTools) {
    if ($toolName -like "*$t") {
        $isEditCapable = $true
        break
    }
}

if (-not $isEditCapable) {
    Write-Allow
    exit 0
}

$restrictedPathPattern = '(?i)(?:^|[\\/])(vendors|build)(?:[\\/]|$)'
if ($raw -match $restrictedPathPattern) {
    Write-Deny -Reason 'Hook policy blocked this write action because it targets a restricted path under vendors/ or build/.'
    exit 0
}

Write-Allow
exit 0
