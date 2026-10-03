#include <windows.h>

// Intentionally incomplete local test library. It is never a game replacement.
extern "C" __declspec(dllexport) int agsGetVersionNumber() {
    return (6 << 22) | (3 << 12);
}
