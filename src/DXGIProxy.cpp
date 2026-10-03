#include <windows.h>
#include <dxgi1_6.h>
#include "ProxySystemDll.h"

namespace {
forza_proxy::SystemDll originalDXGI(L"dxgi.dll");
using CreateFactory = HRESULT(WINAPI*)(REFIID, void**);
using CreateFactory2 = HRESULT(WINAPI*)(UINT, REFIID, void**);
using GetDebugInterface1 = HRESULT(WINAPI*)(UINT, REFIID, void**);

HRESULT ForwardFactory(const char* name, REFIID riid, void** factory) {
    const auto create = reinterpret_cast<CreateFactory>(originalDXGI.Resolve(name));
    if (!create) return originalDXGI.MissingExportResult();
    return create(riid, factory);
}
}

extern "C" FARPROC WINAPI GetOriginalDXGIProcByName(const char* name) noexcept {
    return originalDXGI.Resolve(name);
}

extern "C" FARPROC WINAPI GetOriginalDXGIProcByOrdinal(WORD ordinal) noexcept {
    return originalDXGI.Resolve(reinterpret_cast<LPCSTR>(static_cast<ULONG_PTR>(ordinal)));
}

extern "C" HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** factory) {
    return ForwardFactory("CreateDXGIFactory", riid, factory);
}

extern "C" HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** factory) {
    return ForwardFactory("CreateDXGIFactory1", riid, factory);
}

extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void** factory) {
    const auto create = reinterpret_cast<CreateFactory2>(originalDXGI.Resolve("CreateDXGIFactory2"));
    if (!create) return originalDXGI.MissingExportResult();
    return create(flags, riid, factory);
}

extern "C" HRESULT WINAPI DXGIGetDebugInterface1(UINT flags, REFIID riid, void** debug) {
    const auto get = reinterpret_cast<GetDebugInterface1>(originalDXGI.Resolve("DXGIGetDebugInterface1"));
    if (!get) return originalDXGI.MissingExportResult();
    return get(flags, riid, debug);
}

#ifndef FORZA_PROXY_TESTING
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) {
    return TRUE;
}
#endif