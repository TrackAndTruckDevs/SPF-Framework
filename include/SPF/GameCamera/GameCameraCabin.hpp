#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
class GameCameraCabin : public IGameCamera {
 public:
  struct CameraData {
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraCabin();
  ~GameCameraCabin() override = default;

  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::CabinCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  void SetFov(float fov);

 private:
  void* m_pCameraObject = nullptr;
  CameraData m_cameraData;
  CameraData m_defaultCameraData;
};
}  // namespace GameCamera
SPF_NS_END
