# AMD AGS ABI source

The unmodified `amd_ags.h` is the AMD GPU Services SDK 6.3.1 header:

- Repository: https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK
- Pinned commit: `086c47ed8e427199510887230718cd7c086f9702`
- Source: https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/blob/086c47ed8e427199510887230718cd7c086f9702/ags_lib/inc/amd_ags.h
- SHA-256: `9cadb847d7828dc42555daea7fdb2e4b773339c81b738441a1f35e0af1f45f70`

The unmodified `amd_ags_6_3_0_reference.h` is retained to verify the other explicitly supported ABI:

- Tag: `v6.3.0`
- Pinned commit: `5d8812d703d0335741b6f7ffc37838eeb8b967f7`
- Source: https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/blob/5d8812d703d0335741b6f7ffc37838eeb8b967f7/ags_lib/inc/amd_ags.h
- SHA-256: `6826d07fba96819467f29acba1022346228410b29a0c3fa92f1c5c356723672b`

Both headers carry AMD's MIT license notice. Keep the full notices when redistributing them. No AMD DLL or AMD import library is vendored or redistributed here.

`amd_ags_x64.def` explicitly preserves the released proxy's original export ordinals 1 through 37. The proxy marker uses ordinal 38, preserving compatibility for both name imports and ordinal imports.

`AGSDeviceInfo`, `AGSGPUInfo`, `AGSConfiguration`, and `AGSReturnCode` declarations compare identically after stripping comments and whitespace. The public `agsInitialize`, `agsGetGPUInfo`, and `agsGetVersionNumber` declarations and function pointer typedefs also match. `AGSPolicy.h` checks the x64 sizes and relevant field offsets at compile time. The proxy only interprets the buffer for the exact packed version values 6.3.0 and 6.3.1. Unknown versions, missing version exports, or initialization version mismatches disable memory correction.

The game must supply its own original `amd_ags_x64_real.dll`, copied from a clean installation's `amd_ags_x64.dll`. The proxy loads that file using an absolute UTF-16 path next to the proxy and restricted dependency lookup. Loading and logging happen outside `DllMain`. All 37 original proxy export names are retained, with an additional `ForzaFix_AGS_ProxyMarker_v1` export to reject a proxy mistakenly copied as the original. The marker is checked without calling candidate code before resolving the version. The 35 opaque exports use x64 MASM tail-call trampolines, so the private `CreateFromDevice` and `Destroy` signatures are never guessed. An unavailable opaque export fails explicitly. Missing initialization or GPU-info exports return `AGS_FAILURE`; initialization failures clear the context pointer and clear GPU info only when the requested ABI is verified, preserving the original error code.

For discrete AMD GPUs, memory correction uses `DedicatedVideoMemory` returned by the genuine System32 DXGI adapter. PCI vendor, device, and revision must match uniquely in both lists. APUs, software devices, ambiguous identities, missing/failed DXGI enumeration, zero dedicated memory, equal/larger original reports, and non-AMD GPUs are unchanged. Shared memory and GPU budgets are never changed. This general AMD path remains experimental until tested on each relevant game, GPU, and driver configuration.

`ForzaFix_AMD_VRAM.log` is appended beside the proxy after initialization, with version numbers, original return code, AMD/match/patch counts, the first uniquely matched local value before and after correction, and a compact reason. When no correction is made, the before and after values are identical. It is limited to eight records per process and a 64 KiB file threshold. Logging failures are ignored. It contains no adapter serial numbers or user paths.

Offline verification sources are `tests/AGSPolicyTests.cpp` and `tests/AGSTrampolineTests.cpp` plus `tests/AGSClobberResolver.asm`. The trampoline fixture intentionally clobbers RCX/RDX/R8/R9 and XMM0-3, and tests register arguments, stack arguments, return values, and caching. These tests do not replace an in-game check or an antivirus verdict.
