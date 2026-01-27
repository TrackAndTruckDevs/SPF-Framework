#pragma once
#include "SPF/Namespace.hpp"
#include "SPF/SPF_API/SPF_Localization_API.h"

SPF_NS_BEGIN
namespace Modules::API {

class LocalizationApi {
 public:
  static void FillLocalizationApi(SPF_Localization_API* api);

 private:
  static SPF_Localization_Handle* Loc_GetContext(const char* pluginName);
  static int Loc_GetString(SPF_Localization_Handle* h, const char* key, char* out_buffer, int buffer_size);
  static bool Loc_SetLanguage(SPF_Localization_Handle* h, const char* langCode);
  static const char** Loc_GetAvailableLanguages(SPF_Localization_Handle* h, int* count);
  static const char* Loc_GetFrameworkLanguage();
  static bool Loc_HasLanguage(SPF_Localization_Handle* h, const char* langCode);
};

}  // namespace Modules::API
SPF_NS_END
