/**
 * @file ExampleGeneralAPI.cpp
 * @brief Implementation of the General tab composition.
 *
 * @details WHY sections live in other modules:
 * A developer looking for "how do I save a setting?" opens ExampleConfigAPI and sees
 * the full lifecycle (load, UI, OnSettingChanged) in one place. The General tab only
 * needs a one-screen taste of each — so it calls the same section functions those
 * modules export, with no duplicated widget code.
 */
#include "ExampleGeneralAPI.hpp"

#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExampleConfigAPI.hpp"
#include "ExampleConsoleAPI.hpp"
#include "ExampleGameLogAPI.hpp"
#include "ExampleHooksAPI.hpp"
#include "ExampleKeybindsAPI.hpp"
#include "ExampleLocalizationAPI.hpp"

namespace ExamplePlugin {

void RenderGeneralTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  ui->UI_Text("Hello from the ExamplePlugin window!");

  // Localization: resolve the welcome key at draw time (language can change live).
  RenderLocalizationSection(ui);

  // Config: slider that round-trips through settings.json (see ExampleConfigAPI).
  RenderConfigSection(ui);

  // GameConsole: execute a typed command (requires Policy GameConsole hook).
  RenderConsoleSection(ui);

  // Keybinds: suppress bound action + mouse look while checkboxes are on.
  RenderKeybindBlockingSection(ui);

  // Hooks: toggle the GameStringFormatting detour behavior (hook already installed).
  RenderHooksSection(ui);

  // GameLog: last matching in-game log line (observational API).
  RenderGameLogSection(ui);
}

}  // namespace ExamplePlugin
