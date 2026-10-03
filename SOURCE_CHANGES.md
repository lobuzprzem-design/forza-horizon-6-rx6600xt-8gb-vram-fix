# Source changes in v0.3.1-rc1

The fixed 8 GB behavior from v0.3.0-rc1 has been replaced with an AMD-only reconciliation of AGS local memory against genuine Windows adapter dedicated memory.

D3D12 and DXGI now forward original calls and preserve feature support, requested feature levels, memory budgets and errors. The unsafe ID3D12Device slot 56 write and all vtable modifications were removed.

AGS uses verified upstream structure layouts, a controlled original-DLL loader and error handling. Matching requires a unique AMD adapter; integrated, zero-memory, ambiguous and unsupported layouts are forwarded without changing reported information. Shared memory is not altered.

Forwarding stubs preserve integer and floating-point argument registers and fail deterministically if an undocumented original export is missing. No original game library is redistributed.

The installer/restore scripts verify the package and record the entire earlier DLL state. Original DLL identity and existing mod conflicts are checked before any copy.

See BUILD.md for the actual MSVC/MASM build path. Earlier Zig/Clang notes and old source hashes do not describe this candidate. The release SHA256 manifest is the authoritative inventory of files shipped in this version.

