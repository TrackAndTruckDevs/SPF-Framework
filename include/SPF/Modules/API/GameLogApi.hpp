#pragma once

#include "SPF/SPF_API/SPF_GameLog_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {

class GameLogApi {
 public:
  static void FillGameLogApi(SPF_GameLog_API* api);

 private:
  static SPF_GameLog_Callback_Handle G_RegisterCallback(const char* pluginName, SPF_GameLog_Callback callback, void* user_data);
};

}  // namespace Modules::API
SPF_NS_END
