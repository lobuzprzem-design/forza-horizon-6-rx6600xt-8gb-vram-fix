# v0.3.1-rc1 release checks

- [x] All three DLLs compile from included source.
- [x] Proxy/AGS and installer regression tests pass (including 25 isolated installer scenarios with a genuine AMD original).
- [x] A second build produces identical DLL SHA256 values with the same MSVC toolchain.
- [ ] Release manifest and both archives have recorded SHA256 values.
- [ ] The release has no game DLLs, backups, nested archives or test executables.
- [ ] Fresh Forza works without mods.
- [ ] Candidate works by itself on the fresh RX 6600 XT installation.
- [ ] Restore returns the previous state.
- [ ] New per-file and archive scan results are reviewed.
- [x] Other AMD models are described as experimental and reports requested.
- [x] Source and exact build instructions are prepared locally for Nexus moderators.
- [ ] User's clean test result is recorded before publication.

Publication is deferred until the clean game test, as requested by the user on 3 October 2026.

The completed restore check above is in isolated fixture folders. Restore on the fresh game remains a separate pending check. Antivirus and archive evidence is recorded in the accompanying local VALIDATION.md after packaging; external Nexus/VirusTotal review is still pending.

