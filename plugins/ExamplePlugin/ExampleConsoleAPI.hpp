/**
 * @file ExampleConsoleAPI.hpp
 * @brief Complete example of the SPF GameConsole API — sending commands to the in-game console.
 *
 * @details The console API lets a plugin execute developer commands programmatically
 * (as if the player typed them). Useful for debug tooling, forced states, or
 * integrating with SCS console workflows.
 *
 * DEVELOPER NOTE: For "run a console command from my feature", start here.
 * Requires the "GameConsole" hook in BuildManifest (Policy_AddRequiredHook).
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Renders the Game Console demo section under the General tab.
 * @details Command string lives on g_ctx.consoleCommand so it persists across frames
 * while the user edits it in the InputText widget.
 */
void RenderConsoleSection(SPF_UI_API* ui);

}  // namespace ExamplePlugin
