# Reporting and troubleshooting

If the clean game does not work before installing this fix, resolve that problem first.

If startup fails after installing only this candidate, close the game and restore the previous state using the provided uninstaller. Keep the error text and relevant logs. Do not keep stacking mods to diagnose the problem.

If VRAM remains unchanged, the game may already receive the correct physical capacity, use a different reporting path, or have an unsupported/ambiguous adapter layout. A system-assigned budget below physical VRAM is not automatically a detection error. This candidate preserves that budget.

Other mods using `amd_ags_x64.dll`, `d3d12.dll` or `dxgi.dll` are not established compatible. Test one change at a time and restore between tests.

For a report, include GPU model, physical VRAM, driver and game versions, store edition, before/after VRAM, clean test result and the exact crash/error. Attach relevant diagnostic text after removing personal paths. A working result is useful too.

Antivirus quarantine must be investigated by file hash and the actual detection report. Do not disable protection or claim that a detection is harmless merely because the file is a mod. For Nexus quarantine, moderators require the source and build instructions. Keep the blocked version available for their review.

