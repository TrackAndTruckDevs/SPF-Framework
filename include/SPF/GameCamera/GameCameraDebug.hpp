#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/GameCamera/DebugCameraMode.hpp"
#include "SPF/GameCamera/DebugHudPosition.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace GameCamera {
class GameCameraDebug {
 public:
  GameCameraDebug();

  void SetEnabled(bool enabled);
  bool GetEnabled(bool* out_isEnabled) const;

  void SetMode(DebugCameraMode mode);
  bool GetCurrentMode(DebugCameraMode* out_mode) const;

  // --- HUD & UI ---
  void SetHudVisible(bool visible);
  bool GetHudVisible(bool* out_isVisible) const;
  void SetHudPosition(DebugHudPosition position);
  bool GetHudPosition(DebugHudPosition* out_position) const;
  void SetGameUiVisible(bool visible);
  bool GetGameUiVisible(bool* out_isVisible) const;

 private:
  mutable DebugCameraMode m_currentMode = DebugCameraMode::SIMPLE;
};
}  // namespace GameCamera
SPF_NS_END
