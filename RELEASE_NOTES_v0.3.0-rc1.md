# Forza Horizon 6 RX 6600 XT 8GB VRAM Fix v0.3.0-rc1

Experimental release candidate for AMD Radeon RX 6600 XT 8GB users affected by Forza Horizon 6 reporting about 6GB of available VRAM.

## What changed

- Added `amd_ags_x64.dll` proxy for AMD AGS memory reporting.
- Kept the DXGI and D3D12 VRAM reporting hooks from the working test build.
- Installer now backs up `amd_ags_x64.dll`, `amd_ags_x64_real.dll`, `d3d12.dll`, and `dxgi.dll`.
- Added rollback notes for restoring the original AMD AGS DLL.
- Added SHA256 checksums in `release/SHA256SUMS.txt`.

## Tested on

- AMD Radeon RX 6600 XT 8GB
- Xbox / Microsoft Store install
- Vortex launch
- Game folder used during testing: `C:\XboxGames\Forza Horizon 6\Content`

Observed result on the tested system:

- game launched,
- map loaded,
- graphics menu showed `8192 MB`,
- higher graphics settings became usable compared with the previous 6GB detected state.

## Important warnings

This is experimental. Keep backups.

Online / anti-cheat behavior is not guaranteed. Other GPUs are untested. Game updates may break the behavior.

## Install

1. Close Forza Horizon 6.
2. Close Vortex and the Xbox app.
3. Download and extract the release ZIP.
4. Run `install_8gb_fix.cmd`.
5. Accept the administrator prompt.
6. Launch the game normally or through Vortex.

## Verify

Check the game folder for:

```text
ForzaFix_iGPU_.log
ForzaFix_AGS_8GB.log
```

Useful success lines:

```text
SPOOF: GetDesc1 -> 8GB em: AMD Radeon RX 6600 XT
SPOOF: IDXGIAdapter3::QueryVideoMemoryInfo LOCAL budget ... -> 8GB
AGSProxy: agsInitialize patched 1 device(s) to 8GB localMemoryInBytes
```

## Rollback

Delete `d3d12.dll` and `dxgi.dll`, then restore the original `amd_ags_x64.dll` from the backup folder created by the installer. If `amd_ags_x64_real.dll` is the original game file, copy it back to `amd_ags_x64.dll`.
