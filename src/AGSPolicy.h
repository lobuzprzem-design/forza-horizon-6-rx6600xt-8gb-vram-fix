#pragma once
#include <cstddef>
#include <cstring>

namespace ags_policy {

// Both pinned AMD SDK headers have identical layouts for the structures below.
// Unknown versions are forwarded without interpreting or modifying GPU buffers.
inline bool IsVerifiedVersion(int version) {
    return version == AGS_MAKE_VERSION(6, 3, 0) || version == AGS_MAKE_VERSION(6, 3, 1);
}

static_assert(sizeof(void*) == 8, "The AGS proxy only supports Windows x64");
static_assert(sizeof(AGSConfiguration) == 16, "Unexpected AGSConfiguration ABI");
static_assert(sizeof(AGSGPUInfo) == 32, "Unexpected AGSGPUInfo ABI");
static_assert(sizeof(AGSDeviceInfo) == 120, "Unexpected AGSDeviceInfo ABI");
static_assert(offsetof(AGSGPUInfo, devices) == 24, "Unexpected AGSGPUInfo ABI");
static_assert(offsetof(AGSDeviceInfo, vendorId) == 16, "Unexpected AGSDeviceInfo ABI");
static_assert(offsetof(AGSDeviceInfo, localMemoryInBytes) == 56, "Unexpected AGSDeviceInfo ABI");
static_assert(offsetof(AGSDeviceInfo, sharedMemoryInBytes) == 64, "Unexpected AGSDeviceInfo ABI");

struct PhysicalAdapter {
    unsigned int vendorId;
    unsigned int deviceId;
    unsigned int revisionId;
    unsigned long long dedicatedVideoMemory;
    bool software;
};

struct Diagnostics {
    int amdDevices = 0;
    int matched = 0;
    int patched = 0;
    unsigned long long firstLocalBefore = 0;
    unsigned long long firstLocalAfter = 0;
    const char* reason = "unverified_abi";
};

inline bool SameIdentity(const AGSDeviceInfo& ags, const PhysicalAdapter& physical) {
    return static_cast<unsigned int>(ags.vendorId) == physical.vendorId &&
        static_cast<unsigned int>(ags.deviceId) == physical.deviceId &&
        static_cast<unsigned int>(ags.revisionId) == physical.revisionId;
}

inline int PatchGpuInfo(AGSGPUInfo* info, const PhysicalAdapter* physical, size_t count, Diagnostics* diagnostics = nullptr) {
    if (diagnostics) diagnostics->reason = "no_unique_discrete_amd_match";
    if (!info || !info->devices || info->numDevices <= 0 || info->numDevices > 64 || !physical) {
        if (diagnostics) diagnostics->reason = "empty_or_invalid_gpu_info";
        return 0;
    }
    int patched = 0;
    for (int index = 0; index < info->numDevices; ++index) {
        AGSDeviceInfo& device = info->devices[index];
        if (device.vendorId != 0x1002) continue;
        if (diagnostics) ++diagnostics->amdDevices;
        if (device.isAPU) continue;
        const PhysicalAdapter* match = nullptr;
        size_t matches = 0;
        for (size_t candidate = 0; candidate < count; ++candidate) {
            if (SameIdentity(device, physical[candidate])) {
                match = &physical[candidate];
                ++matches;
            }
        }
        if (matches != 1 || !match || match->software || !match->dedicatedVideoMemory) continue;
        // AGS has no DXGI LUID. Duplicate PCI identities cannot be paired safely.
        int agsMatches = 0;
        for (int candidate = 0; candidate < info->numDevices; ++candidate) {
            if (SameIdentity(info->devices[candidate], *match)) ++agsMatches;
        }
        if (agsMatches != 1) continue;
        if (diagnostics) {
            ++diagnostics->matched;
            if (diagnostics->matched == 1) {
                diagnostics->firstLocalBefore = device.localMemoryInBytes;
                diagnostics->firstLocalAfter = device.localMemoryInBytes;
            }
            diagnostics->reason = "physical_value_not_greater";
        }
        if (match->dedicatedVideoMemory <= device.localMemoryInBytes) continue;
        device.localMemoryInBytes = match->dedicatedVideoMemory;
        if (diagnostics && diagnostics->matched == 1) diagnostics->firstLocalAfter = device.localMemoryInBytes;
        ++patched;
    }
    if (diagnostics) {
        diagnostics->patched = patched;
        if (patched) diagnostics->reason = "physical_memory_corrected";
    }
    return patched;
}

using EnumerateAdapters = bool (*)(PhysicalAdapter*, size_t, size_t*);

inline void PatchFromPhysicalMemory(AGSGPUInfo* info, EnumerateAdapters enumerate, Diagnostics* diagnostics = nullptr) {
    if (!info || !enumerate) {
        if (diagnostics) diagnostics->reason = "gpu_info_not_requested";
        return;
    }
    PhysicalAdapter adapters[64] = {};
    size_t count = 0;
    if (enumerate(adapters, 64, &count) && count <= 64) PatchGpuInfo(info, adapters, count, diagnostics);
    else if (diagnostics) diagnostics->reason = "dxgi_enumeration_unavailable";
}

inline void ClearKnownGpuInfo(AGSGPUInfo* info, int version) {
    if (info && IsVerifiedVersion(version)) std::memset(info, 0, sizeof(*info));
}

inline AGSReturnCode Initialize(AGS_INITIALIZE original, int realVersion, EnumerateAdapters enumerate,
    int requestedVersion, const AGSConfiguration* config, AGSContext** context, AGSGPUInfo* info, Diagnostics* diagnostics = nullptr) {
    if (!original) {
        if (diagnostics) diagnostics->reason = "original_dll_or_initialize_missing";
        if (context) *context = nullptr;
        ClearKnownGpuInfo(info, requestedVersion);
        return AGS_FAILURE;
    }
    const AGSReturnCode result = original(requestedVersion, config, context, info);
    if (result != AGS_SUCCESS) {
        if (diagnostics) diagnostics->reason = "original_initialize_failed";
        if (context) *context = nullptr;
        ClearKnownGpuInfo(info, requestedVersion);
        return result;
    }
    if (IsVerifiedVersion(requestedVersion) && requestedVersion == realVersion && context && *context) {
        PatchFromPhysicalMemory(info, enumerate, diagnostics);
    }
    return result;
}

inline AGSReturnCode GetGpuInfo(AGS_GETGPUINFO original, int realVersion, EnumerateAdapters enumerate,
    AGSContext* context, AGSGPUInfo* info) {
    if (!original) {
        ClearKnownGpuInfo(info, realVersion);
        return AGS_FAILURE;
    }
    const AGSReturnCode result = original(context, info);
    if (result != AGS_SUCCESS) {
        ClearKnownGpuInfo(info, realVersion);
        return result;
    }
    if (IsVerifiedVersion(realVersion) && context) PatchFromPhysicalMemory(info, enumerate);
    return result;
}

} // namespace ags_policy
