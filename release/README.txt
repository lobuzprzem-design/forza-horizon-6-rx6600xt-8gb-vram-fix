Forza Horizon 6 AMD VRAM Reporting Fix v0.3.1-rc1
=================================================

Experimental release candidate. A fresh test in the game is still required.
The earlier RX 6600 XT test does not establish that this new version works.

Purpose and scope
-----------------

This release replaces the old fixed 8 GB spoof with AMD AGS reconciliation.
For a matching AMD discrete adapter, the proxy corrects undervalued AGS local
memory reporting to the physical DedicatedVideoMemory reported by System32
DXGI. It does not raise an already higher AGS value or force every card to 8 GB.

- Shared memory is left unchanged.
- Live memory usage, DXGI budgets and reservations are left unchanged.
- d3d12.dll and dxgi.dll are forwarding proxies without feature/VRAM hooks.
- Supported original AMD AGS file versions: 6.3.0.0 and 6.3.1.0.
- The architecture supports matching AMD discrete adapters; other GPU models
  have not been verified in the game.
- No claim is made that every Forza installation, game update or GPU is supported.
- It does not add physical VRAM, guarantee more FPS, or fix an unreliable
  in-game "current usage" counter.

Included release files
----------------------

amd_ags_x64.dll, d3d12.dll, dxgi.dll
install_8gb_fix.ps1, install_8gb_fix.cmd
uninstall_8gb_fix.ps1, uninstall_8gb_fix.cmd
README.txt, UNINSTALL.txt, LICENSE, SHA256SUMS.txt

The legacy script filenames are retained for compatibility. Their names do
not mean that this version forces an 8 GB capacity. No original game DLL is
distributed. Complete source/build documentation belongs with this release.

Before installation
-------------------

Wait for the game download/update to finish. Close the game normally.
Use a clean, original game installation. Other DLL proxy mods are unsupported.
Keep the complete extracted release outside the game directory.

The installer accepts the preserved known original AGS SHA256 or a valid
AMD-signed AGS library with supported version and non-forwarded AGS exports.
It rejects old/unknown AGS proxies and mismatched amd_ags_x64_real.dll copies.
Existing d3d12.dll/dxgi.dll must have a valid Microsoft signature; unknown
libraries and old proxy builds are refused. Do not guess which DLL is original.

If v0.3.0-rc1 or another manually installed proxy is present, recover the
actual original files using its verified backup before installing this RC.
A newer backup made while a proxy was installed may contain that proxy.
Read UNINSTALL.txt before restoring or deleting any file.

Automatic installation
----------------------

1. Extract the complete release to a separate directory.
2. Run install_8gb_fix.cmd, or use Windows PowerShell:

   powershell -NoProfile -File ".\install_8gb_fix.ps1" -GameDir "C:\XboxGames\Forza Horizon 6\Content" -NonInteractive

3. The script verifies the release hashes before changing game files, checks
   that Forza is closed, validates the original libraries, saves verified
   backups/presence and installs the DLLs using per-file atomic replacements.

No automatic administrator elevation or execution-policy bypass is used.
If the directory is not writable, the script stops. Review your directory
permissions yourself. If Windows policy blocks the unsigned script, stop and
verify the release/provenance or use the documented manual method. This
package does not change execution policy or antivirus settings.

-GameDir accepts any folder containing forzahorizon6.exe. Without -GameDir,
the script checks the standard Xbox path and then asks for the folder.
-NonInteractive stops instead of prompting if no directory was found.

Managed installation and backups
--------------------------------

The game directory contains .forza-amd-vram-fix-state.json after installation.
It points to a unique directory named:

backup-before-forza-amd-vram-fix-v0.3.1-rc1-YYYYMMDD-HHMMSS-GUID

manifest.json records the prior presence/hash and installed hash of four DLLs.
original/ contains the previous DLLs; payload/ retains staged release files.
Additional replacement/recovery snapshots can remain in the same backup.
The installer and uninstaller never delete these backup directories.

Reinstallation over managed state is refused. Use the matching uninstaller
first. If an operation is interrupted, preserve the state and entire backup.
The uninstaller can recover an Installing journal only when every file is
still a recognized pre-install or installed version.

Automatic uninstall
-------------------

Close the game and run uninstall_8gb_fix.cmd from the same extracted release,
or use Windows PowerShell with the same -GameDir and -NonInteractive flags.

The uninstaller checks every managed DLL and backup hash before restoring.
It restores every previously present DLL and removes only managed DLLs which
were absent before installation. A changed/quarantined/missing expected file
causes a stop for review, rather than an unverified overwrite or removal.
It retains backups and removes managed state only after successful restore.

An ordinary I/O failure triggers restoration of the previous operation state.
If recovery itself fails or the game starts, preserve backups/state and close
the game before reviewing/retrying. Do not force-delete the state marker.

Manual installation (not managed by the scripts)
-----------------------------------------------

1. Close the game and verify that the game is fully downloaded/updated.
2. Record the presence and SHA256 of amd_ags_x64.dll, amd_ags_x64_real.dll,
   d3d12.dll and dxgi.dll. Back up every existing file before any replacement.
3. Confirm that amd_ags_x64.dll is an original supported AMD AGS library.
   Stop if another proxy or an unknown/mismatched original copy is present.
4. If amd_ags_x64_real.dll was absent, copy the verified original AGS there.
5. Copy this release's three proxy DLLs into the folder with forzahorizon6.exe.

No managed state is created by manual copying. The scripted uninstaller must
not be used for it. Restore using your recorded hashes/presence and backups
as described in UNINSTALL.txt.

Verification and release status
-------------------------------

Isolated installer tests cover original-byte restoration, file absence, paths
with spaces, integrity drift and injected I/O failures. Transaction fixtures
mock vendor identity; the static PE/refusal checks are tested separately.
These tests do not execute game DLLs and do not prove malware absence.

This RC requires a fresh real-game test before publication or promotion.
Check the proxy diagnostic log, launch stability, the GPU capacity shown by
the game, and practical behavior on the actual hardware. Keep test evidence.

The old release received antivirus detections. A changed build and package
do not by themselves resolve those detections. Use the reports for the exact
new hashes; no false-positive or clean-scan claim is made here. Do not disable
protection or add exclusions to force a quarantined file to run.

Game updates, overlays/proxies, anti-cheat and online compatibility remain
unverified. MIT license: see LICENSE.
