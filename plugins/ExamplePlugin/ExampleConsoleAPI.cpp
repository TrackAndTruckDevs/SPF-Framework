/**
 * @file ExampleConsoleAPI.cpp
 * @brief Implementation of the SPF GameConsole API example.
 *
 * @details PATTERN SUMMARY:
 *   1. User (or your code) provides a command string identical to what they'd type in-game.
 *   2. GCon_ExecuteCommand hands it to the game console subsystem.
 *   3. Log the command for auditability — console side-effects are not always visible.
 *
 * PRECONDITION: BuildManifest must list Policy_AddRequiredHook(h, "GameConsole")
 * so the framework keeps the console hook enabled while this plugin is active.
 */
#include "ExampleConsoleAPI.hpp"

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstddef>

namespace ExamplePlugin {

void RenderConsoleSection(SPF_UI_API* ui) {
  ui->UI_Text("Enter a command to execute in the in-game console:");

  // Fixed char buffer required by UI_InputText (C API). g_ctx.consoleCommand keeps
  // the draft command across frames while the user types.
  ui->UI_InputText("##ConsoleCommand", g_ctx.consoleCommand, sizeof(g_ctx.consoleCommand), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_SameLine(0, 0);

  // UI_ButtonEx: framework-standard hover/active colors + optional tooltip.
  if (ui->UI_ButtonEx("Execute", 0, 0, "Click to run this command in SCS console", NULL)) {
    if (g_ctx.coreAPI && g_ctx.coreAPI->console && g_ctx.consoleCommand[0] != '\0') {
      // Hand the string to the game console — same path as player-typed input.
      g_ctx.coreAPI->console->GCon_ExecuteCommand(g_ctx.consoleCommand);

      char log_buffer[512];
      g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Executed console command: '%s'", g_ctx.consoleCommand);
      g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
    }
  }
  ui->UI_Separator();
}

}  // namespace ExamplePlugin
