#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/GameCamera/GameCameraType.hpp"
#include "SPF/GameCamera/IGameCamera.hpp"

#include <cstddef>


SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraTop
 * @brief Represents the top-down camera (ID 7).
 *
 * This class manages the state and behavior of the top-down camera,
 * providing methods to control its height, speed, and offsets.
 */
class GameCameraTop : public IGameCamera {
 public:
  struct CameraData {
    float minimum_height = 0.0f;
    float maximum_height = 0.0f;
    float speed = 0.0f;
    float x_offset_forward = 0.0f;
    float x_offset_backward = 0.0f;
    float offset_forward = 0.0f;
    float offset_backward = 0.0f;
    float camera_height_factor = 0.0f;
    bool use_adaptive_camera_height = false;
    float near_plane = 0.0f;
    float far_plane = 0.0f;
    bool validation = false;
    float validation_speed_positive = 0.0f;
    float validation_speed_negative = 0.0f;
    float shake_anim_step = 0.0f;
    float shake_anim_scale_min = 0.0f;
    float shake_anim_scale_max = 0.0f;
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraTop();
  ~GameCameraTop() override = default;

  // --- IGameCamera Interface ---
  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::TopCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  const CameraData& GetDefaults() const { return m_defaultCameraData; }

  // --- Public API for Top Camera ---
  bool GetHeight(float* out_min, float* out_max) const;
  bool GetSpeed(float* out_speed) const;
  bool GetOffsets(float* out_forward, float* out_backward) const;
  bool GetOffsetsZ(float* out_forward, float* out_backward) const;
  bool GetAdaptiveSettings(float* out_factor, bool* out_use_adaptive) const;
  bool GetPlaneSettings(float* out_near, float* out_far) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  // Validation
  bool GetValidation(bool* out_enabled) const;
  bool GetValidationSettings(float* out_speed_pos, float* out_speed_neg) const;

  // Shake
  bool GetShakeAnimStep(float* out_val) const;
  bool GetShakeAnimScaleMin(float* out_val) const;
  bool GetShakeAnimScaleMax(float* out_val) const;
  size_t GetShakeAnimCount() const;
  void GetShakeAnim(size_t index, float& x, float& y, float& z) const;

  void SetHeight(float min, float max);
  void SetSpeed(float speed);
  void SetOffsets(float forward, float backward);
  void SetOffsetsZ(float forward, float backward);
  void SetAdaptiveSettings(float factor, bool use_adaptive);
  void SetPlaneSettings(float near_p, float far_p);
  void SetFov(float fov);

  // Validation Setters
  void SetValidation(bool enabled);
  void SetValidationSettings(float speed_pos, float speed_neg);

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
