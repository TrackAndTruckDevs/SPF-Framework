#include "SPF/Data/GameData/Finders/ManagerCoreDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>


SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Unique string anchor to locate the function that loads GameplayManager.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
 * 140854981  48 8D 05 20 30 94 01          LEA RAX,[0x1421979a8] = "/ui/cargo_load_screen.sii"
 */
const char* GAMEPLAY_MANAGER_STR = "/ui/cargo_load_screen.sii";

/**
 * @brief Pattern to find the MOV that loads pGameplayManager from its .data slot.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
 * 140854945  48 8B 05 F4 02 D0 02          MOV RAX,qword ptr [0x143554c40]
 */
const char* GAMEPLAY_MANAGER_SIG = "[MOV r64, [rip+off32]]";

/**
 * @brief Pattern to find the MOV that loads the Environment object from GameplayManager.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
 * 140854952  48 8B 88 90 09 00 00          MOV RCX,qword ptr [RAX + 0x990]
 */
const char* ENV_OBJECT_OFFSET_SIG = "[MOV r64, [r64+off32]]";

/**
 * @brief Unique string anchor to locate the function that loads CameraManager.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14067e2c0[14067e2c0]) ---/
 * 14067ea8a  48 8D 15 D7 5B AE 01          LEA RDX,[0x142164668] = "unknown camera mode '%s'"
 */
const char* CAMERA_MANAGER_STR = "unknown camera mode '%s'";

/**
 * @brief Pattern to find the MOV that loads pCameraManager from its .data slot.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14067e2c0[14067e2c0]) ---/
 * 14067eaac  48 8B 0D F5 61 ED 02          MOV RCX,qword ptr [0x143554ca8]
 */
const char* CAMERA_MANAGER_SIG = "[MOV r64, [rip+off32]]";

}  // namespace

bool ManagerCoreDataFinder::TryFindOffsets(ManagerCoreService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  // ── Phase 1: GameplayManager ──
  {
    auto phase = log.MakePhase("GameplayManager");

    uintptr_t stringXref = PatternFinder::FindFunctionByString(GAMEPLAY_MANAGER_STR, true);
    if (phase.Step(stringXref, "Gameplay manager string XREF", "REF")) {
      uintptr_t addrMov = PatternFinder::Find(stringXref, 32, GAMEPLAY_MANAGER_SIG);
      if (phase.Step(addrMov, "GameplayManager MOV", "RT")) {
        uintptr_t gameplayManager = PatternFinder::GetRipAddress(addrMov, 3, 7);
        phase.Step(gameplayManager, "GameplayManager", "DATA");
        if (PatternFinder::IsValidAddress(gameplayManager)) owner.SetGameplayManagerAddr(gameplayManager);

        uintptr_t addrEnv = PatternFinder::Find(addrMov, 24, ENV_OBJECT_OFFSET_SIG);
        if (phase.Step(addrEnv, "Environment Object MOV", "RT")) {
          int32_t envOffset = PatternFinder::ReadInt32(addrEnv + 3);
          if (phase.StepOffset(envOffset, "Environment Object offset", "OFF")) {
            owner.SetEnvObjectOffset(envOffset);
          }
        }
      }
    }
  }

  // ── Phase 2: CameraManager ──
  {
    auto phase = log.MakePhase("CameraManager");

    uintptr_t stringXref = PatternFinder::FindFunctionByString(CAMERA_MANAGER_STR, false);
    if (phase.Step(stringXref, "Camera manager string XREF", "REF")) {
      uintptr_t addrMov = PatternFinder::Find(stringXref, 64, CAMERA_MANAGER_SIG);
      if (phase.Step(addrMov, "CameraManager MOV", "RT")) {
        uintptr_t cameraManager = PatternFinder::GetRipAddress(addrMov, 3, 7);
        phase.Step(cameraManager, "CameraManager", "DATA");
        if (PatternFinder::IsValidAddress(cameraManager)) owner.SetCameraManagerAddr(cameraManager);
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = PatternFinder::IsValidAddress(owner.GetGameplayManagerAddr()) && PatternFinder::IsValidAddress(owner.GetCameraManagerAddr());

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
