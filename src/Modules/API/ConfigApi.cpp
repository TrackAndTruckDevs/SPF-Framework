#include "SPF/Modules/API/ConfigApi.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Handles/ConfigHandle.hpp"
#include "SPF/Config/IConfigService.hpp"
#include "SPF/Modules/HandleManager.hpp"

#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules::API {

SPF_Config_Handle* ConfigApi::Cfg_GetContext(const char* pluginName) {
    auto& pm = PluginManager::GetInstance();
    if (!pluginName || !pm.GetHandleManager()) return nullptr;
    auto handle = std::make_unique<Handles::ConfigHandle>(pluginName);
    return reinterpret_cast<SPF_Config_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(handle)));
}

int ConfigApi::Cfg_GetString(SPF_Config_Handle* handle, const char* key, const char* defaultValue, char* out_buffer, int buffer_size) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key || !out_buffer || buffer_size <= 0) return 0;

    auto& pm = PluginManager::GetInstance();
    std::string value_str;

    if (!pm.GetConfigService()) {
        value_str = defaultValue;
    } else {
        nlohmann::json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
        const nlohmann::json* valueNode = &result;
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

void ConfigApi::Cfg_SetString(SPF_Config_Handle* handle, const char* key, const char* value) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key || !value) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
    }
}

int64_t ConfigApi::Cfg_GetInt(SPF_Config_Handle* handle, const char* key, int64_t defaultValue) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return defaultValue;
    auto& pm = PluginManager::GetInstance();
    if (!pm.GetConfigService()) return defaultValue;
    
    nlohmann::json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
    const nlohmann::json* valueNode = &result;
    if (result.is_object() && result.contains("_value")) {
        valueNode = &result["_value"];
    }
    return valueNode->get<int64_t>();
}

void ConfigApi::Cfg_SetInt(SPF_Config_Handle* handle, const char* key, int64_t value) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
    }
}

int32_t ConfigApi::Cfg_GetInt32(SPF_Config_Handle* handle, const char* key, int32_t defaultValue) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return defaultValue;
    auto& pm = PluginManager::GetInstance();
    if (!pm.GetConfigService()) return defaultValue;

    nlohmann::json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
    const nlohmann::json* valueNode = &result;
    if (result.is_object() && result.contains("_value")) {
        valueNode = &result["_value"];
    }
    
    // Get the value as the canonical 64-bit integer and then cast it.
    // This is safer than get<int32_t>() which might throw on overflow.
    int64_t value64 = valueNode->get<int64_t>();
    return static_cast<int32_t>(value64);
}

void ConfigApi::Cfg_SetInt32(SPF_Config_Handle* handle, const char* key, int32_t value) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
    }
}

double ConfigApi::Cfg_GetFloat(SPF_Config_Handle* handle, const char* key, double defaultValue) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return defaultValue;
    auto& pm = PluginManager::GetInstance();
    if (!pm.GetConfigService()) return defaultValue;

    nlohmann::json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
    const nlohmann::json* valueNode = &result;
    if (result.is_object() && result.contains("_value")) {
        valueNode = &result["_value"];
    }
    return valueNode->get<double>();
}

void ConfigApi::Cfg_SetFloat(SPF_Config_Handle* handle, const char* key, double value) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
    }
}

bool ConfigApi::Cfg_GetBool(SPF_Config_Handle* handle, const char* key, bool defaultValue) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return defaultValue;
    auto& pm = PluginManager::GetInstance();
    if (!pm.GetConfigService()) return defaultValue;

    nlohmann::json result = pm.GetConfigService()->GetValue(cfgHandle->pluginName, key, defaultValue);
    const nlohmann::json* valueNode = &result;
    if (result.is_object() && result.contains("_value")) {
        valueNode = &result["_value"];
    }
    return valueNode->get<bool>();
}

void ConfigApi::Cfg_SetBool(SPF_Config_Handle* handle, const char* key, bool value) {
    auto* cfgHandle = reinterpret_cast<Handles::ConfigHandle*>(handle);
    if (!cfgHandle || !key) return;
    auto& pm = PluginManager::GetInstance();
    if (pm.GetConfigService()) {
        pm.GetConfigService()->SetValue(cfgHandle->pluginName, key, value);
    }
}

void ConfigApi::FillConfigApi(SPF_Config_API* api) {
    if (!api) return;

    api->GetContext = &ConfigApi::Cfg_GetContext;
    api->GetString = &ConfigApi::Cfg_GetString;
    api->SetString = &ConfigApi::Cfg_SetString;
    api->GetInt = &ConfigApi::Cfg_GetInt;
    api->SetInt = &ConfigApi::Cfg_SetInt;
    api->GetInt32 = &ConfigApi::Cfg_GetInt32;
    api->SetInt32 = &ConfigApi::Cfg_SetInt32;
    api->GetFloat = &ConfigApi::Cfg_GetFloat;
    api->SetFloat = &ConfigApi::Cfg_SetFloat;
    api->GetBool = &ConfigApi::Cfg_GetBool;
    api->SetBool = &ConfigApi::Cfg_SetBool;
}

} // namespace Modules::API
SPF_NS_END
