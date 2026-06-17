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

    // ABI Extension (v1.1)
    api->Hook_FindBackward = &HooksApi::Hook_FindBackward;
    api->Hook_FindString = &HooksApi::Hook_FindString;
    api->Hook_FindFunctionByString = &HooksApi::Hook_FindFunctionByString;
    // ABI Extension (Advanced Lookup)
    api->Hook_GetFunctionStart = &HooksApi::Hook_GetFunctionStart;
    api->Hook_GetFunctionEnd   = &HooksApi::Hook_GetFunctionEnd;
    api->Hook_FindChain = &HooksApi::Hook_FindChain;
    api->Hook_FindVTable = &HooksApi::Hook_FindVTable;
    api->Hook_GetVTableFunction = &HooksApi::Hook_GetVTableFunction;
    api->Hook_FindFunctionByConstant = &HooksApi::Hook_FindFunctionByConstant;

    // --- ABI Extension (v1.2) ---
    api->Reflection_GetAttributeOffset = &HooksApi::Reflection_GetAttributeOffset;
    api->Reflection_ResolveSmartPtr    = &HooksApi::Reflection_ResolveSmartPtr;
    api->Memory_WriteFloat             = &HooksApi::Memory_WriteFloat;
    api->Memory_WriteInt32             = &HooksApi::Memory_WriteInt32;
    api->Memory_ReadVector3            = &HooksApi::Memory_ReadVector3;
    api->Memory_WriteVector3           = &HooksApi::Memory_WriteVector3;
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

uintptr_t HooksApi::Hook_FindBackward(uintptr_t startAddress, size_t searchRange, const char* signature) {
    return Utils::PatternFinder::FindBackward(startAddress, searchRange, signature);
}

uintptr_t HooksApi::Hook_FindString(const char* str) {
    return Utils::PatternFinder::FindString(str);
}

uintptr_t HooksApi::Hook_FindFunctionByString(const char* str, bool findStart, const char* contextSig, size_t contextRange) {
    return Utils::PatternFinder::FindFunctionByString(str, findStart, contextSig, contextRange);
}

uintptr_t HooksApi::Hook_GetFunctionStart(uintptr_t address) {
    return Utils::PatternFinder::GetFunctionStart(address);
}

uintptr_t HooksApi::Hook_GetFunctionEnd(uintptr_t address) {
    return Utils::PatternFinder::GetFunctionEnd(address);
}

uintptr_t HooksApi::Hook_FindChain(const char** signatures, size_t count, size_t maxGap, uintptr_t startAddress, size_t searchRange) {
    if (!signatures || count == 0) return 0;
    std::vector<std::string> sigs;
    for (size_t i = 0; i < count; ++i) {
        sigs.push_back(signatures[i]);
    }
    return Utils::PatternFinder::FindChain(sigs, maxGap, startAddress, searchRange);
}

uintptr_t HooksApi::Hook_FindVTable(const char* signature, int offsetPos, int instructionSize) {
    return Utils::PatternFinder::FindVTable(signature, offsetPos, instructionSize);
}

uintptr_t HooksApi::Hook_GetVTableFunction(uintptr_t vtableAddr, int index) {
    return Utils::PatternFinder::GetVTableFunction(vtableAddr, index);
}

uintptr_t HooksApi::Hook_FindFunctionByConstant(uint32_t constant, bool findStart) {
    return Utils::PatternFinder::FindFunctionByConstant(constant, findStart);
}

uintptr_t HooksApi::Reflection_GetAttributeOffset(const char* className, const char* attributeName) {
    return Utils::PatternFinder::FindAttributeOffset(className, attributeName);
}

uintptr_t HooksApi::Reflection_ResolveSmartPtr(uintptr_t address) {
    if (!address) return 0;
    // Standard SCS engine smart pointer dereference: [ptr + 8] contains the raw object pointer.
    try {
        return *reinterpret_cast<uintptr_t*>(address + 8);
    } catch (...) {
        return 0;
    }
}

void HooksApi::Memory_WriteFloat(uintptr_t address, float value) {
    if (address) *reinterpret_cast<float*>(address) = value;
}

void HooksApi::Memory_WriteInt32(uintptr_t address, int32_t value) {
    if (address) *reinterpret_cast<int32_t*>(address) = value;
}

void HooksApi::Memory_ReadVector3(uintptr_t address, float* outX, float* outY, float* outZ) {
    if (address && outX && outY && outZ) {
        float* p = reinterpret_cast<float*>(address);
        *outX = p[0];
        *outY = p[1];
        *outZ = p[2];
    }
}

void HooksApi::Memory_WriteVector3(uintptr_t address, float x, float y, float z) {
    if (address) {
        float* p = reinterpret_cast<float*>(address);
        p[0] = x;
        p[1] = y;
        p[2] = z;
    }
}

} // namespace Modules::API
SPF_NS_END
