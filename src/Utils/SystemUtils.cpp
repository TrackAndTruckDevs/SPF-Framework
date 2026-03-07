#include "SPF/Utils/SystemUtils.hpp"
#include <Windows.h>
#include <algorithm>
#include <winternl.h>

// Define RtlGetVersion prototype if not available via headers
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

SPF_NS_BEGIN

namespace Utils {

std::string SystemUtils::GetSystemLocaleName() {
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH)) {
        // Convert wchar_t to std::string (ASCII-compatible for locales)
        char mbs[LOCALE_NAME_MAX_LENGTH];
        size_t converted;
        if (wcstombs_s(&converted, mbs, sizeof(mbs), localeName, _TRUNCATE) == 0) {
            return std::string(mbs);
        }
    }
    return "en-US"; // Fallback
}

std::string SystemUtils::GetOSVersionString() {
    std::string osName = "Unknown Windows";
    
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        auto pRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
        if (pRtlGetVersion) {
            RTL_OSVERSIONINFOW osvi = { 0 };
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (pRtlGetVersion(&osvi) == 0) {
                osName = "Windows " + std::to_string(osvi.dwMajorVersion);
                if (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 22000) {
                    osName = "Windows 11";
                }
                osName += " (Build " + std::to_string(osvi.dwBuildNumber) + ")";
            }
        }
    }
    return osName;
}

std::string SystemUtils::GetSystemArchitecture() {
    // Current target is always x64 for ETS2/ATS plugins
    return "x64";
}

} // namespace Utils

SPF_NS_END
