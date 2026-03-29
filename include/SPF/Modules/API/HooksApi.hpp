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
};
}  // namespace Modules::API
SPF_NS_END
