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
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <thread>
#include <atomic>
#define AGS_GCC
#define AGS_EXCLUDE_DIRECTX_TYPES
#include "../src/vendor/ags/amd_ags.h"
#include "../src/AGSPolicy.h"

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s (win32=%lu)\n", message, GetLastError()); std::exit(1); }
}
template<class T> T Proc(HMODULE module, const char* name) {
    const FARPROC result = GetProcAddress(module, name);
    Require(result != nullptr, name);
    return reinterpret_cast<T>(result);
}
std::vector<ags_policy::PhysicalAdapter> PhysicalAdapters() {
    wchar_t path[32768] = {};
    const UINT length = GetSystemDirectoryW(path, 32768);
    Require(length && length + 10 < 32768, "GetSystemDirectoryW");
    constexpr wchar_t suffix[] = L"\\dxgi.dll";
    std::wmemcpy(path + length, suffix, sizeof(suffix) / sizeof(suffix[0]));
    const HMODULE dxgi = LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    Require(dxgi != nullptr, "genuine System32 DXGI load");
    using CreateFactory = HRESULT(WINAPI*)(REFIID, void**);
    const auto create = Proc<CreateFactory>(dxgi, "CreateDXGIFactory1");
    IDXGIFactory1* factory = nullptr;
    Require(SUCCEEDED(create(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory))) && factory,
        "native DXGI factory");
    std::vector<ags_policy::PhysicalAdapter> adapters;
    for (UINT index = 0; ; ++index) {
        IDXGIAdapter1* adapter = nullptr;
        const HRESULT result = factory->EnumAdapters1(index, &adapter);
        if (result == DXGI_ERROR_NOT_FOUND) break;
        Require(SUCCEEDED(result) && adapter, "native adapter enumeration");
        DXGI_ADAPTER_DESC1 desc = {};
        const HRESULT described = adapter->GetDesc1(&desc);
        adapter->Release();
        Require(SUCCEEDED(described), "native adapter descriptor");
        adapters.push_back({ desc.VendorId, desc.DeviceId, desc.Revision,
            static_cast<unsigned long long>(desc.DedicatedVideoMemory),
            (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 });
    }
    factory->Release();
    FreeLibrary(dxgi);
    return adapters;
}
void Describe(const char* source, const AGSGPUInfo& info) {
    Require(info.numDevices >= 0 && info.numDevices <= 64, "bounded AGS device count");
    Require(!info.numDevices || info.devices, "device list exists");
    for (int index = 0; index < info.numDevices; ++index) {
        const auto& device = info.devices[index];
        std::printf("%s[%d] vendor=0x%x device=0x%x revision=0x%x apu=%u local=%llu shared=%llu\n",
            source, index, device.vendorId, device.deviceId, device.revisionId,
            device.isAPU, device.localMemoryInBytes, device.sharedMemoryInBytes);
    }
}
bool Empty(const AGSGPUInfo& info) {
    const AGSGPUInfo empty = {};
    return std::memcmp(&empty, &info, sizeof(info)) == 0;
}
}

int wmain(int argc, wchar_t** argv) {
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
    Require(argc == 3, "usage: AGSRuntimeSmoke.exe <absolute proxy path> <absolute original path or --missing>");
    const HMODULE proxy = LoadLibraryExW(argv[1], nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    Require(proxy != nullptr, "proxy load");
    const auto initialize = Proc<AGS_INITIALIZE>(proxy, "agsInitialize");
    const auto getInfo = Proc<AGS_GETGPUINFO>(proxy, "agsGetGPUInfo");
    if (!std::wcscmp(argv[2], L"--missing-forwarded")) {
        const auto deinitialize = Proc<AGS_DEINITIALIZE>(proxy, "agsDeInitialize");
        deinitialize(nullptr);
        Require(false, "missing forwarded export must fail fast");
    }
    if (!std::wcscmp(argv[2], L"--missing")) {
        AGSContext* context = reinterpret_cast<AGSContext*>(static_cast<uintptr_t>(1));
        AGSGPUInfo info;
        std::memset(&info, 0x55, sizeof(info));
        const AGSReturnCode result = initialize(AGS_MAKE_VERSION(6, 3, 0), nullptr, &context, &info);
        Require(result == AGS_FAILURE && !context && Empty(info), "missing DLL fails and clears supported initialization outputs");
        Require(getInfo(nullptr, nullptr) == AGS_FAILURE, "missing GPU-info export fails");
        std::puts("PASS: actual proxy unavailable-original-or-export fixture");
        FreeLibrary(proxy);
        return 0;
    }
    const HMODULE original = LoadLibraryExW(argv[2], nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    Require(original != nullptr, "original AMD load");
    const auto originalVersion = Proc<AGS_GETVERSIONNUMBER>(original, "agsGetVersionNumber");
    const auto proxyVersion = Proc<AGS_GETVERSIONNUMBER>(proxy, "agsGetVersionNumber");
    const auto originalInitialize = Proc<AGS_INITIALIZE>(original, "agsInitialize");
    const auto originalInfo = Proc<AGS_GETGPUINFO>(original, "agsGetGPUInfo");
    const auto originalDeinitialize = Proc<AGS_DEINITIALIZE>(original, "agsDeInitialize");
    const auto proxyDeinitialize = Proc<AGS_DEINITIALIZE>(proxy, "agsDeInitialize");
    const int version = originalVersion();
    std::atomic<bool> start(false);
    std::atomic<bool> versionMismatch(false);
    std::vector<std::thread> callers;
    for (int worker = 0; worker < 16; ++worker) {
        callers.emplace_back([&]() {
            while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
            for (int call = 0; call < 500; ++call) {
                if (proxyVersion() != version) versionMismatch.store(true, std::memory_order_relaxed);
            }
        });
    }
    start.store(true, std::memory_order_release);
    for (auto& caller : callers) caller.join();
    Require(!versionMismatch.load(), "16 concurrent first-call/version trampolines preserve 8000 results");
    std::printf("AGS native version=%d packed (%d.%d.%d)\n", version,
        version >> 22, (version >> 12) & 1023, version & 4095);
    const auto physical = PhysicalAdapters();
    for (const auto& adapter : physical) {
        std::printf("DXGI vendor=0x%x device=0x%x revision=0x%x dedicated=%llu software=%d\n",
            adapter.vendorId, adapter.deviceId, adapter.revisionId, adapter.dedicatedVideoMemory, adapter.software);
    }
    AGSContext* originalContext = nullptr;
    AGSGPUInfo nativeInfo = {};
    const AGSReturnCode nativeResult = originalInitialize(version, nullptr, &originalContext, &nativeInfo);
    std::printf("native agsInitialize rc=%d context=%s\n", nativeResult, originalContext ? "valid" : "null");
    std::vector<AGSDeviceInfo> baseline;
    if (nativeResult == AGS_SUCCESS) {
        Require(originalContext != nullptr, "native initialized context");
        Describe("native", nativeInfo);
        const AGSReturnCode nativeQuery = originalInfo(originalContext, &nativeInfo);
        Require(nativeQuery == AGS_SUCCESS, "native GPU-info query");
        if (nativeInfo.numDevices) baseline.assign(nativeInfo.devices, nativeInfo.devices + nativeInfo.numDevices);
        Require(originalDeinitialize(originalContext) == AGS_SUCCESS, "native deinitialize");
    }
    AGSContext* proxyContext = nullptr;
    AGSGPUInfo corrected = {};
    const AGSReturnCode proxyResult = initialize(version, nullptr, &proxyContext, &corrected);
    std::printf("proxy agsInitialize rc=%d context=%s\n", proxyResult, proxyContext ? "valid" : "null");
    Require(proxyResult == nativeResult, "proxy preserves native initialization return code");
    if (proxyResult == AGS_SUCCESS) {
        Require(proxyContext != nullptr, "proxy initialized context");
        Describe("proxy_initialize", corrected);
        Require(static_cast<size_t>(corrected.numDevices) == baseline.size(), "device count preserved");
        AGSGPUInfo expected = { nullptr, nullptr, static_cast<int>(baseline.size()), baseline.data() };
        if (ags_policy::IsVerifiedVersion(version)) {
            ags_policy::PatchGpuInfo(&expected, physical.data(), physical.size());
        }
        for (size_t index = 0; index < baseline.size(); ++index) {
            Require(corrected.devices[index].vendorId == baseline[index].vendorId &&
                corrected.devices[index].deviceId == baseline[index].deviceId &&
                corrected.devices[index].revisionId == baseline[index].revisionId, "adapter identity preserved");
            Require(corrected.devices[index].localMemoryInBytes == baseline[index].localMemoryInBytes,
                "corrected init memory equals native DXGI expectation");
            Require(corrected.devices[index].sharedMemoryInBytes == baseline[index].sharedMemoryInBytes,
                "shared memory preserved");
        }
        Require(getInfo(proxyContext, &corrected) == AGS_SUCCESS, "proxy GPU-info query");
        Describe("proxy_query", corrected);
        for (size_t index = 0; index < baseline.size(); ++index) {
            Require(corrected.devices[index].localMemoryInBytes == baseline[index].localMemoryInBytes,
                "corrected query memory equals physical expectation");
            Require(corrected.devices[index].sharedMemoryInBytes == baseline[index].sharedMemoryInBytes,
                "query shared memory preserved");
        }
        Require(proxyDeinitialize(proxyContext) == AGS_SUCCESS, "deinitialize trampoline forwards successfully");
    } else {
        Require(!proxyContext && Empty(corrected), "supported native failure leaves empty proxy outputs");
    }
    FreeLibrary(proxy);
    FreeLibrary(original);
    std::puts("PASS: actual AMD SDK runtime smoke; game behavior and antivirus verdict remain untested");
    return 0;
}
