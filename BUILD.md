# Building and checking v0.3.1-rc1

Requirements: Windows x64, Microsoft Visual Studio 2022 Build Tools with Desktop development with C++, the Windows 10/11 SDK, and PowerShell.

From the repository directory:

```powershell
.\build_proxy.ps1 -target all -RunTests
```

The script finds the x64 MSVC environment with Microsoft's vswhere, assembles forwarding stubs with MASM, then builds D3D12, DXGI and AMD AGS. Outputs are written to `proxy_build/`.

Compilation uses C++17, optimization, a static CRT, stack protection and Control Flow Guard. Linking enables ASLR, DEP, high entropy VA and CET compatibility, and uses deterministic linking with `/Brepro`. No packer, obfuscator, downloads or executable game payloads are used.

Unit tests exercise forwarding semantics and AMD adapter matching/failure behavior. Installer tests use isolated fixture folders, including path spaces, invalid inputs and byte-for-byte restore. These tests do not establish compatibility with Forza or other mods.

The default tests verify the pinned AMD headers and exercise the real MASM forwarding stubs with deliberately clobbered argument registers. `tests/proxy_runtime_smoke.cpp` can also compare the built proxies with the native Windows GPU APIs (link with `uuid.lib` and supply the absolute D3D12 and DXGI proxy paths). `tests/AGSRuntimeSmoke.cpp` requires a separately preserved, genuine AMD original in a private test fixture. That original and all runtime fixture binaries are excluded from the source archive.

Release DLLs must be copied from the controlled build to `release/`; refresh the SHA256 manifest after all release file changes. Include a separate source archive, without game files, backup contents, build intermediates or test executables.

The release directory keeps exact bytes through Git using `.gitattributes`. This prevents text line-ending conversion from invalidating the installer checksums in a source archive.

Record the actual compiler and SDK versions, successful test outputs, DLL SHA256 values and archive SHA256 value. A second controlled build must produce matching DLL hashes before describing the candidate as reproducibly built. Do not claim cross-toolchain byte equality.

The AMD AGS SDK source and revision are recorded next to the vendored header. The runtime supports only verified 6.3.0/6.3.1 structure layouts; a future SDK or game update requires another ABI review.
