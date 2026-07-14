#include "SPF/Modules/API/LocalizationApi.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Handles/LocalizationHandle.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Modules/HandleManager.hpp"  // Required for GetInstance()->m_handleManager
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/SPF_API/SPF_Localization_API.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

// IWYU insists on a direct provider for _s functions.
// MinGW: pull in MSVC-compat decl; MSVC gets them from <cstdio> natively.
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <sec_api/string_s.h>
#endif

SPF_NS_BEGIN
namespace Modules::API {

// Trampolines that are exposed to plugins via the C-API

SPF_Localization_Handle* LocalizationApi::Loc_GetContext(const char* pluginName) {
  auto& pm = SPF::Modules::PluginManager::GetInstance();
  if (!pluginName || !pm.GetHandleManager()) return nullptr;
  auto unique_h = std::make_unique<SPF::Handles::LocalizationHandle>(pluginName);
  return reinterpret_cast<SPF_Localization_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(unique_h)));
}

int LocalizationApi::Loc_GetString(SPF_Localization_Handle* h, const char* key, char* out_buffer, int buffer_size) {
  if (!h || !key || !out_buffer || buffer_size <= 0) return 0;

  auto* l10nHandle = reinterpret_cast<SPF::Handles::LocalizationHandle*>(h);
  std::string result = SPF::Localization::LocalizationManager::GetInstance().Get(l10nHandle->pluginName, key);

  if (result.length() < buffer_size) {
    strcpy_s(out_buffer, buffer_size, result.c_str());
    return static_cast<int>(result.length());
  } else {
    *out_buffer = '\0';                            // Clear buffer on failure
    return static_cast<int>(result.length()) + 1;  // Return required size
  }
}

bool LocalizationApi::Loc_SetLanguage(SPF_Localization_Handle* h, const char* langCode) {
  if (!h || !langCode) return false;
  auto* l10nHandle = reinterpret_cast<SPF::Handles::LocalizationHandle*>(h);
  return SPF::Localization::LocalizationManager::GetInstance().SetComponentLanguage(l10nHandle->pluginName, langCode);
}

const char** LocalizationApi::Loc_GetAvailableLanguages(SPF_Localization_Handle* h, int* count) {
  auto& pm = SPF::Modules::PluginManager::GetInstance();
  if (!h) {
    if (count) *count = 0;
    return nullptr;
  }
  auto* l10nHandle = reinterpret_cast<SPF::Handles::LocalizationHandle*>(h);
  auto& l10n = SPF::Localization::LocalizationManager::GetInstance();

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

const char* LocalizationApi::Loc_GetFrameworkLanguage() {
  static std::string s_framework_lang_cache;
  s_framework_lang_cache = SPF::Localization::LocalizationManager::GetInstance().GetComponentLanguage("framework");
  return s_framework_lang_cache.c_str();
}

bool LocalizationApi::Loc_HasLanguage(SPF_Localization_Handle* h, const char* langCode) {
  if (!h || !langCode) return false;
  auto* l10nHandle = reinterpret_cast<SPF::Handles::LocalizationHandle*>(h);
  return SPF::Localization::LocalizationManager::GetInstance().LanguageFileExists(l10nHandle->pluginName, langCode);
}

void LocalizationApi::FillLocalizationApi(SPF_Localization_API* api) {
  if (!api) return;
  api->Loc_GetContext = &LocalizationApi::Loc_GetContext;
  api->Loc_GetString = &LocalizationApi::Loc_GetString;
  api->Loc_SetLanguage = &LocalizationApi::Loc_SetLanguage;
  api->Loc_GetAvailableLanguages = &LocalizationApi::Loc_GetAvailableLanguages;
  api->Loc_GetFrameworkLanguage = &LocalizationApi::Loc_GetFrameworkLanguage;
  api->Loc_HasLanguage = &LocalizationApi::Loc_HasLanguage;
}

}  // namespace Modules::API
SPF_NS_END
