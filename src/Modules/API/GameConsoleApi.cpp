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

void GameConsoleApi::FillGameConsoleApi(SPF_GameConsole_API* api) {
  if (!api) return;

  api->GCon_ExecuteCommand = &GameConsoleApi::GCon_ExecuteCommand;
  api->GCon_GetCommandCount = &GameConsoleApi::GCon_GetCommandCount;
  api->GCon_GetCommandNameByIndex = &GameConsoleApi::GCon_GetCommandNameByIndex;
}

}  // namespace SPF::Modules::API
