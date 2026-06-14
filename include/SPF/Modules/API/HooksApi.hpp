#pragma once

#include "SPF/SPF_API/SPF_Hooks_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class HooksApi {
 public:
  /**
   * @brief Fills the provided API structure with the correct function pointers.
   *
   * @param api The API structure to fill.
   * @param pRegister A pointer to the registration function (implemented in PluginManager).
   */
  static void FillHooksApi(SPF_Hooks_API* api, SPF_Hook_Register_t pRegister);

 private:
  static SPF_Hook_Handle* Hook_Register(const char* pluginName, const char* hookName, const char* displayName, void* pDetour, void** ppOriginal, const char* signature, bool isEnabled);
  static uintptr_t Hook_FindPattern(const char* signature);
  static uintptr_t Hook_FindPatternFrom(const char* signature, uintptr_t startAddress, size_t searchLength);
  static bool Hook_IsEnabled(SPF_Hook_Handle* h);
  static bool Hook_IsInstalled(SPF_Hook_Handle* h);

  // Memory access
  static int32_t Memory_ReadInt32(uintptr_t address);
  static int8_t Memory_ReadInt8(uintptr_t address);
  static int64_t Memory_ReadInt64(uintptr_t address);
  static float Memory_ReadFloat(uintptr_t address);
  static uintptr_t Memory_GetRipAddress(uintptr_t instructionAddr, int offsetPos, int instructionSize);

  static uintptr_t Hook_FindBackward(uintptr_t startAddress, size_t searchRange, const char* signature);
  static uintptr_t Hook_FindString(const char* str);
  static uintptr_t Hook_FindFunctionByString(const char* str, bool findStart, const char* contextSig, size_t contextRange);
  static uintptr_t Hook_GetFunctionStart(uintptr_t address);
  static uintptr_t Hook_FindChain(const char** signatures, size_t count, size_t maxGap, uintptr_t startAddress, size_t searchRange);
  static uintptr_t Hook_FindVTable(const char* signature, int offsetPos, int instructionSize);
  static uintptr_t Hook_GetVTableFunction(uintptr_t vtableAddr, int index);
  static uintptr_t Hook_FindFunctionByConstant(uint32_t constant, bool findStart);

  // --- Reflection API (v1.2) ---
  static uintptr_t Reflection_GetAttributeOffset(const char* className, const char* attributeName);
  static uintptr_t Reflection_ResolveSmartPtr(uintptr_t address);

  // --- Advanced Memory API (v1.2) ---
  static void Memory_WriteFloat(uintptr_t address, float value);
  static void Memory_WriteInt32(uintptr_t address, int32_t value);
  static void Memory_ReadVector3(uintptr_t address, float* outX, float* outY, float* outZ);
  static void Memory_WriteVector3(uintptr_t address, float x, float y, float z);
  };
  }  // namespace Modules::API
SPF_NS_END
