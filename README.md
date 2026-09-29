# Forza Horizon 6 RX 6600 XT 8GB VRAM Fix

Experimental DirectX 12 / DXGI / AMD AGS proxy fix for systems where Forza Horizon 6 detects an AMD Radeon RX 6600 XT 8 GB card as having only about 6 GB of available VRAM.

This release candidate changes the original AMD Universal Fix behavior from a 4 GB spoof to an 8 GB VRAM report and adds an AMD AGS proxy path tested on one RX 6600 XT system.

Current public version: `v0.3.0-rc1`.

## What This Does

- Places proxy `amd_ags_x64.dll`, `d3d12.dll`, and `dxgi.dll` next to the game executable.
- Spoofs DXGI adapter memory reporting to 8 GB.
- Spoofs D3D12 video memory budget / reservation values to 8 GB.
- Patches AMD AGS GPU memory reporting to 8 GB.
- Keeps the original compatibility hooks for feature level, shader model, tiled resources, enhanced barriers, and mesh shader tier.
- Writes diagnostic information to `ForzaFix_iGPU_.log` and `ForzaFix_AGS_8GB.log`.

This does not include game files and does not modify the game executable.

## Tested Setup

Confirmed working on:

- GPU: MSI AMD Radeon RX 6600 XT 8 GB
- Platform: Xbox / Microsoft Store game install
- Mod launcher: Vortex
- Game folder used during test: `C:\XboxGames\Forza Horizon 6\Content`
- Confirmed log output:
  - `SPOOF: GetDesc1 -> 8GB em: AMD Radeon RX 6600 XT`
  - `PatchDeviceInterfaces: QueryVideoMemoryInfo aplicado via probing dinamico`
  - `AGSProxy: agsInitialize patched 1 device(s) to 8GB localMemoryInBytes`
  - `Patch aplicado: ID3D12Device Spoof de Recursos e VRAM Ativos`

User-visible result on the tested machine: the game launched through Vortex, reached the map, and graphics quality visibly improved compared with the previous 6 GB detected state.

## Download / Release Files

The release package contains:

- `d3d12.dll`
- `dxgi.dll`
- `amd_ags_x64.dll`
- `install_8gb_fix.cmd`
- `install_8gb_fix.ps1`
- `UNINSTALL.txt`
- `LICENSE`
- `README.txt`

## Installation

Recommended method:

1. Close Forza Horizon 6.
2. Close Vortex and Xbox app if they are open.
3. Extract the release package.
4. Run `install_8gb_fix.cmd`.
5. Accept the Windows administrator prompt.
6. Start the game again.

Manual method:

1. Close the game.
2. Open the folder that contains `forzahorizon6.exe`.
3. Back up any existing `amd_ags_x64.dll`, `amd_ags_x64_real.dll`, `d3d12.dll`, and `dxgi.dll`.
4. If `amd_ags_x64_real.dll` does not exist, copy the original game `amd_ags_x64.dll` to `amd_ags_x64_real.dll`.
5. Copy this release's `amd_ags_x64.dll`, `d3d12.dll`, and `dxgi.dll` into that folder.
6. Start the game.

## Uninstall / Rollback

Delete these files from the game folder:

- `d3d12.dll`
- `dxgi.dll`

Restore the original `amd_ags_x64.dll` from the installer backup folder. If `amd_ags_x64_real.dll` is the original game file, you can copy it back to `amd_ags_x64.dll`.

## How To Verify

After launching the game, check:

```text
ForzaFix_iGPU_.log
ForzaFix_AGS_8GB.log
```

Useful successful lines:

```text
=== d3d12.dll proxy com Anti-Crash VRAM carregado ===
=== dxgi.dll proxy carregado ===
PatchDeviceInterfaces: QueryVideoMemoryInfo aplicado via probing dinamico
SPOOF: GetDesc1 -> 8GB em: AMD Radeon RX 6600 XT
SPOOF: IDXGIAdapter3::QueryVideoMemoryInfo LOCAL budget ... -> 8GB
AGSProxy: agsInitialize patched 1 device(s) to 8GB localMemoryInBytes
```

If the game starts but still reports 6 GB, attach `ForzaFix_iGPU_.log` and `ForzaFix_AGS_8GB.log` to an issue.

## Known Risks

This is an experimental proxy DLL fix.

Possible issues:

- crash on launch
- black screen
- conflict with ReShade, OptiScaler, overlays, or other DLL injectors
- future game updates replacing or bypassing the proxy behavior
- anti-cheat / online mode uncertainty

Use at your own risk. Test carefully and keep backups.

## Credits

This work is based on:

- Original project by Joao Lucas / Megadroidgames
- AMD Universal Fix fork by JuniorD-Isael

This RX 6600 XT 8 GB variant changes the VRAM reporting target to 8 GB and packages the tested build for users affected by the 6 GB detection issue.

## License

MIT License. See `LICENSE`.
