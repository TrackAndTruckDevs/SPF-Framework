#include "SPF/Modules/API/HooksApi.hpp"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Modules::API {

void HooksApi::FillHooksApi(SPF_Hooks_API* api, SPF_Hooks_Register_t pRegister) {
    if (!api) return;
    api->Register = pRegister;
    api->FindPattern = &HooksApi::T_FindPattern;
    api->FindPatternFrom = &HooksApi::T_FindPatternFrom;
    api->IsEnabled = &HooksApi::T_IsEnabled;
    api->IsInstalled = &HooksApi::T_IsInstalled;
}

uintptr_t HooksApi::T_FindPattern(const char* signature) {
    return Utils::PatternFinder::Find(signature);
}

uintptr_t HooksApi::T_FindPatternFrom(const char* signature, uintptr_t startAddress, size_t searchLength) {
    return Utils::PatternFinder::Find(startAddress, searchLength, signature);
}

bool HooksApi::T_IsEnabled(SPF_Hook_Handle* handle) {
    if (!handle) return false;
    return reinterpret_cast<SPF::Hooks::IHook*>(handle)->IsEnabled();
}

bool HooksApi::T_IsInstalled(SPF_Hook_Handle* handle) {
    if (!handle) return false;
    return reinterpret_cast<SPF::Hooks::IHook*>(handle)->IsInstalled();
}

} // namespace Modules::API
SPF_NS_END
