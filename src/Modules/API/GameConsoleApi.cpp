#include "SPF/Modules/API/GameConsoleApi.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/SPF_API/SPF_GameConsole_API.h"

SPF_NS_BEGIN
namespace Modules::API {

void GameConsoleApi::GCon_ExecuteCommand(const char* command) {
  if (command) {
    GameConsole::GetInstance().Execute(command);
  }
}

void GameConsoleApi::FillGameConsoleApi(SPF_GameConsole_API* api) {
  if (!api) return;

  api->GCon_ExecuteCommand = &GameConsoleApi::GCon_ExecuteCommand;
}

}  // namespace Modules::API
SPF_NS_END
