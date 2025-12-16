#include "SPF/Data/GameData/Finders/DebugCameraAnimationDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
// Signature to find the UpdateAnimatedFlight function.
// This signature looks for a unique string referenced inside the function,
// then finds the function prologue by scanning backwards.
const char* ANIMATED_FLIGHT_FUNC_SIG = "48 89 5C 24 10 57 48 81 EC ? ? ? ? ? ? B4 24 ? ? ? ? 33 FF ? ? F6";
}  // namespace

bool DebugCameraAnimationDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) {
    return true;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Debug Camera Animation data...");

  bool timerOffsetFound = (owner.GetAnimationTimerOffset() != 0);
  if (!timerOffsetFound) {
    // Find the animation timer offset within the SetDebugCameraMode function.
    uintptr_t pfnSetDebugCameraMode = reinterpret_cast<uintptr_t>(owner.GetDebugCameraModeFunc());
    if (pfnSetDebugCameraMode) {
      // Signature targets the sequence of MOV [RBX + ?], -1.0f followed by the opcode for MOV [RBX+?], RDI
      // This is unique and does not hardcode the second offset.
      // Expected offset: 0xdd0
      const unsigned char pattern[] = {
          0xC7,
          0x83,
          '?',
          '?',
          '?',
          '?',
          0x00,
          0x00,
          0x80,
          0xBF,  // MOV [rbx+????], -1.0f
          0x48,
          0x89,
          0xBB  // MOV [rbx+????], RDI opcode
      };
      uintptr_t sig_addr = Utils::PatternFinder::Find(pfnSetDebugCameraMode, 512, pattern, sizeof(pattern));
      if (sig_addr) {
        // The offset is 2 bytes after the start of the signature (after C7 83)
        int32_t offset = *(int32_t*)(sig_addr + 2);
        owner.SetAnimationTimerOffset(offset);
        logger->Info("--- Found AnimationTimerOffset dynamically: 0x{:X}", offset);
        timerOffsetFound = true;
      } else {
        logger->Warn("Signature for AnimationTimerOffset not found within SetDebugCameraMode. Will retry...");
      }
    } else {
      logger->Warn("SetDebugCameraMode function not found. Cannot find AnimationTimerOffset. Will retry...");
    }
  }

  bool funcFound = (owner.GetUpdateAnimatedFlightFunc() != nullptr);
  if (!funcFound) {
    // The user provided the decompiled code for UpdateAnimatedFlight.
    // We can find it by searching for a unique pattern at its start.
    uintptr_t pfnUpdateAnimatedFlight = Utils::PatternFinder::Find(ANIMATED_FLIGHT_FUNC_SIG);
    if (pfnUpdateAnimatedFlight) {
      owner.SetUpdateAnimatedFlightFunc(reinterpret_cast<void*>(pfnUpdateAnimatedFlight));
      logger->Info("--- Found UpdateAnimatedFlight function at: {:#x}", pfnUpdateAnimatedFlight);
      funcFound = true;
    } else {
      logger->Warn("Signature for UpdateAnimatedFlight function not found. Will retry...");
    }
  }

  m_isReady = timerOffsetFound && funcFound;

  if (m_isReady) {
    logger->Info("Successfully found all debug camera animation data.");
  }

  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
