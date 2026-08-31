#pragma once

#include "SPF/SPF_API/SPF_GameConsole_API.h"

namespace SPF::Modules::API {
class GameConsoleApi {
 public:
  static void FillGameConsoleApi(SPF_GameConsole_API* api);

 private:
  static void GCon_ExecuteCommand(const char* command);
};
}  // namespace SPF::Modules::API
