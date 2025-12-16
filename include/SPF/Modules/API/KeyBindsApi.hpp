#pragma once

#include "SPF/SPF_API/SPF_KeyBinds_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class KeyBindsApi {
 public:
  static void FillKeyBindsApi(SPF_KeyBinds_API* api);

 private:
  static SPF_KeyBinds_Handle* Kbd_GetContext(const char* pluginName);
  static void Kbd_Register(SPF_KeyBinds_Handle* handle, const char* actionName, void (*callback)(void));
  static void Kbd_UnregisterAll(SPF_KeyBinds_Handle* handle);
};
}  // namespace Modules::API
SPF_NS_END
