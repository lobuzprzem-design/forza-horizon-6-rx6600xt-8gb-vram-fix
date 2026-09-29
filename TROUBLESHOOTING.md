# Troubleshooting

## The Game Still Shows 6 GB

Check `ForzaFix_iGPU_.log` in the game folder.

If it contains:

```text
SPOOF: GetDesc1 -> 8GB em: AMD Radeon RX 6600 XT
```

then the proxy loaded and at least part of the DXGI adapter reporting path was patched.

If the in-game menu still shows 6 GB, the game may be reading the value from a different path that is not fully covered yet. Open an issue and attach the log.

## The Game Does Not Start

Try this order:

1. Close the game.
2. Wait 60 seconds.
3. Start the game again.
4. If it still fails, temporarily remove other injectors such as ReShade, OptiScaler, overlays, or additional proxy DLLs.
5. If it still fails, remove `d3d12.dll` and `dxgi.dll` from the game folder.

## Black Screen

Wait up to one minute on the first launch. Shader cache and graphics pipeline setup can take longer after changing proxy DLLs.

If it does not recover, close the game and try once more. If it repeats, uninstall the fix and attach logs to an issue.

## Useful Logs

Attach these when reporting problems:

- `ForzaFix_iGPU_.log`
- `czt_version_proxy.log`, if present
- Windows crash event, if Windows created one
- your GPU model and driver version
- whether Vortex, OptiScaler, ReShade, or overlays were enabled

