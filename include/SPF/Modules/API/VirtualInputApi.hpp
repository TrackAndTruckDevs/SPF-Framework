#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_VirtInput_API.h"

SPF_NS_BEGIN
namespace Modules::API {
class VirtualInputApi {
 public:
  static void FillVirtualInputApi(SPF_VirtInput_API* api);

 private:
  static SPF_VirtualDevice_Handle* Virt_CreateDevice(const char* pluginName, const char* deviceName, const char* displayName, SPF_InputDeviceType type);
  static void Virt_AddButton(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName);
  static void Virt_AddAxis(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName);
  static bool Virt_Register(SPF_VirtualDevice_Handle* h);
  static void Virt_PressButton(SPF_VirtualDevice_Handle* h, const char* inputName);
  static void Virt_ReleaseButton(SPF_VirtualDevice_Handle* h, const char* inputName);
  static void Virt_SetAxisValue(SPF_VirtualDevice_Handle* h, const char* inputName, float value);
};
}  // namespace Modules::API
SPF_NS_END
