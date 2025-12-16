#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Data/GameData/Finders/DebugCameraStateDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"

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
const char* OPEN_FILE_SIG = "48 89 5C 24 10 56 48 83 EC 20 48 8B F2 48 8B D9 48 8B CE BA 01 00 00";

// Signature for the function that formats the state and writes it to the file.
const char* FORMAT_AND_WRITE_SIG = "48 8B C4 53 48 81 EC 90 02 00 00 F3 0F 10 01 48 8B DA F3 0F 10 49 1C";

/**
 * @brief Signature for the function that cycles to the next/previous saved camera state.
 * TO BE IMPLEMENTED.
 */
const char* CYCLE_SAVED_STATE_SIG = "48 89 5C 24 08 57 48 83 EC 40 48 83 B9 C0";

/**
 * @brief Signature for the function that applies a specific saved camera state by index.
 * TO BE IMPLEMENTED.
 */
// const char* APPLY_STATE_SIG = "48 89 5C 24 08 57 48 83 ? ? 0F 10 02 48 8B DA 48 8B F9 48 83 ? ? 0F";

void LogFoundAddress(const char* name, uintptr_t address, bool error = false) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("DebugCameraStateDataFinder");
  if (address == 0 || error) {
    logger->Error("!!! FAILED to find '{}'", name);
  } else {
    logger->Info("--- Found '{}' at: 0x{:X}", name, address);
  }
}
}  // namespace

bool DebugCameraStateDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Debug Camera State data...");

  // --- Find AddCameraState function and StateContextOffset ---
  uintptr_t call_site_anchor = Utils::PatternFinder::Find(SAVE_STATE_CALL_SITE_SIG);

  if (call_site_anchor) {
    // The signature points to the first LEA. Extract the context offset from it.
    int32_t context_offset = *(int32_t*)(call_site_anchor + 3);
    owner.SetStateContextOffset(context_offset);
    logger->Info("--- Found StateContextOffset: 0x{:X}", context_offset);

    // The CALL instruction is at a fixed offset from the start of the signature pattern.
    // 7 bytes for first LEA + 5 bytes for skipped volatile LEA = 12 bytes.
    uintptr_t call_instruction_address = call_site_anchor + 12;
    uintptr_t next_instruction_address = call_instruction_address + 5;
    int32_t relative_offset = *(int32_t*)(call_instruction_address + 1);
    uintptr_t function_address = next_instruction_address + relative_offset;
    LogFoundAddress("AddCameraState", function_address);
    owner.SetAddCameraStateFunc((void*)function_address);
  } else {
    LogFoundAddress("Save State call site", 0, true);
    owner.SetStateContextOffset(0);
    owner.SetAddCameraStateFunc(nullptr);
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
    // Find the instruction that loads the base address of the states array.
    // Expected offset: 0xdb8
    // Signature: JNC loc_short; MOV RAX, qword ptr [RBX + offset]
    const unsigned char pattern[] = {0x73, 0x3F, 0x48, 0x8B, 0x83};
    uintptr_t instruction_addr = Utils::PatternFinder::Find(pfnCycleState, 256, pattern, sizeof(pattern));
    if (instruction_addr) {
      // The offset is 5 bytes after the start of the signature
      int32_t offset = *(int32_t*)(instruction_addr + 5);
      owner.SetStateArrayOffset(offset);
      logger->Info("--- Found StateArrayOffset (states array ptr): 0x{:X}", offset);
    } else {
      LogFoundAddress("StateArrayOffset", 0, true);
    }
  }

  // --- Find StateCountOffset (within CycleSavedState func) ---
  if (pfnCycleState && owner.GetStateCountOffset() == 0) {
    // Find the instruction that compares the state count to zero at the start of the function.
    // Expected offset: 0xdc0
    // Signature: PUSH RDI; SUB RSP, ?; CMP qword ptr [RCX + offset], 0
    const unsigned char pattern[] = {0x57, 0x48, 0x83, 0xEC, '?', 0x48, 0x83, 0xB9};
    uintptr_t instruction_addr = Utils::PatternFinder::Find(pfnCycleState, 256, pattern, sizeof(pattern));
    if (instruction_addr) {
      // The offset is 8 bytes after the start of the signature
      int32_t offset = *(int32_t*)(instruction_addr + 8);
      owner.SetStateCountOffset(offset);
      logger->Info("--- Found StateCountOffset (states count): 0x{:X}", offset);
    } else {
      LogFoundAddress("StateCountOffset", 0, true);
    }
  }

  // --- Find StateCurrentIndexOffset (within CycleSavedState func) ---
  if (pfnCycleState && owner.GetStateCurrentIndexOffset() == 0) {
    // Find the instruction that loads the current state index.
    // Expected offset: 0xda8
    // Signature: XOR EDX,EDX; MOV RAX,[RBX+offset]
    const unsigned char pattern[] = {0x33, 0xD2, 0x48, 0x8B, 0x83};
    uintptr_t instruction_addr = Utils::PatternFinder::Find(pfnCycleState, 256, pattern, sizeof(pattern));
    if (instruction_addr) {
      // The offset is 5 bytes after the start of the signature
      int32_t offset = *(int32_t*)(instruction_addr + 5);
      owner.SetStateCurrentIndexOffset(offset);
      logger->Info("--- Found StateCurrentIndexOffset (current index): 0x{:X}", offset);
    } else {
      LogFoundAddress("StateCurrentIndexOffset", 0, true);
    }
  }

  // --- Find ApplyState (relative to a known anchor within CycleSavedState) ---
  if (pfnCycleState && owner.GetApplyStateFunc() == nullptr) {
    // 1. Find the anchor point (the MOV instruction for the array offset) again.
    const unsigned char anchor_pattern[] = {0x73, 0x3F, 0x48, 0x8B, 0x83};
    uintptr_t anchor_addr = Utils::PatternFinder::Find(pfnCycleState, 256, anchor_pattern, sizeof(anchor_pattern));

    if (anchor_addr) {
      // 2. From the anchor, scan forward for the CALL instruction, preceded by its argument setup.
      // This is more robust than a fixed offset.
      // Signature: MOV RCX, RBX; CALL rel32
      const unsigned char call_pattern[] = {0x48, 0x8B, 0xCB, 0xE8};
      uintptr_t call_anchor_addr = Utils::PatternFinder::Find(anchor_addr, 64, call_pattern, sizeof(call_pattern));

      if (call_anchor_addr) {
        // 3. Calculate the final address from the relative CALL.
        uintptr_t call_instruction_addr = call_anchor_addr + 3;  // Move to the E8 opcode
        uintptr_t next_instruction_addr = call_instruction_addr + 5;
        int32_t relative_offset = *(int32_t*)(call_instruction_addr + 1);
        uintptr_t function_address = next_instruction_addr + relative_offset;
        owner.SetApplyStateFunc((void*)function_address);
        LogFoundAddress("ApplyState", function_address);
      } else {
        LogFoundAddress("ApplyState (call pattern)", 0, true);
      }
    } else {
      LogFoundAddress("ApplyState (anchor pattern)", 0, true);
    }
  }

  // --- Find LoadStatesFromFile (within CycleSavedState func) ---
  if (pfnCycleState && (owner.GetLoadStatesFromFileFunc() == nullptr || owner.GetStateManagerOffset() == 0)) {
    // Find the ADD RCX, offset; CALL rel32 sequence
    uintptr_t instruction_addr = Utils::PatternFinder::Find(pfnCycleState, 256, "48 81 C1 ? ? ? ? E8");
    if (instruction_addr) {
      // Extract the manager offset from the ADD instruction
      int32_t managerOffset = *(int32_t*)(instruction_addr + 3);
      owner.SetStateManagerOffset(managerOffset);
      logger->Info("--- Found StateManagerOffset: 0x{:X}", managerOffset);

      // Calculate the function address from the CALL instruction
      uintptr_t call_instruction_addr = instruction_addr + 7;  // Move to the E8 opcode
      uintptr_t next_instruction_addr = call_instruction_addr + 5;
      int32_t relative_offset = *(int32_t*)(call_instruction_addr + 1);
      uintptr_t function_address = next_instruction_addr + relative_offset;
      owner.SetLoadStatesFromFileFunc((void*)function_address);
      LogFoundAddress("LoadStatesFromFile", function_address);
    } else {
      LogFoundAddress("LoadStatesFromFile call site", 0, true);
    }
  }

  // TODO: Find ApplyStateFunc when approved

  m_isReady = owner.GetAddCameraStateFunc() != nullptr && owner.GetStateContextOffset() != 0 && owner.GetOpenFileForCameraStateFunc() != nullptr &&
              owner.GetFormatAndWriteCameraStateFunc() != nullptr && owner.GetCycleSavedStateFunc() != nullptr && owner.GetStateArrayOffset() != 0 &&
              owner.GetStateCountOffset() != 0 && owner.GetStateCurrentIndexOffset() != 0 && owner.GetApplyStateFunc() != nullptr && owner.GetLoadStatesFromFileFunc() != nullptr &&
              owner.GetStateManagerOffset() != 0;

  if (m_isReady) {
    logger->Info("Successfully found all required Debug State data.");
  } else {
    logger->Error("Failed to find one or more required Debug State data.");
  }
  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
