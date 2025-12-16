#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
class GameCameraWindow : public IGameCamera {
 public:
  struct CameraData {
    float head_offset_x = 0.0f;
    float head_offset_y = 0.0f;
    float head_offset_z = 0.0f;
    float live_yaw = 0.0f;
    float live_pitch = 0.0f;
    float mouse_left_limit = 0.0f;
    float mouse_right_limit = 0.0f;
    float mouse_lr_default = 0.0f;
    float mouse_up_limit = 0.0f;
    float mouse_down_limit = 0.0f;
    float mouse_ud_default = 0.0f;
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraWindow();
  ~GameCameraWindow() override = default;

  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::WindowCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  bool GetHeadOffset(float* out_x, float* out_y, float* out_z) const;
  bool GetLiveRotation(float* out_yaw, float* out_pitch) const;
  bool GetRotationLimits(float* out_left, float* out_right, float* out_up, float* out_down) const;
  bool GetRotationDefaults(float* out_lr, float* out_ud) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  void SetHeadOffset(float x, float y, float z);
  void SetLiveRotation(float yaw, float pitch);
  void SetRotationLimits(float left, float right, float up, float down);
  void SetRotationDefaults(float lr, float ud);
  void SetFov(float fov);

 private:
  void* m_pCameraObject = nullptr;
  CameraData m_cameraData;
  CameraData m_defaultCameraData;
};
}  // namespace GameCamera
SPF_NS_END
