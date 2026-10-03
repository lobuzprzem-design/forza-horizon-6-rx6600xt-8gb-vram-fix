# Install and restore

Test the unmodified game first. This candidate should then be tested by itself, with other mods disabled.

Extract the complete release folder and read `README.txt`. Close Forza and run `install_8gb_fix.cmd`, or invoke the PowerShell installer with its documented game directory argument. The old installer name is kept for continuity; the candidate is no longer a fixed 8 GB patch.

For an explicit folder:

```powershell
powershell -NoProfile -File .\install_8gb_fix.ps1 -GameDir "C:\XboxGames\Forza Horizon 6\Content" -NonInteractive
powershell -NoProfile -File .\uninstall_8gb_fix.ps1 -GameDir "C:\XboxGames\Forza Horizon 6\Content" -NonInteractive
```

Use the actual installation directory for your copy of the game. The scripts do not bypass PowerShell execution policy.

The installer verifies the package, checks the original AMD AGS library and refuses unknown DirectX proxies or an existing managed installation. It preserves the original four DLL paths and creates a manifest with their presence and hashes before applying changes. Keep the backup and state file together.

The backup has a unique `backup-before-forza-amd-vram-fix-v0.3.1-rc1-...` name inside the game directory. `.forza-amd-vram-fix-state.json` points to its manifest and records installation progress. Preserve both if installation is interrupted; the uninstall script can restore a recorded partial installation.

For restore, close the game and run `uninstall_8gb_fix.cmd` or its PowerShell equivalent. Restore uses the earlier presence recorded in the manifest: previously existing DLLs are restored, and files added by this installation are removed. Backup folders are kept.

If another mod or game update has changed an installed file, uninstall stops to avoid overwriting that change. Keep the backup and investigate the conflict.

For a manual installation, first make an external backup of `amd_ags_x64.dll`, `amd_ags_x64_real.dll`, `d3d12.dll` and `dxgi.dll`, and record which paths were absent. Use a genuine AMD AGS original of a supported version. Rename/copy it to `amd_ags_x64_real.dll` before copying the three new proxies. To restore, put back each originally present file and remove only the files that were absent before installation. The automatic uninstaller cannot safely reconstruct a manual installation without its manifest.
