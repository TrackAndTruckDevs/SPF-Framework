#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_GameLog_API.h"

SPF_NS_BEGIN
namespace Modules::API {

class GameLogApi {
 public:
  static void FillGameLogApi(SPF_GameLog_API* api);

 private:
  static SPF_GameLog_Handle* GLog_GetContext(const char* pluginName);
  static SPF_GameLog_Callback_Handle* GLog_RegisterCallback(SPF_GameLog_Handle* h, SPF_GameLog_Callback_t callback, void* userData);
};

}  // namespace Modules::API
SPF_NS_END
