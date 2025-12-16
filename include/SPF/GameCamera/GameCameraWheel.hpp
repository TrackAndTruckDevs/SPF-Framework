#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
class GameCameraWheel : public IGameCamera {
 public:
  struct CameraData {
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float offset_z = 0.0f;
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraWheel();
  ~GameCameraWheel() override = default;

  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::WheelCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  bool GetOffset(float* out_x, float* out_y, float* out_z) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  void SetOffset(float x, float y, float z);
  void SetFov(float fov);

 private:
  void* m_pCameraObject = nullptr;
  CameraData m_cameraData;
  CameraData m_defaultCameraData;
};
}  // namespace GameCamera
SPF_NS_END
