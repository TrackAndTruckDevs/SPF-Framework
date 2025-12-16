#include "SPF/Modules/API/LocalizationApi.hpp"

#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Handles/LocalizationHandle.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Modules/HandleManager.hpp" // Required for GetInstance()->m_handleManager

#include <vector>
#include <string>

SPF_NS_BEGIN
namespace Modules::API {

// Trampolines that are exposed to plugins via the C-API

SPF_Localization_Handle* LocalizationApi::L_GetContext(const char* pluginName) {
    auto& pm = SPF::Modules::PluginManager::GetInstance();
    if (!pluginName || !pm.GetHandleManager()) return nullptr;
    auto handle = std::make_unique<SPF::Handles::LocalizationHandle>(pluginName);
    return reinterpret_cast<SPF_Localization_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(handle)));
}

int LocalizationApi::L_GetString(SPF_Localization_Handle* handle, const char* key, char* out_buffer, int buffer_size) {
    if (!handle || !key || !out_buffer || buffer_size <= 0) return 0;

    auto* l10nHandle = reinterpret_cast<SPF::Handles::LocalizationHandle*>(handle);
    std::string result = SPF::Localization::LocalizationManager::GetInstance().Get(l10nHandle->pluginName, key);

    if (result.length() < buffer_size) {
        strcpy_s(out_buffer, buffer_size, result.c_str());
        return result.length();
    } else {
        *out_buffer = '\0';          // Clear buffer on failure
        return result.length() + 1;  // Return required size
    }
}

bool LocalizationApi::L_SetLanguage(SPF_Localization_Handle* handle, const char* langCode) {
    if (!handle || !langCode) return false;
    auto* l10nHandle = reinterpret_cast<SPF::Handles::LocalizationHandle*>(handle);
    return SPF::Localization::LocalizationManager::GetInstance().SetComponentLanguage(l10nHandle->pluginName, langCode);
}

const char** LocalizationApi::L_GetAvailableLanguages(SPF_Localization_Handle* handle, int* count) {
    auto& pm = SPF::Modules::PluginManager::GetInstance();
    if (!handle) {
        if (count) *count = 0;
        return nullptr;
    }
    auto* l10nHandle = reinterpret_cast<SPF::Handles::LocalizationHandle*>(handle);
    auto& l10n = SPF::Localization::LocalizationManager::GetInstance();
    
    // The cache is now a member of PluginManager, we need to access it through the singleton
    auto& languages_cache = pm.GetL10nAvailableLanguagesCache();
    auto& c_str_cache = pm.GetL10nAvailableLanguagesCStrCache();

    languages_cache = l10n.GetAvailableLanguagesFor(l10nHandle->pluginName);
    c_str_cache.clear();
    c_str_cache.reserve(languages_cache.size());
    for (const auto& lang : languages_cache) {
        c_str_cache.push_back(lang.c_str());
    }
    if (count) {
        *count = static_cast<int>(c_str_cache.size());
    }
    return c_str_cache.data();
}

void LocalizationApi::FillLocalizationApi(SPF_Localization_API* api) {
    api->GetContext = &LocalizationApi::L_GetContext;
    api->GetString = &LocalizationApi::L_GetString;
    api->SetLanguage = &LocalizationApi::L_SetLanguage;
    api->GetAvailableLanguages = &LocalizationApi::L_GetAvailableLanguages;
}

} // namespace Modules::API
SPF_NS_END
