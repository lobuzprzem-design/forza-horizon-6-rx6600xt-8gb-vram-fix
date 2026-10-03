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
#define AGS_GCC
#define AGS_EXCLUDE_DIRECTX_TYPES
#include "../src/vendor/ags/amd_ags.h"
#include "../src/AGSPolicy.h"
#include <cstdio>
#include <cstdlib>

namespace {
constexpr unsigned long long GiB = 1024ULL * 1024ULL * 1024ULL;
int assertions = 0;
void Check(bool condition, const char* message) {
    ++assertions;
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
AGSDeviceInfo Device(int vendor, int device, int revision, unsigned long long local) {
    AGSDeviceInfo result = {};
    result.vendorId = vendor;
    result.deviceId = device;
    result.revisionId = revision;
    result.localMemoryInBytes = local;
    result.sharedMemoryInBytes = 17 * GiB;
    return result;
}
bool IsZero(const AGSGPUInfo& info) {
    const AGSGPUInfo empty = {};
    return std::memcmp(&info, &empty, sizeof(info)) == 0;
}
AGSReturnCode fixtureResult = AGS_SUCCESS;
AGSGPUInfo fixtureInfo = {};
int initializeCalls = 0;
int getInfoCalls = 0;
int enumerateCalls = 0;
int fixtureVersion = 0;
const AGSConfiguration* fixtureConfig = nullptr;
ags_policy::PhysicalAdapter fixtureAdapters[2] = {};
bool FixtureEnumerate(ags_policy::PhysicalAdapter* adapters, size_t capacity, size_t* count) {
    ++enumerateCalls;
    if (capacity < 1) return false;
    adapters[0] = fixtureAdapters[0];
    *count = 1;
    return true;
}
bool MissingDxgi(ags_policy::PhysicalAdapter*, size_t, size_t*) { return false; }
AGSReturnCode FixtureInitialize(int version, const AGSConfiguration* config, AGSContext** context, AGSGPUInfo* info) {
    ++initializeCalls;
    fixtureVersion = version;
    fixtureConfig = config;
    if (context) *context = reinterpret_cast<AGSContext*>(static_cast<uintptr_t>(0x1234));
    if (info) *info = fixtureInfo;
    return fixtureResult;
}
AGSReturnCode FixtureGetInfo(AGSContext*, AGSGPUInfo* info) {
    ++getInfoCalls;
    if (info) *info = fixtureInfo;
    return fixtureResult;
}
AGSReturnCode OpaqueInitialize(int, const AGSConfiguration*, AGSContext** context, AGSGPUInfo*) {
    if (context) *context = reinterpret_cast<AGSContext*>(static_cast<uintptr_t>(0x1234));
    return fixtureResult;
}
AGSReturnCode OpaqueGetInfo(AGSContext*, AGSGPUInfo*) { return fixtureResult; }
}

int main() {
    using namespace ags_policy;
    const int version630 = AGS_MAKE_VERSION(6, 3, 0);
    const int version631 = AGS_MAKE_VERSION(6, 3, 1);
    const int unknown = AGS_MAKE_VERSION(6, 4, 0);
    Check(IsVerifiedVersion(version630) && IsVerifiedVersion(version631), "verified versions accepted");
    Check(!IsVerifiedVersion(unknown) && !IsVerifiedVersion(0), "unknown versions rejected for patching");

    AGSDeviceInfo devices[] = {
        Device(0x1002, 0x73ff, 0xc1, 6 * GiB),
        Device(0x1002, 0x744c, 0xc8, 12 * GiB),
        Device(0x10de, 0x73ff, 0xc1, 4 * GiB),
        Device(0x8086, 0x73ff, 0xc1, 0),
    };
    PhysicalAdapter physical[] = {
        { 0x1002, 0x73ff, 0xc1, 8 * GiB, false },
        { 0x1002, 0x744c, 0xc8, 24 * GiB, false },
        { 0x10de, 0x73ff, 0xc1, 8 * GiB, false },
        { 0x8086, 0x73ff, 0xc1, 4 * GiB, false },
    };
    AGSGPUInfo info = { "driver", "radeon", 4, devices };
    Check(PatchGpuInfo(&info, physical, 4) == 2, "two distinct discrete AMD models corrected");
    Check(devices[0].localMemoryInBytes == 8 * GiB && devices[1].localMemoryInBytes == 24 * GiB,
        "actual per-adapter VRAM used without inventing 8 GB");
    Check(devices[2].localMemoryInBytes == 4 * GiB && devices[3].localMemoryInBytes == 0,
        "NVIDIA and Intel unchanged despite matching IDs");
    for (const auto& device : devices) Check(device.sharedMemoryInBytes == 17 * GiB, "shared memory unchanged");
    Check(PatchGpuInfo(&info, physical, 4) == 0, "idempotent when AGS already reports physical value");

    AGSDeviceInfo single = Device(0x1002, 0x73ff, 0xc1, 6 * GiB);
    info = { nullptr, nullptr, 1, &single };
    PhysicalAdapter candidate = { 0x1002, 0x73ff, 0xc2, 8 * GiB, false };
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "revision mismatch ignored");
    candidate.revisionId = 0xc1;
    candidate.deviceId = 0x744c;
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "device mismatch ignored");
    candidate.deviceId = 0x73ff;
    candidate.vendorId = 0x10de;
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "physical vendor mismatch ignored");
    candidate.vendorId = 0x1002;
    candidate.dedicatedVideoMemory = 0;
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "zero dedicated memory ignored");
    candidate.dedicatedVideoMemory = 8 * GiB;
    single.isAPU = 1;
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "integrated AMD APU ignored");
    single.isAPU = 0;
    candidate.software = true;
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "software adapter ignored");
    candidate.software = false;
    candidate.dedicatedVideoMemory = 4 * GiB;
    Diagnostics noCorrection;
    Check(PatchGpuInfo(&info, &candidate, 1, &noCorrection) == 0, "higher original AGS memory never reduced");
    Check(noCorrection.firstLocalBefore == 6 * GiB && noCorrection.firstLocalAfter == 6 * GiB &&
        noCorrection.matched == 1 && noCorrection.patched == 0,
        "no-correction diagnostics report actual unchanged memory");
    candidate.dedicatedVideoMemory = 8 * GiB;
    Diagnostics correction;
    Check(PatchGpuInfo(&info, &candidate, 1, &correction) == 1 &&
        correction.firstLocalBefore == 6 * GiB && correction.firstLocalAfter == 8 * GiB,
        "correction diagnostics report actual final memory");
    single.localMemoryInBytes = 6 * GiB;
    PhysicalAdapter duplicatePhysical[] = { candidate, candidate };
    Check(PatchGpuInfo(&info, duplicatePhysical, 2) == 0, "ambiguous physical PCI identities ignored");
    AGSDeviceInfo duplicateAgs[] = { single, single };
    info = { nullptr, nullptr, 2, duplicateAgs };
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "ambiguous AGS identities ignored");
    Check(PatchGpuInfo(nullptr, &candidate, 1) == 0, "null GPU info accepted");
    info = {};
    Check(PatchGpuInfo(&info, &candidate, 1) == 0, "empty GPU info accepted");

    fixtureAdapters[0] = candidate;
    single = Device(0x1002, 0x73ff, 0xc1, 6 * GiB);
    fixtureInfo = { "driver", "radeon", 1, &single };
    AGSContext* context = reinterpret_cast<AGSContext*>(static_cast<uintptr_t>(0x9999));
    info = fixtureInfo;
    Check(Initialize(nullptr, version630, FixtureEnumerate, version630, nullptr, &context, &info) == AGS_FAILURE,
        "missing initializer or original DLL returns failure");
    Check(!context && IsZero(info), "missing initializer clears supported outputs");
    AGSConfiguration config = {};
    fixtureResult = AGS_SUCCESS;
    Check(Initialize(FixtureInitialize, version630, FixtureEnumerate, version630, &config, &context, &info) == AGS_SUCCESS,
        "original success preserved");
    Check(context && initializeCalls == 1 && fixtureVersion == version630 && fixtureConfig == &config,
        "original receives arguments unchanged");
    Check(single.localMemoryInBytes == 8 * GiB && enumerateCalls == 1, "supported success uses physical memory");
    single.localMemoryInBytes = 6 * GiB;
    Check(Initialize(FixtureInitialize, version631, FixtureEnumerate, version631, nullptr, &context, &info) == AGS_SUCCESS,
        "6.3.1 supported independently");
    Check(single.localMemoryInBytes == 8 * GiB, "6.3.1 corrected from physical data");
    single.localMemoryInBytes = 6 * GiB;
    const int enumerationBeforeError = enumerateCalls;
    fixtureResult = AGS_NO_AMD_DRIVER_INSTALLED;
    Check(Initialize(FixtureInitialize, version630, FixtureEnumerate, version630, nullptr, &context, &info) == fixtureResult,
        "original initialization failure code preserved");
    Check(!context && IsZero(info) && enumerateCalls == enumerationBeforeError, "original error clears supported outputs without enumeration");
    Check(single.localMemoryInBytes == 6 * GiB, "failed original result not patched");

    fixtureResult = AGS_SUCCESS;
    Initialize(FixtureInitialize, version630, MissingDxgi, version630, nullptr, &context, &info);
    Check(single.localMemoryInBytes == 6 * GiB && context, "missing DXGI leaves original success and memory unchanged");
    Initialize(FixtureInitialize, version631, FixtureEnumerate, version630, nullptr, &context, &info);
    Check(single.localMemoryInBytes == 6 * GiB, "requested and binary ABI mismatch does not patch");
    Initialize(FixtureInitialize, 0, FixtureEnumerate, version630, nullptr, &context, &info);
    Check(single.localMemoryInBytes == 6 * GiB, "missing version export disables patching");

    // An opaque version may have a smaller or different output structure.
    // Use an unreadable address to prove neither success nor error dereferences it.
    AGSGPUInfo* opaqueInfo = reinterpret_cast<AGSGPUInfo*>(static_cast<uintptr_t>(1));
    const int beforeOpaque = enumerateCalls;
    Check(Initialize(OpaqueInitialize, unknown, FixtureEnumerate, unknown, nullptr, &context, opaqueInfo) == AGS_SUCCESS,
        "unknown initialization ABI passed through without dereference");
    Check(GetGpuInfo(OpaqueGetInfo, unknown, FixtureEnumerate, context, opaqueInfo) == AGS_SUCCESS,
        "unknown query ABI passed through without dereference");
    fixtureResult = AGS_INVALID_ARGS;
    Check(Initialize(OpaqueInitialize, unknown, FixtureEnumerate, unknown, nullptr, &context, opaqueInfo) == fixtureResult,
        "unknown initialization error preserves code without writing GPU buffer");
    Check(!context, "unknown initialization error clears context pointer");
    Check(GetGpuInfo(OpaqueGetInfo, unknown, FixtureEnumerate, context, opaqueInfo) == fixtureResult,
        "unknown query error preserves code without writing GPU buffer");
    Check(enumerateCalls == beforeOpaque, "unknown ABI never enumerates adapters");

    info = fixtureInfo;
    Check(GetGpuInfo(nullptr, version630, FixtureEnumerate, context, &info) == AGS_FAILURE,
        "missing query export returns failure");
    Check(IsZero(info), "missing query clears supported output");
    fixtureResult = AGS_ADL_FAILURE;
    Check(GetGpuInfo(FixtureGetInfo, version630, FixtureEnumerate, context, &info) == fixtureResult,
        "original query failure code preserved");
    Check(IsZero(info), "original query error clears supported output");
    fixtureResult = AGS_SUCCESS;
    context = reinterpret_cast<AGSContext*>(static_cast<uintptr_t>(0x1234));
    Check(GetGpuInfo(FixtureGetInfo, version630, FixtureEnumerate, context, &info) == AGS_SUCCESS,
        "query success preserved");
    Check(single.localMemoryInBytes == 8 * GiB && getInfoCalls == 2, "query fixture corrected using physical memory");
    std::printf("PASS: %d AGS policy assertions\n", assertions);
    return 0;
}
