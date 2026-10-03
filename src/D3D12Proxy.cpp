#include <windows.h>
#include <d3d12.h>
#include <cstdlib>
#include "ProxySystemDll.h"

namespace {
forza_proxy::SystemDll originalD3D12(L"d3d12.dll");
using CreateDevice = HRESULT(WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
}

extern "C" FARPROC WINAPI GetOriginalProcByName(const char* name) noexcept {
    return originalD3D12.Resolve(name);
}

extern "C" FARPROC WINAPI GetOriginalProcByOrdinal(WORD ordinal) noexcept {
    return originalD3D12.Resolve(reinterpret_cast<LPCSTR>(static_cast<ULONG_PTR>(ordinal)));
}

// Opaque legacy exports have different return ABIs. A missing entry point must
// fail as a loader error instead of returning an invented value or jumping null.
extern "C" [[noreturn]] void ProxyMissingD3D12Export() noexcept {
    RaiseException(0xC0000139UL, EXCEPTION_NONCONTINUABLE, 0, nullptr);
    std::abort();
}

extern "C" HRESULT WINAPI D3D12CreateDevice(
    IUnknown* adapter, D3D_FEATURE_LEVEL minimumFeatureLevel, REFIID riid, void** device
) {
    const auto create = reinterpret_cast<CreateDevice>(originalD3D12.Resolve("D3D12CreateDevice"));
    if (!create) return originalD3D12.MissingExportResult();
    return create(adapter, minimumFeatureLevel, riid, device);
}

#ifndef FORZA_PROXY_TESTING
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) {
    return TRUE;
}
#endif
