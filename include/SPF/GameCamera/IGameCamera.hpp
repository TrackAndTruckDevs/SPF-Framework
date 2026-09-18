#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/GameCamera/GameCameraType.hpp"


SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class IGameCamera
 * @brief An interface for a specific game camera implementation.
 *
 * This interface defines the contract for all camera controller classes.
 * Each class implementing this interface will manage one specific type of
 * in-game camera (e.g., interior, freecam, chase).
 */
class IGameCamera {
 public:
  virtual ~IGameCamera() = default;

  /**
   * @brief Called by the GameCameraManager when this camera becomes the active one.
   *
   * Implementations should use this to acquire necessary pointers or initialize state.
   */
  virtual void OnActivate() = 0;

  /**
   * @brief Called by the GameCameraManager when this camera is no longer active.
   *
   * Implementations should use this to release pointers or clean up state.
   */
  virtual void OnDeactivate() = 0;

  /**
   * @brief Called every frame by the GameCameraManager if this is the active camera.
   * @param dt Delta time since the last frame.
   */
  virtual void Update(float dt) = 0;

  /**
   * @brief Called once per rendered frame, right before the UI is drawn — i.e. after the
   * game has finished its own per-frame camera computations. Implementations use this to
   * re-assert values the game keeps overwriting after Update() runs (e.g. FOV vs. native zoom).
   * Default is a no-op; only cameras that need this override it.
   */
  virtual void LateUpdate() {}

  /**
   * @brief Returns the specific type of this camera.
   * @return The GameCameraType enum value.
   */
  virtual GameCameraType GetType() const = 0;

  /**
   * @brief Reads the current state from game memory and stores it as the "default" state.
   * This should only be done once when the camera is first initialized.
   */
  virtual void StoreDefaultState() = 0;

  /**
   * @brief Resets the camera's properties in game memory to the stored default state.
   */
  virtual void ResetToDefaults() = 0;

  /**
   * @brief Checks if the default state for this camera has been saved.
   * @return True if the state has been saved, false otherwise.
   */
  bool HasSavedDefaults() const { return m_defaultsSaved; }

 protected:
  // Flag to ensure the default state is only captured once.
  bool m_defaultsSaved = false;

  /**
   * @brief Shared FOV-apply logic for `core_camera`-based cameras (all but the debug/photo
   * cameras): writes fov into both the live FOV field (GameDataCameraService::
   * GetFovBaseOffset) and the "un-zoomed reference" field the game's native speed-FOV
   * system reads from (GetFovZoomBaseOffset), then calls UpdateCameraProjection so the
   * final horiz/vert FOV recompute immediately. Writing only the live field isn't enough —
   * the native system overwrites it again the next time speed or zoom state changes.
   */
  static void ApplyCoreCameraFov(void* pCameraObject, float fov);

  /**
   * @brief Re-asserts just the reference FOV field (GetFovZoomBaseOffset). Call from
   * LateUpdate() while a FOV override is active, so the native per-frame speed-FOV
   * recompute converges to it instead of the vehicle's configured value. Deliberately does
   * NOT touch the live FOV field or call UpdateCameraProjection — doing both every frame
   * fights the native recompute instead of just fixing its source, causing visible flicker.
   */
  static void ReassertCoreCameraFovReference(void* pCameraObject, float fov);
};
}  // namespace GameCamera
SPF_NS_END
