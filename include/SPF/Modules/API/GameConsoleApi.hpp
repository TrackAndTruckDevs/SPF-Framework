#pragma once

#include "SPF/SPF_API/SPF_GameConsole_API.h"

namespace SPF::Modules::API {
class GameConsoleApi {
 public:
  static void FillGameConsoleApi(SPF_GameConsole_API* api);

 private:
  static void GCon_ExecuteCommand(const char* command);
  static int GCon_GetCommandCount();
  static int GCon_GetCommandNameByIndex(int index, char* out_buffer, int buffer_size);
};
}  // namespace SPF::Modules::API
