#pragma once

#include "SPF/SPF_API/SPF_Config_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class ConfigApi {
 public:
  static void FillConfigApi(SPF_Config_API* api);

 private:
  static SPF_Config_Handle* Cfg_GetContext(const char* pluginName);
  static int Cfg_GetString(SPF_Config_Handle* h, const char* key, const char* defaultValue, char* out_buffer, int buffer_size);
  static void Cfg_SetString(SPF_Config_Handle* h, const char* key, const char* value);
  static int64_t Cfg_GetInt(SPF_Config_Handle* h, const char* key, int64_t defaultValue);
  static void Cfg_SetInt(SPF_Config_Handle* h, const char* key, int64_t value);
  static int32_t Cfg_GetInt32(SPF_Config_Handle* h, const char* key, int32_t defaultValue);
  static void Cfg_SetInt32(SPF_Config_Handle* h, const char* key, int32_t value);
  static double Cfg_GetFloat(SPF_Config_Handle* h, const char* key, double defaultValue);
  static void Cfg_SetFloat(SPF_Config_Handle* h, const char* key, double value);
  static bool Cfg_GetBool(SPF_Config_Handle* h, const char* key, bool defaultValue);
  static void Cfg_SetBool(SPF_Config_Handle* h, const char* key, bool value);
  static SPF_JsonValue_Handle* Cfg_GetJsonValueHandle(SPF_Config_Handle* h, const char* key);

  static bool Cfg_HasKey(SPF_Config_Handle* h, const char* key);
  static void Cfg_Save(SPF_Config_Handle* h);
  static void Cfg_RemoveKey(SPF_Config_Handle* h, const char* key);
  static void Cfg_SetJsonString(SPF_Config_Handle* h, const char* key, const char* json_literal);
  static void Cfg_Reload(SPF_Config_Handle* h);
  static int Cfg_GetJsonString(SPF_Config_Handle* h, const char* key, char* out_buffer, int buffer_size);
};
}  // namespace Modules::API
SPF_NS_END
