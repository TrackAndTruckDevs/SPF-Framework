#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace SPF {

GameConsole& GameConsole::GetInstance() {
  static GameConsole instance;
  return instance;
}

bool GameConsole::Install() {
  if (m_hookedAddress != 0) {
    return true;  // Already installed
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);

  /**
   * SEARCH STRATEGY (Verified for Game Version 1.60):
   * We locate the command enqueuer function by finding a reference to the error string:
   * "[cmd] Unknown queue id: %u" (found in FUN_1401dc890).
   *
   * Since this string might be used in multiple places, we use a context signature
   * to match the specific exit block of our target function.
   *
   * Target Code Snippet (Ghidra 1.60):
   * 1401dc95d 48 8d 0d ...   LEA RCX, [s_[cmd]_Unknown_queue_id:_%u_...]
   * 1401dc964 e8 47 bc ...   CALL FUN_1400f85b0  <- m_signature matches here (E8 ?? ?? ?? ??)
   * 1401dc969 32 c0          XOR AL, AL          <- m_signature matches here (32 C0)
   * 1401dc96b 48 81 c4 ...   ADD RSP, 0xc40
   * 1401dc972 5b             POP RBX
   * 1401dc973 c3             RET
   *
   * After finding the Xref, we backtrack to find the function prologue:
   * 1401dc890 40 53          PUSH RBX
   * 1401dc892 48 81 ec ...   SUB RSP, 0xc40
   */
  uintptr_t address = Utils::PatternFinder::FindFunctionByString(
    m_stringSignature.c_str(), 
    true,                      // Auto-backtrack to PUSH RBX / SUB RSP
    m_signature.c_str()        // Context: CALL + XOR AL, AL
  );

  if (address) {
    m_ExecuteGameCommand = reinterpret_cast<ExecuteCommandFn>(address);
    m_hookedAddress = address;  // Save address on success
    logger->Info("Found command execution function (via string xref) at address: {:#x}", address);
    return true;
  } else {
    logger->Error("Could not find command execution function via string/context. The game might have been updated.");
    return false;
  }
}

void GameConsole::Uninstall() {
  if (m_hookedAddress != 0) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Service disabled, clearing function pointer.");
    m_ExecuteGameCommand = nullptr;
    m_hookedAddress = 0;
  }
}

void GameConsole::Remove() {
  // For GameConsole, Remove is the same as Uninstall as it's non-destructive.
  Uninstall();
}

void GameConsole::Execute(const std::string& command) {
  if (m_ExecuteGameCommand) {
    const char* pCommand = command.c_str();
    m_ExecuteGameCommand(&pCommand, 0xffffffff); 
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Warn("Attempted to execute command while service is not active: {}", command);
  }
}
}  // namespace SPF