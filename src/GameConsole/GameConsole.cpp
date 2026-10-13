#include "SPF/GameConsole/GameConsole.hpp"

#include "SPF/Hooks/GameTools/PrismArrayResolver.hpp"
#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/Windows.hpp"  // IWYU pragma: keep

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <intrin.h>
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

bool GameConsole::Execute(const std::string& command, std::string* error) {
  // "cheat" is not in the game's command table — dispatch it here, like a shell
  // dispatches builtins. The gate + dispatcher live behind ExecuteCheat.
  if (command == "cheat" || command.rfind("cheat ", 0) == 0) {
    std::vector<std::string> tokens;
    for (size_t start = 0; start < command.size();) {
      size_t sp = command.find(' ', start);
      if (sp == std::string::npos) {
        tokens.push_back(command.substr(start));
        break;
      }
      if (sp > start) {
        tokens.push_back(command.substr(start, sp - start));
      }
      start = sp + 1;
    }

    std::string sub;
    std::vector<std::string> args;
    if (tokens.size() > 1) {
      sub = tokens[1];
      args.assign(tokens.begin() + 2, tokens.end());
    }
    return ExecuteCheat(sub, args, error);
  }

  if (m_ExecuteGameCommand) {
    const char* pCommand = command.c_str();
    m_ExecuteGameCommand(&pCommand, 0xffffffff);

    // g_developer opens/closes the cheat gate — resync the cached status so the
    // UI checkbox does not poll the cvar every frame.
    constexpr size_t kDeveloperLen = 11;  // strlen("g_developer")
    if (command.rfind("g_developer", 0) == 0 && (command.size() == kDeveloperLen || command[kDeveloperLen] == ' ')) {
      m_cheatsCheckedAt = std::chrono::steady_clock::now();
      size_t vpos = command.find_first_not_of(' ', kDeveloperLen);
      if (vpos == std::string::npos) {
        m_cheatsEnabledValid = false;  // bare query — re-read on next check
      } else {
        char* endp = nullptr;
        long long value = std::strtoll(command.c_str() + vpos, &endp, 10);
        if (endp != command.c_str() + vpos) {
          m_cheatsEnabled = value != 0;
          m_cheatsEnabledValid = true;
        } else {
          m_cheatsEnabledValid = false;
        }
      }
    }
    return true;
  }

  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger(m_name);
  logger->Warn("Attempted to execute command while service is not active: {}", command);
  if (error) *error = "console service is not active";
  return false;
}

namespace {

// Stack cap for the cheat wrapper; the actual size is discovered by ConsoleDataFinder.
constexpr size_t kMaxCheatWrapperSize = 0x40;

// prism array layout (etalon: MailService, DeliverOvsNewRoutesMail).
constexpr size_t kArrayVtableOff = 0x00;

// UI polls IsCheatsEnabled every frame — re-read the cvar at most this often.
constexpr std::chrono::milliseconds kCheatStatusTtl{1000};

using EmptyCtrlFn = uintptr_t (*)();
using PushFn = char (*)(uintptr_t array, uint32_t count, uintptr_t src);
using ArrayDtorFn = char (*)(uintptr_t array);
using CheatGateFn = int (*)(uintptr_t ctx);

// Prism array layout discovered by ConsoleDataFinder Phase 7 (Table A).
struct ArrayLayout {
  int32_t dataOff;
  int32_t sizeOff;
  int32_t capOff;
  int32_t ctrlOff;
  int32_t refcountOff;
  uint64_t refcountInc;
};

void InitArray(uint8_t* array, uintptr_t emptyCtrlAddr, uintptr_t vtableAddr, const ArrayLayout& layout, int32_t ctrlInnerOff) {
  *reinterpret_cast<uintptr_t*>(array + layout.dataOff) = 0;
  *reinterpret_cast<uint64_t*>(array + layout.sizeOff) = 0;
  *reinterpret_cast<uint64_t*>(array + layout.capOff) = 0;

  auto emptyCtrl = reinterpret_cast<EmptyCtrlFn>(emptyCtrlAddr);
  uintptr_t shared = emptyCtrl();
  uintptr_t ctrl = *reinterpret_cast<uintptr_t*>(shared + ctrlInnerOff);
  *reinterpret_cast<uintptr_t*>(array + layout.ctrlOff) = ctrl;
  _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(ctrl + layout.refcountOff), static_cast<__int64>(layout.refcountInc));
  *reinterpret_cast<uintptr_t*>(array + kArrayVtableOff) = vtableAddr;
}

}  // namespace

bool GameConsole::ExecuteCheat(const std::string& subcommand, const std::vector<std::string>& args, std::string* error) {
  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger(m_name);

  auto fail = [&](const std::string& msg) -> bool {
    logger->Error("ExecuteCheat: {}", msg);
    if (error) *error = msg;
    return false;
  };

  if (subcommand.empty()) {
    return fail("subcommand is empty.");
  }

  // Cheat handlers touch live game state (economy, companies, ...) that only
  // exists inside a loaded world. The gate (g_developer + manager) is already
  // open at the main menu, so gate-only protection does not stop pre-world
  // execution from crashing. Core mirrors its m_worldLoadedOnce state here.
  if (!m_worldReady.load(std::memory_order_relaxed)) {
    static constexpr char kMsg[] = "game world is not loaded - cheat commands are unavailable. Load a save first.";
    logger->Warn("ExecuteCheat: {}", kMsg);
    // Surface in the game console too (visible in ToolsWindow).
    if (m_ExecuteGameCommand) {
      const std::string echoCmd = std::string("echo ") + kMsg;
      const char* p = echoCmd.c_str();
      m_ExecuteGameCommand(&p, 0xffffffff);
    }
    if (error) *error = kMsg;
    return false;
  }

  if (m_consoleDataFinder.GetCheatGateAddr() == 0 && !m_consoleDataFinder.TryFind()) {
    return fail("cheat discovery failed.");
  }
  const uintptr_t gateAddr = m_consoleDataFinder.GetCheatGateAddr();
  if (gateAddr == 0) {
    return fail("cheat gate not discovered.");
  }

  auto& strRes = SPF::Hooks::GameTools::PrismStringResolver::GetInstance();
  auto& arrRes = SPF::Hooks::GameTools::PrismArrayResolver::GetInstance();
  if (!strRes.IsInstalled() || !arrRes.IsInstalled()) {
    return fail("prism resolvers are not installed.");
  }

  const ArrayLayout layout{m_consoleDataFinder.GetArrayDataOffset(),
                           m_consoleDataFinder.GetArraySizeOffset(),
                           m_consoleDataFinder.GetArrayCapOffset(),
                           m_consoleDataFinder.GetArrayCtrlOffset(),
                           m_consoleDataFinder.GetCtrlRefcountOffset(),
                           static_cast<uint64_t>(m_consoleDataFinder.GetCtrlRefcountIncrement())};
  if (layout.dataOff <= 0 || layout.sizeOff <= layout.dataOff || layout.capOff <= layout.sizeOff || layout.ctrlOff <= 0 || layout.refcountOff <= 0 || layout.refcountInc == 0) {
    return fail("cheat array layout not discovered.");
  }

  const int32_t tokensOff = m_consoleDataFinder.GetCheatTokensOffset();
  const int64_t wrapperSize = m_consoleDataFinder.GetCheatWrapperSize();
  if (tokensOff <= 0 || wrapperSize <= 0 || static_cast<uint64_t>(wrapperSize) > kMaxCheatWrapperSize) {
    return fail("wrapper layout not discovered (tokensOff=" + std::to_string(tokensOff) + ", wrapperSize=" + std::to_string(wrapperSize) + ").");
  }

  const int32_t ctrlInnerOff = m_consoleDataFinder.GetCheatCtrlInnerOffset();
  const int32_t tempPrismSize = m_consoleDataFinder.GetCheatTempPrismSize();
  if (ctrlInnerOff <= 0 || tempPrismSize <= 0) {
    return fail("shared ctrl/temp prism layout not discovered (ctrlInner=" + std::to_string(ctrlInnerOff) + ", prismSize=" + std::to_string(tempPrismSize) + ").");
  }

  alignas(8) uint8_t ctx[kMaxCheatWrapperSize] = {};
  uint8_t* tokens = ctx + tokensOff;
  InitArray(tokens, arrRes.GetEmptyCtrlAddr(), arrRes.GetArrayVtableAddr(), layout, ctrlInnerOff);

  auto pushFn = reinterpret_cast<PushFn>(arrRes.GetPushAddr());
  bool ok = true;

  auto pushToken = [&](const char* text) {
    std::vector<uint8_t> tmpPrism(static_cast<size_t>(tempPrismSize));
    strRes.InitEmpty(tmpPrism.data());
    ok = strRes.Set(tmpPrism.data(), text);
    if (ok) {
      uintptr_t src = reinterpret_cast<uintptr_t>(tmpPrism.data()) + strRes.GetBufferOffset();
      ok = pushFn(reinterpret_cast<uintptr_t>(tokens), 1, src) != 0;
    }
    strRes.Destroy(tmpPrism.data());
  };

  pushToken("cheat");
  if (ok) pushToken(subcommand.c_str());
  for (const auto& arg : args) {
    if (!ok) break;
    pushToken(arg.c_str());
  }

  auto arrayDtor = reinterpret_cast<ArrayDtorFn>(arrRes.GetDtorAddr());
  bool executed = false;

  if (!ok) {
    arrayDtor(reinterpret_cast<uintptr_t>(tokens));
    return fail("failed to build token array for '" + subcommand + "'.");
  }

  {
    auto gate = reinterpret_cast<CheatGateFn>(gateAddr);
    int result = gate(reinterpret_cast<uintptr_t>(ctx));

    // -2 is ambiguous: the gate returns it when g_developer <= 0 / manager null,
    // and the dispatcher returns it on handler SUCCESS (0xfffffffe).
    // -3 (0xfffffffd) = handler returned 0 or unknown cheat name.
    // Read the cvar to tell the two -2 cases apart.
    if (m_getAndCacheValueFn == 0) {
      m_getAndCacheValueFn = m_consoleDataFinder.GetAndCacheValueAddr();
    }
    int64_t devValue = 0;
    const uintptr_t cvarAddr = m_consoleDataFinder.GetDeveloperCvarAddr();
    if (m_getAndCacheValueFn != 0 && cvarAddr != 0) {
      using GetAndCacheValueFn = int64_t (*)(uintptr_t);
      devValue = reinterpret_cast<GetAndCacheValueFn>(m_getAndCacheValueFn)(cvarAddr);
    } else {
      logger->Error("ExecuteCheat: g_developer cvar not discovered — cannot interpret result {}.", result);
    }

    m_cheatsEnabled = devValue > 0;
    m_cheatsEnabledValid = true;
    m_cheatsCheckedAt = std::chrono::steady_clock::now();

    if (devValue <= 0) {
      logger->Error(
        "ExecuteCheat: gate rejected '{}' — cheat commands are disabled "
        "(g_developer <= 0 or manager null).",
        subcommand);
      if (error) *error = "cheat commands are disabled (g_developer <= 0 or manager null)";
    } else if (result == -2) {
      logger->Info("ExecuteCheat: 'cheat {}' executed.", subcommand);
      executed = true;
    } else {
      logger->Error("ExecuteCheat: 'cheat {}' failed — handler rejected it or unknown cheat ({}).", subcommand, result);
      if (error) *error = "handler rejected the cheat or unknown cheat name";
    }
  }

  arrayDtor(reinterpret_cast<uintptr_t>(tokens));

  return executed;
}

void GameConsole::EnableCheats() { Execute("g_developer 1"); }

bool GameConsole::IsCheatsEnabled() {
  const auto now = std::chrono::steady_clock::now();
  if (m_cheatsEnabledValid && (now - m_cheatsCheckedAt) < kCheatStatusTtl) {
    return m_cheatsEnabled;
  }

  bool enabled = false;
  const bool discovered = m_consoleDataFinder.GetCheatGateAddr() != 0 || m_consoleDataFinder.TryFind();
  if (discovered) {
    const uintptr_t gateAddr = m_consoleDataFinder.GetCheatGateAddr();
    const uintptr_t cvarAddr = m_consoleDataFinder.GetDeveloperCvarAddr();
    if (gateAddr != 0 && cvarAddr != 0) {
      // Gate body (verified via Ghidra disassembly):
      //   PUSH RBX; SUB RSP,0x20; MOV RBX,RCX; LEA RCX,[g_developer cvar]; CALL GetAndCacheValue
      if (m_getAndCacheValueFn == 0) {
        m_getAndCacheValueFn = m_consoleDataFinder.GetAndCacheValueAddr();
      }
      if (m_getAndCacheValueFn != 0) {
        using GetAndCacheValueFn = int64_t (*)(uintptr_t);
        enabled = reinterpret_cast<GetAndCacheValueFn>(m_getAndCacheValueFn)(cvarAddr) > 0;
      }
    }
  }

  m_cheatsEnabled = enabled;
  m_cheatsEnabledValid = true;
  m_cheatsCheckedAt = now;
  return m_cheatsEnabled;
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
        const char* buffer = *reinterpret_cast<const char**>(decoded + m_consoleDataFinder.GetCheatDecodedBufferOffset());
        const uint32_t len = *reinterpret_cast<const uint32_t*>(decoded + m_consoleDataFinder.GetCheatDecodedLenOffset());
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

  // Source 4: cheat subcommands (cheat_manager_t via gate-discovered global).
  // Layout discovered by ConsoleDataFinder from the dispatcher (FUN_1405b62f0).
  size_t cheatCount = 0;
  {
    const uintptr_t managerPtrAddr = m_consoleDataFinder.GetCheatManagerPtrAddr();
    if (managerPtrAddr != 0 && !IsBadReadPtr(reinterpret_cast<void*>(managerPtrAddr), sizeof(uintptr_t))) {
      const uintptr_t manager = *reinterpret_cast<uintptr_t*>(managerPtrAddr);
      if (manager != 0 && !IsBadReadPtr(reinterpret_cast<void*>(manager), 0x18)) {
        const uintptr_t data = *reinterpret_cast<uintptr_t*>(manager + m_consoleDataFinder.GetCheatManagerDataOffset());
        const size_t count = *reinterpret_cast<size_t*>(manager + m_consoleDataFinder.GetCheatManagerCountOffset());
        const int32_t entryStride = m_consoleDataFinder.GetCheatEntryStride();
        const int32_t nameOffset = m_consoleDataFinder.GetCheatEntryNameOffset();
        const size_t bounded = (count < kMaxPerSource) ? count : kMaxPerSource;
        if (data != 0) {
          for (size_t i = 0; i < bounded; ++i) {
            const uintptr_t entry = data + i * entryStride;
            if (IsBadReadPtr(reinterpret_cast<void*>(entry + nameOffset), sizeof(uintptr_t))) break;
            const char* name = *reinterpret_cast<const char**>(entry + nameOffset);
            if (!name || IsBadReadPtr(const_cast<char*>(name), 1) || name[0] == '\0') continue;
            m_commandList.emplace_back(std::string("cheat ") + name);
            ++cheatCount;
          }
        }
      }
    }
  }

  logger->Info("Cached {} console commands (list1={}, list2={}, input={}, cheat={}).", m_commandList.size(), list1Count, list2Count, inputCount, cheatCount);
  return !m_commandList.empty();
}
