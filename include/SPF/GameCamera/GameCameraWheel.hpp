#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/GameCamera/GameCameraType.hpp"
#include "SPF/GameCamera/IGameCamera.hpp"

#include <cstddef>


SPF_NS_BEGIN
namespace GameCamera {
class GameCameraWheel : public IGameCamera {
 public:
  struct CameraData {
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float offset_z = 0.0f;
    float fov_base = 0.0f;
    float shake_anim_step = 0.0f;
    float shake_anim_scale_min = 0.0f;
    float shake_anim_scale_max = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraWheel();
  ~GameCameraWheel() override = default;

  // --- IGameCamera Interface ---
  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::WheelCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  const CameraData& GetDefaults() const { return m_defaultCameraData; }

  // --- Public API for Wheel Camera ---
  bool GetOffset(float* out_x, float* out_y, float* out_z) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  // Shake
  bool GetShakeAnimStep(float* out_val) const;
  bool GetShakeAnimScaleMin(float* out_val) const;
  bool GetShakeAnimScaleMax(float* out_val) const;
  size_t GetShakeAnimCount() const;
  void GetShakeAnim(size_t index, float& x, float& y, float& z) const;

  void SetOffset(float x, float y, float z);
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
