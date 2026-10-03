#requires -Version 5.1
[CmdletBinding()]
param([string]$GameDir, [switch]$NonInteractive, [switch]$LibraryOnly)

$ErrorActionPreference = 'Stop'
$FixVersion = 'v0.3.1-rc1'
$FixStateName = '.forza-amd-vram-fix-state.json'
$FixNames = @('amd_ags_x64.dll', 'd3d12.dll', 'dxgi.dll', 'amd_ags_x64_real.dll')
$FixPayloadNames = @('amd_ags_x64.dll', 'd3d12.dll', 'dxgi.dll')
$KnownProxyHashes = @(
    'f29ccb6f807c40b8900aaf040edf7552eb09b4ac0f17d72b7c6fbb2a1912f470',
    'd36dbcf52c901129cefa4cbbd0381e3036cd0f8352b5874072818ee0030ce625',
    '1647c8df40d4713cfea00559aeb5d9054cfeea8884342ddedb846d405baafdc9',
    '3e187dc746566281117091fcc775697efdab2cf54d42d6af78495ae627223ece',
    '0e40dc448b2c628d913cf7d725b88b5f251ced0a1b0337e6a99d37115f2031d9'
)
$KnownOriginalAgsHash = 'b27b070ca39dc37984fb3dde0187515d36094e72cb881d7a99bd1055befd8da2'

function Get-FixHash {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Assert-FixPlainItem {
    param([string]$Path, [switch]$Directory)
    $item = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Links/junctions are not supported: $Path" }
    if ($item.PSIsContainer -ne [bool]$Directory) { throw "Unexpected file type: $Path" }
}
function Get-FixGameDirectory {
    param([string]$Path, [switch]$NonInteractive)
    if ([string]::IsNullOrWhiteSpace($Path)) {
        $candidate = 'C:\XboxGames\Forza Horizon 6\Content'
        if (Test-Path -LiteralPath (Join-Path $candidate 'forzahorizon6.exe') -PathType Leaf) { $Path = $candidate }
        elseif ($NonInteractive) { throw 'Supply -GameDir with the folder containing forzahorizon6.exe.' }
        else { $Path = (Read-Host 'Folder containing forzahorizon6.exe').Trim().Trim('"') }
    }
    if ([string]::IsNullOrWhiteSpace($Path)) { throw 'No game directory was provided.' }
    $full = [IO.Path]::GetFullPath($Path).TrimEnd('\', '/')
    Assert-FixPlainItem -Path $full -Directory
    Assert-FixPlainItem -Path (Join-Path $full 'forzahorizon6.exe')
    return $full
}
function Assert-FixGameStopped {
    if (@(Get-Process -Name 'forzahorizon6' -ErrorAction SilentlyContinue).Count -gt 0) {
        throw 'Close Forza Horizon 6 normally before changing files. No process is stopped by this tool.'
    }
}
function Get-FixReleaseHashes {
    param([string]$SourceDir)
    Assert-FixPlainItem -Path $SourceDir -Directory
    $manifestPath = Join-Path $SourceDir 'SHA256SUMS.txt'
    Assert-FixPlainItem -Path $manifestPath
    $hashes = @{}
    foreach ($line in [IO.File]::ReadAllLines($manifestPath)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        if ($line -notmatch '^([a-fA-F0-9]{64})[ \t]+\*?([A-Za-z0-9_.-]+)$') { throw 'Malformed SHA256SUMS.txt. Re-extract the complete release.' }
        $hash = $Matches[1].ToLowerInvariant()
        $name = $Matches[2]
        if ($name -eq '.' -or $name -eq '..' -or $hashes.ContainsKey($name)) { throw "Invalid/duplicate manifest entry: $name" }
        $path = Join-Path $SourceDir $name
        Assert-FixPlainItem -Path $path
        if ((Get-FixHash $path) -ne $hash) { throw "Release checksum mismatch: $name. No game files changed." }
        $hashes[$name] = $hash
    }
    foreach ($name in $FixPayloadNames) { if (!$hashes.ContainsKey($name)) { throw "Manifest is missing $name." } }
    return $hashes
}
# Parse exports as bytes. Never load or execute the inspected DLL.
function Get-FixPeExports {
    param([string]$Path)
    Assert-FixPlainItem -Path $Path
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 256 -or $bytes.Length -gt 67108864) { throw 'Invalid PE size.' }
    function Read-U16([int]$offset) {
        if ($offset -lt 0 -or $offset + 2 -gt $bytes.Length) { throw 'PE offset out of bounds.' }
        return [BitConverter]::ToUInt16($bytes, $offset)
    }
    function Read-U32([int]$offset) {
        if ($offset -lt 0 -or $offset + 4 -gt $bytes.Length) { throw 'PE offset out of bounds.' }
        return [BitConverter]::ToUInt32($bytes, $offset)
    }
    if ((Read-U16 0) -ne 0x5a4d) { throw 'Not a PE library.' }
    $pe = [int](Read-U32 0x3c)
    if ((Read-U32 $pe) -ne 0x4550 -or (Read-U16 ($pe + 4)) -ne 0x8664) { throw 'Expected x64 PE.' }
    if (((Read-U16 ($pe + 22)) -band 0x2000) -eq 0) { throw 'Expected a DLL.' }
    $sections = Read-U16 ($pe + 6)
    $optionalSize = Read-U16 ($pe + 20)
    $optional = $pe + 24
    if ($sections -lt 1 -or $sections -gt 96 -or $optionalSize -lt 120 -or (Read-U16 $optional) -ne 0x20b) { throw 'Invalid PE headers.' }
    $exportRva = Read-U32 ($optional + 112)
    $exportSize = Read-U32 ($optional + 116)
    if (!$exportRva -or $exportSize -lt 40) { throw 'DLL has no valid export directory.' }
    $sectionTable = $optional + $optionalSize
    function Convert-Rva([uint32]$rva) {
        for ($i = 0; $i -lt $sections; $i++) {
            $section = $sectionTable + 40 * $i
            $size = Read-U32 ($section + 16)
            $start = Read-U32 ($section + 12)
            if ([uint64]$rva -ge $start -and [uint64]$rva -lt ([uint64]$start + $size)) {
                $offset = [uint64](Read-U32 ($section + 20)) + ([uint64]$rva - $start)
                if ($offset -ge $bytes.Length) { throw 'PE RVA out of bounds.' }
                return [int]$offset
            }
        }
        throw 'PE RVA has no backing section.'
    }
    $directory = Convert-Rva $exportRva
    $functionCount = Read-U32 ($directory + 20)
    $nameCount = Read-U32 ($directory + 24)
    if ($nameCount -lt 1 -or $nameCount -gt 8192 -or $functionCount -gt 8192) { throw 'Invalid PE export count.' }
    $functions = Convert-Rva (Read-U32 ($directory + 28))
    $names = Convert-Rva (Read-U32 ($directory + 32))
    $ordinals = Convert-Rva (Read-U32 ($directory + 36))
    $exports = @{}
    for ($i = 0; $i -lt $nameCount; $i++) {
        $offset = Convert-Rva (Read-U32 ($names + 4 * $i))
        $end = $offset
        while ($end -lt $bytes.Length -and $end - $offset -lt 512 -and $bytes[$end] -ne 0) { $end++ }
        if ($end -ge $bytes.Length -or $bytes[$end] -ne 0) { throw 'Unterminated PE export name.' }
        $name = [Text.Encoding]::ASCII.GetString($bytes, $offset, $end - $offset)
        $ordinal = Read-U16 ($ordinals + 2 * $i)
        if ($ordinal -ge $functionCount -or $exports.ContainsKey($name)) { throw 'Invalid PE export ordinal/name.' }
        $functionRva = Read-U32 ($functions + 4 * $ordinal)
        if (!$functionRva) { throw 'Empty PE export.' }
        $exports[$name] = ([uint64]$functionRva -ge $exportRva -and [uint64]$functionRva -lt ([uint64]$exportRva + $exportSize))
    }
    return $exports
}
function Assert-FixOriginalAgs {
    param([string]$Path)
    $hash = Get-FixHash $Path
    if ($KnownProxyHashes -contains $hash) { throw 'Known AGS proxy detected. Restore the original game library first.' }
    $exports = Get-FixPeExports $Path
    foreach ($name in @('agsInitialize', 'agsDeInitialize', 'agsGetVersionNumber')) {
        if (!$exports.ContainsKey($name) -or $exports[$name]) { throw "AGS original has a missing/forwarded export: $name" }
    }
    if (@($exports.Values | Where-Object { $_ }).Count -gt 0) { throw 'Forwarding/proxy AGS is not accepted as the original.' }
    $version = [Diagnostics.FileVersionInfo]::GetVersionInfo($Path)
    $abi = '{0}.{1}.{2}.{3}' -f $version.FileMajorPart, $version.FileMinorPart, $version.FileBuildPart, $version.FilePrivatePart
    if ($abi -notin @('6.3.0.0', '6.3.1.0')) { throw "Unsupported AGS ABI/version: $abi. Supported: 6.3.0.0 and 6.3.1.0." }
    $signature = Get-AuthenticodeSignature -LiteralPath $Path
    $signedByAmd = $signature.Status -eq 'Valid' -and $null -ne $signature.SignerCertificate -and
        $signature.SignerCertificate.Subject -match '(?i)(^|,\s*)(CN|O)=Advanced Micro Devices(,|$)'
    if ($hash -ne $KnownOriginalAgsHash -and !$signedByAmd) {
        throw 'Unknown AGS library. Only the preserved original hash or a valid AMD-signed supported AGS is accepted.'
    }
}
function Assert-FixPreviousDirectX {
    param([string]$Path)
    if ($KnownProxyHashes -contains (Get-FixHash $Path)) { throw "Old proxy detected: $Path. Restore previous originals first." }
    $null = Get-FixPeExports $Path
    $signature = Get-AuthenticodeSignature -LiteralPath $Path
    if ($signature.Status -ne 'Valid' -or $null -eq $signature.SignerCertificate -or
        $signature.SignerCertificate.Subject -notmatch '(?i)(^|,\s*)(CN|O)=Microsoft (Corporation|Windows)(,|$)') {
        throw "Unknown DirectX library/proxy: $Path. Other proxy mods are not supported; restore original files first."
    }
}
function Assert-FixCurrent {
    param([string]$Path, [bool]$Exists, [AllowNull()][string]$Hash)
    $present = Test-Path -LiteralPath $Path
    if ($present -ne $Exists) { throw "File presence changed: $Path. No unverified overwrite/removal is allowed." }
    if ($present) {
        Assert-FixPlainItem -Path $Path
        if ((Get-FixHash $Path) -ne $Hash) { throw "File changed: $Path. Review it before continuing." }
    }
}
function Write-FixJson {
    param([string]$Path, $Value, [AllowNull()][string]$ExpectedHash, [string]$WorkingDir)
    Assert-FixCurrent -Path $Path -Exists ([bool]$ExpectedHash) -Hash $ExpectedHash
    $temporary = $Path + '.tmp-' + [Guid]::NewGuid().ToString('N')
    [IO.File]::WriteAllText($temporary, ($Value | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
    if ($ExpectedHash) {
        if (!$WorkingDir) { $WorkingDir = [IO.Path]::GetDirectoryName($Path) }
        $previous = Join-Path $WorkingDir ('journal-before-replace-' + [Guid]::NewGuid().ToString('N') + '.json')
        [IO.File]::Replace($temporary, $Path, $previous)
    }
    else { [IO.File]::Move($temporary, $Path) }
    return Get-FixHash $Path
}
function Set-FixFileAtomic {
    param([string]$Source, [string]$Target, [string]$Hash, [bool]$ExpectedExists,
        [AllowNull()][string]$ExpectedHash, [string]$WorkingDir)
    Assert-FixGameStopped
    Assert-FixPlainItem -Path $Source
    $stage = Join-Path $WorkingDir ('stage-' + [Guid]::NewGuid().ToString('N') + '.dll')
    [IO.File]::Copy($Source, $stage, $false)
    Assert-FixCurrent -Path $stage -Exists $true -Hash $Hash
    Assert-FixGameStopped
    Assert-FixCurrent -Path $Target -Exists $ExpectedExists -Hash $ExpectedHash
    if ($ExpectedExists) {
        $previous = Join-Path $WorkingDir ('file-before-replace-' + [Guid]::NewGuid().ToString('N') + '.dll')
        [IO.File]::Replace($stage, $Target, $previous)
    }
    else { [IO.File]::Move($stage, $Target) }
    Assert-FixCurrent -Path $Target -Exists $true -Hash $Hash
}
function Remove-FixFileChecked {
    param([string]$Path, [string]$Hash)
    Assert-FixGameStopped
    Assert-FixCurrent -Path $Path -Exists $true -Hash $Hash
    [IO.File]::Delete($Path)
    if (Test-Path -LiteralPath $Path) { throw "Could not remove managed file: $Path" }
}
function Get-FixCurrentSnapshot {
    param([string]$Directory, [string]$Name)
    $path = Join-Path $Directory $Name
    $present = Test-Path -LiteralPath $path
    $hash = $null
    if ($present) { Assert-FixPlainItem -Path $path; $hash = Get-FixHash $path }
    return [pscustomobject]@{ Name = $Name; Exists = $present; Hash = $hash }
}
function Assert-FixRestorePlan {
    param([string]$GameDir, [string]$BackupDir, $Entries, [bool]$Incomplete)
    Assert-FixPlainItem -Path $BackupDir -Directory
    Assert-FixPlainItem -Path (Join-Path $BackupDir 'original') -Directory
    foreach ($entry in $Entries) {
        if ($entry.BeforeExists) { Assert-FixCurrent -Path (Join-Path (Join-Path $BackupDir 'original') $entry.Name) -Exists $true -Hash $entry.BeforeHash }
        $current = Get-FixCurrentSnapshot -Directory $GameDir -Name $entry.Name
        $installed = $current.Exists -and $current.Hash -eq $entry.InstalledHash
        $unchanged = $current.Exists -eq $entry.BeforeExists -and $current.Hash -eq $entry.BeforeHash
        if (!$installed -and !($Incomplete -and $unchanged)) { throw "Managed file drift: $($entry.Name). No files have been restored." }
    }
}
function Restore-FixBefore {
    param([string]$GameDir, [string]$BackupDir, $Entries)
    Assert-FixRestorePlan -GameDir $GameDir -BackupDir $BackupDir -Entries $Entries -Incomplete $true
    foreach ($entry in $Entries) {
        $current = Get-FixCurrentSnapshot -Directory $GameDir -Name $entry.Name
        if ($current.Exists -eq $entry.BeforeExists -and $current.Hash -eq $entry.BeforeHash) { continue }
        $target = Join-Path $GameDir $entry.Name
        if ($entry.BeforeExists) {
            Set-FixFileAtomic -Source (Join-Path (Join-Path $BackupDir 'original') $entry.Name) -Target $target -Hash $entry.BeforeHash -ExpectedExists $current.Exists -ExpectedHash $current.Hash -WorkingDir $BackupDir
        } else { Remove-FixFileChecked -Path $target -Hash $entry.InstalledHash }
    }
    foreach ($entry in $Entries) { Assert-FixCurrent -Path (Join-Path $GameDir $entry.Name) -Exists $entry.BeforeExists -Hash $entry.BeforeHash }
}
function Invoke-ForzaInstall {
    param([string]$GameDir, [string]$SourceDir, [switch]$NonInteractive)
    $GameDir = Get-FixGameDirectory -Path $GameDir -NonInteractive:$NonInteractive
    $SourceDir = [IO.Path]::GetFullPath($SourceDir).TrimEnd('\', '/')
    if ($GameDir -eq $SourceDir) { throw 'Extract this release outside the game directory before installing.' }
    Assert-FixGameStopped
    $statePath = Join-Path $GameDir $FixStateName
    if (Test-Path -LiteralPath $statePath) { throw 'Managed installation/recovery state exists. Use its matching uninstall script first; do not reinstall over it.' }
    if (Test-Path -LiteralPath (Join-Path $GameDir '.forza-8gb-fix-state.json')) { throw 'Older managed installation exists. Use its matching uninstaller first.' }
    $releaseHashes = Get-FixReleaseHashes $SourceDir
    $ags = Join-Path $GameDir 'amd_ags_x64.dll'
    Assert-FixPlainItem -Path $ags
    Assert-FixOriginalAgs $ags
    $originalAgsHash = Get-FixHash $ags
    $real = Join-Path $GameDir 'amd_ags_x64_real.dll'
    if (Test-Path -LiteralPath $real) {
        Assert-FixPlainItem -Path $real
        Assert-FixOriginalAgs $real
        if ((Get-FixHash $real) -ne $originalAgsHash) { throw 'Existing original AGS copy differs from the current original. Review matching game libraries first.' }
    }
    foreach ($name in @('d3d12.dll', 'dxgi.dll')) {
        $path = Join-Path $GameDir $name
        if (Test-Path -LiteralPath $path) { Assert-FixPlainItem -Path $path; Assert-FixPreviousDirectX $path }
    }
    $entries = @(foreach ($name in $FixNames) {
        $current = Get-FixCurrentSnapshot -Directory $GameDir -Name $name
        $installedHash = $originalAgsHash
        if ($releaseHashes.ContainsKey($name)) { $installedHash = $releaseHashes[$name] }
        [pscustomobject]@{ Name = $name; BeforeExists = $current.Exists; BeforeHash = $current.Hash; InstalledHash = $installedHash }
    })
    $backupName = 'backup-before-forza-amd-vram-fix-' + $FixVersion + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N')
    $backupDir = Join-Path $GameDir $backupName
    $stateCreated = $false
    $stateHash = $null
    try {
        $null = New-Item -ItemType Directory -Path $backupDir -ErrorAction Stop
        $originalDir = Join-Path $backupDir 'original'
        $null = New-Item -ItemType Directory -Path $originalDir -ErrorAction Stop
        foreach ($entry in $entries) {
            Assert-FixCurrent -Path (Join-Path $GameDir $entry.Name) -Exists $entry.BeforeExists -Hash $entry.BeforeHash
            if ($entry.BeforeExists) {
                [IO.File]::Copy((Join-Path $GameDir $entry.Name), (Join-Path $originalDir $entry.Name), $false)
                Assert-FixCurrent -Path (Join-Path $originalDir $entry.Name) -Exists $true -Hash $entry.BeforeHash
            }
        }
        $manifest = [pscustomobject]@{ SchemaVersion = 1; Release = $FixVersion; GameDir = $GameDir; Entries = $entries }
        $manifestHash = Write-FixJson -Path (Join-Path $backupDir 'manifest.json') -Value $manifest -ExpectedHash $null
        $state = [pscustomobject]@{ SchemaVersion = 1; Release = $FixVersion; BackupName = $backupName; ManifestHash = $manifestHash; Status = 'Installing' }
        $stateHash = Write-FixJson -Path $statePath -Value $state -ExpectedHash $null
        $stateCreated = $true
        Assert-FixGameStopped
        # Stage every payload first; use atomic per-file replacement plus a recovery journal.
        $payloadDir = Join-Path $backupDir 'payload'
        $null = New-Item -ItemType Directory -Path $payloadDir -ErrorAction Stop
        foreach ($entry in $entries) {
            $source = Join-Path $SourceDir $entry.Name
            if ($entry.Name -eq 'amd_ags_x64_real.dll') { $source = Join-Path $originalDir 'amd_ags_x64.dll' }
            [IO.File]::Copy($source, (Join-Path $payloadDir $entry.Name), $false)
            Assert-FixCurrent -Path (Join-Path $payloadDir $entry.Name) -Exists $true -Hash $entry.InstalledHash
        }
        foreach ($entry in $entries) {
            Set-FixFileAtomic -Source (Join-Path $payloadDir $entry.Name) -Target (Join-Path $GameDir $entry.Name) -Hash $entry.InstalledHash -ExpectedExists $entry.BeforeExists -ExpectedHash $entry.BeforeHash -WorkingDir $backupDir
        }
        $state.Status = 'Installed'
        $stateHash = Write-FixJson -Path $statePath -Value $state -ExpectedHash $stateHash -WorkingDir $backupDir
        Write-Host "Installed Forza AMD VRAM reporting fix $FixVersion."
        Write-Host "Original files/recovery manifest retained in: $backupDir"
        Write-Host 'This release candidate has not passed a fresh game-runtime test.'
    } catch {
        $cause = $_.Exception.Message
        if ($stateCreated) {
            try {
                Assert-FixGameStopped
                Restore-FixBefore -GameDir $GameDir -BackupDir $backupDir -Entries $entries
                Remove-FixFileChecked -Path $statePath -Hash $stateHash
            } catch {
                throw "Install failed: $cause. Automatic restoration could not finish: $($_.Exception.Message). Preserve state/backup at $backupDir and use uninstall for recovery after reviewing files."
            }
            throw "Install failed; all previous DLL bytes/presence restored. Backup retained at $backupDir. Cause: $cause"
        }
        throw "Install stopped before changing DLLs. Check directory write permissions; no automatic elevation is performed. Cause: $cause. Any created backup is retained at $backupDir."
    }
}
function Get-FixManagedState {
    param([string]$GameDir)
    $path = Join-Path $GameDir $FixStateName
    Assert-FixPlainItem -Path $path
    $stateHash = Get-FixHash $path
    $state = [IO.File]::ReadAllText($path) | ConvertFrom-Json
    if ($state.SchemaVersion -ne 1 -or $state.Release -ne $FixVersion -or $state.Status -notin @('Installing', 'Installed') -or
        $state.BackupName -notmatch '^backup-before-forza-amd-vram-fix-v0\.3\.1-rc1-[0-9]{8}-[0-9]{6}-[a-f0-9]{32}$' -or
        $state.ManifestHash -notmatch '^[a-f0-9]{64}$') { throw 'Invalid/unsupported managed state. Preserve it for review.' }
    $backupDir = Join-Path $GameDir $state.BackupName
    Assert-FixPlainItem -Path $backupDir -Directory
    $manifestPath = Join-Path $backupDir 'manifest.json'
    Assert-FixCurrent -Path $manifestPath -Exists $true -Hash $state.ManifestHash
    $manifest = [IO.File]::ReadAllText($manifestPath) | ConvertFrom-Json
    if ($manifest.SchemaVersion -ne 1 -or $manifest.Release -ne $FixVersion -or $manifest.GameDir -ne $GameDir -or @($manifest.Entries).Count -ne $FixNames.Count) {
        throw 'Backup manifest does not describe this installation.'
    }
    $seen = @{}
    foreach ($entry in $manifest.Entries) {
        if ($entry.Name -notin $FixNames -or $seen.ContainsKey($entry.Name) -or $entry.BeforeExists -isnot [bool] -or
            $entry.InstalledHash -notmatch '^[a-f0-9]{64}$' -or
            ($entry.BeforeExists -and $entry.BeforeHash -notmatch '^[a-f0-9]{64}$') -or
            (!$entry.BeforeExists -and $null -ne $entry.BeforeHash)) { throw 'Invalid backup file entry.' }
        $seen[$entry.Name] = $true
    }
    return [pscustomobject]@{ Path = $path; Hash = $stateHash; State = $state; BackupDir = $backupDir; Entries = @($manifest.Entries) }
}
if (!$LibraryOnly) {
    try { Invoke-ForzaInstall -GameDir $GameDir -SourceDir $PSScriptRoot -NonInteractive:$NonInteractive }
    catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }
}
