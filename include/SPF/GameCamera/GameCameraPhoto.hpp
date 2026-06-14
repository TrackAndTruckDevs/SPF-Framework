#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraPhoto
 * @brief Represents the photo mode camera (ID 13, 'photo_camera').
 */
class GameCameraPhoto : public IGameCamera {
 public:
  struct CameraData {
    // Basic state (Live)
    float live_pitch = 0.0f;
    float live_yaw = 0.0f;
    float live_roll = 0.0f;
    float live_zoom = 0.0f;
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float pos_z = 0.0f;

    // Config parameters (from reflection)
    float mouse_sensitivity = 0.0f;
    float camera_fov = 0.0f;
    // ... more will be added from the table provided later
  };

 public:
  GameCameraPhoto();
  ~GameCameraPhoto() override = default;

  // --- IGameCamera Interface ---
  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::PhotoCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  const CameraData& GetDefaults() const { return m_defaultCameraData; }

  // --- Public API for Photo Camera ---
  bool GetLiveState(float* out_pitch, float* out_yaw, float* out_roll, float* out_zoom) const;
  void SetLiveState(float pitch, float yaw, float roll, float zoom);

  bool GetPosition(float* out_x, float* out_y, float* out_z) const;
  void SetPosition(float x, float y, float z);

  bool GetFov(float* out_fov) const;
  void SetFov(float fov);

 private:
  void* m_pCameraObject = nullptr;
  CameraData m_cameraData;
  CameraData m_defaultCameraData;
};
}  // namespace GameCamera
SPF_NS_END
