#include "SPF/Modules/API/GameConsoleApi.hpp"

#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/SPF_API/SPF_GameConsole_API.h"

namespace SPF::Modules::API {

void GameConsoleApi::GCon_ExecuteCommand(const char* command) {
  if (command) {
    GameConsole::GetInstance().Execute(command);
  }
}

void GameConsoleApi::FillGameConsoleApi(SPF_GameConsole_API* api) {
  if (!api) return;

  api->GCon_ExecuteCommand = &GameConsoleApi::GCon_ExecuteCommand;
}

}  // namespace SPF::Modules::API
