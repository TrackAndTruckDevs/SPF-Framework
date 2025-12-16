/* Signature for the game's central logging function (FUN_14020a1d0 in Ghidra).
 *
 * How it was found:
 * 1. Took a unique string from the game log, e.g.: "[route_data_u::compute_nav_nodes] Cannot find...".
 * 2. Found where this string is used in Ghidra. This led to function FUN_1408d6650.
 * 3. Inside FUN_1408d6650, we saw that it calls another function for logging: FUN_140209e80.
 * 4. Upon entering FUN_140209e80, we found that it is simply a wrapper that, in turn, calls FUN_14020a1d0.
 *
 * Thus, FUN_14020a1d0 is the root function for game logs. If this signature
 * stops working after a game update, repeat this chain of actions to find a new one.
 */
#include "SPF/Hooks/GameLogHook.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Modules/GameLogEventManager.hpp"

#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <vector>
#include <sstream>    // For parsing signature string
#include <stdexcept>  // For std::invalid_argument

#include "MinHook.h"

namespace {
// Keep hook details private to this translation unit
using GameLog_t = void (*)(int, const char*, va_list);
GameLog_t o_GameLog = nullptr;

void Detour_GameLog(int level, const char* format, va_list args) {
  // Log that the detour is being called
  auto detourLogger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("GameLogHook");
  // if (detourLogger) detourLogger->Debug("Detour_GameLog called.");

  va_list args_copy;
  va_copy(args_copy, args);

  char formatted_message[4096];
  /*
   * IMPORTANT ARCHITECTURAL POINT:
   * We do not attempt to call the game's internal formatter. Instead,
   * we intercept the "raw" format string and argument list (va_list)
   * and pass them to the standard, safe vsnprintf function.
   *
   * This works because the format specifiers used by the game (even
   * MSVC-specific ones like %I64X) are mostly compatible with the standard library.
   *
   * This approach balances stability (we don't touch unknown game code)
   * with functionality (we get fully formatted strings).
   */
  if (format) {
    vsnprintf(formatted_message, sizeof(formatted_message), format, args_copy);
  } else {
    formatted_message[0] = '\0';
  }

  va_end(args_copy);

  // Broadcast the raw message to plugins so they can do their own parsing
  SPF::Modules::GameLogEventManager::GetInstance().Broadcast(formatted_message);

  // Also, log it to the framework's "Game" logger, but with the correct level
  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("Game");
  if (logger) {
      if (strstr(formatted_message, "<ERROR>")) {
          logger->Error("{}", formatted_message);
      } else if (strstr(formatted_message, "<WARNING>")) {
          logger->Warn("{}", formatted_message);
      } else {
          logger->Info("{}", formatted_message);
      }
  }

  return o_GameLog(level, format, args);
}
}  // namespace

SPF_NS_BEGIN
namespace Hooks {

GameLogHook::GameLogHook()
    : BaseHook("GameLogHook", "Game Log", "89 4C 24 08 55 53 56 57 41 55 48 8B EC", "framework") {}

GameLogHook& GameLogHook::GetInstance() {
  static GameLogHook instance;
  return instance;
}

void* GameLogHook::GetDetourFunc() {
    return reinterpret_cast<void*>(&Detour_GameLog);
}

void** GameLogHook::GetOriginalFuncPtr() {
    return reinterpret_cast<void**>(&o_GameLog);
}

}  // namespace Hooks
SPF_NS_END