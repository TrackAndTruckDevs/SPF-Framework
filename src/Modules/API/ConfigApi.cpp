#include "SPF/Modules/API/ConfigApi.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Handles/ConfigHandle.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/SPF_API/SPF_Config_API.h"

#include "nlohmann/json_fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <utility>

// IWYU insists on a direct provider for _s functions.
// MinGW: pull in MSVC-compat decl; MSVC gets them from <cstdio> natively.
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <sec_api/string_s.h>
#endif

SPF_NS_BEGIN
namespace Modules::API {

using namespace Logging;

SPF_Config_Handle* ConfigApi::Cfg_GetContext(const char* pluginName) {
  auto& pm = PluginManager::GetInstance();
  if (!pluginName || !pm.GetHandleManager()) return nullptr;
  auto h_unique = std::make_unique<Handles::ConfigHandle>(pluginName);
  return reinterpret_cast<SPF_Config_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(h_unique)));
}

int ConfigApi::Cfg_GetString(SPF_Config_Handle* h, const char* key, const char* defaultValue, char* out_buffer, int buffer_size) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key || !out_buffer || buffer_size <= 0) return 0;

  auto& pm = PluginManager::GetInstance();
  std::string value_str;

  if (!pm.GetConfigService()) {
    value_str = defaultValue;
  } else {
    nlohmann::ordered_json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
    const nlohmann::ordered_json* valueNode = &result;
    if (result.is_object() && result.contains("_value")) {
      valueNode = &result["_value"];
    }
    value_str = valueNode->get<std::string>();
  }

  if (value_str.length() < buffer_size) {
    strcpy_s(out_buffer, buffer_size, value_str.c_str());
    return value_str.length();
  } else {
    *out_buffer = '\0';
    return value_str.length() + 1;  // Return required size
  }
}

void ConfigApi::Cfg_SetString(SPF_Config_Handle* h, const char* key, const char* value) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key || !value) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
  }
}

int64_t ConfigApi::Cfg_GetInt(SPF_Config_Handle* h, const char* key, int64_t defaultValue) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return defaultValue;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService()) return defaultValue;

  nlohmann::ordered_json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
  const nlohmann::ordered_json* valueNode = &result;
  if (result.is_object() && result.contains("_value")) {
    valueNode = &result["_value"];
  }
  return valueNode->get<int64_t>();
}

void ConfigApi::Cfg_SetInt(SPF_Config_Handle* h, const char* key, int64_t value) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
  }
}

int32_t ConfigApi::Cfg_GetInt32(SPF_Config_Handle* h, const char* key, int32_t defaultValue) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return defaultValue;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService()) return defaultValue;

  nlohmann::ordered_json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
  const nlohmann::ordered_json* valueNode = &result;
  if (result.is_object() && result.contains("_value")) {
    valueNode = &result["_value"];
  }

  // Get the value as the canonical 64-bit integer and then cast it.
  // This is safer than get<int32_t>() which might throw on overflow.
  int64_t value64 = valueNode->get<int64_t>();
  return static_cast<int32_t>(value64);
}

void ConfigApi::Cfg_SetInt32(SPF_Config_Handle* h, const char* key, int32_t value) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
  }
}

double ConfigApi::Cfg_GetFloat(SPF_Config_Handle* h, const char* key, double defaultValue) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return defaultValue;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService()) return defaultValue;

  nlohmann::ordered_json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
  const nlohmann::ordered_json* valueNode = &result;
  if (result.is_object() && result.contains("_value")) {
    valueNode = &result["_value"];
  }
  return valueNode->get<double>();
}

void ConfigApi::Cfg_SetFloat(SPF_Config_Handle* h, const char* key, double value) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
  }
}

bool ConfigApi::Cfg_GetBool(SPF_Config_Handle* h, const char* key, bool defaultValue) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return defaultValue;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService()) return defaultValue;

  nlohmann::ordered_json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
  const nlohmann::ordered_json* valueNode = &result;
  if (result.is_object() && result.contains("_value")) {
    valueNode = &result["_value"];
  }
  return valueNode->get<bool>();
}

SPF_JsonValue_Handle* ConfigApi::Cfg_GetJsonValueHandle(SPF_Config_Handle* h, const char* key) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return nullptr;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService()) return nullptr;

  const nlohmann::ordered_json* valueNode = pm.GetConfigService()->GetValuePtr(cfgHandle->pluginName, key);

  // Fix: Unwrap the framework's metadata wrapper if present
  if (valueNode && valueNode->is_object() && valueNode->contains("_value")) {
    valueNode = &((*valueNode)["_value"]);
  }

  return reinterpret_cast<SPF_JsonValue_Handle*>(const_cast<nlohmann::ordered_json*>(valueNode));
}

void ConfigApi::Cfg_SetBool(SPF_Config_Handle* h, const char* key, bool value) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
  }
}

bool ConfigApi::Cfg_HasKey(SPF_Config_Handle* h, const char* key) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return false;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService()) return false;
  return pm.GetConfigService()->HasKey(cfgHandle->pluginName, key);
}

void ConfigApi::Cfg_Save(SPF_Config_Handle* h) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->SaveComponentConfig(cfgHandle->pluginName);
  }
}

void ConfigApi::Cfg_RemoveKey(SPF_Config_Handle* h, const char* key) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->RemoveKey(cfgHandle->pluginName, key);
  }
}

void ConfigApi::Cfg_SetJsonString(SPF_Config_Handle* h, const char* key, const char* json_literal) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key || !json_literal) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    try {
      nlohmann::ordered_json j = nlohmann::ordered_json::parse(json_literal);
      pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, j);
    } catch (const std::exception& e) {
      auto logger = LoggerFactory::GetInstance().GetLogger("ConfigApi");
      if (logger) logger->Error("Cfg_SetJsonString: Failed to parse JSON literal for key '{}'. Error: {}", key, e.what());
    } catch (...) {
      auto logger = LoggerFactory::GetInstance().GetLogger("ConfigApi");
      if (logger) logger->Error("Cfg_SetJsonString: Unknown error while parsing JSON literal for key '{}'.", key);
    }
  }
}

void ConfigApi::Cfg_Reload(SPF_Config_Handle* h) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->ReloadComponentConfig(cfgHandle->pluginName);
  }
}

SPF_Config_Handle* ConfigApi::Cfg_CreateCustomContext(const char* filePath) {
  if (!filePath) return nullptr;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService() || !pm.GetHandleManager()) return nullptr;

  std::string contextId = pm.GetConfigService()->CreateCustomContext(filePath);
  if (contextId.empty()) return nullptr;

  auto h_unique = std::make_unique<Handles::ConfigHandle>(contextId);
  return reinterpret_cast<SPF_Config_Handle*>(pm.GetHandleManager()->RegisterHandle(contextId.c_str(), std::move(h_unique)));
}

void ConfigApi::Cfg_SetAutoSave(SPF_Config_Handle* h, bool enabled) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle) return;
  auto& pm = PluginManager::GetInstance();
  if (pm.GetConfigService()) {
    pm.GetConfigService()->SetAutoSave(cfgHandle->pluginName, enabled);
  }
}

int ConfigApi::Cfg_GetJsonString(SPF_Config_Handle* h, const char* key, char* out_buffer, int buffer_size) {
  auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(h);
  if (!cfgHandle || !key || !out_buffer || buffer_size <= 0) return 0;
  auto& pm = PluginManager::GetInstance();
  if (!pm.GetConfigService()) {
    *out_buffer = '\0';
    return 0;
  }

  try {
    nlohmann::ordered_json j = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, nlohmann::ordered_json());
    if (j.is_null()) {
      *out_buffer = '\0';
      return 0;
    }

    const nlohmann::ordered_json* node = &j;
    // Fix: Unwrap if present in the copy
    if (j.is_object() && j.contains("_value")) {
      node = &j["_value"];
    }

    std::string s = node->dump();
    if (s.length() < static_cast<size_t>(buffer_size)) {
      strcpy_s(out_buffer, buffer_size, s.c_str());
      return static_cast<int>(s.length());
    } else {
      *out_buffer = '\0';
      return static_cast<int>(s.length()) + 1;
    }
  } catch (const std::exception& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("ConfigApi");
    if (logger) logger->Error("Cfg_GetJsonString: Failed to dump JSON for key '{}'. Error: {}", key, e.what());
    *out_buffer = '\0';
    return 0;
  }
}

void ConfigApi::FillConfigApi(SPF_Config_API* api) {
  if (!api) return;

  api->Cfg_GetContext = &ConfigApi::Cfg_GetContext;
  api->Cfg_GetString = &ConfigApi::Cfg_GetString;
  api->Cfg_SetString = &ConfigApi::Cfg_SetString;
  api->Cfg_GetInt = &ConfigApi::Cfg_GetInt;
  api->Cfg_SetInt = &ConfigApi::Cfg_SetInt;
  api->Cfg_GetInt32 = &ConfigApi::Cfg_GetInt32;
  api->Cfg_SetInt32 = &ConfigApi::Cfg_SetInt32;
  api->Cfg_GetFloat = &ConfigApi::Cfg_GetFloat;
  api->Cfg_SetFloat = &ConfigApi::Cfg_SetFloat;
  api->Cfg_GetBool = &ConfigApi::Cfg_GetBool;
  api->Cfg_GetJsonValueHandle = &ConfigApi::Cfg_GetJsonValueHandle;
  api->Cfg_SetBool = &ConfigApi::Cfg_SetBool;

  // Extensions
  api->Cfg_HasKey = &ConfigApi::Cfg_HasKey;
  api->Cfg_Save = &ConfigApi::Cfg_Save;
  api->Cfg_RemoveKey = &ConfigApi::Cfg_RemoveKey;
  api->Cfg_SetJsonString = &ConfigApi::Cfg_SetJsonString;
  api->Cfg_Reload = &ConfigApi::Cfg_Reload;
  api->Cfg_GetJsonString = &ConfigApi::Cfg_GetJsonString;
  api->Cfg_CreateCustomContext = &ConfigApi::Cfg_CreateCustomContext;
  api->Cfg_SetAutoSave = &ConfigApi::Cfg_SetAutoSave;
}

}  // namespace Modules::API
SPF_NS_END
