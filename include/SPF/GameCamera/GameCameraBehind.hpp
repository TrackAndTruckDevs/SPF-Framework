#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"

SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraBehind
 * @brief Represents the chase camera (ID 1, 'behind_rotation_basic').
 *
 * This class manages the state and behavior of the primary chase camera,
 * providing methods to control its distance, elevation, pivot point, and other settings.
 */
class GameCameraBehind : public IGameCamera {
 public:
  struct CameraData {
    // Live State
    float live_pitch = 0.0f;  // 0x14
    float live_yaw = 0.0f;    // 0x4C4
    float live_zoom = 0.0f;   // 0x4C8

    // Distance / Zoom
    float distance_min = 0.0f;                 // 0x470
    float distance_max = 0.0f;                 // 0x474
    float distance_trailer_max_offset = 0.0f;  // 0x478
    float distance_default = 0.0f;             // 0x47C
    float distance_trailer_default = 0.0f;     // 0x480
    float distance_change_speed = 0.0f;        // 0x484
    float distance_laziness_speed = 0.0f;      // 0x488

    // Elevation / Pitch
    float azimuth_laziness_speed = 0.0f;     // 0x48C
    float elevation_min = 0.0f;              // 0x490
    float elevation_max = 0.0f;              // 0x494
    float elevation_default = 0.0f;          // 0x498
    float elevation_trailer_default = 0.0f;  // 0x49C
    float height_limit = 0.0f;               // 0x4A0

    // Pivot
    float pivot_x = 0.0f;  // 0x4A4
    float pivot_y = 0.0f;  // 0x4A8
    float pivot_z = 0.0f;  // 0x4AC

    // Dynamic Offset
    float dynamic_offset_max = 0.0f;             // 0x4B0
    float dynamic_offset_speed_min = 0.0f;       // 0x4B4
    float dynamic_offset_speed_max = 0.0f;       // 0x4B8
    float dynamic_offset_laziness_speed = 0.0f;  // 0x4BC

    // FOV
    float fov_base = 0.0f;
    float fov_horiz_final = 0.0f;
    float fov_vert_final = 0.0f;
  };

 public:
  GameCameraBehind();
  ~GameCameraBehind() override = default;

  // --- IGameCamera Interface ---
  void OnActivate() override;
  void OnDeactivate() override;
  void Update(float dt) override;
  GameCameraType GetType() const override { return GameCameraType::BehindCamera; }
  void StoreDefaultState() override;
  void ResetToDefaults() override;

  // --- Public API for Behind Camera ---
  bool GetLiveState(float* out_pitch, float* out_yaw, float* out_zoom) const;
  bool GetDistanceSettings(float* out_min, float* out_max, float* out_trailer_max_offset, float* out_def, float* out_trailer_def, float* out_change_speed,
                           float* out_laziness) const;
  bool GetElevationSettings(float* out_azimuth_laziness, float* out_min, float* out_max, float* out_def, float* out_trailer_def, float* out_height_limit) const;
  bool GetPivot(float* out_x, float* out_y, float* out_z) const;
  bool GetDynamicOffset(float* out_max, float* out_speed_min, float* out_speed_max, float* out_laziness) const;
  bool GetFov(float* out_fov) const;
  bool GetFinalFov(float* out_horiz, float* out_vert) const;

  void SetLiveState(float pitch, float yaw, float zoom);
  void SetDistanceSettings(float min, float max, float trailer_max_offset, float def, float trailer_def, float change_speed, float laziness);
  void SetElevationSettings(float azimuth_laziness, float min, float max, float def, float trailer_def, float height_limit);
  void SetPivot(float x, float y, float z);
  void SetDynamicOffset(float max, float speed_min, float speed_max, float laziness);
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
