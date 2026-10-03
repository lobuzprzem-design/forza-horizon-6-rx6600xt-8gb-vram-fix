#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
HMODULE LoadSystem(const wchar_t* name) {
    wchar_t directory[MAX_PATH] = {};
    const UINT length = GetSystemDirectoryW(directory, MAX_PATH);
    Check(length && length < MAX_PATH, "system directory");
    const std::wstring path = std::wstring(directory, length) + L"\\" + name;
    HMODULE module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    Check(module != nullptr, "load native system module");
    return module;
}
template<class T> T Resolve(HMODULE module, const char* name) {
    const auto function = reinterpret_cast<T>(GetProcAddress(module, name));
    Check(function != nullptr, name);
    return function;
}
}

int wmain(int argc, wchar_t** argv) {
    Check(argc == 3, "usage: proxy_runtime_smoke.exe <d3d12.dll absolute> <dxgi.dll absolute>");
    const HMODULE systemDXGI = LoadSystem(L"dxgi.dll");
    const HMODULE systemD3D12 = LoadSystem(L"d3d12.dll");
    constexpr DWORD flags = LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32;
    const HMODULE proxyD3D12 = LoadLibraryExW(argv[1], nullptr, flags);
    const HMODULE proxyDXGI = LoadLibraryExW(argv[2], nullptr, flags);
    Check(proxyD3D12 && proxyDXGI, "load newly built local proxies");
    Check(proxyD3D12 != systemD3D12 && proxyDXGI != systemDXGI, "local and native modules distinct");
    using Factory1 = HRESULT(WINAPI*)(REFIID, void**);
    using DeviceCreate = HRESULT(WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
    const auto nativeFactory = Resolve<Factory1>(systemDXGI, "CreateDXGIFactory1");
    const auto localFactory = Resolve<Factory1>(proxyDXGI, "CreateDXGIFactory1");
    const auto nativeCreate = Resolve<DeviceCreate>(systemD3D12, "D3D12CreateDevice");
    const auto localCreate = Resolve<DeviceCreate>(proxyD3D12, "D3D12CreateDevice");
    IDXGIFactory1* factories[2] = {};
    HRESULT factoryResult[2] = {
        nativeFactory(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factories[0])),
        localFactory(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factories[1]))
    };
    Check(factoryResult[0] == factoryResult[1] && SUCCEEDED(factoryResult[0]), "factory1 native/proxy result");
    IDXGIAdapter1* adapters[2] = {};
    for (int i = 0; i < 2; ++i) Check(SUCCEEDED(factories[i]->EnumAdapters1(0, &adapters[i])), "enum first adapter");
    DXGI_ADAPTER_DESC1 desc[2] = {};
    for (int i = 0; i < 2; ++i) Check(SUCCEEDED(adapters[i]->GetDesc1(&desc[i])), "get original GPU desc");
    Check(desc[0].VendorId == desc[1].VendorId && desc[0].DeviceId == desc[1].DeviceId
          && desc[0].DedicatedVideoMemory == desc[1].DedicatedVideoMemory
          && desc[0].SharedSystemMemory == desc[1].SharedSystemMemory
          && desc[0].AdapterLuid.HighPart == desc[1].AdapterLuid.HighPart
          && desc[0].AdapterLuid.LowPart == desc[1].AdapterLuid.LowPart, "GPU identity and memory unchanged");
    std::wprintf(L"GPU: %ls vendor=0x%04X device=0x%04X dedicated=%llu shared=%llu\n", desc[0].Description,
                 desc[0].VendorId, desc[0].DeviceId,
                 static_cast<unsigned long long>(desc[0].DedicatedVideoMemory),
                 static_cast<unsigned long long>(desc[0].SharedSystemMemory));

    const D3D_FEATURE_LEVEL requested[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_12_2,
                                           static_cast<D3D_FEATURE_LEVEL>(0xFFFF)};
    for (const auto level : requested) {
        ID3D12Device* devices[2] = {};
        const HRESULT hr[2] = {
            nativeCreate(adapters[0], level, __uuidof(ID3D12Device), reinterpret_cast<void**>(&devices[0])),
            localCreate(adapters[0], level, __uuidof(ID3D12Device), reinterpret_cast<void**>(&devices[1]))
        };
        std::printf("CreateDevice level=0x%04X native=0x%08lX proxy=0x%08lX\n", level, hr[0], hr[1]);
        Check(hr[0] == hr[1], "requested feature-level result identical");
        if (SUCCEEDED(hr[0])) {
            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options[2] = {};
            HRESULT supports[2] = {};
            for (int i = 0; i < 2; ++i)
                supports[i] = devices[i]->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options[i], sizeof(options[i]));
            Check(supports[0] == supports[1], "native capability HRESULT unchanged");
            if (SUCCEEDED(supports[0]))
                Check(options[0].MeshShaderTier == options[1].MeshShaderTier
                    && options[0].SamplerFeedbackTier == options[1].SamplerFeedbackTier, "native GPU capabilities unchanged");
        }
        for (auto* device : devices) if (device) device->Release();
    }
    const HRESULT nativeNull = nativeCreate(adapters[0], D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
    const HRESULT proxyNull = localCreate(adapters[0], D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
    Check(nativeNull == proxyNull, "native nullptr output semantics identical");
    for (auto* adapter : adapters) adapter->Release();
    for (auto* factory : factories) factory->Release();
    FreeLibrary(proxyDXGI); FreeLibrary(proxyD3D12); FreeLibrary(systemDXGI); FreeLibrary(systemD3D12);
    std::puts("PASS real Windows GPU proxy smoke");
    return 0;
}
