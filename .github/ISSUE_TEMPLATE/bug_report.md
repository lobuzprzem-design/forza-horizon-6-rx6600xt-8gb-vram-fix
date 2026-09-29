---
name: Bug report
about: Report a launch, install, or VRAM detection problem
title: "[Bug]: "
labels: bug
assignees: ""
---

## What happened?

Describe the problem clearly.

## Your setup

- GPU:
- Driver version:
- Windows version:
- Game install type: Xbox app / Microsoft Store / Steam / other
- Game folder:
- Launch method: Vortex / Xbox app / desktop shortcut / other
- Other tools present: ReShade / OptiScaler / overlays / other DLL mods

## What does the game show?

- VRAM shown before installing the fix:
- VRAM shown after installing the fix:
- Does the game reach the menu?
- Does the game reach the map?

## Logs

Attach or paste the relevant lines from:

```text
ForzaFix_iGPU_.log
ForzaFix_AGS_8GB.log
```

Useful success lines look like:

```text
SPOOF: GetDesc1 -> 8GB em: AMD Radeon RX 6600 XT
SPOOF: IDXGIAdapter3::QueryVideoMemoryInfo LOCAL budget ... -> 8GB
AGSProxy: agsInitialize patched 1 device(s) to 8GB localMemoryInBytes
```

## Screenshots

If possible, add screenshots of the graphics menu before and after installing the fix.
