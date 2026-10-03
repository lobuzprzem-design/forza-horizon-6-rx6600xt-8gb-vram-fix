#pragma once

#include <windows.h>
#include <mutex>
#include <string>

// Test builds replace only the OS boundary; production always uses the APIs below.
#ifdef FORZA_PROXY_TESTING
extern "C" UINT ProxyTestGetSystemDirectoryW(LPWSTR, UINT);
extern "C" HMODULE ProxyTestLoadLibraryExW(LPCWSTR, DWORD);
extern "C" FARPROC ProxyTestGetProcAddress(HMODULE, LPCSTR);
#endif

namespace forza_proxy {
class SystemDll final {
public:
    explicit SystemDll(const wchar_t* name) noexcept : name_(name) {}

    FARPROC Resolve(LPCSTR name) noexcept {
        if (!name) return nullptr;
        try {
            EnsureLoaded();
            if (!module_) return nullptr;
#ifdef FORZA_PROXY_TESTING
            return ProxyTestGetProcAddress(module_, name);
#else
            return GetProcAddress(module_, name);
#endif
        } catch (...) {
            return nullptr;
        }
    }

    HRESULT MissingExportResult() noexcept {
        try {
            EnsureLoaded();
            return HRESULT_FROM_WIN32(module_ ? ERROR_PROC_NOT_FOUND : loadError_);
        } catch (...) {
            return E_FAIL;
        }
    }

private:
    void EnsureLoaded() {
        std::call_once(loaded_, [this] {
            wchar_t systemDirectory[MAX_PATH] = {};
#ifdef FORZA_PROXY_TESTING
            const UINT length = ProxyTestGetSystemDirectoryW(systemDirectory, MAX_PATH);
#else
            const UINT length = GetSystemDirectoryW(systemDirectory, MAX_PATH);
#endif
            if (!length || length >= MAX_PATH) {
                loadError_ = length ? ERROR_INSUFFICIENT_BUFFER : ERROR_PATH_NOT_FOUND;
                return;
            }
            const std::wstring path = std::wstring(systemDirectory, length) + L"\\" + name_;
#ifdef FORZA_PROXY_TESTING
            module_ = ProxyTestLoadLibraryExW(path.c_str(), LOAD_LIBRARY_SEARCH_SYSTEM32);
#else
            module_ = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
#endif
            if (!module_) {
                const DWORD error = GetLastError();
                loadError_ = error ? error : ERROR_MOD_NOT_FOUND;
            }
        });
    }

    const wchar_t* name_;
    std::once_flag loaded_;
    HMODULE module_ = nullptr;
    DWORD loadError_ = ERROR_MOD_NOT_FOUND;
};
}