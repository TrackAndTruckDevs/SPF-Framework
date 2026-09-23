/**
 * @file ExampleHooksAPI.cpp
 * @brief Implementation of the SPF Hooks API example — signature-scan hook + detour.
 *
 * @details LIFECYCLE:
 *   OnActivated → Hook_Register(signature → detour, trampoline out)
 *   (runtime)   → Detour_GameStringFormatting replaces game calls while enabled
 *   OnUnload    → null trampoline; framework removes the hook entry
 *
 * WHY a signature, not a fixed address? Game updates shift code layout.
 * Byte patterns stay valid across minor patches when the instruction sequence is unique.
 */
#include "ExampleHooksAPI.hpp"

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstring>  // strstr — match the quit-button localization token in the detour

namespace ExamplePlugin {

void HooksAPI_OnActivated() {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->hooks) {
    return;
  }

  // Byte signature of the target function in game memory.
  // Format is the framework's pattern language (instruction mnemonics + wildcards).
  const char* signature = "[MOV [r64+off8], r64] [MOV [r64+off8], r64] [MOV [r64+off8], r64] [PUSH r64] [PUSH R8-R15] [PUSH R8-R15] [PUSH R8-R15] [PUSH R8-R15] [MOV r32, imm32] [CALL rel32] [SUB r64, r64] [MOV r64, r64] [MOV r64, r64]";

  // Hook_Register:
  //   - scans for `signature`
  //   - writes the jump to our detour
  //   - stores the trampoline (original code) into &g_ctx.o_GameStringFormatting
  //   - `true` = enable immediately
  g_ctx.coreAPI->hooks->Hook_Register(PLUGIN_NAME, "GameStringFormattingHook", "Game String Formatting Hook", reinterpret_cast<void*>(Detour_GameStringFormatting), reinterpret_cast<void**>(&g_ctx.o_GameStringFormatting), signature, true);
  g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Registered 'GameStringFormatting' hook.");
}

void HooksAPI_OnUnload() {
  // Framework drops the hook; clear our trampoline so no stale call reaches dead code.
  g_ctx.o_GameStringFormatting = nullptr;
}

void* Detour_GameStringFormatting(void* pOutput, const char** ppInput) {
  // Runtime gate: UI checkbox flips g_ctx.isModificationActive without re-registering the hook.
  if (g_ctx.isModificationActive) {
    const char* inputKey = ppInput ? *ppInput : nullptr;
    // Match the quit-button token inside the localization key the game passed in.
    if (inputKey && strstr(inputKey, ">@@quit_game@@</font>")) {
      // Swap the input pointer to our markup: red bold text using the game's own tags.
      // We only redirect the pointer — the original formatter still processes the string.
      static const char* modifiedQuitButton =
        "<img src=/material/ui/white.mat xscale=stretch yscale=stretch color=@@clr_list_item_bg_s@@><ret><align hstyle=center vstyle=center><font face=/font/normal_bold.font "
        "xscale=1.4 yscale=1.4><color value=FF0000FF>@@quit_game@@</font></align>";
      *ppInput = modifiedQuitButton;

      if (g_ctx.loadAPI && g_ctx.loadAPI->logger) {
        g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Overriding 'quit_game' button color.");
      }
    }
  }

  // CRITICAL: always invoke the original via the trampoline.
  // Skipping it means the game never formats the string → missing/broken UI text or crash.
  if (g_ctx.o_GameStringFormatting) {
    return g_ctx.o_GameStringFormatting(pOutput, ppInput);
  }
  return nullptr;
}

void RenderHooksSection(SPF_UI_API* ui) {
  ui->UI_Text("This checkbox controls a function hook:");
  // Toggle only — detour is already installed; it reads this flag every invocation.
  ui->UI_Checkbox("Make 'Quit' button red", &g_ctx.isModificationActive);
}

}  // namespace ExamplePlugin
