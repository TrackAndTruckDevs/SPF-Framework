#pragma once

#include "SPF/GameCamera/IGameCamera.hpp"
#include <cstdint>

SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraDebugState
 * @brief Manages the functionality related to the 'animated' debug camera mode,
 * including saving and replaying camera states.
 */
class GameCameraDebugState {
 public:
  // Define the structure for a single camera state/keyframe
  struct alignas(16) CameraState {
    // The structure of data the native AddCameraState function expects.
    // This is based on reverse-engineering and corresponds to 9 floats.
    float pos_x, pos_y, pos_z;
    float mystery_float;  // Unknown value at offset 0x4C from camera object base
    float q_x, q_y, q_z, q_w;
    float fov;
    // Pad to 48 bytes (12 floats) to satisfy alignas(16) requirement.
    float pad[3];
  };

  GameCameraDebugState();

  void SaveState();
  void ApplyState(int index);
  void CycleState(int direction);
  void ReloadStatesFromFile();

  /**
   * @brief Clears all camera states from the game's memory.
   * This is a direct manipulation and does not involve file I/O.
   */
  void ClearAllStatesInMemory();

  /**
   * @brief Appends a single camera state to the end of the game's in-memory list.
   * @param state The camera state data to add.
   */
  void AddStateToMemory(const CameraState& state);

  /**
   * @brief Overwrites the camera state at a specific index with new data in memory.
   * Does not change the total number of states.
   * @param index The zero-based index of the state to edit.
   * @param newState The new camera state data.
   * @return True if the index was valid and the state was edited, false otherwise.
   */
  bool EditStateInMemory(int index, const CameraState& newState);

  /**
   * @brief Deletes the camera state at a specific index from memory.
   * @param index The zero-based index of the state to delete.
   */
  void DeleteStateInMemory(int index);

  int GetStateCount() const;
  bool GetState(int index, CameraState& out_state) const;
  int GetCurrentStateIndex() const;

 private:
  // --- Helper methods for the save process ---
  uintptr_t OpenFile();
  bool WriteStateToFile(uintptr_t fileHandle, const CameraState& state);
  void CloseFile(uintptr_t fileHandle);

  // --- Helper for accessing state data ---
  uintptr_t GetStateAddress(int index) const;

  static constexpr size_t NATIVE_STATE_SIZE = 0x24;  // 36 bytes (9 floats)
};
}  // namespace GameCamera
SPF_NS_END
