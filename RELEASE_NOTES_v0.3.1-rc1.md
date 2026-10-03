# v0.3.1-rc1 — experimental AMD memory reporting update

This candidate attempts support for AMD Radeon cards with different VRAM capacities. It uses Windows-reported dedicated memory for the matching AMD adapter rather than imposing 8 GB on every card. Other AMD models are experimental; please report working cards, unchanged VRAM, startup failures and crashes in Nexus comments.

Changes:

- Remove the incorrect D3D12 slot 56 memory hook and graphics capability overrides.
- Forward D3D12 and DXGI to their genuine Windows libraries.
- Reconcile AMD AGS local memory only for a unique matching AMD adapter with real dedicated memory, preserving shared memory and the Windows budget.
- Support verified AMD AGS 6.3.0 and 6.3.1 layouts; preserve errors and skip unknown layouts.
- Load original libraries from controlled absolute paths outside DllMain.
- Preserve all proxy exports and forward their calling conventions.
- Verify hashes, refuse unknown proxy conflicts, preserve a complete backup manifest and restore the previous state.
- Include the full source, build script and regression tests.

Status: local candidate; clean Forza test and publication are pending. The previous version's successful RX 6600 XT test does not validate this build. Scan approval is not claimed.

For each report, provide GPU/VRAM, driver, game/store version, before/after VRAM, whether other mods were absent and the exact error if something fails.
