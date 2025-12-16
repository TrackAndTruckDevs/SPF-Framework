#pragma once
#include "SPF/Namespace.hpp"
#include "SPF/SPF_API/SPF_Localization_API.h"

SPF_NS_BEGIN
namespace Modules::API {

class LocalizationApi {
 public:
  static void FillLocalizationApi(SPF_Localization_API* api);

 private:
  static SPF_Localization_Handle* L_GetContext(const char* pluginName);
  static int L_GetString(SPF_Localization_Handle* handle, const char* key, char* out_buffer, int buffer_size);
  static bool L_SetLanguage(SPF_Localization_Handle* handle, const char* langCode);
  static const char** L_GetAvailableLanguages(SPF_Localization_Handle* handle, int* count);
};

}  // namespace Modules::API
SPF_NS_END
