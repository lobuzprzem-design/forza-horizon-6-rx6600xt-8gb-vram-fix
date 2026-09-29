# Install Guide

## Before Installing

Close:

- Forza Horizon 6
- Vortex
- Xbox app

This prevents Windows from locking the DLL files while they are being replaced.

## Automatic Install

1. Extract the release ZIP.
2. Run `install_8gb_fix.cmd`.
3. Approve the Windows administrator prompt.
4. Wait until the installer says it is done.
5. Start Forza Horizon 6 normally or through Vortex.

The installer creates a backup folder in the game directory before copying the new files.

## Manual Install

1. Open the folder containing `forzahorizon6.exe`.
2. Back up existing `amd_ags_x64.dll`, `amd_ags_x64_real.dll`, `d3d12.dll`, and `dxgi.dll` if they exist.
3. If `amd_ags_x64_real.dll` does not exist, copy the original game `amd_ags_x64.dll` to `amd_ags_x64_real.dll`.
4. Copy the release versions of `amd_ags_x64.dll`, `d3d12.dll`, and `dxgi.dll` into that folder.
5. Start the game.

## Uninstall

Delete these two files from the game folder:

- `d3d12.dll`
- `dxgi.dll`

Then restore the original `amd_ags_x64.dll` from the backup folder created by the installer.

If you installed manually and `amd_ags_x64_real.dll` is the original game file, copy `amd_ags_x64_real.dll` back to `amd_ags_x64.dll`.
