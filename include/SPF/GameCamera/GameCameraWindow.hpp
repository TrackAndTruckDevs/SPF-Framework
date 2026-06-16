#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"
#include <cstdint>
#include <cstddef>

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
    bool relative_headtracking_azimuth = false;
    int32_t auto_center_move_direction = 0;
    float fov_base = 0.0f;
    float shake_anim_step = 0.0f;
    float shake_anim_scale_min = 0.0f;
    float shake_anim_scale_max = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraWindow();
  ~GameCameraWindow() override = default;

  // --- IGameCamera Interface ---
  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::WindowCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  const CameraData& GetDefaults() const { return m_defaultCameraData; }

  // --- Public API for Window Camera ---
  bool GetHeadOffset(float* out_x, float* out_y, float* out_z) const;
  bool GetLiveRotation(float* out_yaw, float* out_pitch) const;
  bool GetRotationLimits(float* out_left, float* out_right, float* out_up, float* out_down) const;
  bool GetRotationDefaults(float* out_lr, float* out_ud) const;
  bool GetRelativeHeadtrackingAzimuth(bool* out_val) const;
  bool GetAutoCenterMoveDirection(int32_t* out_val) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  // Shake
  bool GetShakeAnimStep(float* out_val) const;
  bool GetShakeAnimScaleMin(float* out_val) const;
  bool GetShakeAnimScaleMax(float* out_val) const;
  size_t GetShakeAnimCount() const;
  void GetShakeAnim(size_t index, float& x, float& y, float& z) const;

  void SetHeadOffset(float x, float y, float z);
  void SetLiveRotation(float yaw, float pitch);
  void SetRotationLimits(float left, float right, float up, float down);
  void SetRotationDefaults(float lr, float ud);
  void SetRelativeHeadtrackingAzimuth(bool val);
  void SetAutoCenterMoveDirection(int32_t val);
  void SetFov(float fov);

  // Shake Setters
  void SetShakeAnimStep(float val);
  void SetShakeAnimScaleMin(float val);
  void SetShakeAnimScaleMax(float val);
  void SetShakeAnim(size_t index, float x, float y, float z);

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
