#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraInterior
 * @brief Represents the interior cabin camera (ID 2).
 *
 * This class manages the state and behavior of the in-cabin camera,
 * providing methods to control its position, rotation, and field of view.
 */
class GameCameraInterior : public IGameCamera {
 public:
  struct CameraData {
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
    float seat_pos_x = 0.0f;
    float seat_pos_y = 0.0f;
    float seat_pos_z = 0.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float limit_left = 0.0f;
    float limit_right = 0.0f;
    float limit_up = 0.0f;
    float limit_down = 0.0f;
    float mouse_lr_default = 0.0f;
    float mouse_ud_default = 0.0f;
  };

 public:
  GameCameraInterior();
  ~GameCameraInterior() override = default;

  // --- IGameCamera Interface ---
  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::InteriorCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  // --- Public API for Interior Camera ---
  bool GetSeatPosition(float* out_x, float* out_y, float* out_z) const;
  bool GetHeadRotation(float* out_yaw, float* out_pitch) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;
  bool GetRotationLimits(float* out_left, float* out_right, float* out_up, float* out_down) const;
  bool GetRotationDefaults(float* out_lr, float* out_ud) const;

  void SetSeatPosition(float x, float y, float z);
  void SetHeadRotation(float yaw, float pitch);
  void SetFov(float fov);
  void SetRotationLimits(float left, float right, float up, float down);
  void SetRotationDefaults(float lr, float ud);

 private:
  // Pointer to the raw game camera object.
  void* m_pCameraObject = nullptr;
  // Local copy of the camera's data, updated each frame.
  CameraData m_cameraData;
  // A snapshot of the camera's data at initialization, used for the "Reset" button.
  CameraData m_defaultCameraData;
  bool m_defaultsSaved = false;
};
}  // namespace GameCamera
SPF_NS_END
