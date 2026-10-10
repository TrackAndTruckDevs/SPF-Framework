#include "SPF/GameConsole/GameConsole.hpp"

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/Windows.hpp"  // IWYU pragma: keep

#include <cstddef>
#include <cstdint>
#include <psapi.h>
#include <string>
#include <vector>
#include <winnt.h>

GameConsole& GameConsole::GetInstance() {
  static GameConsole instance;
  return instance;
}

bool GameConsole::Install() {
  if (m_hookedAddress != 0) {
    return true;  // Already installed
  }

  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger(m_name);

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
  uintptr_t address = SPF::Utils::PatternFinder::FindFunctionByString(m_stringSignature.c_str(),
                                                                      true,                // Auto-backtrack to PUSH RBX / SUB RSP
                                                                      m_signature.c_str()  // Context: CALL + XOR AL, AL
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
    auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger(m_name);
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
    auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Warn("Attempted to execute command while service is not active: {}", command);
  }
}

const std::vector<std::string>& GameConsole::GetCommandList() {
  if (!m_commandListBuilt) {
    BuildCommandList();
    m_commandListBuilt = true;
  }
  return m_commandList;
}

bool GameConsole::BuildCommandList() {
  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger(m_name);

  if (!m_consoleDataFinder.TryFind()) {
    logger->Error("Console command registry discovery failed; command list unavailable.");
    return false;
  }

  constexpr size_t kMaxPerSource = 4096;

  auto appendName = [&](const char* name) {
    if (!name || IsBadReadPtr(const_cast<char*>(name), 1) || name[0] == '\0') return;
    m_commandList.emplace_back(name);
  };

  // Source 1: console command list (head -> ... -> sentinel).
  size_t list1Count = 0;
  {
    const uintptr_t sentinel = m_consoleDataFinder.GetConsoleListSentinelAddr();
    uintptr_t node = *reinterpret_cast<uintptr_t*>(m_consoleDataFinder.GetConsoleListHeadAddr());
    while (node != 0 && node != sentinel && list1Count < kMaxPerSource) {
      if (IsBadReadPtr(reinterpret_cast<void*>(node), sizeof(uintptr_t))) break;
      const char* name = *reinterpret_cast<const char**>(node + m_consoleDataFinder.GetConsoleNameOffset());
      appendName(name);
      ++list1Count;
      node = *reinterpret_cast<uintptr_t*>(node + m_consoleDataFinder.GetConsoleNextOffset());
    }
  }

  // Source 2: script command list (head -> ... -> null), name inline at +0x08.
  size_t list2Count = 0;
  {
    const int32_t nameOffset = m_consoleDataFinder.GetScriptNameOffset();
    const int32_t nextOffset = m_consoleDataFinder.GetScriptNextOffset();
    uintptr_t node = *reinterpret_cast<uintptr_t*>(m_consoleDataFinder.GetScriptListHeadAddr());
    while (node != 0 && list2Count < kMaxPerSource) {
      if (IsBadReadPtr(reinterpret_cast<void*>(node), static_cast<SIZE_T>(nextOffset + sizeof(uintptr_t)))) break;
      appendName(reinterpret_cast<const char*>(node + nameOffset));
      ++list2Count;
      node = *reinterpret_cast<uintptr_t*>(node + nextOffset);
    }
  }

  // Source 3: input constants array — element+8 holds 3 packed SCS tokens
  // (3 qwords), decoded into a dotted string by the game's token-decode
  // helper (FUN_1402156c0, discovered as inputConstCopyNameFn).
  size_t inputCount = 0;
  {
    const size_t count = *reinterpret_cast<size_t*>(m_consoleDataFinder.GetInputConstSizeAddr());
    const uintptr_t data = *reinterpret_cast<uintptr_t*>(m_consoleDataFinder.GetInputConstDataAddr());
    const uintptr_t decodeFn = m_consoleDataFinder.GetInputConstCopyNameFnAddr();
    if (data != 0 && decodeFn != 0) {
      const size_t bounded = (count < kMaxPerSource) ? count : kMaxPerSource;
      using DecodeTokensFn = void (*)(void* outString, const void* tokens);
      for (size_t i = 0; i < bounded; ++i) {
        const uintptr_t slot = data + i * sizeof(uintptr_t);
        if (IsBadReadPtr(reinterpret_cast<void*>(slot), sizeof(uintptr_t))) break;
        const uintptr_t element = *reinterpret_cast<uintptr_t*>(slot);
        if (element == 0) continue;

        // Decoded-string object layout (verified from FUN_1402156c0):
        // +0x00 vtable, +0x08 buffer*, +0x10 length (u32), +0x14 capacity (u32),
        // +0x18 inline buffer (capacity 0x27).
        alignas(16) uint8_t decoded[0x40] = {};
        reinterpret_cast<DecodeTokensFn>(decodeFn)(decoded, reinterpret_cast<const void*>(element + m_consoleDataFinder.GetInputConstNameOffset()));
        const char* buffer = *reinterpret_cast<const char**>(decoded + 0x08);
        const uint32_t len = *reinterpret_cast<uint32_t*>(decoded + 0x10);
        if (buffer != nullptr && len != 0 && !IsBadReadPtr(const_cast<char*>(buffer), len)) {
          m_commandList.emplace_back(buffer, len);
          ++inputCount;
        }
        const uintptr_t vtable = *reinterpret_cast<uintptr_t*>(decoded);
        if (vtable != 0) {
          const uintptr_t dtor = *reinterpret_cast<uintptr_t*>(vtable);
          reinterpret_cast<void (*)(void*, int)>(dtor)(decoded, 0);
        }
      }
    }
  }

  logger->Info("Cached {} console commands (list1={}, list2={}, input={}).", m_commandList.size(), list1Count, list2Count, inputCount);
  return !m_commandList.empty();
}
