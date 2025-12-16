#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
class GameCameraTV : public IGameCamera {
 public:
  struct CameraData {
    float max_distance = 0.0f;
    float prefab_uplift_x = 0.0f;
    float prefab_uplift_y = 0.0f;
    float prefab_uplift_z = 0.0f;
    float road_uplift_x = 0.0f;
    float road_uplift_y = 0.0f;
    float road_uplift_z = 0.0f;
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraTV();
  ~GameCameraTV() override = default;

  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::TVCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  bool GetMaxDistance(float* out_max_distance) const;
  bool GetPrefabUplift(float* out_x, float* out_y, float* out_z) const;
  bool GetRoadUplift(float* out_x, float* out_y, float* out_z) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  void SetMaxDistance(float distance);
  void SetPrefabUplift(float x, float y, float z);
  void SetRoadUplift(float x, float y, float z);
  void SetFov(float fov);

 private:
  void* m_pCameraObject = nullptr;
  CameraData m_cameraData;
  CameraData m_defaultCameraData;
};
}  // namespace GameCamera
SPF_NS_END
