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
  static int GCon_IsCheatsEnabled();
  static void GCon_EnableCheats();
  static int GCon_ExecuteCommandEx(const char* command, char* out_error, int error_size);
};
}  // namespace SPF::Modules::API
