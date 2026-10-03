# Changelog

## v0.3.1-rc1 — experimental candidate, game test pending

- Replace fixed 8 GB behavior with experimental AMD adapter-specific dedicated-memory reconciliation in AGS.
- Remove the unsafe D3D12 slot 56 hook, capability spoofing, sleeps and vtable writes.
- Preserve the Windows memory budget, shared memory and real hardware capabilities.
- Load genuine original DLLs from controlled paths, preserve proxy exports and report original errors.
- Verify AGS ABI 6.3.0/6.3.1 and skip ambiguous/non-AMD/APU/unknown cases.
- Add checked backup/restore and package integrity checks; refuse unknown DLL conflicts.
- Add source regression tests and a complete MSVC/MASM build for all three DLLs.

The new candidate is released experimentally without a Forza test. Other AMD cards are experimental; community reports are requested.

## v0.3.0-rc1

Release candidate for the current RX 6600 XT 8 GB test build.

### Changed

- Added `amd_ags_x64.dll` proxy to patch AMD AGS local memory reporting to 8 GB.
- Kept the DXGI and D3D12 VRAM reporting paths from the previous working build.
- Updated installer backup logic for `amd_ags_x64.dll`, `amd_ags_x64_real.dll`, `d3d12.dll`, and `dxgi.dll`.
- Added rollback guidance for restoring the original AMD AGS DLL.
- Updated release documentation for Nexus Mods / Vortex users.

### Tested

- Confirmed on one AMD Radeon RX 6600 XT 8 GB system.
- Confirmed Forza Horizon 6 launched through Vortex.
- Confirmed the graphics menu showed 8192 MB on the tested machine.
- Confirmed useful log line: `AGSProxy: agsInitialize patched 1 device(s) to 8GB localMemoryInBytes`.

### Notes

- This is still experimental.
- Other AMD GPUs are untested.
- Online / anti-cheat behavior is not guaranteed.
- Game updates may break the behavior.

## v0.1.0-rx6600xt-8gb

Initial RX 6600 XT 8 GB test release.

### Changed

- Changed reported D3D12 video memory budget from 4 GB to 8 GB.
- Changed reported D3D12 available reservation memory from 4 GB to 8 GB.
- Changed DXGI adapter `DedicatedVideoMemory` spoof target from 4 GB to 8 GB.
- Updated log strings from `4GB` to `8GB`.
- Added an installer script that backs up existing `d3d12.dll` and `dxgi.dll` before replacing them.

### Tested

- Confirmed launch through Vortex.
- Confirmed `ForzaFix_iGPU_.log` reports `GetDesc1 -> 8GB` for AMD Radeon RX 6600 XT.
- Confirmed game reached the map and remained responsive during initial observation.

### Notes

- This release is experimental.
- Only RX 6600 XT 8 GB has been validated so far.
- Other AMD GPUs may work, but should be considered untested.
