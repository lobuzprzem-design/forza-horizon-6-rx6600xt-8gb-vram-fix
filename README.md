# Forza Horizon 6 AMD VRAM Reporting Fix

Experimental AMD update candidate: `v0.3.1-rc1`.

This update attempts to correct understated VRAM reporting for AMD Radeon cards with different dedicated memory capacities. It replaces the earlier fixed 8 GB approach with the dedicated memory capacity reported by Windows for the matching AMD adapter. This is an experimental public candidate that has not yet been tested in Forza Horizon 6.

## What changes

- AMD AGS local memory reporting can be corrected when Windows reports a larger dedicated capacity for one unambiguously matching AMD adapter.
- AMD AGS 6.3.0 and 6.3.1 are supported. Unknown layouts are forwarded without changing the GPU information.
- D3D12 and DXGI calls are forwarded to the original Windows libraries.
- Physical memory, shared memory, the operating system's memory budget and GPU feature support remain the values supplied by the hardware and driver.
- Missing original libraries and exports produce an error instead of falsely reporting success.

The update does not add physical VRAM or enable unsupported graphics features. Integrated adapters with no dedicated memory, ambiguous adapter matches and unsupported AGS versions are left unchanged. Other AMD models remain experimental; successful results on one GPU do not establish compatibility with every AMD card.

## Test first on a clean game

1. Confirm that a fresh Forza installation starts and plays without any mods.
2. Close the game, extract the candidate and read `release/README.txt`.
3. Install only this fix, keeping the backup created by the installer.
4. Compare reported VRAM, startup and gameplay with the clean baseline.
5. Try other mods individually only after the fix passes the clean test.

ReShade and other mods using the same DLL names can conflict with this package. The installer refuses unknown DLL replacements rather than overwriting another mod.

## Installation and restore

See [INSTALL.md](INSTALL.md) and the instructions shipped in `release/`. The installer verifies package hashes before changing files, records the earlier state and creates a backup. The uninstall script checks the installed files and backup before restoring the previous state.

No automatic administrator elevation or persistent security setting changes are performed. If Windows policy or folder permissions block installation, the script stops with an explanation.

## Please report your results

Leave a Nexus comment or open a GitHub issue with:

- AMD GPU model and physical VRAM capacity;
- graphics driver and game version, and Xbox/Microsoft Store or Steam;
- VRAM shown before and after installing the update;
- whether you tested without other mods;
- whether startup and gameplay work, and any error or crash;
- relevant lines from `ForzaFix_AMD_VRAM.log`, if created beside the installed DLL.

Report successful configurations too. Review any attached logs and remove personal information before posting them.

## Status and scan history

The earlier `v0.3.0-rc1` archive was quarantined on Nexus. The linked reports contained detections in its three DLLs. A static review also found an incorrect D3D12 method hook and unsafe failure handling. The new candidate removes that hook and changes the original library loading and installation paths.

This is not a claim that the old detections were false positives or that the new build is antivirus approved. New scan results and the clean game test will be recorded before publication.

The previous RX 6600 XT 8 GB test belongs to the older candidate and does not validate this update. Online compatibility has not been established.

## Source and build

See [BUILD.md](BUILD.md). All three DLLs are built locally from the included sources with Microsoft MSVC x64 and the Windows SDK. No game DLL is distributed.

## Credits and license

Based on the proxy work by Joao Lucas / Megadroidgames and JuniorD-Isael. AMD AGS declarations retain the upstream AMD license and attribution. See `CREDITS.md`, the vendored SDK license, and `LICENSE`.
