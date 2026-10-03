#requires -Version 5.1
# Dummy EXE/DLL data is NEVER executed. Transaction tests mock vendor validation;
# static PE parser, original/proxy refusals and CLI entrypoint are tested separately.
param([string]$OriginalAgsPath)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $repoRoot 'release\uninstall_8gb_fix.ps1') -LibraryOnly
$script:ActualAtomic = (Get-Item Function:\Set-FixFileAtomic).ScriptBlock
$script:ActualRemove = (Get-Item Function:\Remove-FixFileChecked).ScriptBlock
$script:ActualManifest = (Get-Item Function:\Get-FixReleaseHashes).ScriptBlock
$script:ActualStopped = (Get-Item Function:\Assert-FixGameStopped).ScriptBlock
$script:ActualAgs = (Get-Item Function:\Assert-FixOriginalAgs).ScriptBlock
$script:ActualDirectX = (Get-Item Function:\Assert-FixPreviousDirectX).ScriptBlock
$script:TestCount = 0
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('forza installer tests ' + [Guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $testRoot
function Assert-True($Value, [string]$Message) { if (!$Value) { throw "ASSERTION FAILED: $Message" } }
function Assert-Throws([scriptblock]$Action, [string]$Pattern) {
    $message = $null
    try { & $Action } catch { $message = $_.Exception.Message }
    if ($null -eq $message -or $message -notmatch $Pattern) { throw "Expected failure /$Pattern/, got: $message" }
}
function Write-Fixture([string]$Path, [string]$Text) { [IO.File]::WriteAllText($Path, $Text, (New-Object Text.UTF8Encoding($false))) }
function New-Fixture([string]$Name, [switch]$ExistingDirectX, [switch]$ExistingReal) {
    $base = Join-Path $testRoot $Name
    $game = Join-Path $base 'dummy game with spaces'
    $source = Join-Path $base 'release with spaces'
    $null = New-Item -ItemType Directory -Path $game
    $null = New-Item -ItemType Directory -Path $source
    Write-Fixture (Join-Path $game 'forzahorizon6.exe') 'DUMMY DATA ONLY; NEVER EXECUTE'
    Write-Fixture (Join-Path $game 'amd_ags_x64.dll') 'fixture-original-ags-version-6.3.0'
    if ($ExistingDirectX) {
        Write-Fixture (Join-Path $game 'd3d12.dll') 'fixture-original-directx-d3d12'
        Write-Fixture (Join-Path $game 'dxgi.dll') 'fixture-original-directx-dxgi'
    }
    if ($ExistingReal) { Write-Fixture (Join-Path $game 'amd_ags_x64_real.dll') 'fixture-original-ags-version-6.3.0' }
    foreach ($name in $FixPayloadNames) { Write-Fixture (Join-Path $source $name) ("fixture-release-" + $name) }
    $lines = @(foreach ($name in $FixPayloadNames) { (Get-FixHash (Join-Path $source $name)) + '  ' + $name })
    [IO.File]::WriteAllLines((Join-Path $source 'SHA256SUMS.txt'), $lines)
    $before = @(foreach ($name in $FixNames) { Get-FixCurrentSnapshot -Directory $game -Name $name })
    return [pscustomobject]@{ Game = $game; Source = $source; Before = $before }
}
function Get-Snapshot([string]$Game) { return @(foreach ($name in $FixNames) { Get-FixCurrentSnapshot $Game $name }) }
function Assert-Snapshot([string]$Game, $Snapshot) {
    foreach ($item in $Snapshot) { Assert-FixCurrent -Path (Join-Path $Game $item.Name) -Exists $item.Exists -Hash $item.Hash }
}
$FixtureValidators = {
    function Assert-FixOriginalAgs([string]$Path) {
        Assert-FixPlainItem $Path
        if (![IO.File]::ReadAllText($Path).StartsWith('fixture-original-ags-')) { throw 'Fixture rejects unknown AGS.' }
    }
    function Assert-FixPreviousDirectX([string]$Path) {
        Assert-FixPlainItem $Path
        if (![IO.File]::ReadAllText($Path).StartsWith('fixture-original-directx-')) { throw 'Fixture rejects unknown DirectX.' }
    }
    function Assert-FixGameStopped {}
}
$AtomicFailure = {
    function Set-FixFileAtomic {
        param([string]$Source, [string]$Target, [string]$Hash, [bool]$ExpectedExists,
            [AllowNull()][string]$ExpectedHash, [string]$WorkingDir)
        $script:AtomicCalls++
        if ($script:AtomicCalls -in $script:FailAtomicCalls) { throw 'Injected atomic I/O failure.' }
        & $script:ActualAtomic @PSBoundParameters
    }
}
function Invoke-Test([string]$Name, [scriptblock]$Action) {
    & $Action
    $script:TestCount++
    Write-Host "PASS $Name"
}
function New-PeFixture([string]$Path, [switch]$Forwarded) {
    $bytes = New-Object byte[] 4096
    function Put16([int]$Offset, [uint16]$Value) { [Array]::Copy([BitConverter]::GetBytes($Value), 0, $bytes, $Offset, 2) }
    function Put32([int]$Offset, [uint32]$Value) { [Array]::Copy([BitConverter]::GetBytes($Value), 0, $bytes, $Offset, 4) }
    Put16 0 0x5a4d; Put32 0x3c 128; Put32 128 0x4550; Put16 132 0x8664
    Put16 134 1; Put16 148 240; Put16 150 0x2000; Put16 152 0x20b
    Put32 264 0x1000; Put32 268 512
    Put32 404 0x1000; Put32 408 3584; Put32 412 512
    Put32 532 3; Put32 536 3; Put32 540 0x1040; Put32 544 0x1050; Put32 548 0x1060
    $stringOffset = 640
    $names = @('agsInitialize','agsDeInitialize','agsGetVersionNumber')
    for ($i = 0; $i -lt 3; $i++) {
        $functionRva = 0x1400 + 16 * $i
        if ($Forwarded) { $functionRva = 0x1170 }
        Put32 (576 + 4 * $i) $functionRva
        Put32 (592 + 4 * $i) ($stringOffset - 512 + 0x1000)
        Put16 (608 + 2 * $i) $i
        $nameBytes = [Text.Encoding]::ASCII.GetBytes($names[$i])
        [Array]::Copy($nameBytes, 0, $bytes, $stringOffset, $nameBytes.Length)
        $stringOffset += $nameBytes.Length + 1
    }
    [IO.File]::WriteAllBytes($Path, $bytes)
}
try {
    Invoke-Test 'path spaces; restore ALL previous DLL bytes; retain backup' {
        . $FixtureValidators
        $f = New-Fixture 'full restore' -ExistingDirectX -ExistingReal
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        foreach ($name in $FixPayloadNames) { Assert-True ((Get-FixHash (Join-Path $f.Game $name)) -eq (Get-FixHash (Join-Path $f.Source $name))) "Installed $name" }
        $managed = Get-FixManagedState $f.Game
        Assert-True ($managed.State.Status -eq 'Installed') 'Committed state'
        Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive
        Assert-Snapshot $f.Game $f.Before
        Assert-True (Test-Path -LiteralPath $managed.BackupDir) 'Backup retained'
        Assert-True (!(Test-Path -LiteralPath (Join-Path $f.Game $FixStateName))) 'State removed'
    }
    Invoke-Test 'restore absence; never remove unrelated file' {
        . $FixtureValidators
        $f = New-Fixture 'absence restore'
        Write-Fixture (Join-Path $f.Game 'unrelated.dll') 'unrelated-original-bytes'
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive
        Assert-Snapshot $f.Game $f.Before
        Assert-True ([IO.File]::ReadAllText((Join-Path $f.Game 'unrelated.dll')) -eq 'unrelated-original-bytes') 'Unrelated bytes preserved'
    }
    Invoke-Test 'managed reinstall refuses without new writes' {
        . $FixtureValidators
        $f = New-Fixture 'reinstall'
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        $installed = Get-Snapshot $f.Game
        $stateHash = Get-FixHash (Join-Path $f.Game $FixStateName)
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'Managed installation'
        Assert-Snapshot $f.Game $installed
        Assert-True ((Get-FixHash (Join-Path $f.Game $FixStateName)) -eq $stateHash) 'State unchanged'
    }
    Invoke-Test 'corrupt payload refuses before backup or target writes' {
        . $FixtureValidators
        $f = New-Fixture 'bad hash'
        Write-Fixture (Join-Path $f.Source 'dxgi.dll') 'tampered'
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'checksum mismatch'
        Assert-Snapshot $f.Game $f.Before
        Assert-True (@(Get-ChildItem -LiteralPath $f.Game -Directory).Count -eq 0) 'No backup on bad payload'
    }
    Invoke-Test 'manifest traversal rejected' {
        . $FixtureValidators
        $f = New-Fixture 'manifest traversal'
        Write-Fixture (Join-Path $f.Source 'SHA256SUMS.txt') (('a' * 64) + '  ..\unrelated.dll')
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'Malformed'
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'missing original AGS refuses before backup' {
        . $FixtureValidators
        $f = New-Fixture 'missing AGS'
        [IO.File]::Delete((Join-Path $f.Game 'amd_ags_x64.dll'))
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'cannot find|does not exist'
        Assert-True (@(Get-ChildItem -LiteralPath $f.Game -Directory).Count -eq 0) 'No write without original'
    }
    Invoke-Test 'real validator rejects unknown non-PE AGS' {
        . $FixtureValidators
        function Assert-FixOriginalAgs([string]$Path) { & $script:ActualAgs -Path $Path }
        $f = New-Fixture 'unknown AGS'
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'Invalid PE size|Not a PE'
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'known old AGS proxy hash is rejected explicitly' {
        function Get-FixHash([string]$Path) { return 'f29ccb6f807c40b8900aaf040edf7552eb09b4ac0f17d72b7c6fbb2a1912f470' }
        $path = Join-Path $testRoot 'known proxy dummy.dll'
        Write-Fixture $path 'dummy'
        Assert-Throws { & $script:ActualAgs -Path $path } 'Known AGS proxy'
    }
    Invoke-Test 'mismatched real AGS copy refuses' {
        . $FixtureValidators
        $f = New-Fixture 'mismatched real' -ExistingReal
        Write-Fixture (Join-Path $f.Game 'amd_ags_x64_real.dll') 'fixture-original-ags-other'
        $before = Get-Snapshot $f.Game
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'differs'
        Assert-Snapshot $f.Game $before
    }
    Invoke-Test 'real DirectX validator rejects unknown library' {
        . $FixtureValidators
        function Assert-FixPreviousDirectX([string]$Path) { & $script:ActualDirectX -Path $Path }
        $f = New-Fixture 'unknown DirectX' -ExistingDirectX
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'Invalid PE size|Not a PE'
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'running-game guard refuses; no kill' {
        . $FixtureValidators
        function Get-Process { param([string]$Name, $ErrorAction) return [pscustomobject]@{ ProcessName = $Name } }
        function Assert-FixGameStopped { & $script:ActualStopped }
        $f = New-Fixture 'running game'
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'Close Forza'
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'same source/game directory refused' {
        . $FixtureValidators
        $f = New-Fixture 'same folder'
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Game -NonInteractive } 'outside the game'
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'partial install failure restores exact previous bytes and absence' {
        . $FixtureValidators; . $AtomicFailure
        $script:AtomicCalls = 0; $script:FailAtomicCalls = @(2)
        $f = New-Fixture 'partial install' -ExistingDirectX -ExistingReal
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'all previous DLL bytes/presence restored'
        Assert-Snapshot $f.Game $f.Before
        Assert-True (!(Test-Path -LiteralPath (Join-Path $f.Game $FixStateName))) 'State removed after recovery'
        Assert-True (@(Get-ChildItem -LiteralPath $f.Game -Directory).Count -eq 1) 'Failed-install backup retained'
    }
    Invoke-Test 'payload changed after preflight fails before target commit' {
        . $FixtureValidators
        function Get-FixReleaseHashes([string]$SourceDir) {
            $hashes = & $script:ActualManifest $SourceDir
            Write-Fixture (Join-Path $SourceDir 'dxgi.dll') 'later tamper'
            return $hashes
        }
        $f = New-Fixture 'payload drift'
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'all previous DLL bytes/presence restored'
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'uninstall refuses user drift before any restoration' {
        . $FixtureValidators
        $f = New-Fixture 'uninstall drift'
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        Write-Fixture (Join-Path $f.Game 'dxgi.dll') 'new user DLL'
        $snapshot = Get-Snapshot $f.Game
        Assert-Throws { Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive } 'Managed file drift'
        Assert-Snapshot $f.Game $snapshot
    }
    Invoke-Test 'corrupted backup refuses without removing installed files' {
        . $FixtureValidators
        $f = New-Fixture 'backup drift'
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        $managed = Get-FixManagedState $f.Game
        Write-Fixture (Join-Path (Join-Path $managed.BackupDir 'original') 'amd_ags_x64.dll') 'bad backup'
        $snapshot = Get-Snapshot $f.Game
        Assert-Throws { Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive } 'File changed'
        Assert-Snapshot $f.Game $snapshot
    }
    Invoke-Test 'state backup traversal rejected' {
        . $FixtureValidators
        $f = New-Fixture 'state traversal'
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        $managed = Get-FixManagedState $f.Game
        $managed.State.BackupName = '..\outside'
        $null = Write-FixJson -Path $managed.Path -Value $managed.State -ExpectedHash $managed.Hash
        $snapshot = Get-Snapshot $f.Game
        Assert-Throws { Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive } 'Invalid/unsupported'
        Assert-Snapshot $f.Game $snapshot
    }
    Invoke-Test 'partial uninstall failure restores installed bytes and state' {
        . $FixtureValidators
        $f = New-Fixture 'partial uninstall' -ExistingDirectX -ExistingReal
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        $snapshot = Get-Snapshot $f.Game
        $stateHash = Get-FixHash (Join-Path $f.Game $FixStateName)
        . $AtomicFailure
        $script:AtomicCalls = 0; $script:FailAtomicCalls = @(2)
        Assert-Throws { Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive } 'pre-uninstall DLL bytes/presence restored'
        Assert-Snapshot $f.Game $snapshot
        Assert-True ((Get-FixHash (Join-Path $f.Game $FixStateName)) -eq $stateHash) 'State unchanged'
    }
    Invoke-Test 'failed managed deletion rolls back uninstall' {
        . $FixtureValidators
        $f = New-Fixture 'delete failure'
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        $snapshot = Get-Snapshot $f.Game
        $script:RemoveCalls = 0
        function Remove-FixFileChecked {
            param([string]$Path, [string]$Hash)
            $script:RemoveCalls++
            if ($script:RemoveCalls -eq 1) { throw 'Injected delete failure.' }
            & $script:ActualRemove @PSBoundParameters
        }
        Assert-Throws { Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive } 'pre-uninstall DLL bytes/presence restored'
        Assert-Snapshot $f.Game $snapshot
    }
    Invoke-Test 'failed state deletion restores all installed DLLs' {
        . $FixtureValidators
        $f = New-Fixture 'state delete failure'
        Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
        $snapshot = Get-Snapshot $f.Game
        function Remove-FixFileChecked {
            param([string]$Path, [string]$Hash)
            if ([IO.Path]::GetFileName($Path) -eq $FixStateName) { throw 'Injected state delete failure.' }
            & $script:ActualRemove @PSBoundParameters
        }
        Assert-Throws { Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive } 'pre-uninstall DLL bytes/presence restored'
        Assert-Snapshot $f.Game $snapshot
    }
    Invoke-Test 'Installing journal recovers interrupted mixed state' {
        . $FixtureValidators; . $AtomicFailure
        $f = New-Fixture 'interrupted recovery'
        $script:AtomicCalls = 0; $script:FailAtomicCalls = @(2,3)
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'restoration could not finish'
        Assert-True ((Get-FixManagedState $f.Game).State.Status -eq 'Installing') 'Recovery journal retained'
        $script:FailAtomicCalls = @()
        Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'write denied stops without automatic elevation' {
        . $FixtureValidators
        $f = New-Fixture 'write denied'
        function New-Item {
            [CmdletBinding()]
            param([string]$ItemType, [string]$Path)
            if ($Path -match 'backup-before-forza-amd-vram-fix') { throw 'Injected access denied' }
            Microsoft.PowerShell.Management\New-Item @PSBoundParameters
        }
        Assert-Throws { Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive } 'no automatic elevation'
        Assert-Snapshot $f.Game $f.Before
    }
    Invoke-Test 'static x64 PE export parser rejects forwarded AGS and unsupported ABI' {
        $path = Join-Path $testRoot 'static PE only.dll'
        New-PeFixture $path
        $exports = Get-FixPeExports $path
        Assert-True ($exports.Count -eq 3 -and !$exports['agsInitialize']) 'Real export parsed'
        New-PeFixture $path -Forwarded
        $exports = Get-FixPeExports $path
        Assert-True $exports['agsInitialize'] 'Forwarding detected'
        Assert-Throws { & $script:ActualAgs -Path $path } 'missing/forwarded'
        New-PeFixture $path
        Assert-Throws { & $script:ActualAgs -Path $path } 'Unsupported AGS ABI'
    }
    Invoke-Test 'CLI uninstaller honors explicit noninteractive GameDir with spaces' {
        $f = New-Fixture 'CLI path spaces'
        $shell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
        $previousPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        $cliOutput = & $shell -NoProfile -File (Join-Path $repoRoot 'release\uninstall_8gb_fix.ps1') -GameDir $f.Game -NonInteractive 2>&1
        $cliExit = $LASTEXITCODE
        $ErrorActionPreference = $previousPreference
        Assert-True ($cliExit -eq 1) 'Unmanaged folder exits with failure'
        Assert-True (($cliOutput -join ' ') -match 'forza-amd-vram-fix-state|Close Forza') 'Provided path or running-game guard used'
        Assert-Snapshot $f.Game $f.Before
    }
    if ($OriginalAgsPath) {
        Invoke-Test 'real signed-original/current-payload fixture; no validation mocks; no DLL executed' {
            Assert-FixOriginalAgs -Path $OriginalAgsPath
            $f = New-Fixture 'real original integration'
            [IO.File]::Copy($OriginalAgsPath, (Join-Path $f.Game 'amd_ags_x64.dll'), $true)
            Get-ChildItem -LiteralPath (Join-Path $repoRoot 'release') -File |
                Where-Object { $_.Name -ne 'SHA256SUMS.txt' } |
                ForEach-Object { [IO.File]::Copy($_.FullName, (Join-Path $f.Source $_.Name), $true) }
            $lines = @(Get-ChildItem -LiteralPath $f.Source -File |
                Where-Object { $_.Name -ne 'SHA256SUMS.txt' } |
                ForEach-Object { (Get-FixHash $_.FullName) + '  ' + $_.Name })
            [IO.File]::WriteAllLines((Join-Path $f.Source 'SHA256SUMS.txt'), $lines)
            $before = Get-Snapshot $f.Game
            Invoke-ForzaInstall -GameDir $f.Game -SourceDir $f.Source -NonInteractive
            foreach ($name in $FixPayloadNames) {
                Assert-FixCurrent -Path (Join-Path $f.Game $name) -Exists $true -Hash (Get-FixHash (Join-Path $f.Source $name))
            }
            $managed = Get-FixManagedState $f.Game
            Invoke-ForzaUninstall -GameDir $f.Game -NonInteractive
            Assert-Snapshot $f.Game $before
            Assert-True (Test-Path -LiteralPath $managed.BackupDir) 'Real-fixture backup retained before test cleanup'
        }
    }
    Write-Host "PASS: $script:TestCount isolated installer tests. No real game or DLL code executed."
} finally {
    $resolvedTestRoot = [IO.Path]::GetFullPath($testRoot)
    $resolvedTempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (!$resolvedTestRoot.StartsWith($resolvedTempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedTestRoot) -notmatch '^forza installer tests [a-f0-9]{32}$') { throw 'Refusing cleanup outside generated test directory.' }
    Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
}
