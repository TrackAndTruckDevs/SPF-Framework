#pragma once

#include "SPF/SPF_API/SPF_KeyBinds_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class KeyBindsApi {
 public:
  static void FillKeyBindsApi(SPF_KeyBinds_API* api);

 private:
  static SPF_KeyBinds_Handle* Kbind_GetContext(const char* pluginName);
  static void Kbind_Register(SPF_KeyBinds_Handle* h, const char* actionName, void (*callback)(void));
  static void Kbind_UnregisterAll(SPF_KeyBinds_Handle* h);
  static void Kbind_SetBlockState(SPF_KeyBinds_Handle* h, const char* actionName, bool block);
  static float Kbind_GetActionValue(SPF_KeyBinds_Handle* h, const char* actionName);

  static int Kbind_GetBindingCount(SPF_KeyBinds_Handle* h, const char* actionName);
  static SPF_BindingType Kbind_GetBindingType(SPF_KeyBinds_Handle* h, const char* actionName, int index);
  static SPF_ActivationBehavior Kbind_GetBindingBehavior(SPF_KeyBinds_Handle* h, const char* actionName, int index);
  static SPF_PressType Kbind_GetBindingPressType(SPF_KeyBinds_Handle* h, const char* actionName, int index);
  static SPF_InputMode Kbind_GetBindingMode(SPF_KeyBinds_Handle* h, const char* actionName, int index);
  static SPF_AxisSide Kbind_GetBindingSide(SPF_KeyBinds_Handle* h, const char* actionName, int index);
  static SPF_AccumulatorMode Kbind_GetBindingAccumulatorMode(SPF_KeyBinds_Handle* h, const char* actionName, int index);
  static int Kbind_GetBindingName(SPF_KeyBinds_Handle* h, const char* actionName, int index, char* out_buffer, int buffer_size);
};
}  // namespace Modules::API
SPF_NS_END
