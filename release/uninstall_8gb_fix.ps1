#requires -Version 5.1
[CmdletBinding()]
param([string]$GameDir, [switch]$NonInteractive, [switch]$LibraryOnly)
$ErrorActionPreference = 'Stop'
$uninstallGameDir = $GameDir
$uninstallNonInteractive = $NonInteractive
$uninstallLibraryOnly = $LibraryOnly
. (Join-Path $PSScriptRoot 'install_8gb_fix.ps1') -LibraryOnly
$GameDir = $uninstallGameDir
$NonInteractive = $uninstallNonInteractive
$LibraryOnly = $uninstallLibraryOnly

function Invoke-ForzaUninstall {
    param([string]$GameDir, [switch]$NonInteractive)
    $GameDir = Get-FixGameDirectory -Path $GameDir -NonInteractive:$NonInteractive
    Assert-FixGameStopped
    $managed = Get-FixManagedState $GameDir
    Assert-FixRestorePlan -GameDir $GameDir -BackupDir $managed.BackupDir -Entries $managed.Entries -Incomplete ($managed.State.Status -eq 'Installing')
    $attemptDir = Join-Path $managed.BackupDir ('uninstall-attempt-' + [Guid]::NewGuid().ToString('N'))
    $null = New-Item -ItemType Directory -Path $attemptDir -ErrorAction Stop
    $snapshots = @(foreach ($entry in $managed.Entries) {
        $current = Get-FixCurrentSnapshot -Directory $GameDir -Name $entry.Name
        if ($current.Exists) {
            [IO.File]::Copy((Join-Path $GameDir $entry.Name), (Join-Path $attemptDir $entry.Name), $false)
            Assert-FixCurrent -Path (Join-Path $attemptDir $entry.Name) -Exists $true -Hash $current.Hash
        }
        $current
    })
    try {
        Assert-FixGameStopped
        Assert-FixCurrent -Path $managed.Path -Exists $true -Hash $managed.Hash
        Restore-FixBefore -GameDir $GameDir -BackupDir $managed.BackupDir -Entries $managed.Entries
        Remove-FixFileChecked -Path $managed.Path -Hash $managed.Hash
        Write-Host 'Previous DLL bytes and file presence restored. Managed state removed.'
        Write-Host "Backup retained: $($managed.BackupDir)"
    } catch {
        $cause = $_.Exception.Message
        try {
            Assert-FixGameStopped
            # Validate ALL current files and backup bytes before reversing a partial uninstall.
            Assert-FixRestorePlan -GameDir $GameDir -BackupDir $managed.BackupDir -Entries $managed.Entries -Incomplete $true
            Assert-FixCurrent -Path $managed.Path -Exists $true -Hash $managed.Hash
            foreach ($snapshot in $snapshots) {
                if ($snapshot.Exists) { Assert-FixCurrent -Path (Join-Path $attemptDir $snapshot.Name) -Exists $true -Hash $snapshot.Hash }
            }
            foreach ($snapshot in $snapshots) {
                $current = Get-FixCurrentSnapshot -Directory $GameDir -Name $snapshot.Name
                if ($current.Exists -eq $snapshot.Exists -and $current.Hash -eq $snapshot.Hash) { continue }
                $path = Join-Path $GameDir $snapshot.Name
                if ($snapshot.Exists) {
                    Set-FixFileAtomic -Source (Join-Path $attemptDir $snapshot.Name) -Target $path -Hash $snapshot.Hash -ExpectedExists $current.Exists -ExpectedHash $current.Hash -WorkingDir $attemptDir
                } else { Remove-FixFileChecked -Path $path -Hash $current.Hash }
            }
            foreach ($snapshot in $snapshots) { Assert-FixCurrent -Path (Join-Path $GameDir $snapshot.Name) -Exists $snapshot.Exists -Hash $snapshot.Hash }
        } catch {
            throw "Uninstall failed: $cause. Rollback could not finish: $($_.Exception.Message). Preserve backups/state and review changed files before retrying."
        }
        throw "Uninstall failed; pre-uninstall DLL bytes/presence restored and managed state retained. Cause: $cause"
    }
}
if (!$LibraryOnly) {
    try { Invoke-ForzaUninstall -GameDir $GameDir -NonInteractive:$NonInteractive }
    catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }
}
