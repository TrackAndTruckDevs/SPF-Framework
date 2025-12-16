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
  static void FillHooksApi(SPF_Hooks_API* api, SPF_Hooks_Register_t pRegister);

 private:
  // Trampolines for stateless hook functions
  static uintptr_t T_FindPattern(const char* signature);
  static uintptr_t T_FindPatternFrom(const char* signature, uintptr_t startAddress, size_t searchLength);
  static bool T_IsEnabled(SPF_Hook_Handle* handle);
  static bool T_IsInstalled(SPF_Hook_Handle* handle);
};
}  // namespace Modules::API
SPF_NS_END
