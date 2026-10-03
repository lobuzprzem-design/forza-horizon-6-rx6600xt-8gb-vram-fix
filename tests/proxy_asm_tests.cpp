#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Link this test with the actual src/D3D12ProxyStubs.asm and the clobber helper.
extern "C" double D3D12PIXReportCounter(int, double, int, double, int, double, int, double);
extern "C" double D3D12Ordinal99(int, double, int, double, int, double, int, double);
extern "C" std::uint64_t GetBehaviorValue(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t,
                                         std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t);
extern "C" void ClobberForwardingArguments();

namespace {
int nativeCalls = 0;
bool expectedMissingExport = false;
void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
double NativeMixed(int a, double b, int c, double d, int e, double f, int g, double h) {
    ++nativeCalls;
    Check(a == 17 && b == 1.25 && c == 33 && d == 2.5 && e == 49 && f == 3.75 && g == 65 && h == 5.0,
          "floating, integer and stack arguments preserved");
    return 9.25;
}
std::uint64_t NativeInteger(std::uint64_t a, std::uint64_t b, std::uint64_t c, std::uint64_t d,
                            std::uint64_t e, std::uint64_t f, std::uint64_t g, std::uint64_t h) {
    ++nativeCalls;
    Check(a == 0x1111111111ULL && b == 2 && c == 3 && d == 4 && e == 5 && f == 6 && g == 7 && h == 0x8888888888ULL,
          "64-bit integer and stack arguments preserved");
    return 0xAABBCCDDEEFF0011ULL;
}
}

extern "C" FARPROC WINAPI GetOriginalProcByName(const char* name) {
    if (expectedMissingExport) return nullptr;
    FARPROC target = std::strcmp(name, "D3D12PIXReportCounter") == 0
        ? reinterpret_cast<FARPROC>(&NativeMixed) : reinterpret_cast<FARPROC>(&NativeInteger);
    ClobberForwardingArguments();
    return target;
}
extern "C" FARPROC WINAPI GetOriginalProcByOrdinal(WORD ordinal) {
    Check(ordinal == 99, "ordinal preserved");
    ClobberForwardingArguments();
    return reinterpret_cast<FARPROC>(&NativeMixed);
}
extern "C" [[noreturn]] void ProxyMissingD3D12Export() noexcept {
    Check(expectedMissingExport, "missing-export callback only used when resolver fails");
    std::puts("PASS proxy ASM register/stack forwarding and missing-export branch");
    std::exit(0);
}

int main() {
    Check(D3D12PIXReportCounter(17, 1.25, 33, 2.5, 49, 3.75, 65, 5.0) == 9.25, "named float return preserved");
    Check(D3D12Ordinal99(17, 1.25, 33, 2.5, 49, 3.75, 65, 5.0) == 9.25, "ordinal float return preserved");
    Check(GetBehaviorValue(0x1111111111ULL, 2, 3, 4, 5, 6, 7, 0x8888888888ULL) == 0xAABBCCDDEEFF0011ULL,
          "64-bit return preserved");
    Check(nativeCalls == 3, "all stubs tail-call originals exactly once");
    expectedMissingExport = true;
    D3D12PIXReportCounter(17, 1.25, 33, 2.5, 49, 3.75, 65, 5.0);
    Check(false, "missing opaque export must enter failure callback");
    return 1;
}
