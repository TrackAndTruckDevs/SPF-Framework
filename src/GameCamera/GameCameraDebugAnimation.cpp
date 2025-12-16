#include "SPF/GameCamera/GameCameraDebugAnimation.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/GameCamera/GameCameraManager.hpp"  // Required for ApplyState
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraDebugAnimation::GameCameraDebugAnimation() {
  // This is now initialized in the Update method to ensure it's ready when needed.
}

void GameCameraDebugAnimation::Update(float dt) {
  if (m_state == PlaybackState::Playing) {
    UpdateAnimation(dt);
  }
}

void GameCameraDebugAnimation::Play(int startIndex) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugAnimation");
  int currentFrame = GetCurrentFrame();

  // Case 1: Resume from pause. This happens if we are paused and the UI wants to play from the same frame we are on.
  if (m_state == PlaybackState::Paused && startIndex == currentFrame) {
    m_state = PlaybackState::Playing;
    logger->Info("Animation playback resumed from frame {} progress {:.2f}", GetCurrentFrame(), GetCurrentFrameProgress());
    return;
  }

  // Case 2: Start fresh (from Stopped, or from a new frame).
  // This logic is now simplified: the UI tells us where to start, and we do it.
  logger->Info("Starting playback from frame: {}", startIndex);
  GoToFrame(startIndex);             // This sets the index and resets the timer to 0.0
  m_state = PlaybackState::Playing;  // And now we play.
}

void GameCameraDebugAnimation::Pause() {
  if (m_state == PlaybackState::Playing) {
    m_state = PlaybackState::Paused;
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugAnimation");
    logger->Info("Animation playback paused.");
  }
}

void GameCameraDebugAnimation::Stop() {
  if (m_state == PlaybackState::Stopped) {
    return;
  }

  m_state = PlaybackState::Stopped;
  uintptr_t pDebugCamera = GetDebugCameraObject();
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  intptr_t modeOffset = dataService.GetDebugCameraModeOffset();
  intptr_t currentIndexOffset = dataService.GetStateCurrentIndexOffset();
  intptr_t timerOffset = dataService.GetAnimationTimerOffset();

  if (pDebugCamera) {
    if (modeOffset) *(int*)(pDebugCamera + modeOffset) = 0;                  // Revert to a safe, default camera mode (SIMPLE)
    if (currentIndexOffset) *(int*)(pDebugCamera + currentIndexOffset) = 0;  // Reset index to 0
    if (timerOffset) *(float*)(pDebugCamera + timerOffset) = 0.0f;           // Reset timer to 0.0
  }
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugAnimation");
  logger->Info("Animation playback stopped and reset.");
}

void GameCameraDebugAnimation::GoToFrame(int frameIndex) {
  if (m_state == PlaybackState::Stopped) {
    if (!PrepareForPlayback()) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraDebugAnimation");
      logger->Error("GoToFrame failed: could not prepare for playback.");
      return;
    }
  }

  uintptr_t pDebugCamera = GetDebugCameraObject();
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  intptr_t currentIndexOffset = dataService.GetStateCurrentIndexOffset();
  intptr_t stateCountOffset = dataService.GetStateCountOffset();
  intptr_t timerOffset = dataService.GetAnimationTimerOffset();

  if (!pDebugCamera || !currentIndexOffset || !stateCountOffset || !timerOffset) {
    return;
  }

  int stateCount = *(int*)(pDebugCamera + stateCountOffset);
  int finalIndex = frameIndex;

  // Validate the index
  if (finalIndex < 0 || finalIndex >= stateCount) {
    finalIndex = 0;
  }

  *(int*)(pDebugCamera + currentIndexOffset) = finalIndex;
  *(float*)(pDebugCamera + timerOffset) = 0.0f;

  // Apply the state visually so the user sees the jump immediately
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->ApplyState(finalIndex);
  }

  m_state = PlaybackState::Paused;  // Go to a frame implies pausing there
}

void GameCameraDebugAnimation::ScrubTo(float position) {
  if (m_state == PlaybackState::Stopped) {
    if (!PrepareForPlayback()) {
      return;  // Cannot scrub if we can't even prepare the system
    }
  }

  uintptr_t pDebugCamera = GetDebugCameraObject();
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  intptr_t currentIndexOffset = dataService.GetStateCurrentIndexOffset();
  intptr_t stateCountOffset = dataService.GetStateCountOffset();
  intptr_t timerOffset = dataService.GetAnimationTimerOffset();

  if (!pDebugCamera || !currentIndexOffset || !stateCountOffset || !timerOffset) {
    return;
  }

  int stateCount = *(int*)(pDebugCamera + stateCountOffset);
  if (stateCount <= 1) return;

  // Clamp position to valid range
  if (position < 0.0f) position = 0.0f;
  if (position > static_cast<float>(stateCount - 1)) position = static_cast<float>(stateCount - 1);

  int index = static_cast<int>(position);
  float progress = position - index;

  *(int*)(pDebugCamera + currentIndexOffset) = index;
  *(float*)(pDebugCamera + timerOffset) = progress;

  // Force the animation function to calculate and apply the interpolated state for this exact moment
  UpdateAnimation(0.0f);

  m_state = PlaybackState::Paused;  // Scrubbing always leaves the state paused
}

void GameCameraDebugAnimation::SetReverse(bool isReversed) { m_isReversed = isReversed; }

GameCameraDebugAnimation::PlaybackState GameCameraDebugAnimation::GetPlaybackState() const { return m_state; }

int GameCameraDebugAnimation::GetCurrentFrame() const {
  uintptr_t pDebugCamera = GetDebugCameraObject();
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  intptr_t currentIndexOffset = dataService.GetStateCurrentIndexOffset();

  if (pDebugCamera && currentIndexOffset) {
    return *(int*)(pDebugCamera + currentIndexOffset);
  }
  return 0;
}

float GameCameraDebugAnimation::GetCurrentFrameProgress() const {
  uintptr_t pDebugCamera = GetDebugCameraObject();
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  intptr_t timerOffset = dataService.GetAnimationTimerOffset();

  if (pDebugCamera && timerOffset) {
    return *(float*)(pDebugCamera + timerOffset);
  }
  return 0.0f;
}

bool GameCameraDebugAnimation::IsReversed() const { return m_isReversed; }

bool GameCameraDebugAnimation::PrepareForPlayback() {
  uintptr_t pDebugCamera = GetDebugCameraObject();
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();

  intptr_t modeOffset = dataService.GetDebugCameraModeOffset();
  auto pfnSetHudVisibility = reinterpret_cast<void(__fastcall*)(uintptr_t, bool)>(dataService.GetSetHudVisibilityFunc());

  if (!pDebugCamera || !modeOffset || !pfnSetHudVisibility) {
    return false;
  }

  // Set mode to a safe dummy value (SIMPLE) to avoid conflicts with the game's main update loop
  *(int*)(pDebugCamera + modeOffset) = 0;

  // Hide the standard debug HUD for a clean view
  pfnSetHudVisibility(pDebugCamera, false);

  return true;
}

uintptr_t GameCameraDebugAnimation::GetDebugCameraObject() const {
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pDebugCameraContext = dataService.GetDebugCameraContextPtr();
  if (!pDebugCameraContext) {
    return 0;
  }

  // The context pointer itself contains the pointer to the actual object.
  return *(uintptr_t*)pDebugCameraContext;
}

void GameCameraDebugAnimation::UpdateAnimation(float dt) {
  uintptr_t pDebugCamera = GetDebugCameraObject();
  auto& dataService = Data::GameData::GameDataCameraService::GetInstance();
  m_pfnUpdateAnimatedFlight = reinterpret_cast<UpdateAnimatedFlightFunc>(dataService.GetUpdateAnimatedFlightFunc());
  intptr_t modeOffset = dataService.GetDebugCameraModeOffset();

  if (!pDebugCamera || !m_pfnUpdateAnimatedFlight || !modeOffset) {
    Stop();  // Stop playback if something is wrong
    return;
  }

  // Swap to the ANIMATED mode just for the duration of the call
  int originalMode = *(int*)(pDebugCamera + modeOffset);
  *(int*)(pDebugCamera + modeOffset) = 4;  // ANIMATED

  float final_dt = m_isReversed ? -dt : dt;
  m_pfnUpdateAnimatedFlight(pDebugCamera, final_dt);

  // Revert to the previous mode immediately after
  *(int*)(pDebugCamera + modeOffset) = originalMode;
}
}  // namespace GameCamera
SPF_NS_END
