Forza Horizon 6 RX 6600 XT 8GB VRAM Fix v0.3.0-rc1
=====================================================

Experimental release candidate for systems where Forza Horizon 6 detects an
AMD Radeon RX 6600 XT 8GB card as having about 6GB of available video memory.

What it is meant to fix
-----------------------

On the tested PC the game reported about 6GB of available VRAM and showed
video-memory overload warnings even though the card has 8GB. After installing
this proxy set, the game reported 8192 MB in the graphics menu.

Included files
--------------

- amd_ags_x64.dll
- d3d12.dll
- dxgi.dll
- install_8gb_fix.cmd
- install_8gb_fix.ps1
- LICENSE
- README.txt

Tested setup
------------

- GPU: AMD Radeon RX 6600 XT 8GB
- Install type: Xbox / Microsoft Store game install
- Game folder used in testing: C:\XboxGames\Forza Horizon 6\Content
- Launcher/mod manager used in testing: Vortex
- Result: game launched, graphics menu showed 8192 MB, higher graphics
  settings were usable on the tested machine.

Important notes
---------------

- This is experimental.
- This is not a performance miracle and it does not turn the GPU into a
  stronger card.
- It fixes/works around the wrong VRAM reporting path seen on the tested PC.
- FPS still depends on resolution, graphics settings, OBS, background apps,
  GPU load, and CPU load.
- The in-game "current VRAM usage" value may still be unreliable.
- Keep backups.
- Use at your own risk.

Automatic install
-----------------

1. Close Forza Horizon 6.
2. Close Vortex and the Xbox app if they are open.
3. Extract this release folder.
4. Run install_8gb_fix.cmd.
5. Accept the Windows administrator prompt.
6. Start the game again.

Manual install
--------------

1. Close the game.
2. Open the folder that contains forzahorizon6.exe.
3. Back up these files if they already exist:
   - amd_ags_x64.dll
   - amd_ags_x64_real.dll
   - d3d12.dll
   - dxgi.dll
4. If amd_ags_x64_real.dll does not exist, copy the original game
   amd_ags_x64.dll to amd_ags_x64_real.dll.
5. Copy this release's amd_ags_x64.dll, d3d12.dll and dxgi.dll into the game
   folder.
6. Start the game.

Rollback / uninstall
--------------------

1. Close the game.
2. Delete:
   - d3d12.dll
   - dxgi.dll
3. Restore the original amd_ags_x64.dll from the installer backup folder.
   If amd_ags_x64_real.dll is the original game file, you can copy it back to
   amd_ags_x64.dll.

How to verify
-------------

After launching the game, check the game folder for:

- ForzaFix_iGPU_.log
- ForzaFix_AGS_8GB.log

Useful successful lines include:

- SPOOF: GetDesc1 -> 8GB em: AMD Radeon RX 6600 XT
- SPOOF: IDXGIAdapter3::QueryVideoMemoryInfo LOCAL budget ... -> 8GB
- AGSProxy: agsInitialize patched 1 device(s) to 8GB localMemoryInBytes

Known limitations
-----------------

- Confirmed only on one RX 6600 XT 8GB system so far.
- Other AMD GPUs are untested.
- Game updates can break the behavior.
- Other proxy/injection tools can conflict with this.
- Online/anti-cheat behavior is not guaranteed.

Do not upload this elsewhere as a final stable release until the release notes
and test evidence are complete.
