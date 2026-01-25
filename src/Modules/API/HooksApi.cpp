#include "SPF/Modules/API/HooksApi.hpp"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Modules::API {

void HooksApi::FillHooksApi(SPF_Hooks_API* api, SPF_Hook_Register_t pRegister) {
    if (!api) return;
    api->Hook_Register = pRegister;
    api->Hook_FindPattern = &HooksApi::Hook_FindPattern;
    api->Hook_FindPatternFrom = &HooksApi::Hook_FindPatternFrom;
    api->Hook_IsEnabled = &HooksApi::Hook_IsEnabled;
    api->Hook_IsInstalled = &HooksApi::Hook_IsInstalled;
}

uintptr_t HooksApi::Hook_FindPattern(const char* signature) {
    return Utils::PatternFinder::Find(signature);
}

uintptr_t HooksApi::Hook_FindPatternFrom(const char* signature, uintptr_t startAddress, size_t searchLength) {
    return Utils::PatternFinder::Find(startAddress, searchLength, signature);
}

bool HooksApi::Hook_IsEnabled(SPF_Hook_Handle* h) {
    if (!h) return false;
    return reinterpret_cast<SPF::Hooks::IHook*>(h)->IsEnabled();
}

bool HooksApi::Hook_IsInstalled(SPF_Hook_Handle* h) {
    if (!h) return false;
    return reinterpret_cast<SPF::Hooks::IHook*>(h)->IsInstalled();
}

} // namespace Modules::API
SPF_NS_END
