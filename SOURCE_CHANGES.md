# Source Changes

This RX 6600 XT variant keeps the original proxy approach and changes the reported memory target to 8 GB.

## D3D12Proxy.cpp

Changed the D3D12 reported video memory budget constant to:

```cpp
static constexpr uint64_t kReportedVideoMemoryBudget = 8ULL * 1024ULL * 1024ULL * 1024ULL;
```

The hook updates the D3D12 video memory info fields:

```cpp
memoryInfoFields[0] = kReportedVideoMemoryBudget; // Budget = 8 GB
memoryInfoFields[3] = kReportedVideoMemoryBudget; // AvailableForReservation = 8 GB
```

The diagnostic log string was updated to report 8 GB.

## DXGIProxy.cpp

Changed the reported DXGI dedicated video memory constant to:

```cpp
static constexpr UINT64 kReportedDedicatedVideoMemory = 8ULL * 1024ULL * 1024ULL * 1024ULL;
```

The adapter hooks update:

- `DXGI_ADAPTER_DESC::DedicatedVideoMemory`
- `DXGI_ADAPTER_DESC1::DedicatedVideoMemory`
- `DXGI_ADAPTER_DESC2::DedicatedVideoMemory`

The diagnostic log strings were updated to report 8 GB for `GetDesc`, `GetDesc1`, and `GetDesc2`.

## Build Notes

The release DLLs were built locally with portable Zig / Clang after Visual Studio Build Tools installation was not available in the test environment.

Release DLL hashes:

```text
3E187DC746566281117091FCC775697EFDAB2CF54D42D6AF78495AE627223ECE  d3d12.dll
0E40DC448B2C628D913CF7D725B88B5F251CED0A1B0337E6A99D37115F2031D9  dxgi.dll
```

## Runtime Evidence From Test Machine

The tested RX 6600 XT system produced these useful log lines:

```text
PatchDeviceInterfaces: QueryVideoMemoryInfo aplicado via probing dinamico
Patch aplicado: ID3D12Device Spoof de Recursos e VRAM Ativos (lock system ativo)
SPOOF: GetDesc1 -> 8GB em: AMD Radeon RX 6600 XT
```

