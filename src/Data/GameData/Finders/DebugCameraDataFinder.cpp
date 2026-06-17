#include "SPF/Data/GameData/Finders/DebugCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {

// String anchor for the SetDebugCameraMode function.
// This function updates the camera mode and logs the change to the console.
// Ghidra Reference (SetDebugCameraMode v1.60+ @ 1405443a0):
// 1405444a0 48 8d 0d 79 c5 be 01    LEA RCX,[s_Camera_mode_set_to_'%s'_142130a20]
const char* DEBUG_MODE_SET_STR = "Camera mode set to '%s'";

// String anchor for the DebugCamera_SetHUDVisibility function.
// Ghidra Reference (DebugCamera_SetHUDVisibility v1.60+ @ 1405416e0):
// 14054174c 48 8d 05 85 e7 be 01    LEA RAX,[s_debug_camera_hud_14212fed8]
const char* DEBUG_HUD_SCRIPT_STR = "debug_camera_hud";

// String anchor for the DebugCamera_HandleInput function.
// Ghidra Reference (DebugCamera_HandleInput v1.60+ @ 14053e7b0):
// 14053efb6 48 8d 0d 6b 0d bf 01    LEA RCX,[s_Camera_roll_speed:_%.2f_14212fd28]
const char* DEBUG_ROLL_SPEED_STR = "Camera roll speed: %.2f";

// String anchor for the DebugCamera_SetSelectedActor function.
// Ghidra Reference (DebugCamera_SetSelectedActor v1.60+ @ 140544230):
// 14054429f 48 8d 0d 7a c6 be 01    LEA RCX,[s_Selected_actor:_ID:_%u_142130920]
const char* SELECTED_ACTOR_STR = "Selected actor: ID: %u";

// String anchor for the Position Lock toggle log.
// Ghidra Reference (v1.60+ @ 140542a97):
// 140542a97 48 8d 15 d2 d7 be 01    LEA RDX,[s_<color_value=%02X00ffff>pos_lock_142130270]
const char* POS_LOCK_STR = "<color value=%02X00ffff>pos lock";

// String anchor for the Orbit Mode toggle log.
// Ghidra Reference (v1.60+ @ 140542bc7):
// 140542bc7 48 8d 15 fa d9 be 01    LEA RDX,[s_<color_value=%02X00ffff>orbit_1421305c8]
const char* ORBIT_STR = "<color value=%02X00ffff>orbit";

// String anchor for the Orbit Speed info log.
// Ghidra Reference (v1.60+ @ 140542c92):
// 140542c92 48 8d 15 cf d8 be 01    LEA RDX,[s_<color_value=ff00ffff>%.2f/%.2f_1421305e8]
const char* ORBIT_SPEED_STR = "<color value=ff00ffff>%.2f/%.2f";

// String anchor for the Hovered Object (Actor) type.
// Ghidra Reference (v1.60+ @ 140542878):
// 140542878 48 8d 05 b1 db be 01    LEA RAX,[s_Actor_142130430]
const char* ACTOR_STR = "Actor";

}  // namespace

bool DebugCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Debug Camera data (Dynamic Search)...");

  bool all_found = true;

  // --- 1. Find standalone functions ---

  // 1.1 Find SetDebugCameraMode function
  /*
   * Logic: 
   * Use the unique log string "Camera mode set to '%s'" to locate the function.
   * 
   * Ghidra Reference (SetDebugCameraMode v1.60+ @ 1405443a0):
   * 1405444a0 48 8d 0d 79 c5 be 01    LEA RCX,[s_Camera_mode_set_to_'%s'_142130a20]
   */
  uintptr_t pfnSetMode = Utils::PatternFinder::FindFunctionByString(DEBUG_MODE_SET_STR, true);
  if (pfnSetMode) {
    owner.SetDebugCameraModeFunc(reinterpret_cast<void*>(pfnSetMode));
    logger->Info("--- Found SetDebugCameraMode at: 0x{:X}", pfnSetMode);

    // 1.1.0 Find Debug Camera Mode offset inside SetDebugCameraMode
    /*
     * Logic:
     * At the beginning of SetDebugCameraMode, it compares the current mode with the new one.
     * 1405443af 39 91 5c 04 00 00    CMP  dword ptr [RCX + 0x45c],EDX
     * 
     * Pattern: 39 [80-BF]
     * - 39: CMP opcode
     * - [80-BF]: ModR/M byte for [REG + 32-bit displacement]
     */
    uintptr_t addrModeSet = Utils::PatternFinder::Find(pfnSetMode, 64, "39 [80-BF]");
    if (addrModeSet) {
      int32_t offset = Utils::PatternFinder::ReadInt32(addrModeSet + 2);
      if (Utils::PatternFinder::IsSaneOffset(offset)) {
        owner.SetDebugCameraModeOffset(offset);
        logger->Info("--- Found Debug Camera Mode offset (via SetMode): 0x{:X}", offset);
      } else { logger->Error("Debug Mode offset (via SetMode) INVALID (0x{:X})", offset); }
    } else { logger->Warn("FAILED to find Debug Camera Mode offset anchor inside SetDebugCameraMode"); }

    // 1.1.1 Find CVar object and GetAndCacheValue function inside SetDebugCameraMode
    /*
     * Logic: 
     * We search for the fundamental architectural pattern of CVar retrieval:
     * 1. LEA [REG], [RIP+...] (Load CVar object pointer)
     * 2. CALL GetAndCacheValue
     * 3. TEST ...             (Validate return value)
     * 
     * Pattern: 48 8D [0D-3D] ?? ?? ?? ?? E8 ?? ?? ?? ?? 85
     * - [0D-3D]: Allows LEA to use any primary register (RCX, RDX, R8, R9, etc.)
     * - 85: Validates that a TEST instruction exists.
     * 
     * Ghidra Reference (SetDebugCameraMode v1.60+ @ 1405443c0):
     * 1405443c0 48 8d 0d e9 e1 6a 02    LEA RCX,[PTR_PTR_142bf25b0]
     * 1405443c7 e8 84 3c c8 ff          CALL GetAndCacheValue
     * 1405443cc 85 c0                   TEST EAX,EAX
     */
    uintptr_t cvarScan = Utils::PatternFinder::Find(pfnSetMode, 128, "48 8D [0D-3D] ?? ?? ?? ?? E8 ?? ?? ?? ?? 85");
    if (cvarScan) {
      uintptr_t base_ptr = Utils::PatternFinder::GetRipAddress(cvarScan, 3, 7);
      uintptr_t pfnGetAndCache = Utils::PatternFinder::GetRipAddress(cvarScan + 7, 1, 5);

      if (base_ptr && pfnGetAndCache) {
        owner.SetCacheableCvarObjectPtr(base_ptr);
        logger->Info("--- Found Cacheable CVar Object: 0x{:X}", base_ptr);
        logger->Info("--- Found GetAndCacheValue at: 0x{:X}", pfnGetAndCache);

        // 1.1.2 Find the internal value offset within GetAndCacheValue function
        /*
         * Logic: 
         * We look for the epilogue of GetAndCacheValue where it stores the result and the cache flag:
         * 1. MOV [REG + offset], EAX   (Store the retrieved CVar value)
         * 2. MOV [REG + offset-2], 1   (Set the 'is_cached' flag)
         * 3. ADD RSP, ...              (Stack cleanup)
         * 
         * Pattern: 89 [80-BF] ?? ?? ?? ?? C6 [80-BF] ?? ?? ?? ?? 01 48 83 C4
         * - 89 [80-BF]: MOV with 32-bit displacement for any primary register (RDI, RSI, RBX, etc.)
         * - C6 ... 01:  Store constant 1 into the flag byte.
         * - 48 83 C4:   Standard stack pointer adjustment.
         * 
         * Ghidra Reference (GetAndCacheValue v1.60+ @ 1401c8107):
         * 1401c8107 89 87 18 01 00 00    MOV  dword ptr [RDI + 0x118],EAX
         * 1401c810d c6 87 16 01 00 00 01 MOV  byte ptr [RDI + 0x116],0x1
         * 1401c8114 48 83 c4 20          ADD  RSP,0x20
         */
        uintptr_t sig_off = Utils::PatternFinder::Find(pfnGetAndCache, 512, "89 [80-BF] ?? ?? ?? ?? C6 [80-BF] ?? ?? ?? ?? ?? 48");
        if (sig_off) {
          int32_t offset = Utils::PatternFinder::ReadInt32(sig_off + 2);
          if (Utils::PatternFinder::IsSaneOffset(offset)) {
            owner.SetCvarValueOffset(offset);
            logger->Info("--- Found CVar Value Offset: 0x{:X}", offset);
          } else { logger->Error("CVar Value Offset INVALID (0x{:X})", offset); all_found = false; }
        } else { logger->Error("FAILED to find CVar Value Offset epilogue inside GetAndCacheValue"); all_found = false; }
      } else { logger->Error("FAILED to resolve CVar Object or Cache Function RIP from SetDebugCameraMode"); all_found = false; }
    } else { logger->Error("FAILED to find CVar discovery sequence inside SetDebugCameraMode"); all_found = false; }
  } else { 
    logger->Error("FAILED to find SetDebugCameraMode string anchor '{}'", DEBUG_MODE_SET_STR); 
    all_found = false; 
  }


  // 1.2 Find Game UI Visible (Clean UI) offset
  /*
   * Logic: 
   * 1. Find the unique string anchor "Camera roll speed: %.2f".
   * 2. Find the instruction that loads this string (XREF).
   * 3. From that XREF, search backward for the Clean UI toggle logic.
   * 
   * Pattern: 80 [B0-BF] ?? ?? ?? ?? 00 0F
   * - 80 [B0-BF]: CMP with 32-bit displacement for any register (typically RSI/RBX).
   * - 00: Check against zero.
   * - 0F: Start of next instruction (SETZ is 0F 94 C0).
   * 
   * Ghidra Reference (v1.60+ @ 14053ed61):
   * 14053ed61 80 be 58 04 00 00 00    CMP  byte ptr [RSI + 0x458],0x0
   * 14053ed68 0f 94 c0                SETZ AL
   */
  uintptr_t rollStrAddr = Utils::PatternFinder::FindString(DEBUG_ROLL_SPEED_STR);
  if (rollStrAddr) {
    auto xrefs = Utils::PatternFinder::FindXrefs(rollStrAddr);
    if (!xrefs.empty()) {
      uintptr_t addrUI = Utils::PatternFinder::FindBackward(xrefs[0], 2048, "80 [B0-BF] ?? ?? ?? ?? 00 0F");
      if (addrUI) {
        int32_t off = Utils::PatternFinder::ReadInt32(addrUI + 2);
        if (Utils::PatternFinder::IsSaneOffset(off)) {
          owner.SetGameUiVisibleOffset(off);
          logger->Info("--- Found Game UI Visible offset: 0x{:X}", off);
        } else { logger->Error("Game UI offset INVALID (0x{:X})", off); all_found = false; }
        } else { logger->Error("FAILED to find Game UI Visible anchor via backward search from 0x{:X}", xrefs[0]); all_found = false; }

        // 1.2.2 Find PosLock, RotLock, and OrbitMode functions and offsets
        /*
         * Logic: 
         * Further down in HandleInput, there is a sequence of three UI toggles:
         * 1. 14053f162 Position Lock toggle (CMP [RSI+0x3E8], R13B / ... / CALL SetPositionLock)
         * 2. 14053f186 Rotation Lock toggle (CMP [RSI+0x3E9], R13B / ... / CALL SetRotationLock)
         * 3. 14053f1aa Orbit Mode toggle    (CMP [RSI+0x425], R13B / ... / CALL SetOrbitMode)
         * 
         * All use the same structural pattern: 44 38 [80-BF] ?? ?? ?? ?? 48 8B [C8-CF] 0F 94 [C0-C7] E8
         */
        uintptr_t scanPos = xrefs[0];
        const char* togglePattern = "44 38 [80-BF] ?? ?? ?? ?? 48 8B [C8-CF] 0F 94 [C0-C7] E8";

        // --- 1. SetPositionLock ---
        uintptr_t posLockCall = Utils::PatternFinder::Find(scanPos, 2048, togglePattern);
        if (posLockCall) {
          int32_t off = Utils::PatternFinder::ReadInt32(posLockCall + 3);
          uintptr_t pfn = Utils::PatternFinder::GetRipAddress(posLockCall + 11, 1, 5);
          if (Utils::PatternFinder::IsSaneOffset(off) && pfn) {
            owner.SetDebugPosLockOffset(off);
            owner.SetSetPositionLockFunc(reinterpret_cast<void*>(pfn));
            logger->Info("--- Found SetPositionLock at 0x{:X} (Offset: 0x{:X})", pfn, off);
            scanPos = posLockCall + 16;
          } else { logger->Error("SetPositionLock data INVALID"); all_found = false; }
        } else { logger->Error("FAILED to find SetPositionLock call site"); all_found = false; }

        // --- 2. SetRotationLock ---
        uintptr_t rotLockCall = Utils::PatternFinder::Find(scanPos, 512, togglePattern);
        if (rotLockCall) {
          int32_t off = Utils::PatternFinder::ReadInt32(rotLockCall + 3);
          uintptr_t pfn = Utils::PatternFinder::GetRipAddress(rotLockCall + 11, 1, 5);
          if (Utils::PatternFinder::IsSaneOffset(off) && pfn) {
            owner.SetDebugRotLockOffset(off);
            owner.SetSetRotationLockFunc(reinterpret_cast<void*>(pfn));
            logger->Info("--- Found SetRotationLock at 0x{:X} (Offset: 0x{:X})", pfn, off);
            scanPos = rotLockCall + 16;
          } else { logger->Error("SetRotationLock data INVALID"); all_found = false; }
        } else { logger->Error("FAILED to find SetRotationLock call site"); all_found = false; }

        // --- 3. SetOrbitMode ---
        uintptr_t orbitCall = Utils::PatternFinder::Find(scanPos, 512, togglePattern);
        if (orbitCall) {
          int32_t off = Utils::PatternFinder::ReadInt32(orbitCall + 3);
          uintptr_t pfn = Utils::PatternFinder::GetRipAddress(orbitCall + 11, 1, 5);
          if (Utils::PatternFinder::IsSaneOffset(off) && pfn) {
            owner.SetDebugOrbitOffset(off);
            owner.SetSetOrbitModeFunc(reinterpret_cast<void*>(pfn));
            logger->Info("--- Found SetOrbitMode at 0x{:X} (Offset: 0x{:X})", pfn, off);
          } else { logger->Error("SetOrbitMode data INVALID"); all_found = false; }
        } else { logger->Error("FAILED to find SetOrbitMode call site"); all_found = false; }

      // 1.2.3 Find HUD Visibility (0x540) and HUD Position (0x454) offsets
      /*
       * Logic: 
       * Both HUD visibility and position are updated at the end of HandleInput.
       * We find the call sites for SetHUDVisibility and SetHUDPosition, then search backward.
       */
      uintptr_t pfnSetHudVis = reinterpret_cast<uintptr_t>(owner.GetSetHudVisibilityFunc());
      uintptr_t pfnSetHudPos = reinterpret_cast<uintptr_t>(owner.GetSetDebugHudPositionFunc());

      if (pfnSetHudVis && pfnSetHudPos) {
        // Search for call sites inside HandleInput
        uintptr_t fnStart = Utils::PatternFinder::GetFunctionStart(xrefs[0]);
        uintptr_t fnEnd = Utils::PatternFinder::GetFunctionEnd(fnStart);

        if (fnStart && fnEnd) {
          // --- HUD Visibility offset ---
          // Anchor: CMP byte ptr [REG + offset], BL / MOV RCX, RSI / SETZ DL / CALL SetHUDVisibility
          // 14054160d Pattern: 38 [80-BF] ?? ?? ?? ?? 48 8B [C8-CF] 0F 94 [C0-C7] E8
          auto visXrefs = Utils::PatternFinder::FindXrefs(pfnSetHudVis);
          for (uintptr_t xref : visXrefs) {
            if (xref >= fnStart && xref <= fnEnd) {
              uintptr_t addr = Utils::PatternFinder::FindBackward(xref, 64, "38 [80-BF]");
              if (addr) {
                int32_t off = Utils::PatternFinder::ReadInt32(addr + 2);
                if (Utils::PatternFinder::IsSaneOffset(off)) {
                  owner.SetHudVisibleOffset(off);
                  logger->Info("--- Found HUD Visible offset: 0x{:X}", off);
                  break;
                }
              }
            }
          }
          if (owner.GetHudVisibleOffset() == 0) { logger->Error("FAILED to find HUD Visible offset in HandleInput"); all_found = false; }

          // --- HUD Position offset ---
          // Anchor: MOV EDX, [REG + offset] / MOV RCX, RSI / INC EDX / AND EDX, 3 / CALL SetHUDPosition
          //140541632 Pattern: 8B [90-97] ?? ?? ?? ?? 48 8B [C8-CF] FF [C0-C7] 83 [E0-E7] 03 E8
          auto posXrefs = Utils::PatternFinder::FindXrefs(pfnSetHudPos);
          for (uintptr_t xref : posXrefs) {
            if (xref >= fnStart && xref <= fnEnd) {
              uintptr_t addr = Utils::PatternFinder::FindBackward(xref, 64, "8B [90-97]");
              if (addr) {
                int32_t off = Utils::PatternFinder::ReadInt32(addr + 2);
                if (Utils::PatternFinder::IsSaneOffset(off)) {
                  owner.SetHudPositionOffset(off);
                  logger->Info("--- Found HUD Position offset: 0x{:X}", off);
                  break;
                }
              }
            }
          }
          if (owner.GetHudPositionOffset() == 0) { logger->Error("FAILED to find HUD Position offset in HandleInput"); all_found = false; }
        }
      }
    } else { logger->Error("FAILED to find XREFs for ROLL_SPEED string"); all_found = false; }
  } else { logger->Error("FAILED to find ROLL_SPEED string anchor '{}'", DEBUG_ROLL_SPEED_STR); all_found = false; }

  // 2.1 Find DebugCamera_SetSelectedActor function
    /*
   * Logic: 
   * 1. Find the unique log string "Selected actor: ID: %u".
   * 2. Find the instruction that loads this string (XREF).
   * 3. Trace back to the start of the function (DebugCamera_SetSelectedActor).
   * 
   * Ghidra Reference (DebugCamera_SetSelectedActor v1.60+ @ 140544230):
   * 14054429f 48 8d 0d 7a c6 be 01    LEA RCX,[s_Selected_actor:_ID:_%u_142130920]
   */
  uintptr_t pfnSelected = Utils::PatternFinder::FindFunctionByString(SELECTED_ACTOR_STR, true);
  if (pfnSelected) {
    owner.SetSetSelectedActorFunc(reinterpret_cast<void*>(pfnSelected));
    logger->Info("--- Found SetSelectedActor at: 0x{:X}", pfnSelected);
  } else { logger->Error("FAILED to find SetSelectedActor via string anchor '{}'", SELECTED_ACTOR_STR); all_found = false; }

  // 2.2 Find Pos/Rot Lock offsets (0x3E8/0x3E9) via string anchor
  /*
   * Logic:
   * 1. Find the local usage of string "<color value=%02X00ffff>pos lock" inside HandleInput.
   * 2. Search backward for the CMP instruction that checks the lock state.
   * 
   * Ghidra Reference (v1.60+ @ 140542a90):
   * 140542a90 45 38 a6 e8 03 00 00    CMP  byte ptr [R14 + 0x3e8],R12B
   * 140542a97 48 8d 15 d2 d7 be 01    LEA  RDX,[s_<color_value=%02X00ffff>pos_lock_142130270]
   */
  uintptr_t posLockUsage = Utils::PatternFinder::FindFunctionByString(POS_LOCK_STR, false);
  if (posLockUsage) {
    uintptr_t addrLock = Utils::PatternFinder::FindBackward(posLockUsage, 64, "45 [38-39] [86-8E]");
    if (addrLock) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrLock + 3);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetDebugPosLockOffset(off);
        owner.SetDebugRotLockOffset(off + 1);
        logger->Info("--- Found Pos/Rot Lock offsets: 0x{:X} / 0x{:X}", off, off + 1);
      } else { logger->Error("Pos/Rot Lock offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Error("FAILED to find CMP anchor backward from '{}' usage", POS_LOCK_STR); all_found = false; }
  } else { logger->Error("FAILED to find local usage of string anchor '{}'", POS_LOCK_STR); all_found = false; }

  // 2.4 Find Orbit offset (0x425) via string anchor
  /*
   * Logic:
   * 1. Find the local usage of string "<color value=%02X00ffff>orbit" inside HandleInput.
   * 2. Search backward for the CMP instruction that checks the orbit state.
   * 
   * Ghidra Reference (v1.60+ @ 140542bc0):
   * 140542bc0 45 38 a6 25 04 00 00    CMP  byte ptr [R14 + 0x425],R12B
   * 140542bc7 48 8d 15 fa d9 be 01    LEA  RDX,[s_<color_value=%02X00ffff>orbit_1421305c8]
   */
  uintptr_t orbitUsage = Utils::PatternFinder::FindFunctionByString(ORBIT_STR, false);
  if (orbitUsage) {
    uintptr_t addrOrbit = Utils::PatternFinder::FindBackward(orbitUsage, 64, "45 [38-39] [86-8E]");
    if (addrOrbit) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrOrbit + 3);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetDebugOrbitOffset(off);
        logger->Info("--- Found Orbit offset (via string): 0x{:X}", off);
      } else { logger->Error("Orbit offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Error("FAILED to find CMP anchor backward from '{}' usage", ORBIT_STR); all_found = false; }
  } else { logger->Error("FAILED to find local usage of string anchor '{}'", ORBIT_STR); all_found = false; }

  // 2.5 Find Selected Object (0x4A8) and Orbit Speed (0x420) offsets via string anchor
  /*
   * Logic:
   * 1. Find the local usage of string "<color value=ff00ffff>%.2f/%.2f" inside HandleInput.
   * 2. Search backward for the instruction sequence that loads the object and speed.
   * 
   * Ghidra Reference (v1.60+ @ 140542c6a):
   * 140542c6a 49 8b 8e a8 04 00 00    MOV    RCX, qword ptr [R14 + 0x4a8]
   * 140542c71 f3 41 0f 10 b6 20 04 00 00 MOVSS  XMM6, dword ptr [R14 + 0x420]
   * ...
   * 140542c92 48 8d 15 cf d8 be 01    LEA    RDX, [s_<color_value=ff00ffff>%.2f/%.2f_1421305e8]
   * 
   * Pattern: 49 8B [88-8F] ?? ?? ?? ?? F3 41 0F 10 [B0-B7]
   */
  uintptr_t orbitSpeedUsage = Utils::PatternFinder::FindFunctionByString(ORBIT_SPEED_STR, false);
  if (orbitSpeedUsage) {
    uintptr_t addr = Utils::PatternFinder::FindBackward(orbitSpeedUsage, 128, "49 8B [88-8F]");
    if (addr) {
      int32_t offSel = Utils::PatternFinder::ReadInt32(addr + 3);
      int32_t offSpd = Utils::PatternFinder::ReadInt32(addr + 12);
      if (Utils::PatternFinder::IsSaneOffset(offSel) && Utils::PatternFinder::IsSaneOffset(offSpd)) {
        owner.SetDebugSelectedObjectPtrOffset(offSel);
        owner.SetDebugOrbitSpeedOffset(offSpd);
        logger->Info("--- Found SelectedObject(0x{:X}) and OrbitSpeed(0x{:X})", offSel, offSpd);
      } else { logger->Error("SelectedObj/Speed offset INVALID (0x{:X}/0x{:X})", offSel, offSpd); all_found = false; }
    } else { logger->Error("FAILED to find MOV anchor backward from '{}' usage", ORBIT_SPEED_STR); all_found = false; }
  } else { logger->Error("FAILED to find local usage of string anchor '{}'", ORBIT_SPEED_STR); all_found = false; }

  // 2.6 Find Hovered Object (Actor) offset (0x4A0) via string anchor
  /*
   * Logic:
   * 1. Find the local usage of string "Actor" inside HandleInput.
   * 2. Search backward for the CMP instruction that checks the hovered object state.
   * 
   * Ghidra Reference (v1.60+ @ 14054286e):
   * 14054286e 49 83 be a0 04 00 00 00 CMP  qword ptr [R14 + 0x4a0],0x0
   * 140542876 74 09                   JZ   LAB_140542881
   * 140542878 48 8d 05 b1 db be 01    LEA  RAX,[s_Actor_142130430]
   * 
   * Pattern: 49 83 [B8-BF]
   */
  uintptr_t actorUsage = Utils::PatternFinder::FindFunctionByString(ACTOR_STR, false);
  if (actorUsage) {
    uintptr_t addrHover = Utils::PatternFinder::FindBackward(actorUsage, 64, "49 83 [B8-BF]");
    if (addrHover) {
      int32_t off = Utils::PatternFinder::ReadInt32(addrHover + 3);
      if (Utils::PatternFinder::IsSaneOffset(off)) {
        owner.SetDebugHoveredObjectPtrOffset(off);
        logger->Info("--- Found Hovered Object (Actor) offset (via string): 0x{:X}", off);
      } else { logger->Error("Hovered Object offset INVALID (0x{:X})", off); all_found = false; }
    } else { logger->Error("FAILED to find CMP anchor backward from '{}' usage", ACTOR_STR); all_found = false; }
  } else { logger->Error("FAILED to find local usage of string anchor '{}'", ACTOR_STR); all_found = false; }

  /*
   * Logic: 
   * 1. Find the unique string anchor "debug_camera_hud".
   * 2. Find the instruction that loads this string (XREF).
   * 3. Trace back to the start of the function (DebugCamera_SetHUDVisibility).
   * 
   * Ghidra Reference (DebugCamera_SetHUDVisibility v1.60+ @ 1405416e0):
   * 14054174c 48 8d 05 85 e7 be 01    LEA RAX,[s_debug_camera_hud_14212fed8]
   */
  uintptr_t pfnSetHudVis = Utils::PatternFinder::FindFunctionByString(DEBUG_HUD_SCRIPT_STR, true);
  if (pfnSetHudVis) {
    owner.SetSetHudVisibilityFunc(reinterpret_cast<void*>(pfnSetHudVis));
    logger->Info("--- Found SetHudVisibility at: 0x{:X}", pfnSetHudVis);

    // 2.3 Find DebugCamera_SetHUDPosition function
    /*
     * Logic: 
     * Inside SetHUDVisibility, there is a sequence of two calls:
     * 1. CALL DebugCamera_SetHUDPosition
     * 2. MOV RCX, RBX (Reload camera object)
     * 3. CALL DebugCamera_RenderInfoOverlay
     * 
     * Pattern: E8 ?? ?? ?? ?? 48 8B [C8-CF] E8
     * Ghidra Reference (SetHUDVisibility v1.60+ @ 1405417f8):
     * 1405417f8 e8 33 15 00 00    CALL DebugCamera_SetHUDPosition
     * 1405417fd 48 8b cb          MOV  RCX,RBX
     * 140541800 e8 9b 04 00 00    CALL DebugCamera_RenderInfoOverlay
     */
    uintptr_t callSite = Utils::PatternFinder::Find(reinterpret_cast<uintptr_t>(owner.GetSetHudVisibilityFunc()), 1024, "E8 ?? ?? ?? ?? 48 8B [C8-CF] E8");
    if (callSite) {
      uintptr_t pfnSetHudPos = Utils::PatternFinder::GetRipAddress(callSite, 1, 5);
      if (pfnSetHudPos) {
        owner.SetSetDebugHudPositionFunc(reinterpret_cast<void*>(pfnSetHudPos));
        logger->Info("--- Found SetDebugHudPosition at: 0x{:X}", pfnSetHudPos);

        // 2.3.1 Find HUD Position Offset (0x454) inside SetDebugHudPosition
        // Ghidra Reference (v1.60+ @ 140542d4f):
        // 140542d4f 44 8b 8b 54 04 00 00    MOV R9D, dword ptr [RBX + 0x454]
        // Pattern: 44 8B [80-BF]
        uintptr_t addrPos = Utils::PatternFinder::Find(pfnSetHudPos, 128, "44 8B [80-BF]");
        if (addrPos) {
          int32_t off = Utils::PatternFinder::ReadInt32(addrPos + 3);
          if (Utils::PatternFinder::IsSaneOffset(off)) {
            owner.SetHudPositionOffset(off);
            logger->Info("--- Found HUD Position offset (via SetHudPos): 0x{:X}", off);
          }
        }

        // 2.3.2 Find HUD Visibility Object Offset (0x538) inside SetDebugHudPosition
        // Ghidra Reference (v1.60+ @ 140542d3f):
        // 140542d3f 48 8b 89 38 05 00 00    MOV RCX, qword ptr [RCX + 0x538]
        // Pattern: 48 8B [80-BF]
        uintptr_t addrVis = Utils::PatternFinder::Find(pfnSetHudPos, 32, "48 8B [80-BF]");
        if (addrVis) {
          int32_t off = Utils::PatternFinder::ReadInt32(addrVis + 3);
          if (Utils::PatternFinder::IsSaneOffset(off)) {
            owner.SetHudVisibleOffset(off);
            logger->Info("--- Found HUD Visibility offset (via SetHudPos): 0x{:X}", off);
          }
        }
      } else { 
        logger->Error("FAILED to resolve SetDebugHudPosition RIP from call site"); 
        all_found = false; 
      }
    } else { 
      logger->Warn("FAILED to find SetDebugHudPosition call site inside SetHUDVisibility"); 
      all_found = false; 
    }
  } else { 
    logger->Error("FAILED to find SetHudVisibility via string anchor '{}'", DEBUG_HUD_SCRIPT_STR); 
    all_found = false; 
  }

  // --- 3. Find the pDebugCamera context pointer dynamically ---
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnGetCamObj = reinterpret_cast<uintptr_t>(cameraHooks.GetGetCameraObjectFunc());
  
  // GetCameraManager() handles the pointer dereferencing and version-specific adjustments (like v1.59).
  uintptr_t pStandardManager = owner.GetCameraManager();

  if (pStandardManager && pfnGetCamObj) {
      /*
       * HOW-TO-FIND Camera Array Offset:
       * We look inside GetCameraObjectByID function.
       * It adds a base offset to the Camera Manager and then reads the array pointer.
       * 
       * Assembly (v1.60+ @ 1404f01f4):
       * 1404f01f4 48 83 c1 30       - ADD  RCX, 0x30
       * 1404f01f8 4c 3b 41 10       - CMP  R8, [RCX + 0x10]
       * ...
       * 1404f01fe 48 8b 41 08       - MOV  RAX, [RCX + 0x08]
       * 
       * Pattern: 48 83 [C0-C7] ?? 4C 3B [40-47] ?? 73 ?? 48 8B [40-47]
       * 
       * STRUCTURE OF THE CAMERA ARRAY (pDebugCameraContext at StandardManager + 0x38):
       * This is a pointer to an array of pointers (context) managed by the game engine.
       * Each offset corresponds to a GameCameraType ID:
       * 
       * +0x00: DeveloperFreeCamera (ID 0)
       * +0x08: BehindCamera        (ID 1)
       * +0x10: InteriorCamera      (ID 2)
       * +0x18: BumperCamera        (ID 3)
       * +0x20: WindowCamera        (ID 4)
       * +0x28: CabinCamera         (ID 5)
       * +0x30: WheelCamera         (ID 6)
       * +0x38: TopCamera           (ID 7)
       * +0x40: ???                 (ID 8)
       * +0x48: TVCamera            (ID 9)
       * +0x50: ???                 (ID 10)
       * +0x58: ???                 (ID 11)
       * +0x60: ???                 (ID 12)
       * +0x68: PhotoCamera         (ID 13)
       */
      const char* p_array_logic = "48 83 [C0-C7] ?? 4C 3B [40-47] ?? 73 ?? 48 8B [40-47]";
      uintptr_t addr = Utils::PatternFinder::Find(pfnGetCamObj, 128, p_array_logic);
      
      if (addr) {
        int8_t baseOff = Utils::PatternFinder::ReadInt8(addr + 3);
        int8_t subOff = Utils::PatternFinder::ReadInt8(addr + 13);
        int32_t finalOffset = static_cast<int32_t>(baseOff) + static_cast<int32_t>(subOff);

        uintptr_t pDebugCameraContext = *reinterpret_cast<uintptr_t*>(pStandardManager + finalOffset);
        if (pDebugCameraContext) {
          owner.SetDebugCameraContextPtr(pDebugCameraContext);
          logger->Info("--- Found Camera Array Offset: 0x{:X} (Base: 0x{:X}, Sub: 0x{:X})", finalOffset, baseOff, subOff);
          logger->Info("--- Found pDebugCameraContext (Array Base) at: 0x{:X}", pDebugCameraContext);
        } else {
          logger->Error("pDebugCameraContext is NULL at 0x{:X}", pStandardManager + finalOffset);
          all_found = false;
        }
        } else {
        logger->Error("FAILED to find Camera Array Offset logic in GetCameraObjectByID");
        all_found = false;
        }

    } else {
      logger->Error("StandardManager or GetCameraObjectByID function is NULL");
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
