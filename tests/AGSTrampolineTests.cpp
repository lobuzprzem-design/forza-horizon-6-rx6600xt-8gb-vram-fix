#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>

// These deliberately chosen opaque fixture signatures exercise the x64 ABI.
// They do not claim to describe the private AMD functions with the same names.
extern "C" uintptr_t agsDriverExtensionsDX12_CreateFromDevice(
    uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
extern "C" double agsDriverExtensionsDX12_Destroy(
    double, double, double, double, double, double, double, double);
extern "C" int agsDriverExtensionsDX11_SetDepthBounds(void*, void*, bool, float, float);

namespace {
int assertions = 0;
int resolutions = 0;
void Check(bool condition, const char* message) {
    ++assertions;
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
uintptr_t IntegerFixture(uintptr_t a, uintptr_t b, uintptr_t c, uintptr_t d,
    uintptr_t e, uintptr_t f, uintptr_t g, uintptr_t h) {
    Check(a == 0x1111111111111111ULL && b == 0x2222222222222222ULL &&
        c == 0x3333333333333333ULL && d == 0x4444444444444444ULL,
        "all four integer argument registers preserved");
    Check(e == 0x5555555555555555ULL && f == 0x6666666666666666ULL &&
        g == 0x7777777777777777ULL && h == 0x8888888888888888ULL,
        "all four integer stack arguments preserved");
    return 0x123456789abcdef0ULL;
}
double FloatingFixture(double a, double b, double c, double d, double e, double f, double g, double h) {
    Check(a == 1.25 && b == 2.5 && c == 3.75 && d == 4.125,
        "XMM0, XMM1, XMM2, XMM3 preserved");
    Check(e == 5.25 && f == 6.5 && g == 7.75 && h == 8.125,
        "floating-point stack arguments preserved");
    return 98.25;
}
int DepthFixture(void* context, void* dxContext, bool enabled, float minimum, float maximum) {
    Check(context == reinterpret_cast<void*>(static_cast<uintptr_t>(0x1111)) &&
        dxContext == reinterpret_cast<void*>(static_cast<uintptr_t>(0x2222)) && enabled,
        "mixed pointer and bool arguments preserved");
    Check(minimum == 0.125f && maximum == 0.75f, "DX11 floating register and stack arguments preserved");
    return 6;
}
}

extern "C" void* TestResolveAGS(const char* name) {
    ++resolutions;
    if (!std::strcmp(name, "agsDriverExtensionsDX12_CreateFromDevice")) return reinterpret_cast<void*>(&IntegerFixture);
    if (!std::strcmp(name, "agsDriverExtensionsDX12_Destroy")) return reinterpret_cast<void*>(&FloatingFixture);
    if (!std::strcmp(name, "agsDriverExtensionsDX11_SetDepthBounds")) return reinterpret_cast<void*>(&DepthFixture);
    std::abort();
}

int main() {
    for (int pass = 0; pass < 2; ++pass) {
        Check(agsDriverExtensionsDX12_CreateFromDevice(
            0x1111111111111111ULL, 0x2222222222222222ULL, 0x3333333333333333ULL, 0x4444444444444444ULL,
            0x5555555555555555ULL, 0x6666666666666666ULL, 0x7777777777777777ULL, 0x8888888888888888ULL) ==
            0x123456789abcdef0ULL, "integer return value preserved");
        Check(agsDriverExtensionsDX12_Destroy(1.25, 2.5, 3.75, 4.125, 5.25, 6.5, 7.75, 8.125) == 98.25,
            "floating-point return value preserved");
        Check(agsDriverExtensionsDX11_SetDepthBounds(
            reinterpret_cast<void*>(static_cast<uintptr_t>(0x1111)),
            reinterpret_cast<void*>(static_cast<uintptr_t>(0x2222)), true, 0.125f, 0.75f) == 6,
            "real public DX11 signature return value preserved");
    }
    Check(resolutions == 3, "each symbol resolved once then cached");
    std::printf("PASS: %d AGS x64 trampoline assertions\n", assertions);
    return 0;
}
