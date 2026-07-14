#include "SPF/Data/GameData/Finders/DebugCameraStateDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>


SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/**
 * @brief Signature for the call site of the function that saves a camera state.
 * This signature is extremely robust. It finds a unique sequence of instructions:
 * LEA -> (a volatile instruction we skip) -> CALL -> LEA (for a log string).
 * This allows us to reliably find the call site and extract both the function
 * pointer and a required data offset.
 *
 * HOW-TO-FIND:
 * 1. Find where the "Camera state saved" string is used in the code.
 * 2. This will lead to a `LEA RCX, [rip+...]` instruction.
 * 3. The instruction immediately BEFORE this LEA should be a `CALL` to the save state function.
 * 4. A few bytes before that CALL is a `LEA RCX, [RSI+offset]` instruction.
 * 5. This signature combines these stable points, using wildcards to skip the volatile
 *    stack-relative instruction between them.
 *
 * EXPECTED ASSEMBLY:
 *   48 8D 8E ?? ?? ?? ??     - LEA RCX,[RSI+StateContextOffset]
 *   ?? ?? ?? ?? ??          - 5 bytes of volatile instructions (e.g., LEA RDX,[RSP+...])
 *   E8 ?? ?? ?? ??           - CALL AddAnimatedCameraState
 *   48 8D 0D ?? ?? ?? ??     - LEA RCX,[rip+"Camera state saved"]
 */
const char* SAVE_STATE_CALL_SITE_SIG = "48 8D 8E ? ? ? ? ?? ?? ?? ?? ?? E8 ? ? ? ? 48 8D 0D";

// Signature for the function that opens the camera state file.
const char* OPEN_FILE_SIG = "48 89 5C 24 10 56 48 83 ? ? 48 8B F2 48 8B D9 48 8B CE BA";

// Signature for the function that formats the state and writes it to the file.
const char* FORMAT_AND_WRITE_SIG = "48 8B C4 53 48 81 EC ? ? ? ? F3 0F 10 01 48 8B DA F3 0F 10";

/**
 * @brief Signature for the function that cycles to the next/previous saved camera state.
 * TO BE IMPLEMENTED.
 */
const char* CYCLE_SAVED_STATE_SIG = "48 89 5C 24 08 57 48 83 EC ? 48 83 B9 ? ? ? ? ? ? ? fa 48 8b d9 75 10";

/**
 * @brief Signature for the function that applies a specific saved camera state by index.
 * TO BE IMPLEMENTED.
 */
// const char* APPLY_STATE_SIG = "48 89 5C 24 08 57 48 83 ? ? 0F 10 02 48 8B DA 48 8B F9 48 83 ? ? 0F";

void LogFoundAddress(const char* name, uintptr_t address, bool error = false) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("DebugCameraStateDataFinder");
  if (address == 0 || error) {
    logger->Error("FAILED to find '{}'", name);
  } else {
    logger->Debug("--- Found '{}' at: 0x{:X}", name, address);
  }
}
}  // namespace

bool DebugCameraStateDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Debug Camera State data (Dynamic Search)...");

  bool all_found = true;

  // --- 1. Find AddCameraState function and StateContextOffset ---
  /*
   * Ghidra: 1404b145d
   * Anchor: LEA RCX, [RSI + 0xDB0]; ...; CALL AddAnimatedCameraState; LEA RCX, [rip + "Camera state saved"]
   */
  const char* p_save_anchor = "48 8D 8E ?? ?? ?? ?? 48 8D 54 24 ?? E8 ?? ?? ?? ?? 48 8D 0D";
  uintptr_t call_site_anchor = Utils::PatternFinder::Find(p_save_anchor);

  if (call_site_anchor) {
    int32_t ctxOff = Utils::PatternFinder::ReadInt32(call_site_anchor + 3);
    if (Utils::PatternFinder::IsSaneOffset(ctxOff)) {
      owner.SetStateContextOffset(ctxOff);
      owner.SetStateManagerOffset(ctxOff);  // Context and Manager share the same offset
      logger->Debug("--- Found StateContextOffset: 0x{:X}", ctxOff);
    } else {
      logger->Error("StateContextOffset INVALID (0x{:X})", ctxOff);
      all_found = false;
    }

    uintptr_t pAddState = Utils::PatternFinder::GetRipAddress(call_site_anchor + 12, 1, 5);
    if (pAddState) {
      owner.SetAddCameraStateFunc((void*)pAddState);
      logger->Debug("--- Found 'AddCameraState' at: 0x{:X}", pAddState);
    } else {
      logger->Error("FAILED to resolve AddCameraState address");
      all_found = false;
    }
  } else {
    logger->Error("FAILED to find Save State call site anchor");
    all_found = false;
  }

  // --- Find OpenFileForCameraState ---
  uintptr_t pfnOpenFile = (uintptr_t)owner.GetOpenFileForCameraStateFunc();
  if (!pfnOpenFile) {
    pfnOpenFile = Utils::PatternFinder::Find(OPEN_FILE_SIG);
    LogFoundAddress("OpenFileForCameraState", pfnOpenFile, !pfnOpenFile);
    owner.SetOpenFileForCameraStateFunc((void*)pfnOpenFile);
  }

  // --- Find FormatAndWriteCameraState ---
  uintptr_t pfnFormatAndWrite = (uintptr_t)owner.GetFormatAndWriteCameraStateFunc();
  if (!pfnFormatAndWrite) {
    pfnFormatAndWrite = Utils::PatternFinder::Find(FORMAT_AND_WRITE_SIG);
    LogFoundAddress("FormatAndWriteCameraState", pfnFormatAndWrite, !pfnFormatAndWrite);
    owner.SetFormatAndWriteCameraStateFunc((void*)pfnFormatAndWrite);
  }

  // --- Find CycleSavedState ---
  uintptr_t pfnCycleState = (uintptr_t)owner.GetCycleSavedStateFunc();
  if (!pfnCycleState) {
    pfnCycleState = Utils::PatternFinder::Find(CYCLE_SAVED_STATE_SIG);
    LogFoundAddress("CycleSavedState", pfnCycleState, !pfnCycleState);
    owner.SetCycleSavedStateFunc((void*)pfnCycleState);
  }

  // --- Find StateArrayOffset (within CycleSavedState func) ---
  if (pfnCycleState && owner.GetStateArrayOffset() == 0) {
    // Anchor: MOV [RBX+index], RDX; CMP RDX, RCX; JNC ...; MOV RAX, [RBX+array]
    const char* p_arr = "48 89 93 ?? ?? ?? ?? 48 3B D1 ?? ?? 48 8B 83";
    uintptr_t addr = Utils::PatternFinder::Find(pfnCycleState, 512, p_arr);
    if (addr) {
      int32_t off = Utils::PatternFinder::ReadInt32(addr + 15);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetStateArrayOffset(off);
        logger->Debug("--- Found StateArrayOffset: 0x{:X}", off);
      } else {
        logger->Error("StateArrayOffset INVALID (0x{:X})", off);
        all_found = false;
      }
    } else {
      logger->Error("FAILED to find StateArrayOffset anchor");
      all_found = false;
    }
  }

  // --- Find StateCountOffset (within CycleSavedState func) ---
  if (pfnCycleState && owner.GetStateCountOffset() == 0) {
    // Anchor: CMP qword ptr [RCX + offset], 0; MOVZX EDI, DL
    const char* p_count = "48 83 B9 ?? ?? ?? ?? ?? 0F B6 FA";
    uintptr_t addr = Utils::PatternFinder::Find(pfnCycleState, 256, p_count);
    if (addr) {
      int32_t off = Utils::PatternFinder::ReadInt32(addr + 3);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetStateCountOffset(off);
        logger->Debug("--- Found StateCountOffset: 0x{:X}", off);
      } else {
        logger->Error("StateCountOffset INVALID (0x{:X})", off);
        all_found = false;
      }
    } else {
      logger->Error("FAILED to find StateCountOffset anchor");
      all_found = false;
    }
  }

  // --- Find StateCurrentIndexOffset (within CycleSavedState func) ---
  if (pfnCycleState && owner.GetStateCurrentIndexOffset() == 0) {
    // Anchor: XOR EDX, EDX; MOV RAX, [RBX + offset]; TEST DIL, DIL
    const char* p_idx = "33 D2 48 8B 83 ?? ?? ?? ?? 40 84 FF";
    uintptr_t addr = Utils::PatternFinder::Find(pfnCycleState, 256, p_idx);
    if (addr) {
      int32_t off = Utils::PatternFinder::ReadInt32(addr + 5);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetStateCurrentIndexOffset(off);
        logger->Debug("--- Found StateCurrentIndexOffset: 0x{:X}", off);
      } else {
        logger->Error("StateCurrentIndexOffset INVALID (0x{:X})", off);
        all_found = false;
      }
    } else {
      logger->Error("FAILED to find StateCurrentIndexOffset anchor");
      all_found = false;
    }
  }

  // --- Find ApplyState (within CycleSavedState) ---
  if (pfnCycleState && owner.GetApplyStateFunc() == nullptr) {
    // Anchor: MOV RCX, RBX; CALL rel32; MOV RDX, [RBX + index]
    const char* p_apply = "48 8B CB E8 ?? ?? ?? ?? 48 8B 93";
    uintptr_t addr = Utils::PatternFinder::Find(pfnCycleState, 512, p_apply);
    if (addr) {
      uintptr_t pFunc = Utils::PatternFinder::GetRipAddress(addr + 3, 1, 5);
      if (pFunc) {
        owner.SetApplyStateFunc((void*)pFunc);
        logger->Debug("--- Found 'ApplyState' at: 0x{:X}", pFunc);
      } else {
        logger->Error("FAILED to resolve ApplyState address");
        all_found = false;
      }
    } else {
      logger->Error("FAILED to find ApplyState anchor");
      all_found = false;
    }
  }

  // --- Find LoadStatesFromFile (within CycleSavedState func) ---
  if (pfnCycleState && (owner.GetLoadStatesFromFileFunc() == nullptr || owner.GetStateManagerOffset() == 0)) {
    // Anchor: ADD RCX, offset; CALL rel32; TEST AL, AL
    const char* p_load = "48 81 C1 ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0";
    uintptr_t addr = Utils::PatternFinder::Find(pfnCycleState, 256, p_load);
    if (addr) {
      int32_t off = Utils::PatternFinder::ReadInt32(addr + 3);
      uintptr_t pFunc = Utils::PatternFinder::GetRipAddress(addr + 7, 1, 5);

      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetStateManagerOffset(off);
        logger->Debug("--- Found StateManagerOffset: 0x{:X}", off);
      } else {
        logger->Error("StateManagerOffset INVALID (0x{:X})", off);
        all_found = false;
      }

      if (pFunc) {
        owner.SetLoadStatesFromFileFunc((void*)pFunc);
        logger->Debug("--- Found 'LoadStatesFromFile' at: 0x{:X}", pFunc);
      } else {
        logger->Error("FAILED to resolve LoadStatesFromFile address");
        all_found = false;
      }
    } else {
      logger->Error("FAILED to find LoadStatesFromFile anchor");
      all_found = false;
    }
  }

  m_isReady = all_found;
  if (m_isReady) {
    logger->Info("Successfully found all required Debug State data dynamically.");
  } else {
    logger->Error("Failed to find one or more required Debug State data.");
  }
  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
