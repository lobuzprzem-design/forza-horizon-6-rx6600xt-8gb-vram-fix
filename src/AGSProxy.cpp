#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" IMAGE_DOS_HEADER __ImageBase;

struct AGSContext;

enum AGSReturnCode {
    AGS_SUCCESS = 0,
};

enum AGSAsicFamily {
    AGSAsicFamily_Unknown = 0,
};

struct AGSDisplayInfo;

struct AGSDeviceInfo {
    const char* adapterString;
    AGSAsicFamily asicFamily;
    unsigned int flags;
    int vendorId;
    int deviceId;
    int revisionId;
    int numCUs;
    int numWGPs;
    int numROPs;
    int coreClock;
    int memoryClock;
    int memoryBandwidth;
    float teraFlops;
    unsigned long long localMemoryInBytes;
    unsigned long long sharedMemoryInBytes;
    int numDisplays;
    AGSDisplayInfo* displays;
    int eyefinityEnabled;
    int eyefinityGridWidth;
    int eyefinityGridHeight;
    int eyefinityResolutionX;
    int eyefinityResolutionY;
    int eyefinityBezelCompensated;
    int adlAdapterIndex;
    int reserved;
};

struct AGSGPUInfo {
    const char* driverVersion;
    const char* radeonSoftwareVersion;
    int numDevices;
    AGSDeviceInfo* devices;
};

struct AGSConfiguration {
    void* allocCallback;
    void* freeCallback;
};

static constexpr unsigned long long kReportedMemoryBytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;

using agsInitialize_t = AGSReturnCode(__stdcall*)(int, const AGSConfiguration*, AGSContext**, AGSGPUInfo*);
using agsGetGPUInfo_t = AGSReturnCode(__stdcall*)(AGSContext*, AGSGPUInfo*);

static HMODULE g_realAgs = nullptr;
static agsInitialize_t g_realInitialize = nullptr;
static agsGetGPUInfo_t g_realGetGPUInfo = nullptr;

static void Log(const char* message) {
    FILE* f = nullptr;
    fopen_s(&f, "ForzaFix_AGS_8GB.log", "a");
    if (!f) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    std::fprintf(f, "%02u:%02u:%02u - %s\n", st.wHour, st.wMinute, st.wSecond, message);
    std::fclose(f);
}

static HMODULE LoadRealAgs() {
    if (g_realAgs) return g_realAgs;

    char modulePath[MAX_PATH] = {};
    GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), modulePath, MAX_PATH);

    char* slash = std::strrchr(modulePath, '\\');
    if (!slash) {
        Log("AGSProxy: could not resolve module directory");
        return nullptr;
    }

    slash[1] = '\0';
    std::strncat(modulePath, "amd_ags_x64_real.dll", MAX_PATH - std::strlen(modulePath) - 1);

    g_realAgs = LoadLibraryA(modulePath);
    if (!g_realAgs) {
        Log("AGSProxy: failed to load amd_ags_x64_real.dll");
        return nullptr;
    }

    g_realInitialize = reinterpret_cast<agsInitialize_t>(GetProcAddress(g_realAgs, "agsInitialize"));
    g_realGetGPUInfo = reinterpret_cast<agsGetGPUInfo_t>(GetProcAddress(g_realAgs, "agsGetGPUInfo"));
    Log("AGSProxy: loaded real amd_ags_x64_real.dll");
    return g_realAgs;
}

static void PatchGpuInfo(AGSGPUInfo* info, const char* source) {
    if (!info || !info->devices || info->numDevices <= 0) {
        return;
    }

    int patched = 0;
    for (int i = 0; i < info->numDevices; ++i) {
        AGSDeviceInfo& device = info->devices[i];
        const bool looksLikeAmd = device.vendorId == 0x1002;
        const bool hasName = device.adapterString && std::strstr(device.adapterString, "AMD");
        const bool shouldPatch = looksLikeAmd || hasName || device.localMemoryInBytes < kReportedMemoryBytes;

        if (shouldPatch) {
            device.localMemoryInBytes = kReportedMemoryBytes;
            if (device.sharedMemoryInBytes < kReportedMemoryBytes) {
                device.sharedMemoryInBytes = kReportedMemoryBytes;
            }
            ++patched;
        }
    }

    char line[256] = {};
    std::snprintf(line, sizeof(line), "AGSProxy: %s patched %d device(s) to 8GB localMemoryInBytes", source, patched);
    Log(line);
}

extern "C" __declspec(dllexport) AGSReturnCode __stdcall agsInitialize(
    int agsVersion,
    const AGSConfiguration* config,
    AGSContext** context,
    AGSGPUInfo* gpuInfo
) {
    LoadRealAgs();
    if (!g_realInitialize) {
        Log("AGSProxy: agsInitialize missing in real DLL");
        return AGS_SUCCESS;
    }

    AGSReturnCode rc = g_realInitialize(agsVersion, config, context, gpuInfo);
    if (rc == AGS_SUCCESS) {
        PatchGpuInfo(gpuInfo, "agsInitialize");
    }
    return rc;
}

extern "C" __declspec(dllexport) AGSReturnCode __stdcall agsGetGPUInfo(
    AGSContext* context,
    AGSGPUInfo* gpuInfo
) {
    LoadRealAgs();
    if (!g_realGetGPUInfo) {
        Log("AGSProxy: agsGetGPUInfo missing in real DLL");
        return AGS_SUCCESS;
    }

    AGSReturnCode rc = g_realGetGPUInfo(context, gpuInfo);
    if (rc == AGS_SUCCESS) {
        PatchGpuInfo(gpuInfo, "agsGetGPUInfo");
    }
    return rc;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        Log("AGSProxy: amd_ags_x64.dll proxy loaded");
        LoadRealAgs();
    }
    return TRUE;
}
