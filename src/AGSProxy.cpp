#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_2.h>
#include <cstddef>
#include <cwchar>
#include <cstdio>

// AGS_GCC suppresses SDK dllexport annotations. All SDK types stay unmodified.
#define AGS_GCC
#define AGS_EXCLUDE_DIRECTX_TYPES
#include "vendor/ags/amd_ags.h"
#include "AGSPolicy.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {
INIT_ONCE g_realAgsOnce = INIT_ONCE_STATIC_INIT;
HMODULE g_realAgs = nullptr;
AGS_INITIALIZE g_realInitialize = nullptr;
AGS_GETGPUINFO g_realGetGPUInfo = nullptr;
int g_realVersion = 0;
INIT_ONCE g_dxgiOnce = INIT_ONCE_STATIC_INIT;
HMODULE g_systemDxgi = nullptr;
using CreateFactory1 = HRESULT(WINAPI*)(REFIID, void**);
CreateFactory1 g_createFactory1 = nullptr;
volatile LONG g_diagnosticLines = 0;

void LogInitialization(int requestedVersion, AGSReturnCode result, const ags_policy::Diagnostics& diagnostic) {
    // At most eight short records per process, and never grow beyond 64 KiB.
    if (InterlockedIncrement(&g_diagnosticLines) > 8) return;
    wchar_t path[32768] = {};
    const DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path,
        static_cast<DWORD>(sizeof(path) / sizeof(path[0])));
    if (!length || length >= sizeof(path) / sizeof(path[0])) return;
    wchar_t* slash = std::wcsrchr(path, L'\\');
    if (!slash) return;
    constexpr wchar_t fileName[] = L"ForzaFix_AMD_VRAM.log";
    const size_t prefix = static_cast<size_t>(slash + 1 - path);
    if (prefix + sizeof(fileName) / sizeof(fileName[0]) > sizeof(path) / sizeof(path[0])) return;
    std::wmemcpy(slash + 1, fileName, sizeof(fileName) / sizeof(fileName[0]));
    const HANDLE file = CreateFileW(path, FILE_APPEND_DATA | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(file, &size) || size.QuadPart >= 65536) { CloseHandle(file); return; }
    char line[320] = {};
    const int bytes = std::snprintf(line, sizeof(line),
        "agsInitialize requested=%d real=%d rc=%d amd=%d matched=%d patched=%d firstLocal=%llu->%llu reason=%s\r\n",
        requestedVersion, g_realVersion, static_cast<int>(result), diagnostic.amdDevices,
        diagnostic.matched, diagnostic.patched, diagnostic.firstLocalBefore,
        diagnostic.firstLocalAfter, diagnostic.reason);
    if (bytes > 0 && static_cast<size_t>(bytes) < sizeof(line)) {
        DWORD written = 0;
        WriteFile(file, line, static_cast<DWORD>(bytes), &written, nullptr);
    }
    CloseHandle(file);
}

BOOL CALLBACK InitializeRealAgs(PINIT_ONCE, PVOID, PVOID*) {
    // Resolve relative to this proxy, never the process working directory.
    wchar_t path[32768] = {};
    const DWORD length = GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase), path,
        static_cast<DWORD>(sizeof(path) / sizeof(path[0])));
    if (!length || length >= sizeof(path) / sizeof(path[0])) return TRUE;
    wchar_t* slash = std::wcsrchr(path, L'\\');
    if (!slash) return TRUE;
    constexpr wchar_t realName[] = L"amd_ags_x64_real.dll";
    const size_t prefix = static_cast<size_t>(slash + 1 - path);
    if (prefix + sizeof(realName) / sizeof(realName[0]) > sizeof(path) / sizeof(path[0])) return TRUE;
    std::wmemcpy(slash + 1, realName, sizeof(realName) / sizeof(realName[0]));
    g_realAgs = LoadLibraryExW(path, nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!g_realAgs) return TRUE;
    // A copied proxy has a different module handle. Reject its marker before
    // calling any version trampoline, which would recursively enter INIT_ONCE.
    if (g_realAgs == reinterpret_cast<HMODULE>(&__ImageBase) ||
        GetProcAddress(g_realAgs, "ForzaFix_AGS_ProxyMarker_v1")) {
        FreeLibrary(g_realAgs);
        g_realAgs = nullptr;
        return TRUE;
    }
    g_realInitialize = reinterpret_cast<AGS_INITIALIZE>(GetProcAddress(g_realAgs, "agsInitialize"));
    g_realGetGPUInfo = reinterpret_cast<AGS_GETGPUINFO>(GetProcAddress(g_realAgs, "agsGetGPUInfo"));
    const auto getVersion = reinterpret_cast<AGS_GETVERSIONNUMBER>(GetProcAddress(g_realAgs, "agsGetVersionNumber"));
    if (getVersion) g_realVersion = getVersion();
    return TRUE;
}

void LoadRealAgs() {
    InitOnceExecuteOnce(&g_realAgsOnce, InitializeRealAgs, nullptr, nullptr);
}

BOOL CALLBACK InitializeSystemDxgi(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t path[32768] = {};
    const UINT length = GetSystemDirectoryW(path, static_cast<UINT>(sizeof(path) / sizeof(path[0])));
    constexpr wchar_t suffix[] = L"\\dxgi.dll";
    if (!length || length + sizeof(suffix) / sizeof(suffix[0]) > sizeof(path) / sizeof(path[0])) return TRUE;
    std::wmemcpy(path + length, suffix, sizeof(suffix) / sizeof(suffix[0]));
    g_systemDxgi = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (g_systemDxgi) {
        g_createFactory1 = reinterpret_cast<CreateFactory1>(GetProcAddress(g_systemDxgi, "CreateDXGIFactory1"));
    }
    return TRUE;
}

bool EnumeratePhysicalAdapters(ags_policy::PhysicalAdapter* adapters, size_t capacity, size_t* count) {
    *count = 0;
    InitOnceExecuteOnce(&g_dxgiOnce, InitializeSystemDxgi, nullptr, nullptr);
    if (!g_createFactory1) return false;
    IDXGIFactory1* factory = nullptr;
    const HRESULT created = g_createFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
    if (FAILED(created) || !factory) return false;
    bool complete = false;
    for (UINT index = 0; ; ++index) {
        IDXGIAdapter1* adapter = nullptr;
        const HRESULT result = factory->EnumAdapters1(index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) { complete = true; break; }
        if (FAILED(result) || !adapter) break;
        DXGI_ADAPTER_DESC1 desc = {};
        const HRESULT described = adapter->GetDesc1(&desc);
        adapter->Release();
        if (FAILED(described) || *count == capacity) break;
        adapters[*count] = { desc.VendorId, desc.DeviceId, desc.Revision,
            static_cast<unsigned long long>(desc.DedicatedVideoMemory),
            (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 };
        ++*count;
    }
    factory->Release();
    // A partial list cannot establish adapter identity unambiguously.
    if (!complete) *count = 0;
    return complete;
}
} // namespace

// The marker identifies our proxy without executing code in a candidate module.
extern "C" int ForzaFix_AGS_ProxyMarker_v1() { return 1; }

// Opaque exports use x64 tail-call trampolines. No private signatures are guessed.
extern "C" FARPROC GetOriginalAGSProcByName(const char* name) {
    LoadRealAgs();
    FARPROC proc = g_realAgs ? GetProcAddress(g_realAgs, name) : nullptr;
    if (proc) return proc;
    RaiseFailFastException(nullptr, nullptr, 0);
    TerminateProcess(GetCurrentProcess(), ERROR_PROC_NOT_FOUND);
    return nullptr;
}

extern "C" AGSReturnCode agsInitialize(
    int agsVersion, const AGSConfiguration* config, AGSContext** context, AGSGPUInfo* gpuInfo) {
    LoadRealAgs();
    ags_policy::Diagnostics diagnostic;
    const AGSReturnCode result = ags_policy::Initialize(g_realInitialize, g_realVersion,
        EnumeratePhysicalAdapters, agsVersion, config, context, gpuInfo, &diagnostic);
    LogInitialization(agsVersion, result, diagnostic);
    return result;
}

extern "C" AGSReturnCode agsGetGPUInfo(AGSContext* context, AGSGPUInfo* gpuInfo) {
    LoadRealAgs();
    return ags_policy::GetGpuInfo(g_realGetGPUInfo, g_realVersion,
        EnumeratePhysicalAdapters, context, gpuInfo);
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) {
    // No file I/O, COM calls, or LoadLibrary under the loader lock.
    return TRUE;
}
