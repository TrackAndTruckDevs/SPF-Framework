#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/GameCamera/GameCameraType.hpp"
#include "SPF/GameCamera/IGameCamera.hpp"

#include <cstddef>
#include <vector>


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
    float near_plane = 0.0f;
    float far_plane = 0.0f;
    float mouse_sensitivity = 0.0f;
    float shake_step = 0.0f;
    float shake_min = 0.0f;
    float shake_max = 0.0f;
    float hand_shake_limit = 0.0f;
    float hand_shake_speed = 0.0f;
    float zoom_fov_factor = 0.0f;
    float zoom_speed = 0.0f;

    struct AzimuthRangeData {
      float start_azimuth, end_azimuth;
      bool outside;
      float start_up_limit, end_up_limit;
      float start_down_limit, end_down_limit;
      float start_up_down_default, end_up_down_default;
      float start_left_right_default, end_left_right_default;
      float start_head_x, start_head_y, start_head_z;
      float end_head_x, end_head_y, end_head_z;
    };
    std::vector<AzimuthRangeData> azimuth_overrides_defaults;

    struct Vec3 {
      float x, y, z;
    };
    std::vector<Vec3> shake_anim_defaults;
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
  bool GetOutside(bool* out_val) const;

  void SetSeatPosition(float x, float y, float z);
  void SetHeadRotation(float yaw, float pitch);
  void SetFov(float fov);
  void SetRotationLimits(float left, float right, float up, float down);
  void SetRotationDefaults(float lr, float ud);
  void SetOutside(bool val);

  // --- Core Camera Individual API ---
  bool GetNearPlane(float* out_val) const;
  void SetNearPlane(float val);
  bool GetFarPlane(float* out_val) const;
  void SetFarPlane(float val);
  bool GetMouseSensitivity(float* out_val) const;
  void SetMouseSensitivity(float val);

  // --- Shake Individual API ---
  bool GetShakeAnimStep(float* out_val) const;
  void SetShakeAnimStep(float val);
  bool GetShakeAnimScaleMin(float* out_val) const;
  void SetShakeAnimScaleMin(float val);
  bool GetShakeAnimScaleMax(float* out_val) const;
  void SetShakeAnimScaleMax(float val);

  // --- Hand Shake Individual API ---
  bool GetHandShakeLimit(float* out_val) const;
  void SetHandShakeLimit(float val);
  bool GetHandShakeSpeed(float* out_val) const;
  void SetHandShakeSpeed(float val);

  // --- Interior Logic Individual API ---
  bool GetZoomFovFactor(float* out_val) const;
  void SetZoomFovFactor(float val);
  bool GetZoomSpeed(float* out_val) const;
  void SetZoomSpeed(float val);

  // --- Shake Individual API ---
  size_t GetAzimuthOverridesCount() const;
  void* GetAzimuthOverrideAddress(size_t index) const;

  bool GetAzimuthOverrideOutside(size_t index, bool* out_val) const;
  void SetAzimuthOverrideOutside(size_t index, bool val);

  bool GetAzimuthOverrideStartAzimuth(size_t index, float* out_val) const;
  void SetAzimuthOverrideStartAzimuth(size_t index, float val);

  bool GetAzimuthOverrideEndAzimuth(size_t index, float* out_val) const;
  void SetAzimuthOverrideEndAzimuth(size_t index, float val);

  bool GetAzimuthOverrideStartUpLimit(size_t index, float* out_val) const;
  void SetAzimuthOverrideStartUpLimit(size_t index, float val);

  bool GetAzimuthOverrideEndUpLimit(size_t index, float* out_val) const;
  void SetAzimuthOverrideEndUpLimit(size_t index, float val);

  bool GetAzimuthOverrideStartDownLimit(size_t index, float* out_val) const;
  void SetAzimuthOverrideStartDownLimit(size_t index, float val);

  bool GetAzimuthOverrideEndDownLimit(size_t index, float* out_val) const;
  void SetAzimuthOverrideEndDownLimit(size_t index, float val);

  bool GetAzimuthOverrideStartUpDownDefault(size_t index, float* out_val) const;
  void SetAzimuthOverrideStartUpDownDefault(size_t index, float val);

  bool GetAzimuthOverrideEndUpDownDefault(size_t index, float* out_val) const;
  void SetAzimuthOverrideEndUpDownDefault(size_t index, float val);

  bool GetAzimuthOverrideStartLeftRightDefault(size_t index, float* out_val) const;
  void SetAzimuthOverrideStartLeftRightDefault(size_t index, float val);

  bool GetAzimuthOverrideEndLeftRightDefault(size_t index, float* out_val) const;
  void SetAzimuthOverrideEndLeftRightDefault(size_t index, float val);

  bool GetAzimuthOverrideStartHeadOffset(size_t index, float* out_x, float* out_y, float* out_z) const;
  void SetAzimuthOverrideStartHeadOffset(size_t index, float x, float y, float z);

  bool GetAzimuthOverrideEndHeadOffset(size_t index, float* out_x, float* out_y, float* out_z) const;
  void SetAzimuthOverrideEndHeadOffset(size_t index, float x, float y, float z);

  // --- Public API for Shake Animation ---
  size_t GetShakeAnimCount() const;
  bool GetShakeAnim(size_t index, float* out_x, float* out_y, float* out_z) const;
  void SetShakeAnim(size_t index, float x, float y, float z);

  // --- Defaults Access ---
  const CameraData& GetDefaults() const { return m_defaultCameraData; }

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
