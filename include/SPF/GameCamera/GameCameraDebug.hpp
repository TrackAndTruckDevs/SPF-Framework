#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/GameCamera/DebugCameraMode.hpp"
#include "SPF/GameCamera/DebugHudPosition.hpp"

#include <cstdint>
// #include <vector>
// #include <string>

SPF_NS_BEGIN
namespace GameCamera {
// struct TextureResource {
//   int dx11Id;
//   std::string name;
//   int width;
//   int height;
// };

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

  // --- New Debug Controls ---
  void SetPosLock(bool locked);
  bool GetPosLock(bool* out_locked) const;
  void SetRotLock(bool locked);
  bool GetRotLock(bool* out_locked) const;
  void SetOrbitMode(bool enabled);
  bool GetOrbitMode(bool* out_enabled) const;
  void SetOrbitSpeed(float speed);
  bool GetOrbitSpeed(float* out_speed) const;

  // --- Object Selection ---
  uintptr_t GetSelectedObjectPtr() const;
  void SetSelectedObjectPtr(uintptr_t ptr);
  uintptr_t GetHoveredObjectPtr() const;

  // // --- PiP Texture ---
  // uintptr_t GetPipTextureSrv() const;
  // uintptr_t GetGpsTextureSrv() const;
  // uintptr_t GetMirrorTextureSrv(int index) const;
  // uintptr_t GetTextureSrvByPath(const std::string& path) const;

  // void SetSelectedTextureId(int id);
  // int GetSelectedTextureId() const;
  // int GetTextureCount() const;
  // bool IsRenderTargetImage(int id) const;

  // std::vector<TextureResource> GetAvailableTextures();

 private:
  mutable DebugCameraMode m_currentMode = DebugCameraMode::SIMPLE;
  // int m_selectedTextureId = 4;
};
}  // namespace GameCamera
SPF_NS_END
