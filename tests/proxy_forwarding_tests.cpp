#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
#include <thread>
#include <vector>

#define FORZA_PROXY_TESTING
#include "../src/D3D12Proxy.cpp"
#include "../src/DXGIProxy.cpp"

extern "C" FARPROC WINAPI GetOriginalProcByName(const char*) noexcept;
extern "C" FARPROC WINAPI GetOriginalProcByOrdinal(WORD) noexcept;
extern "C" FARPROC WINAPI GetOriginalDXGIProcByName(const char*) noexcept;
extern "C" FARPROC WINAPI GetOriginalDXGIProcByOrdinal(WORD) noexcept;

namespace {
std::string mode;
std::atomic<int> d3dLoads{0}, dxgiLoads{0}, systemCalls{0};
std::atomic<int> createCalls{0}, factoryRoute{0}, seenFeatureLevel{0};
std::atomic<UINT> seenFlags{0};
std::atomic<IUnknown*> seenAdapter{nullptr};
std::atomic<void**> seenOutput{nullptr};
std::atomic<const GUID*> seenIID{nullptr};
std::atomic<HRESULT> nativeResult{S_OK};
unsigned long long nativeDevice[] = {0xABCDEF01ULL, 0xABCDEF02ULL};
unsigned long long nativeFactory[] = {0x12345601ULL, 0x12345602ULL};
const GUID requestedIID = {0xFEDCBA98, 0x7654, 0x3210, {1,2,3,4,5,6,7,8}};

void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

HRESULT WINAPI NativeCreateDevice(IUnknown* adapter, D3D_FEATURE_LEVEL feature, REFIID iid, void** output) {
    ++createCalls;
    seenAdapter = adapter; seenFeatureLevel = feature; seenIID = &iid; seenOutput = output;
    if (SUCCEEDED(nativeResult.load()) && output) *output = nativeDevice;
    return nativeResult.load();
}
HRESULT Factory(int route, UINT flags, REFIID iid, void** output) {
    factoryRoute = route; seenFlags = flags; seenIID = &iid; seenOutput = output;
    if (SUCCEEDED(nativeResult.load()) && output) *output = nativeFactory;
    return nativeResult.load();
}
HRESULT WINAPI NativeFactory0(REFIID iid, void** output) { return Factory(0, 0, iid, output); }
HRESULT WINAPI NativeFactory1(REFIID iid, void** output) { return Factory(1, 0, iid, output); }
HRESULT WINAPI NativeFactory2(UINT flags, REFIID iid, void** output) { return Factory(2, flags, iid, output); }
HRESULT WINAPI NativeDebug(UINT flags, REFIID iid, void** output) { return Factory(3, flags, iid, output); }
}

extern "C" UINT ProxyTestGetSystemDirectoryW(LPWSTR output, UINT capacity) {
    ++systemCalls;
    if (mode == "--bad-system-directory") return capacity;
    constexpr wchar_t path[] = L"C:\\Windows\\System32";
    const UINT length = static_cast<UINT>(std::wcslen(path));
    if (capacity <= length) return length + 1;
    wcscpy_s(output, capacity, path);
    return length;
}
extern "C" HMODULE ProxyTestLoadLibraryExW(LPCWSTR path, DWORD flags) {
    Check(flags == LOAD_LIBRARY_SEARCH_SYSTEM32, "dependencies restricted to System32");
    const bool d3d = std::wcscmp(path, L"C:\\Windows\\System32\\d3d12.dll") == 0;
    const bool dxgi = std::wcscmp(path, L"C:\\Windows\\System32\\dxgi.dll") == 0;
    Check(d3d || dxgi, "absolute system DLL path");
    if (d3d) ++d3dLoads; else ++dxgiLoads;
    if (mode == "--missing-module") { SetLastError(ERROR_ACCESS_DENIED); return nullptr; }
    return reinterpret_cast<HMODULE>(static_cast<ULONG_PTR>(d3d ? 0x1000 : 0x2000));
}
extern "C" FARPROC ProxyTestGetProcAddress(HMODULE module, LPCSTR name) {
    if (mode == "--missing-symbol") return nullptr;
    if (reinterpret_cast<ULONG_PTR>(name) <= 0xFFFF) {
        Check(reinterpret_cast<ULONG_PTR>(name) == 99, "ordinal forwarded unchanged");
        return reinterpret_cast<FARPROC>(&NativeCreateDevice);
    }
    if (module == reinterpret_cast<HMODULE>(0x1000)) {
        if (std::strcmp(name, "D3D12CreateDevice") == 0)
            return reinterpret_cast<FARPROC>(&NativeCreateDevice);
    } else {
        if (std::strcmp(name, "CreateDXGIFactory") == 0) return reinterpret_cast<FARPROC>(&NativeFactory0);
        if (std::strcmp(name, "CreateDXGIFactory1") == 0) return reinterpret_cast<FARPROC>(&NativeFactory1);
        if (std::strcmp(name, "CreateDXGIFactory2") == 0) return reinterpret_cast<FARPROC>(&NativeFactory2);
        if (std::strcmp(name, "DXGIGetDebugInterface1") == 0) return reinterpret_cast<FARPROC>(&NativeDebug);
    }
    return nullptr;
}

int main(int argc, char** argv) {
    mode = argc > 1 ? argv[1] : "--forward";
    if (mode != "--forward") {
        const HRESULT error = mode == "--missing-module" ? HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)
            : mode == "--bad-system-directory" ? HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
            : HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        void* output = reinterpret_cast<void*>(0x1111);
        Check(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, requestedIID, &output) == error, "D3D12 loader failure");
        Check(CreateDXGIFactory(requestedIID, &output) == error, "factory0 loader failure");
        Check(CreateDXGIFactory1(requestedIID, &output) == error, "factory1 loader failure");
        Check(CreateDXGIFactory2(7, requestedIID, &output) == error, "factory2 loader failure");
        Check(DXGIGetDebugInterface1(9, requestedIID, &output) == error, "debug loader failure");
        Check(output == reinterpret_cast<void*>(0x1111), "failed resolver leaves outputs untouched");
        Check(createCalls == 0, "missing module never calls original");
        Check(systemCalls == 2, "one initialization per module after failure");
        if (mode == "--bad-system-directory") Check(d3dLoads == 0 && dxgiLoads == 0, "truncated path never loaded");
        std::printf("PASS %s\n", mode.c_str());
        return 0;
    }

    // Concurrent first use exercises actual call_once initialization, with OS calls mocked.
    std::vector<std::thread> workers;
    for (int i = 0; i < 16; ++i) workers.emplace_back([] {
        void* output = nullptr;
        Check(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, requestedIID, &output) == S_OK, "concurrent D3D12");
        Check(CreateDXGIFactory1(requestedIID, &output) == S_OK, "concurrent DXGI");
    });
    for (auto& worker : workers) worker.join();
    Check(d3dLoads == 1 && dxgiLoads == 1 && systemCalls == 2, "load once under concurrency");

    IUnknown* adapter = reinterpret_cast<IUnknown*>(0xAABB);
    void* output = nullptr;
    Check(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_2, requestedIID, &output) == S_OK, "D3D12 success");
    Check(seenAdapter == adapter && seenFeatureLevel == D3D_FEATURE_LEVEL_12_2, "adapter and requested level unchanged");
    Check(seenIID == &requestedIID && seenOutput == &output, "IID and output pointer unchanged");
    Check(output == nativeDevice && nativeDevice[0] == 0xABCDEF01ULL && nativeDevice[1] == 0xABCDEF02ULL,
          "returned device passed unchanged without COM hooks");
    Check(CreateDXGIFactory(requestedIID, &output) == S_OK && factoryRoute == 0, "factory0 goes to factory0");
    Check(CreateDXGIFactory1(requestedIID, &output) == S_OK && factoryRoute == 1, "factory1 goes to factory1");
    Check(CreateDXGIFactory2(0x1234, requestedIID, &output) == S_OK && factoryRoute == 2 && seenFlags == 0x1234,
          "factory2 flags unchanged");
    Check(DXGIGetDebugInterface1(0x5678, requestedIID, &output) == S_OK && factoryRoute == 3 && seenFlags == 0x5678,
          "debug flags unchanged");
    Check(output == nativeFactory && nativeFactory[0] == 0x12345601ULL && nativeFactory[1] == 0x12345602ULL,
          "returned factory passed unchanged without COM hooks");

    nativeResult = E_INVALIDARG;
    output = reinterpret_cast<void*>(0x1111);
    Check(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, requestedIID, &output) == E_INVALIDARG, "native D3D12 failure");
    Check(output == reinterpret_cast<void*>(0x1111), "native error outputs not changed");
    Check(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, requestedIID, nullptr) == E_INVALIDARG && seenOutput == nullptr,
          "D3D12 nullptr passed through");
    Check(CreateDXGIFactory(requestedIID, nullptr) == E_INVALIDARG && seenOutput == nullptr, "factory0 nullptr/error");
    Check(CreateDXGIFactory1(requestedIID, nullptr) == E_INVALIDARG && seenOutput == nullptr, "factory1 nullptr/error");
    Check(CreateDXGIFactory2(7, requestedIID, nullptr) == E_INVALIDARG && seenOutput == nullptr, "factory2 nullptr/error");
    Check(DXGIGetDebugInterface1(9, requestedIID, nullptr) == E_INVALIDARG && seenOutput == nullptr, "debug nullptr/error");

    nativeResult = S_FALSE;
    Check(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, requestedIID, nullptr) == S_FALSE, "non-S_OK success preserved");
    Check(GetOriginalProcByName("D3D12CreateDevice") == reinterpret_cast<FARPROC>(&NativeCreateDevice), "named export resolver");
    Check(GetOriginalProcByOrdinal(99) == reinterpret_cast<FARPROC>(&NativeCreateDevice), "ordinal resolver");
    Check(GetOriginalDXGIProcByName("CreateDXGIFactory1") == reinterpret_cast<FARPROC>(&NativeFactory1), "DXGI named resolver");
    Check(GetOriginalProcByName(nullptr) == nullptr && GetOriginalDXGIProcByName(nullptr) == nullptr, "null resolver argument");
    Check(GetOriginalProcByName("NotAnExport") == nullptr, "unknown export resolver");

    // Separate instances isolate cached failure states without spawning processes.
    mode = "--missing-module";
    forza_proxy::SystemDll inaccessible(L"d3d12.dll");
    Check(inaccessible.Resolve("D3D12CreateDevice") == nullptr, "module load failure");
    Check(inaccessible.MissingExportResult() == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), "loader error retained");
    const int loadsAfterFailure = d3dLoads.load();
    Check(inaccessible.Resolve("D3D12CreateDevice") == nullptr && d3dLoads == loadsAfterFailure,
          "failure initialization cached");
    mode = "--missing-symbol";
    forza_proxy::SystemDll missingSymbol(L"dxgi.dll");
    Check(missingSymbol.Resolve("CreateDXGIFactory1") == nullptr, "missing original symbol");
    Check(missingSymbol.MissingExportResult() == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND), "missing symbol HRESULT");
    mode = "--bad-system-directory";
    const int beforeBadPath = d3dLoads.load();
    forza_proxy::SystemDll badDirectory(L"d3d12.dll");
    Check(badDirectory.Resolve("D3D12CreateDevice") == nullptr, "truncated system directory rejected");
    Check(badDirectory.MissingExportResult() == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "bad directory HRESULT");
    Check(d3dLoads == beforeBadPath, "bad path never loaded");
    std::puts("PASS --forward");
    return 0;
}
