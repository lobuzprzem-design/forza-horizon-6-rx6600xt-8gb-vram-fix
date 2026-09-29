$ErrorActionPreference = "Stop"

$DefaultGameDir = "C:\XboxGames\Forza Horizon 6\Content"
$SourceDir = $PSScriptRoot
$ProxyDllNames = @("d3d12.dll", "dxgi.dll", "amd_ags_x64.dll")
$RealAgsName = "amd_ags_x64_real.dll"

$CandidateDirs = @(
    $PSScriptRoot,
    $DefaultGameDir,
    (Join-Path $env:ProgramFiles "WindowsApps")
)

function Find-GameDir {
    foreach ($dir in $CandidateDirs) {
        if (!$dir -or !(Test-Path -LiteralPath $dir)) {
            continue
        }

        $directExe = Join-Path $dir "forzahorizon6.exe"
        if (Test-Path -LiteralPath $directExe) {
            return $dir
        }
    }

    return $null
}

function Hash-FileOrNull {
    param([string]$Path)

    if (!(Test-Path -LiteralPath $Path)) {
        return $null
    }

    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "Administrator rights are needed to copy files into the game folder."
    Start-Process -FilePath "powershell.exe" -Verb RunAs -ArgumentList @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-NoExit",
        "-File",
        $PSCommandPath
    )
    exit
}

$GameDir = Find-GameDir
if (!$GameDir) {
    Write-Host "Could not auto-detect the game folder."
    Write-Host "Enter the folder that contains forzahorizon6.exe."
    Write-Host "Example: $DefaultGameDir"
    $typed = Read-Host "Game folder"
    if ([string]::IsNullOrWhiteSpace($typed)) {
        throw "No game folder was provided."
    }
    $GameDir = $typed.Trim('"')
}

$ExePath = Join-Path $GameDir "forzahorizon6.exe"
if (!(Test-Path -LiteralPath $ExePath)) {
    throw "forzahorizon6.exe was not found in: $GameDir"
}

foreach ($name in $ProxyDllNames) {
    $source = Join-Path $SourceDir $name
    if (!(Test-Path -LiteralPath $source)) {
        throw "Missing release file: $source"
    }
}

$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$BackupDir = Join-Path $GameDir ("backup-before-8gb-vram-fix-" + $Stamp)
New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null

Write-Host "Forza Horizon 6 RX 6600 XT 8GB VRAM Fix v0.3.0-rc1"
Write-Host "Game folder: $GameDir"
Write-Host "Backup folder: $BackupDir"
Write-Host ""

foreach ($name in @($ProxyDllNames + $RealAgsName)) {
    $target = Join-Path $GameDir $name
    if (Test-Path -LiteralPath $target) {
        Copy-Item -LiteralPath $target -Destination (Join-Path $BackupDir $name) -Force
        Write-Host "Backed up: $name"
    }
}

$sourceAgsProxy = Join-Path $SourceDir "amd_ags_x64.dll"
$targetAgs = Join-Path $GameDir "amd_ags_x64.dll"
$targetAgsReal = Join-Path $GameDir $RealAgsName
$sourceAgsHash = Hash-FileOrNull $sourceAgsProxy
$targetAgsHash = Hash-FileOrNull $targetAgs

if ((Test-Path -LiteralPath $targetAgs) -and !(Test-Path -LiteralPath $targetAgsReal)) {
    if ($targetAgsHash -and $sourceAgsHash -and ($targetAgsHash -ne $sourceAgsHash)) {
        Copy-Item -LiteralPath $targetAgs -Destination $targetAgsReal -Force
        Write-Host "Saved original amd_ags_x64.dll as amd_ags_x64_real.dll"
    } else {
        Write-Host "amd_ags_x64.dll already looks like this proxy. No real AGS copy was created."
    }
}

foreach ($name in $ProxyDllNames) {
    $source = Join-Path $SourceDir $name
    $target = Join-Path $GameDir $name
    Copy-Item -LiteralPath $source -Destination $target -Force
    Write-Host "Installed: $name"
}

Write-Host ""
Write-Host "Installed file hashes:"
Get-FileHash -LiteralPath @(
    (Join-Path $GameDir "amd_ags_x64.dll"),
    (Join-Path $GameDir "d3d12.dll"),
    (Join-Path $GameDir "dxgi.dll")
) -Algorithm SHA256 |
    Select-Object Hash, Path |
    Format-Table -AutoSize

if (Test-Path -LiteralPath $targetAgsReal) {
    Write-Host ""
    Write-Host "Original AGS library kept at:"
    Write-Host $targetAgsReal
}

Write-Host ""
Write-Host "Done. Start Forza Horizon 6 normally or through Vortex."
Write-Host "Rollback: delete dxgi.dll and d3d12.dll, then restore amd_ags_x64.dll from the backup or from amd_ags_x64_real.dll."
Write-Host "Backup folder:"
Write-Host $BackupDir
Write-Host ""
Read-Host "Press Enter to close this window"
