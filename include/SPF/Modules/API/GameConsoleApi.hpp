#pragma once

#include "SPF/SPF_API/SPF_GameConsole_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class GameConsoleApi {
 public:
  static void FillGameConsoleApi(SPF_GameConsole_API* api);

 private:
  static void T_ExecuteCommand(const char* command);
};
}  // namespace Modules::API
SPF_NS_END
