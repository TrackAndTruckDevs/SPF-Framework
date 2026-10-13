#include "SPF/Modules/API/GameConsoleApi.hpp"

#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/SPF_API/SPF_GameConsole_API.h"

#include <cstdio>
#include <string>

namespace SPF::Modules::API {

void GameConsoleApi::GCon_ExecuteCommand(const char* command) {
  if (command) {
    GameConsole::GetInstance().Execute(command);
  }
}

int GameConsoleApi::GCon_GetCommandCount() {
  const auto& list = GameConsole::GetInstance().GetCommandList();
  return static_cast<int>(list.size());
}

int GameConsoleApi::GCon_GetCommandNameByIndex(int index, char* out_buffer, int buffer_size) {
  if (!out_buffer || buffer_size <= 0) return 0;
  out_buffer[0] = '\0';

  const auto& list = GameConsole::GetInstance().GetCommandList();
  if (index < 0 || static_cast<size_t>(index) >= list.size()) return 0;

  const std::string& name = list[static_cast<size_t>(index)];
  std::snprintf(out_buffer, static_cast<size_t>(buffer_size), "%s", name.c_str());
  return 1;
}

int GameConsoleApi::GCon_IsCheatsEnabled() {
  return GameConsole::GetInstance().IsCheatsEnabled() ? 1 : 0;
}

void GameConsoleApi::GCon_EnableCheats() {
  GameConsole::GetInstance().EnableCheats();
}

int GameConsoleApi::GCon_ExecuteCommandEx(const char* command, char* out_error, int error_size) {
  if (out_error && error_size > 0) {
    out_error[0] = '\0';
  }
  if (!command) {
    if (out_error && error_size > 0) {
      std::snprintf(out_error, static_cast<size_t>(error_size), "%s", "command is NULL");
    }
    return 0;
  }

  std::string error;
  const bool ok = GameConsole::GetInstance().Execute(command, &error);
  if (out_error && error_size > 0) {
    std::snprintf(out_error, static_cast<size_t>(error_size), "%s", error.c_str());
  }
  return ok ? 1 : 0;
}

void GameConsoleApi::FillGameConsoleApi(SPF_GameConsole_API* api) {
  if (!api) return;

  api->GCon_ExecuteCommand = &GameConsoleApi::GCon_ExecuteCommand;
  api->GCon_GetCommandCount = &GameConsoleApi::GCon_GetCommandCount;
  api->GCon_GetCommandNameByIndex = &GameConsoleApi::GCon_GetCommandNameByIndex;
  api->GCon_IsCheatsEnabled = &GameConsoleApi::GCon_IsCheatsEnabled;
  api->GCon_EnableCheats = &GameConsoleApi::GCon_EnableCheats;
  api->GCon_ExecuteCommandEx = &GameConsoleApi::GCon_ExecuteCommandEx;
}

}  // namespace SPF::Modules::API
