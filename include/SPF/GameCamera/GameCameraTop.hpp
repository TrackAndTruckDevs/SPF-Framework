#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

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

  // --- Public API for Top Camera ---
  bool GetHeight(float* out_min, float* out_max) const;
  bool GetSpeed(float* out_speed) const;
  bool GetOffsets(float* out_forward, float* out_backward) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  void SetHeight(float min, float max);
  void SetSpeed(float speed);
  void SetOffsets(float forward, float backward);
  void SetFov(float fov);

 private:
  // Pointer to the raw game camera object.
  void* m_pCameraObject = nullptr;
  // Local copy of the camera's data, updated each frame.
  CameraData m_cameraData;
  // A snapshot of the camera's data at initialization, used for the "Reset" button.
  CameraData m_defaultCameraData;
};
}  // namespace GameCamera
SPF_NS_END
