#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraFree
 * @brief Represents the developer free camera (ID 0).
 *
 * This class manages the state and behavior of the free camera,
 * providing methods to control its position, rotation, and field of view.
 */
class GameCameraFree : public IGameCamera {
 public:
  struct CameraData {
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float pos_z = 0.0f;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    float roll = 0.0f;
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
    float speed = 0.0f;
  };

 public:
  GameCameraFree();
  ~GameCameraFree() override = default;

  // --- IGameCamera Interface ---
  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::DeveloperFreeCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  // --- Public API for Free Camera ---
  bool GetPosition(float* out_x, float* out_y, float* out_z) const;
  bool GetFreecamMysteryFloat(float* out_mystery) const;
  bool GetQuaternion(float* out_x, float* out_y, float* out_z, float* out_w) const;
  bool GetOrientation(float* out_mouse_x, float* out_mouse_y, float* out_roll) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;
  bool GetSpeed(float* out_speed) const;

  void SetPosition(float x, float y, float z);
  void SetOrientation(float mouse_x, float mouse_y, float roll);
  void SetFov(float fov);
  void SetSpeed(float speed);

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
