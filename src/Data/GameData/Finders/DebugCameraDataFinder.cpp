#include "SPF/Data/GameData/Finders/DebugCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {

// Signature for the SetDebugCameraMode function.
const char* SET_DEBUG_MODE_SIG = "48 89 5C ? ? 57 48 83 EC 50 8B FA 48 8B D9 39 91";

// Signature for the SetHUDVisibility function.
const char* SET_HUD_VISIBILITY_SIG = "48 89 5C 24 10 55 56 57 48 83 EC 40 48 8D B1 30 05 00 00 0F B6 EA";

// Signature for the SetDebugHudPosition function.
const char* SET_DEBUG_HUD_POSITION_SIG = "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 8B FA 48 8B 89 30 05 00 00 48 85 C9";

// --- Signatures for dynamic CVar value finding ---

/*
 * Signature to find the base pointer of the cacheable CVar object.
 * HOW-TO-FIND:
 * 1. Find the GetAndCacheValue function.
 * 2. Find its cross-references (XREFs).
 * 3. Land in the 'SetupAndRenderView' function.
 * 4. The signature targets the unique sequence of instructions before the call to GetAndCacheValue:
 *    - 48 8D 0D...  (lea rcx, [rip+...])  <- This loads the pointer we need.
 *    - 4C 89 88...  (mov [rax+...], r9)
 *    - E8...        (call GetAndCacheValue)
 *    - 85 C0        (test eax, eax)
 */
const char* CACHEABLE_CVAR_PTR_SIG = "48 8D 0D ? ? ? ? 4C 89 88 ? ? ? ? E8 ? ? ? ? 85 C0";

/*
 * Signature to find the dynamic offset of the value within the CVar object.
 * This signature is searched for *inside* the GetAndCacheValue function.
 * HOW-TO-FIND:
 * 1. Go to the GetAndCacheValue function.
 * 2. Find the instruction that writes the cached value: mov dword ptr [rdi+0x118], eax
 * 3. The signature targets this instruction and its neighbors for uniqueness:
 *    - 48 8B 5C 24 30  (mov rbx, [rsp+30])
 *    - 89 87...        (mov [rdi+offset], eax) <- This contains the offset we need.
 *    - C6 87...        (mov [rdi+offset-2], 1)
 */
const char* CVAR_VALUE_OFFSET_SIG = "48 8B 5C 24 30 89 87 ? ? ? ? C6 87";

// Signature for the GetAndCacheValue function itself, to provide a search range.
const char* GET_AND_CACHE_VALUE_SIG = "40 57 48 83 EC 20 48 83 B9";

// Signature for the Clean UI toggle logic.
const char* CLEAN_UI_TOGGLE_SIG = "80 BE ? ? ? ? 00 0F 94 C0 88 86 ? ? ? ? F2 0F 10 05";

// Signature for the HUD Visibility read logic.
const char* HUD_VISIBLE_READ_SIG = "38 9E ? ? ? ? 48 8B CE 0F 94 C2";

// Signature for the HUD Position read logic.
const char* HUD_POSITION_READ_SIG = "8B 96 ? ? ? ? 48 8B CE FF C2 83 E2 03";
}  // namespace

bool DebugCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Debug Camera data (Dynamic Search)...");

  bool all_found = true;

  // --- 1. Find CVar object pointer and value offset ---
  uintptr_t sig_cvar_ptr = Utils::PatternFinder::Find(CACHEABLE_CVAR_PTR_SIG);
  if (sig_cvar_ptr) {
    uintptr_t base_ptr = Utils::PatternFinder::GetRipAddress(sig_cvar_ptr, 3, 7);
    if (base_ptr) {
      owner.SetCacheableCvarObjectPtr(base_ptr);
      logger->Info("--- Found Cacheable CVar Object: 0x{:X}", base_ptr);
    } else { logger->Error("FAILED to resolve CVar Object RIP"); all_found = false; }
  } else { logger->Warn("FAILED to find Cacheable CVar Object anchor"); all_found = false; }

  uintptr_t get_and_cache_func = Utils::PatternFinder::Find(GET_AND_CACHE_VALUE_SIG);
  if (get_and_cache_func) {
    uintptr_t sig_off = Utils::PatternFinder::Find(get_and_cache_func, 256, CVAR_VALUE_OFFSET_SIG);
    if (sig_off) {
      int32_t offset = Utils::PatternFinder::ReadInt32(sig_off + 7);
      if (Utils::PatternFinder::IsSaneOffset(offset)) {
        owner.SetCvarValueOffset(offset);
        logger->Info("--- Found CVar Value Offset: 0x{:X}", offset);
      } else { logger->Error("CVar Value Offset INVALID (0x{:X})", offset); all_found = false; }
    } else { logger->Warn("FAILED to find CVar Value Offset anchor"); all_found = false; }
  }

  // --- 2. Find standalone functions ---
  uintptr_t pfnSetMode = Utils::PatternFinder::Find(SET_DEBUG_MODE_SIG);
  if (pfnSetMode) {
    owner.SetDebugCameraModeFunc(reinterpret_cast<void*>(pfnSetMode));
    logger->Info("--- Found SetDebugCameraMode at: 0x{:X}", pfnSetMode);
  } else { logger->Warn("FAILED to find SetDebugCameraMode signature"); all_found = false; }

  // 2.1 Find SetSelectedActor function
  // Signature provided by user: MOV [RSP+8], RBX; PUSH RDI; SUB RSP, 20; MOV RDI, RDX; MOV RBX, RCX; CMP RDX, [RCX+4A0]
  uintptr_t pfnSetSelected = Utils::PatternFinder::Find("48 89 5C ? ? 57 48 83 ? ? 48 8B FA 48 8B D9 48 3B 91 ? ? ? ? 0F 84");
  if (pfnSetSelected) {
    owner.SetSetSelectedActorFunc(reinterpret_cast<void*>(pfnSetSelected));
    logger->Info("--- Found SetSelectedActor at: 0x{:X}", pfnSetSelected);
  } else { logger->Warn("FAILED to find SetSelectedActor signature"); all_found = false; }

  // 2.2 Find SetPositionLock function
  // Signature based on disassembly start: MOV RAX, RSP; MOV [RAX+18], RBX; PUSH RDI; SUB RSP, 50; MOVZX EDI, DL
  uintptr_t pfnSetPosLock = Utils::PatternFinder::Find("48 8B C4 48 89 ? ? 57 48 83 ? ? 0F B6 FA 48 8B D9 38 91 ? ? ? ?");
  if (pfnSetPosLock) {
    owner.SetSetPositionLockFunc(reinterpret_cast<void*>(pfnSetPosLock));
    logger->Info("--- Found SetPositionLock at: 0x{:X}", pfnSetPosLock);
  } else { logger->Warn("FAILED to find SetPositionLock signature"); all_found = false; }

  // 2.3 Find SetRotationLock function
  // Signature: MOVZX EAX, DL; CMP [RCX+3E1], DL; JZ ...; MOV [RCX+3E1], DL
  uintptr_t pfnSetRotLock = Utils::PatternFinder::Find("0F B6 C2 38 91 ? ? ? ? 74 26 88 91 ? ? ? ? 84 C0");
  if (pfnSetRotLock) {
    owner.SetSetRotationLockFunc(reinterpret_cast<void*>(pfnSetRotLock));
    logger->Info("--- Found SetRotationLock at: 0x{:X}", pfnSetRotLock);
  } else { logger->Warn("FAILED to find SetRotationLock signature"); all_found = false; }

  // 2.4 Find SetOrbitMode function
  // Signature based on disassembly start: MOV RAX, RSP; MOV [RAX+18], RBX; MOV [RAX+20], RDI; PUSH RBP; ...
  uintptr_t pfnSetOrbit = Utils::PatternFinder::Find("48 8B C4 48 89 ? ? 48 89 ? ? 55 48 8D ? ? 48 81 EC ? ? ? ? 0F B6 FA 48 8B D9");
  if (pfnSetOrbit) {
    owner.SetSetOrbitModeFunc(reinterpret_cast<void*>(pfnSetOrbit));
    logger->Info("--- Found SetOrbitMode at: 0x{:X}", pfnSetOrbit);
  } else { logger->Warn("FAILED to find SetOrbitMode signature"); all_found = false; }

  uintptr_t pfnSetHudVis = Utils::PatternFinder::Find(SET_HUD_VISIBILITY_SIG);
  if (pfnSetHudVis) {
    owner.SetSetHudVisibilityFunc(reinterpret_cast<void*>(pfnSetHudVis));
    logger->Info("--- Found SetHudVisibility at: 0x{:X}", pfnSetHudVis);
  } else { logger->Warn("FAILED to find SetHudVisibility signature"); all_found = false; }

  uintptr_t pfnSetHudPos = Utils::PatternFinder::Find(SET_DEBUG_HUD_POSITION_SIG);
  if (pfnSetHudPos) {
    owner.SetSetDebugHudPositionFunc(reinterpret_cast<void*>(pfnSetHudPos));
    logger->Info("--- Found SetDebugHudPosition at: 0x{:X}", pfnSetHudPos);
  } else { logger->Warn("FAILED to find SetDebugHudPosition signature"); all_found = false; }

  // --- 3. Find the pDebugCamera context pointer dynamically ---
  uintptr_t pStandardManagerAddr = owner.GetStandardManagerPtrAddr();
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnGetCamObj = reinterpret_cast<uintptr_t>(cameraHooks.GetGetCameraObjectFunc());

  if (pStandardManagerAddr && pfnGetCamObj) {
    uintptr_t pStandardManager = *reinterpret_cast<uintptr_t*>(pStandardManagerAddr);
    if (pStandardManager) {
      /*
       * HOW-TO-FIND Camera Array Offset:
       * We look inside GetCameraObjectByID function.
       * It adds a base offset to RCX (StandardManager) and then reads the array pointer.
       * 
       * Assembly (v1.58):
       * 48 83 C1 30       - ADD RCX, 0x30
       * 4C 3B 41 10       - CMP R8, [RCX + 0x10]
       * ...
       * 48 8B 41 08       - MOV RAX, [RCX + 0x08]
       * 
       * Combined offset = 0x30 + 0x08 = 0x38.
       */
      const char* p_array_logic = "48 83 C1 ?? ?? ?? ?? ?? 73 ?? 48 8B 41 ??";
      uintptr_t addr = Utils::PatternFinder::Find(pfnGetCamObj, 128, p_array_logic);
      
      int32_t finalOffset = 0x38; // Default fallback for v1.58
      if (addr) {
        int8_t baseOff = Utils::PatternFinder::ReadInt8(addr + 3);
        int8_t subOff = Utils::PatternFinder::ReadInt8(addr + 13);
        finalOffset = static_cast<int32_t>(baseOff) + static_cast<int32_t>(subOff);
        logger->Info("--- Dynamically found Camera Array Offset: 0x{:X} (0x{:X} + 0x{:X})", finalOffset, baseOff, subOff);
      } else {
        logger->Warn("Could not find Camera Array Offset logic in GetCameraObjectByID. Using fallback 0x38.");
      }

      uintptr_t pDebugCameraContext = *reinterpret_cast<uintptr_t*>(pStandardManager + finalOffset);
      if (pDebugCameraContext) {
        owner.SetDebugCameraContextPtr(pDebugCameraContext);
        logger->Info("--- Found pDebugCameraContext (Array Base) at: 0x{:X}", pDebugCameraContext);
      } else { logger->Error("pDebugCameraContext is NULL at 0x{:X}", pStandardManager + finalOffset); all_found = false; }
    } else { logger->Error("StandardManager is NULL"); all_found = false; }
  } else { logger->Error("StandardManager address or GetCameraObjectByID function is NULL"); all_found = false; }

  // --- 4. Find internal offsets within DebugCamera_HandleInput and RenderInfoOverlay ---
  uintptr_t pfnHandleInput = cameraHooks.GetDebugCameraHandleInputFunc();

  if (pfnHandleInput) {
    constexpr size_t SEARCH_RANGE_HUGE = 32768;

    // 4.1 Game UI Visible (0x450)
    // Anchor: CMP byte ptr [RSI + offset], 0; SETZ AL; MOV byte ptr [RSI + offset], AL; MOVSD
    // HOW-TO-FIND: Search for Clean UI toggle logic in HandleInput.
    uintptr_t addrUI = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "80 BE ? ? ? ? ? 0F 94 C0 88 86 ? ? ? ? F2 0F 10 05");
    if (addrUI) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrUI + 2);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetGameUiVisibleOffset(off);
        logger->Info("--- Found Game UI Visible offset: 0x{:X}", off);
      } else { logger->Error("Game UI offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Warn("FAILED to find Game UI Visible anchor"); all_found = false; }

    // 4.2 HUD Visibility (0x538)
    // Anchor: CMP byte ptr [RSI + offset], BL; MOV RCX, RSI; SETZ DL
    // HOW-TO-FIND: Search for HUD Visibility read logic.
    uintptr_t addrHUDVis = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "38 9E ? ? ? ? 48 8B CE 0F 94 C2");
    if (addrHUDVis) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrHUDVis + 2);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetHudVisibleOffset(off);
        logger->Info("--- Found HUD Visible offset: 0x{:X}", off);
      } else { logger->Error("HUD Visible offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Warn("FAILED to find HUD Visible anchor"); all_found = false; }

    // 4.3 HUD Position (0x44C)
    // Anchor: MOV EDX, [RSI + offset]; MOV RCX, RSI; INC EDX; AND EDX, 3
    // HOW-TO-FIND: Search for HUD Position read logic.
    uintptr_t addrHUDPos = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "8B 96 ? ? ? ? 48 8B CE FF C2 83 E2 03");
    if (addrHUDPos) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrHUDPos + 2);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetHudPositionOffset(off);
        logger->Info("--- Found HUD Position offset: 0x{:X}", off);
      } else { logger->Error("HUD Position offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Warn("FAILED to find HUD Position anchor"); all_found = false; }

    // 4.4 Debug Camera Mode (0x454)
    // Anchor: MOV EAX, [RSI + offset]; LEA R14, [rip + ...]
    uintptr_t addrMode = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "8B 86 ? ? ? ? 4C 8D 35");
    if (addrMode) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrMode + 2);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetDebugCameraModeOffset(off);
        logger->Info("--- Found Debug Camera Mode offset: 0x{:X}", off);
      } else { logger->Error("Debug Mode offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Warn("FAILED to find Debug Camera Mode anchor"); all_found = false; }

    // 4.5 Pos/Rot Locks (0x3E0/0x3E1)
    // Anchor: CMP byte ptr [R14 + offset], R12B; ... ; BF 33 00 00 00
    uintptr_t addrLock = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "45 38 A6 ? ? ? ? 48 8D 15 ? ? ? ? BF ? ? ? ?");
    if (addrLock) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrLock + 3);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetDebugPosLockOffset(off);
        owner.SetDebugRotLockOffset(off + 1);
        logger->Info("--- Found Pos/Rot Lock offsets: 0x{:X} / 0x{:X}", off, off + 1);
      } else { logger->Error("Lock offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Warn("FAILED to find Pos/Rot Lock anchor"); all_found = false; }

    // 4.6 Orbit (0x41D)
    // Anchor: CMP byte ptr [R14 + offset], R12B; ... ; ORBIT string usage
    uintptr_t addrOrbit = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "45 38 A6 ? ? ? ? 48 8D 15 ? ? ? ? 48 8D");
    if (addrOrbit) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrOrbit + 3);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetDebugOrbitOffset(off);
        logger->Info("--- Found Orbit offset: 0x{:X}", off);
      } else { logger->Error("Orbit offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Warn("FAILED to find Orbit anchor"); all_found = false; }

    // 4.7 Selected Object (0x4A0) and Orbit Speed (0x418)
    // Anchor: MOV RCX, [R14 + offset]; MOVSS XMM6, [R14 + offset]; ADD RCX, 0x10
    uintptr_t addrSpeed = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "49 8B 8E ? ? ? ? F3 41 0F 10 B6 ? ? ? ? 48 83");
    if (addrSpeed) {
      int32_t offSel = Utils::PatternFinder::ReadInt32(addrSpeed + 3);
      int32_t offSpd = Utils::PatternFinder::ReadInt32(addrSpeed + 12);
      if (Utils::PatternFinder::IsSaneOffset(offSel) && Utils::PatternFinder::IsSaneOffset(offSpd)) {
        owner.SetDebugSelectedObjectPtrOffset(offSel);
        owner.SetDebugOrbitSpeedOffset(offSpd);
        logger->Info("--- Found SelectedObject(0x{:X}) and OrbitSpeed(0x{:X})", offSel, offSpd);
      } else { logger->Error("SelectedObj/Speed INVALID (0x{:X}/0x{:X})", offSel, offSpd); all_found = false; }
    } else { logger->Warn("FAILED to find SelectedObj/Speed anchor"); all_found = false; }

    // 4.8 Hovered Object (Actor) (0x498)
    // Anchor: CMP qword ptr [R14 + offset], 0; JZ ...; LEA RAX, "Actor"
    uintptr_t addrHover = Utils::PatternFinder::Find(pfnHandleInput, SEARCH_RANGE_HUGE, "49 83 BE ? ? ? ? 00 74 09 48 8D 05 ? ? ? ? ? ? 49 83");
    if (addrHover) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrHover + 3);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetDebugHoveredObjectPtrOffset(off);
        logger->Info("--- Found Hovered Object (Actor) offset: 0x{:X}", off);
      } else { logger->Error("Hovered Object offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Warn("FAILED to find Hovered Object anchor"); all_found = false; }

  } else {
    logger->Warn("DebugCamera_HandleInput not ready.");
    all_found = false;
  }

  m_isReady = all_found;
  if (m_isReady) {
    logger->Info("Successfully found all debug camera data dynamically.");
  } else {
    logger->Error("Failed to find one or more debug camera data.");
  }

  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
