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

    // Memory Access API
    api->Memory_ReadInt32 = &Utils::PatternFinder::ReadInt32;
    api->Memory_ReadInt8 = &Utils::PatternFinder::ReadInt8;
    api->Memory_ReadInt64 = &Utils::PatternFinder::ReadInt64;
    api->Memory_ReadFloat = &Utils::PatternFinder::ReadFloat;
    api->Memory_GetRipAddress = &Utils::PatternFinder::GetRipAddress;
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

int32_t HooksApi::Memory_ReadInt32(uintptr_t address) {
    return Utils::PatternFinder::ReadInt32(address);
}

int8_t HooksApi::Memory_ReadInt8(uintptr_t address) {
    return Utils::PatternFinder::ReadInt8(address);
}

int64_t HooksApi::Memory_ReadInt64(uintptr_t address) {
    return Utils::PatternFinder::ReadInt64(address);
}

float HooksApi::Memory_ReadFloat(uintptr_t address) {
    return Utils::PatternFinder::ReadFloat(address);
}

uintptr_t HooksApi::Memory_GetRipAddress(uintptr_t instructionAddr, int offsetPos, int instructionSize) {
    return Utils::PatternFinder::GetRipAddress(instructionAddr, offsetPos, instructionSize);
}

} // namespace Modules::API
SPF_NS_END
