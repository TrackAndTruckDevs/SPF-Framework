#pragma once

#include "SPF/SPF_API/SPF_VirtInput_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class VirtualInputApi {
 public:
  static void FillVirtualInputApi(SPF_Input_API* api);

 private:
  static SPF_VirtualDevice_Handle* I_CreateDevice(const char* pluginName, const char* deviceName, const char* displayName, SPF_InputDeviceType type);
  static void I_AddButton(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName);
  static void I_AddAxis(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName);
  static bool I_Register(SPF_VirtualDevice_Handle* handle);
  static void I_PressButton(SPF_VirtualDevice_Handle* handle, const char* inputName);
  static void I_ReleaseButton(SPF_VirtualDevice_Handle* handle, const char* inputName);
  static void I_SetAxisValue(SPF_VirtualDevice_Handle* handle, const char* inputName, float value);
};
}  // namespace Modules::API
SPF_NS_END