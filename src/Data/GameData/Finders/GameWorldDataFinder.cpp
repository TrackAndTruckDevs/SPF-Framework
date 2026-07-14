#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

bool WorldDataFinder::TryFindOffsets(GameWorldService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for GameWorld (Engine & Time) data using provided Ghidra signatures...");

  bool all_found = true;
  const size_t SEARCH_RANGE = 4096;
  uintptr_t pfnUpdateEnv = 0;  // Address of UpdateEnvironmentState
  uintptr_t addr = 0;          // General purpose address variable

  // 1. Find the entry point of the UpdateTimeAdvance function.

  /**
   * SEARCH STRATEGY (Verified for Game Version 1.60):
   * This function (UpdateTimeAdvance) handles time scrubbing during events
   * like resting, using the ferry, or using Photo Mode.
   * We locate it by searching for the time format string "%u:%02u".
   *
   * Since this common string might be used in multiple UI functions, we use a
   * context signature that matches the specific math block where seconds are
   * calculated (using the magic constant 0x88888889 for division by 60).
   *
   * Target Code Snippet (Ghidra 1.60):
   * 1415f8a64 b8 89 88 88 88     MOV        EAX,0x88888889
   * 1415f8a69 4c 8d 05 48 ad...  LEA        R8,[s_%u:%02u_1421637b8]
   * 1415f8a70 41 f7 e6           MUL        R14D
   * 1415f8a73 48 8d 4c 24 40     LEA        RCX=>local_48,[RSP + 0x40]
   *
   * After finding the string reference (Xref), we use GetFunctionStart (.pdata)
   * to accurately find the function's entry point:
   * 1415f8920 48 8b c4           MOV        RAX,RSP
   * 1415f8923 55                 PUSH       RBP
   * 1415f8924 48 81 ec 80...     SUB        RSP,0x80
   */
  const char* UPDATE_TIME_STRING = "%u:%02u";
  // Context Strategy: Match magic constant 0x88888889 followed by a flexible LEA [RIP+...]
  // 89 88 88 88      -> Magic constant (partial for 0x88888889)
  // [0-16?]          -> Gap for compiler reordering (allows 0 to 16 bytes)
  // [48-4F] 8D       -> REX (48-4F) + LEA opcode (8D)
  // [05-3D]          -> ModR/M for RIP-relative addressing (any register)
  const char* UPDATE_TIME_CONTEXT = "89 88 88 88 [0-16?] [48-4F] 8D [05-3D]";
  uintptr_t pfnUpdateTimeAdvance = Utils::PatternFinder::FindFunctionByString(UPDATE_TIME_STRING, true, UPDATE_TIME_CONTEXT);

  if (!pfnUpdateTimeAdvance) {
    logger->Error("1. Failed to find UpdateTimeAdvance function start.");
    return false;
  }
  logger->Debug("1. UpdateTimeAdvance found at 0x{:X}", pfnUpdateTimeAdvance);

  /*
   * 1.1 [OFFSET: Environment Object]
   * This offset points to the actual environment object within the manager.
   * Verified as 0x990 in v1.60.
   *
   * Ghidra 1.60 Analysis:
   * 1415f8973 [48-4F] 8B [80-BF] 90 09 00 00  MOV RBX, qword ptr [RBX + 0x990]
   * 1415f897a 49 8B C9                        MOV RCX, R9 (Marker)
   *
   * Strategy: Match MOV reg, [reg + offset32] followed by the RCX/R9 register bridge.
   */
  const char* p_obj_offset = "[48-4F] 8B [80-BF] ?? ?? ?? ?? 49 8B [C0-CF]";
  uintptr_t addrObj = Utils::PatternFinder::Find(pfnUpdateTimeAdvance, SEARCH_RANGE, p_obj_offset);
  if (addrObj) {
    int32_t envOffset = Utils::PatternFinder::ReadInt32(addrObj + 3);
    if (Utils::PatternFinder::IsSaneOffset(envOffset)) {
      owner.SetEnvObjectOffset(envOffset);
      logger->Debug("1.1 [OFFSET: Environment Object] Found: 0x{:X}", envOffset);
    } else {
      logger->Error("1.1 [OFFSET: Environment Object] Offset (0x{:X}) is insane.", envOffset);
      all_found = false;
    }
  } else {
    logger->Error("1.1 [OFFSET: Environment Object] FAILED to find signature.");
    all_found = false;
  }

  /*
   * 2. [DATA: Global Managers] (Environment & Time)
   * Strategy: Use the stable string "[used_vehicles]..." as an anchor in UpdateGameSession.
   * In version 1.60+, both Environment and TimeManager pointers are loaded sequentially.
   *
   * Ghidra 1.60 Analysis (UpdateGameSession):
   * 1408527e6 48 8d 0d ...       LEA RCX, [s_[used_vehicles]...]
   * ...
   * 1408527f5 48 8b 05 d0 24...  MOV RAX, qword ptr [DAT_143554c40] (Environment)
   * 140852803 48 8b 05 76 24...  MOV RAX, qword ptr [DAT_143554c80] (TimeManager)
   */
  const char* UNIQUE_LOG_STR = "[used_vehicles] %Iu used truck offers have expired";
  uintptr_t usedVehiclesXref = Utils::PatternFinder::FindFunctionByString(UNIQUE_LOG_STR, false);
  if (usedVehiclesXref) {
    // 2.1 [DATA: Environment Manager Pointer]
    addr = Utils::PatternFinder::Find(usedVehiclesXref, 64, "[48-4F] 8B [05-3D] ?? ?? ?? ??");
    if (addr) {
      uintptr_t envPtr = Utils::PatternFinder::GetRipAddress(addr, 3, 7);
      if (envPtr) {
        owner.SetEnvironmentBasePtr(envPtr);
        logger->Debug("2.1 [DATA: Environment Manager] Found at 0x{:X}", envPtr);

        // 2.2 [DATA: Time Manager Pointer]
        // Search for the second MOV instruction which loads the TimeManager (v1.60 behavior)
        uintptr_t addrTime = Utils::PatternFinder::Find(addr + 7, 64, "[48-4F] 8B [05-3D] ?? ?? ?? ??");
        if (addrTime) {
          uintptr_t timePtr = Utils::PatternFinder::GetRipAddress(addrTime, 3, 7);
          if (timePtr) {
            owner.SetTimeMgrPtrAddr(timePtr);
            logger->Debug("2.2 [DATA: Time Manager] Found at 0x{:X}", timePtr);
          } else {
            logger->Error("2.2 [DATA: Time Manager] Failed to resolve RIP address.");
            all_found = false;
          }
        } else {
          // Fallback for v1.59: TimeManager and EnvironmentManager share the same pointer
          owner.SetTimeMgrPtrAddr(envPtr);
          logger->Info("2.2 [DATA: Time Manager] Second MOV not found. Using Environment pointer as fallback (v1.59 style).");
        }

        // 2.3 [DATA: Environment Adjustment] detection: look for LEA reg, [reg + adjustment]
        uintptr_t addrLea = Utils::PatternFinder::Find(addr, 64, "48 8D [40-BF]");
        if (addrLea) {
          int8_t imm8 = Utils::PatternFinder::ReadInt8(addrLea + 3);
          owner.SetEnvironmentAdjustment(static_cast<intptr_t>(imm8));
          logger->Info("2.3 [DATA: Environment Adjustment] Detected: {} (via LEA)", imm8);
        }
      } else {
        logger->Error("2.1 [DATA: Environment Manager] Failed to resolve RIP address.");
        all_found = false;
      }
    } else {
      logger->Error("2.1 [DATA: Environment Manager] FAILED to find signature.");
      all_found = false;
    }
  } else {
    logger->Error("2. [DATA: Global Managers] FAILED to find unique string reference.");
    all_found = false;
  }

  // 3. Find the entry point of the UpdateSimulationTime function.
  /**
   * SEARCH STRATEGY (Updated for Game Version 1.60):
   * This function manages the in-game clock and simulation timing.
   * Between version 1.5x and 1.60, the compiler changed the prologue:
   *
   * v1.5x (Old):
   * 140418320 40 56          PUSH RSI
   * 140418322 48 83 ec 60    SUB  RSP, 0x60
   *
   * v1.60 (New):
   * 14048a3e0 40 55          PUSH RBP
   * 14048a3e2 56             PUSH RSI
   * 14048a3e3 48 81 ec a8... SUB  RSP, 0xa8
   *
   * We use a "Flexible Signature" to match both versions:
   * 40 [0-1?] 56             -> Optional PUSH RBP + PUSH RSI
   * 48 [81-83] ec [1-4?]     -> Match SUB RSP with either 1-byte or 4-byte operand
   * [3-30?]                  -> Skip variable initialization logic
   * e8 ?? ?? ?? ??           -> CALL IsSimulationPaused
   */
  const char* UPDATE_SIM_TIME_SIG = "40 [0-1?] 56 48 [81-83] ec [1-4?] [3-30?] e8 ?? ?? ?? ?? 84 c0 0f 85 ?? ?? ?? ?? [40-4F] 8b [05-3D]";
  uintptr_t pfnUpdateSimTime = Utils::PatternFinder::Find(UPDATE_SIM_TIME_SIG);

  if (pfnUpdateSimTime) {
    logger->Debug("3. [CALL: UpdateSimulationTime] Found at 0x{:X}", pfnUpdateSimTime);
  } else {
    logger->Error("3. Failed to find UpdateSimulationTime function start.");
    return false;
  }

  /*
   * 3.1 [DATA: Simulation Time Offset & CALL: UpdateEnvironmentState]
   * This logic block at the end of UpdateSimulationTime saves the updated
   * time (ms and sec) and triggers the environment state refresh.
   *
   * Ghidra 1.60 Analysis (Address: 14048a5c9):
   * 14048a5c9 f3 41 0f 11 b9 7c 3e 00 00  MOVSS dword ptr [R9 + 0x3e7c], XMM7
   * 14048a5d2 41 89 81 78 3e 00 00        MOV dword ptr [R9 + 0x3e78], EAX
   * 14048a5d9 e8 12 9e 04 00              CALL UpdateEnvironmentState
   *
   * Chain Strategy:
   * 1. MOVSS [reg+off], XMM -> F3 [40-4F] 0F 11 [80-BF] ?? ?? ?? ??
   * 2. MOV   [reg+off], EAX -> [40-4F] 89 [80-BF] ?? ?? ?? ??
   * 3. CALL  UpdateState    -> E8
   */
  const std::vector<std::string> timeUpdateChain = {"F3 [40-4F] 0F 11 [80-BF] ?? ?? ?? ??", "[40-4F] 89 [80-BF] ?? ?? ?? ??", "E8 ?? ?? ?? ??"};

  addr = Utils::PatternFinder::FindChain(timeUpdateChain, 16, pfnUpdateSimTime);
  if (addr) {
    // Find the MOV instruction within the chain (14048a5d2)
    uintptr_t addrMov = Utils::PatternFinder::Find(addr, 32, "[40-4F] 89 [80-BF] ?? ?? ?? ??");
    if (addrMov) {
      int32_t timeOffset = Utils::PatternFinder::ReadInt32(addrMov + 3);
      if (Utils::PatternFinder::IsSaneOffset(timeOffset)) {
        owner.SetTimeOffset(timeOffset);
        logger->Debug("3.1 [DATA: Simulation Time Offset] Found: 0x{:X}", timeOffset);
      }
    }

    // Find the CALL instruction (14048a5d9)
    uintptr_t addrCall = Utils::PatternFinder::Find(addr, 32, "E8");
    if (addrCall) {
      uintptr_t pfnUpdateEnv_found = Utils::PatternFinder::GetRipAddress(addrCall, 1, 5);
      if (pfnUpdateEnv_found) {
        owner.SetUpdateFnAddr(pfnUpdateEnv_found);
        logger->Debug("3.1 [CALL: UpdateEnvironmentState] Found at 0x{:X}", pfnUpdateEnv_found);
      }
    }
  } else {
    logger->Error("3.1 [DATA: Simulation Time] FAILED to find logic chain.");
    all_found = false;
  }

  const size_t SIM_TIME_SEARCH_RANGE = 1024;

  /*
   * 3.2 [OFFSET: Simulation Time] (Verified for v1.59 & v1.60)
   * This offset reads the current simulation time (seconds) from the Time object.
   * v1.59: MOV [reg+0x15C] happens BEFORE magic constant 0x6C16C16D.
   * v1.60: MOV [reg+0x19C] happens AFTER magic constant 0x6C16C16D.
   *
   * Ghidra 1.60 Analysis (Address: 14048a448):
   * 14048a3e0 40 55              PUSH RBP             <- Anchor 1 (Start)
   * ...
   * 14048a448 b8 6d c1 16 6c     MOV EAX, 0x6C16C16D  <- Anchor 2 (Constant)
   * 14048a44d 44 8b bd 9c 01...  MOV R15D, dword ptr [RBP + 0x19c] <- Target
   *
   * Strategy: Find the constant first, then look for the MOV instruction
   * in the immediate vicinity (both directions). This ensures compatibility
   * with both 1.59 and 1.60 instruction ordering.
   */
  uintptr_t addrConst = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, "B8 6D C1 16 6C");
  if (addrConst) {
    // Search in a +/- 32 byte window around the constant
    uintptr_t searchStart = (addrConst > 32) ? (addrConst - 32) : pfnUpdateSimTime;
    uintptr_t addrMov = Utils::PatternFinder::Find(searchStart, 128, "[40-4F] 8B [80-BF] ?? ?? ?? ??");

    if (addrMov) {
      int32_t simTimeOff = Utils::PatternFinder::ReadInt32(addrMov + 3);
      if (Utils::PatternFinder::IsSaneOffset(simTimeOff)) {
        owner.SetSimulationTimeOffset(simTimeOff);
        logger->Debug("3.2 [OFFSET: Simulation Time] Found: 0x{:X}", simTimeOff);
      } else {
        logger->Error("3.2 [OFFSET: Simulation Time] Offset (0x{:X}) is insane.", simTimeOff);
        all_found = false;
      }
    } else {
      logger->Error("3.2 [OFFSET: Simulation Time] FAILED to find MOV instruction around constant.");
      all_found = false;
    }
  } else {
    logger->Error("3.2 [OFFSET: Simulation Time] FAILED to find magic constant.");
    all_found = false;
  }

  /*
   * 3.3 [OFFSET: Sub-Minute Seconds] (Eternal Signature)
   * This signature tracks where sub-minute seconds are added and then saved.
   *
   * Ghidra 1.60 Analysis:
   * 14048a4d7 f3 0f 58 8d a0 01 00 00  ADDSS  XMM1, dword ptr [RBP + 0x1a0]
   * 14048a4df 41 0f 2f c8              COMISS XMM1, XMM8
   * 14048a4e3 f3 0f 11 8d a0 01 00 00  MOVSS  dword ptr [RBP + 0x1a0], XMM1
   * 14048a4eb 73 05                    JNC    LAB_14048a4f2
   *
   * Strategy: Match ADDSS [Reg + offset] followed by float comparison (COMISS).
   * This core logic of adding delta time and checking for minute overflow is stable.
   */
  const char* p_sub_sec_off = "F3 0F 58 [80-BF] ?? ?? ?? ?? [0-16?] 0F 2F";
  addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, p_sub_sec_off);
  if (addr) {
    int32_t subSecOff = Utils::PatternFinder::ReadInt32(addr + 4);
    if (Utils::PatternFinder::IsSaneOffset(subSecOff)) {
      owner.SetSubMinuteSecondsOffset(subSecOff);
      logger->Debug("3.3 [OFFSET: Sub-Minute Seconds] Found: 0x{:X}", subSecOff);
    } else {
      logger->Error("3.3 [OFFSET: Sub-Minute Seconds] (0x{:X}) is insane.", subSecOff);
      all_found = false;
    }
  } else {
    logger->Error("3.3 [OFFSET: Sub-Minute Seconds] FAILED to find signature.");
    all_found = false;
  }

  /*
   * 3.4 [OFFSET: Map Scale] (Eternal Signature)
   * This multiplier converts real seconds to game seconds (e.g. 1:19).
   *
   * Ghidra 1.60 Analysis:
   * 14048a490 f3 44 0f 10 96 b4 31 00 00  MOVSS XMM10, [RSI + 0x31b4]
   * 14048a499 41 d1 ec                   SHR R12D, 0x1
   *
   * Strategy: Match a floating-point operation (MOVSS 0F 10 or MULSS 0F 59)
   * followed by a bitwise shift right (SHR C1/D1). This math sequence
   * for game time scaling is a core engine characteristic.
   */
  const std::vector<std::string> mapScaleChain = {"F3 [40-4F] 0F [10-59] [80-BF] ?? ?? ?? ??", "[40-4F] [C1-D1] [E8-EF]"};

  addr = Utils::PatternFinder::FindChain(mapScaleChain, 10, pfnUpdateSimTime);
  if (addr) {
    // Extract 32-bit offset from MOVSS/MULSS
    // Instruction: F3 REX 0F OP ModRM [OFFSET] (Offset is at +5)
    int32_t scaleOff = Utils::PatternFinder::ReadInt32(addr + 5);
    if (Utils::PatternFinder::IsSaneOffset(scaleOff)) {
      owner.SetMapScaleOffset(scaleOff);
      logger->Debug("3.4 [OFFSET: Map Scale] Found: 0x{:X}", scaleOff);
    } else {
      logger->Error("3.4 [OFFSET: Map Scale] Offset (0x{:X}) is insane.", scaleOff);
      all_found = false;
    }
  } else {
    logger->Error("3.4 [OFFSET: Map Scale] FAILED to find logic chain.");
    all_found = false;
  }

  /*
   * 3.5 [OFFSET: Real Play Time (Min & Sec)] (Eternal Signature)
   * Strategy: Find the seconds update and then search nearby for the minute update.
   * v1.59 uses INC [reg+off] (FF 86), v1.60 uses LEA [reg+off] (48 8D).
   *
   * Ghidra 1.60 Analysis:
   * 14048a6fc f3 0f 58 85 94 1b 00 00  ADDSS XMM0, [RBP + 0x1b94] (Sec)
   * 14048a704 0f 2f 05 fd e0 f4 01     COMISS XMM0, [DAT_1423d8808]
   * ...
   * 14048a740 48 8d 8d 98 1b 00 00     LEA RCX, [RBP + 0x1b98]    (Min)
   */
  const char* p_real_sec_sig = "0F 58 [80-BF] ?? ?? ?? ?? 0F 2F 05";
  addr = Utils::PatternFinder::Find(pfnUpdateSimTime, 1024, p_real_sec_sig);
  if (addr) {
    // Check if there is a prefix (F3 or REX) before 0F
    uint8_t prefix = *(uint8_t*)(addr - 1);
    int32_t realSecOff = 0;

    if (prefix == 0xF3 || (prefix >= 0x40 && prefix <= 0x4F)) {
      realSecOff = Utils::PatternFinder::ReadInt32(addr + 3);
    } else {
      realSecOff = Utils::PatternFinder::ReadInt32(addr + 2);
    }

    // Search forward for minute update (LEA 48 8D or INC FF)
    // We use two simple patterns for maximum reliability
    uintptr_t addrMin = Utils::PatternFinder::Find(addr, 160, "48 8D [80-BF] ?? ?? ?? ??");
    if (!addrMin) addrMin = Utils::PatternFinder::Find(addr, 160, "FF [80-BF] ?? ?? ?? ??");

    if (addrMin) {
      uint8_t minOp = *(uint8_t*)addrMin;
      int opOff = (minOp == 0x48) ? 3 : 2;
      int32_t realMinOff = Utils::PatternFinder::ReadInt32(addrMin + opOff);

      if (Utils::PatternFinder::IsSaneOffset(realSecOff) && Utils::PatternFinder::IsSaneOffset(realMinOff)) {
        owner.SetRealPlaySecondsOffset(realSecOff);
        owner.SetRealPlayTimeOffset(realMinOff);
        logger->Debug("3.5 [OFFSET: Real Play Time] Found Sec: 0x{:X}, Min: 0x{:X}", realSecOff, realMinOff);
      } else {
        logger->Error("3.5 [OFFSET: Real Play Time] Offsets (Sec: 0x{:X}, Min: 0x{:X}) are insane.", realSecOff, realMinOff);
        all_found = false;
      }
    } else {
      logger->Error("3.5 [OFFSET: Real Play Time] FAILED to find minute instruction.");
      all_found = false;
    }
  } else {
    logger->Error("3.5 [OFFSET: Real Play Time] FAILED to find seconds signature.");
    all_found = false;
  }

  /*
   * 3.6 [OFFSET: Real Delta Time] (Updated for v1.60)
   * Strategy: Match the conversion of ticks to double (0F 2A) followed by the time constant.
   * Ghidra 1.60: 14048a437 (CVTSI2SD 0xB28) ... 14048a448 (MOV EAX, 0x6C16C16D)
   */
  const char* p_delta_sig = "0F 2A [80-BF] ?? ?? ?? ?? [0-16?] B8 6D C1 16 6C";
  addr = Utils::PatternFinder::Find(pfnUpdateSimTime, 1024, p_delta_sig);
  if (addr) {
    // addr points to 0F. Offset is at addr + 3.
    int32_t deltaTimeOff = Utils::PatternFinder::ReadInt32(addr + 3);
    if (Utils::PatternFinder::IsSaneOffset(deltaTimeOff)) {
      owner.SetRealDeltaTimeOffset(deltaTimeOff);
      logger->Debug("3.6 [OFFSET: Real Delta Time] Found: 0x{:X}", deltaTimeOff);
    } else {
      logger->Error("3.6 [OFFSET: Real Delta Time] Offset (0x{:X}) is insane.", deltaTimeOff);
      all_found = false;
    }
  } else {
    logger->Error("3.6 [OFFSET: Real Delta Time] FAILED to find signature.");
    all_found = false;
  }

  /*
   * 3.7 [OFFSET: Skybox Auto-update] (Eternal Signature)
   * Strategy: Match the load of Environment object into R9 followed by a comparison.
   * v1.59 uses CMP [R9+off], 0 (83 B9), v1.60 uses CMP [R9+off], reg (39 B1).
   *
   * Ghidra 1.60 Analysis:
   * 14048a5ab 4C 8B 88 90 09 00 00  MOV R9, qword ptr [RAX + 0x990]
   * 14048a5b2 45 39 B1 0C 47 00 00  CMP dword ptr [R9 + 0x470C], R14D
   */
  const char* p_skybox_sig = "4C 8B [80-BF] ?? ?? ?? ?? [41-45] [39-83] [80-BF]";
  addr = Utils::PatternFinder::Find(pfnUpdateSimTime, 1024, p_skybox_sig);
  if (addr) {
    // Find the start of the CMP instruction (it follows the 7-byte MOV)
    uintptr_t addrCmp = addr + 7;
    int32_t skyboxOff = Utils::PatternFinder::ReadInt32(addrCmp + 3);
    if (Utils::PatternFinder::IsSaneOffset(skyboxOff)) {
      owner.SetSkyboxAutoUpdateOffset(skyboxOff);
      logger->Debug("3.7 [OFFSET: Skybox Auto-update] Found: 0x{:X}", skyboxOff);
    } else {
      logger->Error("3.7 [OFFSET: Skybox Auto-update] Offset (0x{:X}) is insane.", skyboxOff);
      all_found = false;
    }
  } else {
    logger->Error("3.7 [OFFSET: Skybox Auto-update] FAILED to find signature.");
    all_found = false;
  }

  // 4. Find the entry point of the CoreEngine_UpdateLoop function (Verified for v1.60).
  /**
   * SEARCH STRATEGY (Updated for Game Version 1.60):
   * This is the main heart-beat loop of the engine.
   * We locate it via the unique log string related to shader time reset.
   *
   * Ghidra 1.60 Analysis (Address: 140416a70):
   * 140416a70 48 8b c4           MOV RAX, RSP
   * ...
   * 1404170b2 48 8d 0d c7 f0 cf 01  LEA RCX, [s_Forcing_restart_shader_time()...]
   * 1404170b9 e8 c2 14 ce ff        CALL LogFunction
   *
   * Note: This function contains critical offsets for Warp, Pause, and Delta Time.
   */
  const char* CORE_LOOP_STRING = "Forcing restart_shader_time() to avoid inaccu";
  uintptr_t pfnCoreEngineLoop = Utils::PatternFinder::FindFunctionByString(CORE_LOOP_STRING, true);

  if (!pfnCoreEngineLoop) {
    logger->Error("4. Failed to find CoreEngine_UpdateLoop function start.");
    return false;
  }
  logger->Debug("4. [CALL: CoreEngine_UpdateLoop] Found at 0x{:X}", pfnCoreEngineLoop);

  const size_t CORE_SEARCH_RANGE = 4096;

  /*
   * 4.1 [OFFSET: Global Warp] (Verified for v1.59 & v1.60)
   * This multiplier controls the overall speed of the game engine (console "warp").
   * Strategy: Match the load of the warp factor followed by the load of constant 1.0f.
   *
   * Ghidra 1.60 Analysis (Address: 140416f1e):
   * 140416f1e f3 41 0f 10 97 8c 08 00 00  MOVSS XMM2, dword ptr [R15 + 0x88c]
   * 140416f27 f3 44 0f 10 0d 18 0f fc 01  MOVSS XMM9, dword ptr [DAT_1423d7e48] ; Value 1.0f
   *
   * Chain Strategy:
   * 1. MOVSS XMM?, [R15 + offset] -> F3 41 0F 10 [80-BF] ?? ?? ?? ??
   * 2. MOVSS XMM?, [RIP + offset] -> F3 [40-4F] 0F 10 0D ?? ?? ?? ??
   */
  const std::vector<std::string> warpChain = {"F3 41 0F 10 [80-BF] ?? ?? ?? ??", "F3 [40-4F] 0F 10 0D ?? ?? ?? ??"};

  addr = Utils::PatternFinder::FindChain(warpChain, 10, pfnCoreEngineLoop);
  if (addr) {
    // Extract 32-bit offset from the first instruction in the chain
    // Instruction: F3 41 0F 10 ModRM [OFFSET] (Offset is at +5)
    int32_t warpOff = Utils::PatternFinder::ReadInt32(addr + 5);
    if (Utils::PatternFinder::IsSaneOffset(warpOff)) {
      owner.SetGlobalWarpOffset(warpOff);
      logger->Debug("4.1 [OFFSET: Global Warp] Found: 0x{:X}", warpOff);
    } else {
      logger->Error("4.1 [OFFSET: Global Warp] Offset (0x{:X}) is insane.", warpOff);
      all_found = false;
    }
  } else {
    logger->Error("4.1 [OFFSET: Global Warp] FAILED to find logic chain.");
    all_found = false;
  }

  /*
   * 4.2 [OFFSET: Pause Status] (Updated for v1.60)
   * Strategy: Match the toggle logic (CMP -> JZ -> MOV).
   * Added flexibility for intermediate register moves and REX prefixes.
   *
   * Ghidra 1.60 Analysis (Address: 140416f06):
   * 140416f06 41 38 97 91 0a 00 00  CMP byte ptr [R15 + 0xa91], DL
   * 140416f0d 74 0f                 JZ  LAB_140416f1e
   * 140416f0f 49 8b cf              MOV RCX, R15
   * 140416f12 41 88 97 91 0a 00 00  MOV byte ptr [R15 + 0xa91], DL
   */
  const char* p_pause_sig = "41 38 [80-BF] ?? ?? ?? ?? 74 ?? [40-4F] 8B [C0-CF] [40-4F] 88 [80-BF]";
  addr = Utils::PatternFinder::Find(pfnCoreEngineLoop, 4096, p_pause_sig);
  if (addr) {
    // Offset is at byte 3 of the first instruction (CMP)
    int32_t pauseOff = Utils::PatternFinder::ReadInt32(addr + 3);
    if (Utils::PatternFinder::IsSaneOffset(pauseOff)) {
      owner.SetPauseStatusOffset(pauseOff);
      logger->Debug("4.2 [OFFSET: Pause Status] Found: 0x{:X}", pauseOff);
    } else {
      logger->Error("4.2 [OFFSET: Pause Status] Offset (0x{:X}) is insane.", pauseOff);
      all_found = false;
    }
  } else {
    logger->Error("4.2 [OFFSET: Pause Status] FAILED to find signature block.");
    all_found = false;
  }

  /*
   * 5. [OFFSET: Engine Halt Counters] (Updated for v1.60)
   * These counters control the "frozen" state of the engine sub-systems.
   * Strategy: Match the entire complex decrement and state check logic block.
   *
   * Ghidra 1.60 Analysis (Address: 1403ea7a1):
   * 1403ea7a1 ff 8f ac 0a 00 00  DEC dword ptr [RDI + 0xaac] ; Traffic Halt
   * 1403ea7a7 ff 8f a8 0a 00 00  DEC dword ptr [RDI + 0xaa8] ; Simulation Halt
   * 1403ea7ad 83 e8 01           SUB EAX, 0x1
   * 1403ea7b0 89 87 a0 0a 00 00  MOV dword ptr [RDI + 0xaa0], EAX ; Global Halt
   * 1403ea7cb e8 20 e7 02 00     CALL FUN_140418ef0
   *
   * Master Signature Strategy:
   * Link all three decrements and the follow-up logic into one flexible pattern.
   */
  const char* HALT_COUNTERS_SIG =
    "FF [88-8F] ?? ?? ?? ?? "          // 1. DEC [reg + traffic_off]
    "FF [88-8F] ?? ?? ?? ?? "          // 2. DEC [reg + sim_off]
    "83 [E8-EF] 01 "                   // 3. SUB reg, 1
    "89 [80-BF] ?? ?? ?? ?? "          // 4. MOV [reg + global_off], reg
    "75 [0-100?] "                     // 5. JNZ (Jump over)
    "[40-45] 38 [80-BF] ?? ?? ?? ?? "  // 6. CMP [reg + off], reg8
    "74 [0-32?] "                      // 7. JZ
    "[48-4F] 8B [C0-CF] "              // 8. MOV RCX, RDI (Register bridge)
    "[40-45] 88 [80-BF] ?? ?? ?? ?? "  // 9. MOV [reg + off], reg8
    "E8";                              // 10. CALL (Final anchor)

  addr = Utils::PatternFinder::Find(HALT_COUNTERS_SIG);
  if (addr) {
    // Extract 32-bit offsets from the first instructions
    int32_t trafficOff = Utils::PatternFinder::ReadInt32(addr + 2);
    int32_t simOff = Utils::PatternFinder::ReadInt32(addr + 8);
    int32_t globalOff = Utils::PatternFinder::ReadInt32(addr + 17);

    if (Utils::PatternFinder::IsSaneOffset(trafficOff) && Utils::PatternFinder::IsSaneOffset(simOff) && Utils::PatternFinder::IsSaneOffset(globalOff)) {
      owner.SetTrafficHaltOffset(trafficOff);
      owner.SetSimulationHaltOffset(simOff);
      owner.SetGlobalHaltOffset(globalOff);
      logger->Debug("5. [OFFSET: Engine Halt] Found Global: 0x{:X}, Sim: 0x{:X}, Traffic: 0x{:X}", globalOff, simOff, trafficOff);
    } else {
      logger->Error("5. [OFFSET: Engine Halt] One or more offsets (0x{:X}, 0x{:X}, 0x{:X}) are insane.", globalOff, simOff, trafficOff);
      all_found = false;
    }
  } else {
    logger->Error("5. [OFFSET: Engine Halt] FAILED to find master signature.");
    all_found = false;
  }

  m_isReady = all_found;
  if (m_isReady) logger->Info("GameWorldDataFinder: All critical offsets found. Service is READY.");

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
