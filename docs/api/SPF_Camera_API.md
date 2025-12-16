# SPF Camera API

The SPF Camera API provides extensive control over the in-game camera system. It allows plugins to switch between camera types, read and modify parameters for each camera (like FOV, position, rotation), and access the advanced debug camera features, including state saving and animation playback.

## Getting the API

To use the camera API, you first need to get a pointer to the `SPF_Camera_API` struct from the framework. This is typically done during your plugin's initialization phase by requesting the API by its unique name.

**Example: C**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Camera_API.h"

// Global pointer to the Camera API
SPF_Camera_API* s_cameraAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_cameraAPI = (SPF_Camera_API*)params->GetAPI(SPF_API_CAMERA);
    
    if (s_cameraAPI) {
        // The API was successfully acquired and is ready to use.
        // For example, you can now switch the camera:
        // s_cameraAPI->SwitchTo(SPF_CAMERA_BEHIND);
    }
}
```

## Data Types

This section describes the primary `enums` and `structs` used by the Camera API functions.

### SPF_CameraType (enum)

This enum defines the available in-game camera types that you can control or query.

| Value | Description |
|---|---|
| `SPF_CAMERA_DEVELOPER_FREE` | The free-roam developer camera (`0`). |
| `SPF_CAMERA_BEHIND` | Standard third-person chase camera (`1`). |
| `SPF_CAMERA_INTERIOR` | First-person camera inside the cabin (`2`). |
| `SPF_CAMERA_BUMPER` | Camera mounted on the vehicle's bumper (`3`). |
| `SPF_CAMERA_WINDOW` | View from the side window (`4`). |
| `SPF_CAMERA_CABIN` | Static camera inside the cabin, looking at the seat (`5`). |
| `SPF_CAMERA_WHEEL` | Camera mounted near the front wheel (`6`). |
| `SPF_CAMERA_TOP_BASIC` | Top-down view camera (`7`). |
| `SPF_CAMERA_TV` | Predefined cinematic TV camera angles (`9`). |

### SPF_CameraState_t (struct)

Represents a snapshot of a camera's state, used by the debug camera system for saving and loading camera positions, orientations, and FOV.

```c
typedef struct {
    float pos_x, pos_y, pos_z;
    float mystery_float; // An unknown value used by the game's internal state
    float q_x, q_y, q_z, q_w; // Quaternion for orientation
    float fov;
} SPF_CameraState_t;
```
*   `pos_x, pos_y, pos_z`: The world-space coordinates of the camera.
*   `q_x, q_y, q_z, q_w`: The orientation of the camera represented as a quaternion.
*   `fov`: The base Field of View at the time the state was saved.

### SPF_DebugCameraMode (enum)
Defines the different behavior modes for the debug camera.

| Value | Description |
|---|---|
| `SPF_DEBUG_CAMERA_MODE_SIMPLE` | Basic debug camera movement. |
| `SPF_DEBUG_CAMERA_MODE_VIDEO` | Smoother movement suitable for recording video. |
| `SPF_DEBUG_CAMERA_MODE_TRAFFIC` | A mode optimized for observing AI traffic. |
| `SPF_DEBUG_CAMERA_MODE_CINEMATIC`| Cinematic movement mode. |
| `SPF_DEBUG_CAMERA_MODE_ANIMATED` | Plays an animation using saved camera states. |
| `SPF_DEBUG_CAMERA_MODE_OVERSIZE` | A mode for observing oversized transport jobs. |

### SPF_DebugHudPosition (enum)
Defines the screen corner for the debug camera's information HUD.

| Value | Description |
|---|---|
| `SPF_DEBUG_HUD_POSITION_TOP_LEFT` |  |
| `SPF_DEBUG_HUD_POSITION_BOTTOM_LEFT` |  |
| `SPF_DEBUG_HUD_POSITION_TOP_RIGHT` |  |
| `SPF_DEBUG_HUD_POSITION_BOTTOM_RIGHT` |  |

### SPF_AnimPlaybackState (enum)
Represents the current playback status of a debug camera animation.

| Value | Description |
|---|---|
| `SPF_ANIM_STOPPED` | The animation is not running. |
| `SPF_ANIM_PLAYING` | The animation is currently playing. |
| `SPF_ANIM_PAUSED` | The animation is paused. |

## Function Reference

All API functions are accessed as function pointers through the `SPF_Camera_API` struct obtained during initialization. All `Get...` functions that return a `bool` will return `true` on success and `false` on failure (e.g., if the camera type is not active or data is not available).

## Function Reference

All API functions are accessed as function pointers through the `SPF_Camera_API` struct obtained during initialization. All `Get...` functions that return a `bool` will return `true` on success and `false` on failure (e.g., if the camera type is not active or data is not available).

### General Camera Functions

These functions operate on the camera system as a whole.

---
**`void SwitchTo(SPF_CameraType cameraType)`**
Switches the currently active in-game camera to the specified type.
*   **Parameters:**
    *   `cameraType`: The camera to switch to, from the `SPF_CameraType` enum.
*   **Example:**
    ```c
    // Switch to the behind-the-truck camera
    s_cameraAPI->SwitchTo(SPF_CAMERA_BEHIND);
    ```

---
**`bool GetCurrentCamera(SPF_CameraType* out_cameraType)`**
Gets the type of the currently active camera.
*   **Parameters:**
    *   `out_cameraType`: A pointer to an `SPF_CameraType` variable where the current camera type will be stored.
*   **Returns:** `true` if the camera type could be determined, `false` otherwise.
*   **Example:**
    ```c
    SPF_CameraType current_cam_type;
    if (s_cameraAPI->GetCurrentCamera(&current_cam_type)) {
        if (current_cam_type == SPF_CAMERA_INTERIOR) {
            // We are in the interior camera
        }
    }
    ```

---
**`void ResetToDefaults(SPF_CameraType cameraType)`**
Resets all settings for a specific camera to their default values.
*   **Parameters:**
    *   `cameraType`: The camera whose settings should be reset.

---
**`bool GetCameraWorldCoordinates(float* x, float* y, float* z)`**
Gets the world coordinates (position) of the currently active camera.
*   **Parameters:**
    *   `x`, `y`, `z`: Pointers to floats where the camera's world position will be stored.
*   **Returns:** `true` on success, `false` otherwise.

<br>

### Interior Camera (`SPF_CAMERA_INTERIOR`)

This group of functions allows you to read and modify parameters specifically for the interior (in-cabin) camera.

---
**`GetInteriorSeatPos(float* x, float* y, float* z)` / `SetInteriorSeatPos(float x, float y, float z)`**
Gets or sets the current XYZ coordinates of the seat position, which adjusts the camera's viewpoint.

---
**`GetInteriorHeadRot(float* yaw, float* pitch)` / `SetInteriorHeadRot(float yaw, float pitch)`**
Gets or sets the current head rotation of the driver.
*   `yaw`: Horizontal rotation (looking left/right).
*   `pitch`: Vertical rotation (looking up/down).

---
**`GetInteriorFov(float* fov)` / `SetInteriorFov(float fov)`**
Gets or sets the base Field of View (FOV) for the interior camera.

---
**`GetInteriorFinalFov(float* out_horiz, float* out_vert)`**
Gets the final, calculated Field of View (FOV) after all dynamic adjustments (like UI-based FOV changes) have been applied.

---
**`GetInteriorRotationLimits(float* left, float* right, float* up, float* down)` / `SetInteriorRotationLimits(float left, float right, float up, float down)`**
Gets or sets the maximum rotation angles for the interior camera view.

---
**`GetInteriorRotationDefaults(float* lr, float* ud)` / `SetInteriorRotationDefaults(float lr, float ud)`**
Gets or sets the default rotation values (left/right and up/down) that the camera resets to.


### Behind Camera (`SPF_CAMERA_BEHIND`)

Functions for controlling the third-person chase camera.

---
**`GetBehindLiveState(float* pitch, float* yaw, float* zoom)`**
Gets the live orientation and zoom level of the camera.

---
**`GetBehindDistanceSettings(...)` / `SetBehindDistanceSettings(...)`**
Gets or sets the camera's distance from the truck, including min/max, default, and trailer-specific values.

---
**`GetBehindElevationSettings(...)` / `SetBehindElevationSettings(...)`**
Gets or sets the camera's elevation and azimuth (horizontal follow) behavior.

---
**`GetBehindPivot(float* x, float* y, float* z)` / `SetBehindPivot(float x, float y, float z)`**
Gets or sets the camera's pivot point offset from the truck.

---
**`GetBehindDynamicOffset(...)` / `SetBehindDynamicOffset(...)`**
Gets or sets the dynamic offset that changes based on vehicle speed.

---
**`GetBehindFov(float* fov)` / `SetBehindFov(float fov)`**
Gets or sets the base Field of View (FOV) for the behind camera.

<br>

### Top-Down Camera (`SPF_CAMERA_TOP_BASIC`)

Functions for controlling the top-down "satellite" view camera.

---
**`GetTopHeight(float* min_height, float* max_height)` / `SetTopHeight(float min_height, float max_height)`**
Gets or sets the minimum and maximum height range for the camera.

---
**`GetTopSpeed(float* speed)` / `SetTopSpeed(float speed)`**
Gets or sets the camera's movement speed.

---
**`GetTopOffsets(float* forward, float* backward)` / `SetTopOffsets(float forward, float backward)`**
Gets or sets the maximum forward and backward offset limits for the camera.

---
**`GetTopFov(float* fov)` / `SetTopFov(float fov)`**
Gets or sets the base Field of View (FOV) for the top-down camera.

<br>

### Other Vehicle-Mounted Cameras

This section covers the simpler, fixed-position cameras.

---
**Window Camera (`SPF_CAMERA_WINDOW`)**

This camera exposes functions to get and set the head offset (`GetWindowHeadOffset`/`SetWindowHeadOffset`), live rotation (`GetWindowLiveRotation`/`SetWindowLiveRotation`), rotation limits (`GetWindowRotationLimits`/`SetWindowRotationLimits`), default rotation (`GetWindowRotationDefaults`/`SetWindowRotationDefaults`), and Field of View (`GetWindowFov`/`SetWindowFov`).

---
**Bumper Camera (`SPF_CAMERA_BUMPER`)**

This camera allows you to get and set the XYZ offset from the vehicle's bumper (`GetBumperOffset`/`SetBumperOffset`) and its Field of View (`GetBumperFov`/`SetBumperFov`).

---
**Wheel Camera (`SPF_CAMERA_WHEEL`)**

Similarly, this camera allows getting and setting the XYZ offset from the vehicle's wheel (`GetWheelOffset`/`SetWheelOffset`) and its Field of View (`GetWheelFov`/`SetWheelFov`).

---
**Cabin Camera (`SPF_CAMERA_CABIN`)**

This camera exposes functions to get and set its base Field of View (`GetCabinFov`/`SetCabinFov`).

---
**TV Camera (`SPF_CAMERA_TV`)**

The TV camera API allows you to control its maximum distance (`GetTVMaxDistance`/`SetTVMaxDistance`), its uplift near prefabs (`GetTVPrefabUplift`/`SetTVPrefabUplift`) and on roads (`GetTVRoadUplift`/`SetTVRoadUplift`), and its Field of View (`GetTVFov`/`SetTVFov`).


<br>

### Free Camera (`SPF_CAMERA_DEVELOPER_FREE`)

This API provides control over the game's free-roam developer camera.

---
**`GetFreePosition(float* x, float* y, float* z)` / `SetFreePosition(float x, float y, float z)`**
Gets or sets the world-space XYZ position of the free camera.

---
**`GetFreeQuaternion(float* x, float* y, float* z, float* w)`**
Gets the camera's orientation as a quaternion.

---
**`GetFreeOrientation(float* mouse_x, float* mouse_y, float* roll)` / `SetFreeOrientation(float mouse_x, float mouse_y, float roll)`**
Gets or sets the camera's orientation using the internal "mouse look" and roll values.

---
**`GetFreeFov(float* fov)` / `SetFreeFov(float fov)`**
Gets or sets the base Field of View (FOV) for the free camera.

---
**`GetFreeSpeed(float* speed)` / `SetFreeSpeed(float speed)`**
Gets or sets the movement speed of the free camera.

<br>

### Debug Camera System

The SPF Camera API exposes a powerful debug camera system that allows for saving, loading, and animating camera positions. This is separate from the standard free camera.

---
**`EnableDebugCamera(bool enable)` / `GetDebugCameraEnabled(bool* out_isEnabled)`**
Enables or disables the entire debug camera system.

---
**`SetDebugCameraMode(SPF_DebugCameraMode mode)` / `GetDebugCameraMode(SPF_DebugCameraMode* out_mode)`**
Gets or sets the current behavior mode of the debug camera (e.g., Simple, Video, Cinematic, Animated). See `SPF_DebugCameraMode` for details.

---
**`SetDebugHudVisible(bool visible)` / `GetDebugHudVisible(bool* out_isVisible)`**
Shows or hides the debug camera's on-screen information HUD.

---
**`SetDebugHudPosition(SPF_DebugHudPosition position)` / `GetDebugHudPosition(SPF_DebugHudPosition* out_position)`**
Gets or sets the screen corner for the debug HUD.

---
**`SetDebugGameUiVisible(bool visible)` / `GetDebugGameUiVisible(bool* out_isVisible)`**
Shows or hides the main game UI (dashboard, mirrors, etc.) while the debug camera is active.

<br>

#### Debug Camera State Management

This API allows you to work with a list of saved camera states (positions).

---
**`GetStateCount()`**
Returns the total number of saved camera states.

---
**`GetCurrentStateIndex()`**
Returns the index of the currently active camera state.

---
**`GetState(int index, SPF_CameraState_t* out_state)`**
Retrieves the data for a specific camera state by its index. See `SPF_CameraState_t`.

---
**`ApplyState(int index)`**
Applies a saved state, instantly moving the camera to that state's position and orientation.

---
**`CycleState(int direction)`**
Cycles to the next (if direction > 0) or previous (if direction < 0) state in the list.

---
**`SaveCurrentState()`**
Saves the camera's current position, orientation, and FOV as a new state at the end of the list.

---
**`ReloadStatesFromFile()`**
Reloads all camera states from the configuration file, discarding any in-memory changes.

---
**`ClearAllStatesInMemory()`**
Clears all camera states currently held in memory.

---
**`AddStateInMemory(const SPF_CameraState_t* state)`**
Adds a new camera state to the in-memory list.

---
**`EditStateInMemory(int index, const SPF_CameraState_t* newState)`**
Edits an existing in-memory camera state at a specific index.

---
**`DeleteStateInMemory(int index)`**
Deletes an in-memory camera state at a specific index.

<br>

#### Debug Camera Animation Control

These functions control the playback of an animation using the list of saved camera states. This is typically used with `SPF_DEBUG_CAMERA_MODE_ANIMATED`.

---
**`Anim_Play(int startIndex)`**
Starts playing the camera animation sequence from a given state index.

---
**`Anim_Pause()`**
Pauses the currently playing animation.

---
**`Anim_Stop()`**
Stops the animation and resets it.

---
**`Anim_GoToFrame(int frameIndex)`**
Instantly jumps to a specific frame (state) in the animation sequence.

---
**`Anim_ScrubTo(float position)`**
Scrubs the animation to a specific position, where `0.0` is the beginning and `1.0` is the end.

---
**`Anim_SetReverse(bool isReversed)`**
Sets the animation to play in reverse.

---
**`Anim_GetPlaybackState()`**
Returns the current `SPF_AnimPlaybackState` (e.g., Playing, Paused, Stopped).

---
**`Anim_GetCurrentFrame()`**
Returns the index of the current frame (state) in the animation.

---
**`Anim_GetCurrentFrameProgress()`**
Returns the interpolation progress (0.0 to 1.0) between the current frame and the next.

---
**`Anim_IsReversed()`**
Checks if the animation is currently set to play in reverse.

