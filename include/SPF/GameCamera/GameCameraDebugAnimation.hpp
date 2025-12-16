#pragma once

#include "SPF/Namespace.hpp"
#include <cstdint>

SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraDebugAnimation
 * @brief Provides a high-level, clean API for controlling the debug camera animation system.
 */
class GameCameraDebugAnimation {
 public:
  enum class PlaybackState { Stopped, Playing, Paused };

  GameCameraDebugAnimation();

  /**
   * @brief Updates the animation playback state. Should be called every frame.
   * @param dt Delta time from the last frame.
   */
  void Update(float dt);

  /**
   * @brief Plays the animation.
   * If paused, it resumes from the current position.
   * If stopped, it starts from the given index (or 0 if invalid).
   * @param startIndex The index of the state to start from. If -1, resumes from pause or starts from the beginning.
   */
  void Play(int startIndex = -1);

  /**
   * @brief Pauses the animation at its current position.
   */
  void Pause();

  /**
   * @brief Stops the animation and resets the camera to normal control.
   */
  void Stop();

  /**
   * @brief Jumps to a specific frame and pauses the animation.
   * @param frameIndex The index of the camera state to move to.
   */
  void GoToFrame(int frameIndex);

  /**
   * @brief Scrubs the animation to a specific point in time and applies the interpolated state.
   * @param position The exact position on the timeline (e.g., 5.3 for 30% between frame 5 and 6).
   */
  void ScrubTo(float position);

  /**
   * @brief Sets the playback direction.
   * @param isReversed True to play in reverse, false for forward.
   */
  void SetReverse(bool isReversed);

  // --- Getters for UI/API consumers ---
  PlaybackState GetPlaybackState() const;
  int GetCurrentFrame() const;
  float GetCurrentFrameProgress() const;  // Returns the interpolation progress [0.0, 1.0)
  bool IsReversed() const;

 private:
  // --- Native Function Pointers ---
  using UpdateAnimatedFlightFunc = void(__fastcall*)(uintptr_t, float);
  UpdateAnimatedFlightFunc m_pfnUpdateAnimatedFlight = nullptr;

  // --- State ---
  PlaybackState m_state = PlaybackState::Stopped;
  bool m_isReversed = false;

  /**
   * @brief Performs the one-time setup to take control of the animation system.
   * @return True on success, false if essential pointers are missing.
   */
  bool PrepareForPlayback();

  /**
   * @brief Gets the pointer to the main debug camera object from the game's context.
   * @return A valid pointer, or 0 if not found.
   */
  uintptr_t GetDebugCameraObject() const;

  /**
   * @brief The core logic for updating the animation, callable internally.
   */
  void UpdateAnimation(float dt);
};
}  // namespace GameCamera
SPF_NS_END
