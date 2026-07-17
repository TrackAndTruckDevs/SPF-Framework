#include "SPF/Data/GameData/Finders/DebugCameraAnimationDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

namespace {
/*
 * Anchor #1: Animation Timer Offset
 * Found inside SetDebugCameraMode:
 * MOV dword ptr [RBX + offset], -1.0f (C7 83 ...)
 * MOV qword ptr [RBX + offset], RDI   (48 89 BB ...)
 */
const char* ANIMATION_TIMER_SIG = "C7 83 ?? ?? ?? ?? ?? ?? ?? ?? 48 89 BB ?? ?? ?? ??";

/*
 * Anchor #2: UpdateAnimatedFlight function
 * Signature for the function prologue and initial register setup.
 */
const char* ANIMATED_FLIGHT_FUNC_SIG = "48 89 5C ?? ?? 57 48 81 EC ?? ?? ?? ?? ?? ?? B4 24 ?? ?? ?? ?? 33 FF ?? ?? F6";
}  // namespace

bool DebugCameraAnimationDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) {
    return true;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Debug Camera Animation data (Dynamic Search)...");

  bool timerOffsetFound = (owner.GetAnimationTimerOffset() != 0);
  if (!timerOffsetFound) {
    uintptr_t pfnSetDebugCameraMode = reinterpret_cast<uintptr_t>(owner.GetDebugCameraModeFunc());
    if (pfnSetDebugCameraMode) {
      uintptr_t sig_addr = Utils::PatternFinder::Find(pfnSetDebugCameraMode, 512, ANIMATION_TIMER_SIG);
      if (sig_addr) {
        // Offset is at byte 2 of the instruction: C7 83 [OFFSET]
        int32_t offset = Utils::PatternFinder::ReadInt32(sig_addr + 2);
        if (Utils::PatternFinder::IsSaneOffset(offset)) {
          owner.SetAnimationTimerOffset(offset);
          logger->Debug("Anchor #1: AnimationTimerOffset = 0x{:X}", offset);
          timerOffsetFound = true;
        } else {
          logger->Error("Anchor #1: AnimationTimerOffset INVALID (0x{:X})", offset);
        }
      } else {
        logger->Warn("Anchor #1: FAILED to find AnimationTimer signature in SetDebugCameraMode");
      }
    } else {
      logger->Warn("SetDebugCameraMode function not found in owner. Cannot search for timer offset.");
    }
  }

  bool funcFound = (owner.GetUpdateAnimatedFlightFunc() != nullptr);
  if (!funcFound) {
    uintptr_t pfnUpdateAnimatedFlight = Utils::PatternFinder::Find(ANIMATED_FLIGHT_FUNC_SIG);
    if (pfnUpdateAnimatedFlight) {
      owner.SetUpdateAnimatedFlightFunc(reinterpret_cast<void*>(pfnUpdateAnimatedFlight));
      logger->Debug("Anchor #2: UpdateAnimatedFlight found at 0x{:X}", pfnUpdateAnimatedFlight);
      funcFound = true;
    } else {
      logger->Warn("Anchor #2: FAILED to find UpdateAnimatedFlight function signature globally");
    }
  }

  m_isReady = timerOffsetFound && funcFound;
  if (m_isReady) {
    logger->Info("Successfully found all Debug Camera Animation data.");
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
