/**                                                                                               
* @file SPF_Camera_API.h                                                                          
* @brief API for controlling and inspecting all in-game cameras.
*                                                                                                 
* @details This API provides exhaustive control over various game camera types, 
* including interior, behind (orbit), top-down, and the developer (free) camera. 
* It allows plugins to switch active cameras, move seat positions, adjust FOV, 
* and create custom camera animations.
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Camera Types**: Use 'SPF_CameraType' to identify which camera system you want to
*    manipulate. Not all functions work for all types (e.g., 'SetInteriorSeatPos' only 
*    affects the interior camera).
*                                                                                                 
* 2. **Angular Units**: CRITICAL: All rotation values (yaw, pitch, roll) are handled in 
*    **RADIANS**, not degrees. 
*    - Range for yaw: -PI to +PI (-3.14159 to +3.14159).
*    - Range for pitch: -PI/2 to +PI/2 (-1.57079 to +1.57079).
*                                                                                                 
* 3. **Coordinate Systems**: 
*    - **Local**: Relative to the truck's cabin or pivot point.
*    - **World**: Global game coordinates.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // Switch to interior camera
* api->Cam_SwitchTo(SPF_CAMERA_INTERIOR);
*
* // Move the driver's seat slightly back (X, Y, Z in meters)
* api->Cam_SetInteriorSeatPos(0.0f, 0.0f, -0.1f);
*
* // Look 45 degrees to the right (value in radians)
* api->Cam_SetInteriorHeadRot(0.785f, 0.0f);
* @endcode                                                                                        
*/ 

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file SPF_Camera_API.h
 * @brief Defines the public C-API for interacting with the game's camera system.
 */

/**
 * @enum SPF_CameraType
 * @brief Defines the available camera types using their internal game names.
 */
typedef enum {
    SPF_CAMERA_DEVELOPER_FREE = 0,
    SPF_CAMERA_BEHIND = 1,
    SPF_CAMERA_INTERIOR = 2,
    SPF_CAMERA_BUMPER = 3,
    SPF_CAMERA_WINDOW = 4,
    SPF_CAMERA_CABIN = 5, // Corresponds to "cabin_camera"
    SPF_CAMERA_WHEEL = 6,
    SPF_CAMERA_TOP_BASIC = 7, // Corresponds to "top_camera"
    SPF_CAMERA_TV = 9, // Corresponds to "predefined_tv_camera"
} SPF_CameraType;

/**
 * @brief Switches the active in-game camera.
 * 
 * @param cameraType The camera to switch to, from the SPF_CameraType enum.
 */
typedef void (*SPF_Camera_SwitchTo_t)(SPF_CameraType cameraType);

/**
 * @brief Gets a pointer to the raw camera object for a given ID.
 * 
 * @param manager A pointer to the game's standard manager. This should be the pointer
 *                found by the GameCamera service.
 * @param index The ID of the camera object to retrieve.
 * @return A void pointer to the camera object, or NULL if not found.
 */
typedef void* (*SPF_Camera_GetCameraObject_t)(void* manager, int index);

/**

 * @brief Gets the type of the currently active camera.

 * 

 * @param out_cameraType Pointer to write the current camera type to.

 * @return True if the camera type could be determined, false otherwise.

 */

typedef bool (*SPF_Camera_GetCurrentCamera_t)(SPF_CameraType* out_cameraType);



/**

 * @brief Resets a specific camera's settings to their default values.

 * 

 * @param cameraType The camera to reset, from the SPF_CameraType enum.

 */

typedef void (*SPF_Camera_ResetToDefaults_t)(SPF_CameraType cameraType);



// --- Interior Camera Specific Functions ---


/**
 * @brief Gets the current seat position for the interior camera.
 * @param[out] x Pointer to a float to store the X-coordinate.
 * @param[out] y Pointer to a float to store the Y-coordinate.
 * @param[out] z Pointer to a float to store the Z-coordinate.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetInteriorSeatPos_t)(float* x, float* y, float* z);

/**
 * @brief Sets the seat position for the interior camera.
 * @param x The new X-coordinate.
 * @param y The new Y-coordinate.
 * @param z The new Z-coordinate.
 */
typedef void (*SPF_Camera_SetInteriorSeatPos_t)(float x, float y, float z);

/**
 * @brief Gets the current head rotation for the interior camera.
 * @param[out] yaw Pointer to a float to store the yaw (horizontal rotation).
 * @param[out] pitch Pointer to a float to store the pitch (vertical rotation).
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetInteriorHeadRot_t)(float* yaw, float* pitch);

/**
 * @brief Sets the head rotation for the interior camera.
 * @param yaw The new yaw (horizontal rotation).
 * @param pitch The new pitch (vertical rotation).
 */
typedef void (*SPF_Camera_SetInteriorHeadRot_t)(float yaw, float pitch);

/**
 * @brief Gets the base Field of View (FOV) for the interior camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetInteriorFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the interior camera,
 *        taking into account any dynamic adjustments.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetInteriorFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the interior camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetInteriorFov_t)(float fov);

/**
 * @brief Gets the rotation limits for the interior camera view.
 * @param[out] left Pointer to a float to store the maximum left rotation.
 * @param[out] right Pointer to a float to store the maximum right rotation.
 * @param[out] up Pointer to a float to store the maximum upward rotation.
 * @param[out] down Pointer to a float to store the maximum downward rotation.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetInteriorRotationLimits_t)(float* left, float* right, float* up, float* down);

/**
 * @brief Sets the rotation limits for the interior camera view.
 * @param left The new maximum left rotation.
 * @param right The new maximum right rotation.
 * @param up The new maximum upward rotation.
 * @param down The new maximum downward rotation.
 */
typedef void (*SPF_Camera_SetInteriorRotationLimits_t)(float left, float right, float up, float down);

/**
 * @brief Gets the default rotation values for the interior camera.
 * @param[out] lr Pointer to a float to store the default left/right rotation.
 * @param[out] ud Pointer to a float to store the default up/down rotation.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetInteriorRotationDefaults_t)(float* lr, float* ud);

/**
 * @brief Sets the default rotation values for the interior camera.
 * @param lr The new default left/right rotation.
 * @param ud The new default up/down rotation.
 */
typedef void (*SPF_Camera_SetInteriorRotationDefaults_t)(float lr, float ud);



// --- Behind Camera Specific Functions ---

/**
 * @brief Gets the live state of the behind camera, including pitch, yaw, and zoom level.
 * @param[out] pitch Pointer to a float to store the current pitch.
 * @param[out] yaw Pointer to a float to store the current yaw.
 * @param[out] zoom Pointer to a float to store the current zoom level.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBehindLiveState_t)(float* pitch, float* yaw, float* zoom);

/**
 * @brief Sets the live state of the behind camera.
 * @param pitch, yaw, zoom New values in RADIANS for angles.
 */
typedef void (*SPF_Camera_SetBehindLiveState_t)(float pitch, float yaw, float zoom);

/**
 * @brief Gets the distance settings for the behind camera.
 * @param[out] min Pointer to store the minimum distance.
 * @param[out] max Pointer to store the maximum distance.
 * @param[out] trailer_max_offset Pointer to store the max offset when a trailer is attached.
 * @param[out] def Pointer to store the default distance.
 * @param[out] trailer_def Pointer to store the default distance with a trailer.
 * @param[out] change_speed Pointer to store the speed of distance change.
 * @param[out] laziness Pointer to store the camera's distance laziness.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBehindDistanceSettings_t)(float* min, float* max, float* trailer_max_offset, float* def, float* trailer_def, float* change_speed, float* laziness);

/**
 * @brief Sets the distance settings for the behind camera.
 * @param min The minimum distance.
 * @param max The maximum distance.
 * @param trailer_max_offset The max offset when a trailer is attached.
 * @param def The default distance.
 * @param trailer_def The default distance with a trailer.
 * @param change_speed The speed of distance change.
 * @param laziness The camera's distance laziness.
 */
typedef void (*SPF_Camera_SetBehindDistanceSettings_t)(float min, float max, float trailer_max_offset, float def, float trailer_def, float change_speed, float laziness);

/**
 * @brief Gets the elevation and azimuth settings for the behind camera.
 * @param[out] azimuth_laziness Pointer to store the azimuth follow laziness.
 * @param[out] min Pointer to store the minimum elevation.
 * @param[out] max Pointer to store the maximum elevation.
 * @param[out] def Pointer to store the default elevation.
 * @param[out] trailer_def Pointer to store the default elevation with a trailer.
 * @param[out] height_limit Pointer to store the height limit.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBehindElevationSettings_t)(float* azimuth_laziness, float* min, float* max, float* def, float* trailer_def, float* height_limit);

/**
 * @brief Sets the elevation and azimuth settings for the behind camera.
 * @param azimuth_laziness The azimuth follow laziness.
 * @param min The minimum elevation.
 * @param max The maximum elevation.
 * @param def The default elevation.
 * @param trailer_def The default elevation with a trailer.
 * @param height_limit The height limit.
 */
typedef void (*SPF_Camera_SetBehindElevationSettings_t)(float azimuth_laziness, float min, float max, float def, float trailer_def, float height_limit);

/**
 * @brief Gets the pivot point offset for the behind camera.
 * @param[out] x Pointer to store the X-coordinate of the pivot.
 * @param[out] y Pointer to store the Y-coordinate of the pivot.
 * @param[out] z Pointer to store the Z-coordinate of the pivot.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBehindPivot_t)(float* x, float* y, float* z);

/**
 * @brief Sets the pivot point offset for the behind camera.
 * @param x The new X-coordinate of the pivot.
 * @param y The new Y-coordinate of the pivot.
 * @param z The new Z-coordinate of the pivot.
 */
typedef void (*SPF_Camera_SetBehindPivot_t)(float x, float y, float z);

/**
 * @brief Gets the dynamic offset settings based on vehicle speed.
 * @param[out] max Pointer to store the maximum dynamic offset.
 * @param[out] speed_min Pointer to store the speed at which the offset starts.
 * @param[out] speed_max Pointer to store the speed at which the offset is maximal.
 * @param[out] laziness Pointer to store the offset's application laziness.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBehindDynamicOffset_t)(float* max, float* speed_min, float* speed_max, float* laziness);

/**
 * @brief Sets the dynamic offset settings based on vehicle speed.
 * @param max The maximum dynamic offset.
 * @param speed_min The speed at which the offset starts.
 * @param speed_max The speed at which the offset is maximal.
 * @param laziness The offset's application laziness.
 */
typedef void (*SPF_Camera_SetBehindDynamicOffset_t)(float max, float speed_min, float speed_max, float laziness);

/**
 * @brief Gets the base Field of View (FOV) for the behind camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBehindFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the behind camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBehindFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the behind camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetBehindFov_t)(float fov);



// --- Top Camera Specific Functions ---

/**
 * @brief Gets the height range for the top-down camera.
 * @param[out] min_height Pointer to store the minimum height.
 * @param[out] max_height Pointer to store the maximum height.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTopHeight_t)(float* min_height, float* max_height);

/**
 * @brief Gets the movement speed of the top-down camera.
 * @param[out] speed Pointer to store the camera's movement speed.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTopSpeed_t)(float* speed);

/**
 * @brief Gets the forward and backward offset limits for the top-down camera.
 * @param[out] forward Pointer to store the maximum forward offset.
 * @param[out] backward Pointer to store the maximum backward offset.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTopOffsets_t)(float* forward, float* backward);

// DEPRECATED: typedef bool (*SPF_Camera_GetTopCameraSettings_t)(float* min_height, float* max_height, float* speed, float* forward_offset, float* backward_offset);

/**
 * @brief Sets the height range for the top-down camera.
 * @param min_height The new minimum height.
 * @param max_height The new maximum height.
 */
typedef void (*SPF_Camera_SetTopHeight_t)(float min_height, float max_height);

/**
 * @brief Sets the movement speed of the top-down camera.
 * @param speed The new movement speed.
 */
typedef void (*SPF_Camera_SetTopSpeed_t)(float speed);

/**
 * @brief Sets the forward and backward offset limits for the top-down camera.
 * @param forward The new maximum forward offset.
 * @param backward The new maximum backward offset.
 */
typedef void (*SPF_Camera_SetTopOffsets_t)(float forward, float backward);

/**
 * @brief Gets the base Field of View (FOV) for the top-down camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTopFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the top-down camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTopFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the top-down camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetTopFov_t)(float fov);



// --- Window Camera Specific Functions ---

/**
 * @brief Gets the head offset from the window position.
 * @param[out] x Pointer to store the X-coordinate of the offset.
 * @param[out] y Pointer to store the Y-coordinate of the offset.
 * @param[out] z Pointer to store the Z-coordinate of the offset.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWindowHeadOffset_t)(float* x, float* y, float* z);

/**
 * @brief Gets the live rotation values (yaw and pitch) of the window camera.
 * @param[out] yaw Pointer to store the current yaw.
 * @param[out] pitch Pointer to store the current pitch.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWindowLiveRotation_t)(float* yaw, float* pitch);

/**
 * @brief Gets the rotation limits for the window camera view.
 * @param[out] left Pointer to store the maximum left rotation.
 * @param[out] right Pointer to store the maximum right rotation.
 * @param[out] up Pointer to store the maximum upward rotation.
 * @param[out] down Pointer to store the maximum downward rotation.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWindowRotationLimits_t)(float* left, float* right, float* up, float* down);

/**
 * @brief Gets the default rotation values for the window camera.
 * @param[out] lr Pointer to store the default left/right rotation.
 * @param[out] ud Pointer to store the default up/down rotation.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWindowRotationDefaults_t)(float* lr, float* ud);

// DEPRECATED: typedef bool (*SPF_Camera_GetWindowSettings_t)(float* head_offset_x, float* head_offset_y, float* head_offset_z, float* live_yaw, float* live_pitch, float* mouse_left_limit, float* mouse_right_limit, float* mouse_lr_default, float* mouse_up_limit, float* mouse_down_limit, float* mouse_ud_default);
// DEPRECATED: typedef void (*SPF_Camera_SetWindowSettings_t)(float head_offset_x, float head_offset_y, float head_offset_z, float live_yaw, float live_pitch, float mouse_left_limit, float mouse_right_limit, float mouse_lr_default, float mouse_up_limit, float mouse_down_limit, float mouse_ud_default);

/**
 * @brief Sets the head offset from the window position.
 * @param x The new X-coordinate of the offset.
 * @param y The new Y-coordinate of the offset.
 * @param z The new Z-coordinate of the offset.
 */
typedef void (*SPF_Camera_SetWindowHeadOffset_t)(float x, float y, float z);

/**
 * @brief Sets the live rotation values for the window camera.
 * @param yaw The new yaw value.
 * @param pitch The new pitch value.
 */
typedef void (*SPF_Camera_SetWindowLiveRotation_t)(float yaw, float pitch);

/**
 * @brief Sets the rotation limits for the window camera view.
 * @param left The new maximum left rotation.
 * @param right The new maximum right rotation.
 * @param up The new maximum upward rotation.
 * @param down The new maximum downward rotation.
 */
typedef void (*SPF_Camera_SetWindowRotationLimits_t)(float left, float right, float up, float down);

/**
 * @brief Sets the default rotation values for the window camera.
 * @param lr The new default left/right rotation.
 * @param ud The new default up/down rotation.
 */
typedef void (*SPF_Camera_SetWindowRotationDefaults_t)(float lr, float ud);

/**
 * @brief Gets the base Field of View (FOV) for the window camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWindowFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the window camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWindowFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the window camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetWindowFov_t)(float fov);



// --- Bumper Camera Specific Functions ---

/**
 * @brief Gets the offset from the vehicle's bumper.
 * @param[out] offset_x Pointer to store the X-coordinate of the offset.
 * @param[out] offset_y Pointer to store the Y-coordinate of the offset.
 * @param[out] offset_z Pointer to store the Z-coordinate of the offset.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBumperOffset_t)(float* offset_x, float* offset_y, float* offset_z);

// DEPRECATED: typedef bool (*SPF_Camera_GetBumperSettings_t)(float* offset_x, float* offset_y, float* offset_z);

/**
 * @brief Sets the offset from the vehicle's bumper.
 * @param offset_x The new X-coordinate of the offset.
 * @param offset_y The new Y-coordinate of the offset.
 * @param offset_z The new Z-coordinate of the offset.
 */
typedef void (*SPF_Camera_SetBumperOffset_t)(float offset_x, float offset_y, float offset_z);

/**
 * @brief Gets the base Field of View (FOV) for the bumper camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBumperFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the bumper camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetBumperFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the bumper camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetBumperFov_t)(float fov);



// --- Wheel Camera Specific Functions ---

/**
 * @brief Gets the offset from the vehicle's wheel.
 * @param[out] offset_x Pointer to store the X-coordinate of the offset.
 * @param[out] offset_y Pointer to store the Y-coordinate of the offset.
 * @param[out] offset_z Pointer to store the Z-coordinate of the offset.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWheelOffset_t)(float* offset_x, float* offset_y, float* offset_z);

// DEPRECATED: typedef bool (*SPF_Camera_GetWheelSettings_t)(float* offset_x, float* offset_y, float* offset_z);

/**
 * @brief Sets the offset from the vehicle's wheel.
 * @param offset_x The new X-coordinate of the offset.
 * @param offset_y The new Y-coordinate of the offset.
 * @param offset_z The new Z-coordinate of the offset.
 */
typedef void (*SPF_Camera_SetWheelOffset_t)(float offset_x, float offset_y, float offset_z);

/**
 * @brief Gets the base Field of View (FOV) for the wheel camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWheelFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the wheel camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWheelFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the wheel camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetWheelFov_t)(float fov);



// --- Cabin Camera Specific Functions ---

/**
 * @brief Gets the base Field of View (FOV) for the cabin camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetCabinFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the cabin camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetCabinFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the cabin camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetCabinFov_t)(float fov);



// --- TV Camera Specific Functions ---

/**
 * @brief Gets the maximum distance for the TV camera.
 * @param[out] max_distance Pointer to store the maximum distance value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTVMaxDistance_t)(float* max_distance);

/**
 * @brief Gets the camera uplift offset when near a prefab.
 * @param[out] x Pointer to store the X-coordinate of the uplift.
 * @param[out] y Pointer to store the Y-coordinate of the uplift.
 * @param[out] z Pointer to store the Z-coordinate of the uplift.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTVPrefabUplift_t)(float* x, float* y, float* z);

/**
 * @brief Gets the camera uplift offset when on a regular road.
 * @param[out] x Pointer to store the X-coordinate of the uplift.
 * @param[out] y Pointer to store the Y-coordinate of the uplift.
 * @param[out] z Pointer to store the Z-coordinate of the uplift.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTVRoadUplift_t)(float* x, float* y, float* z);

// DEPRECATED: typedef bool (*SPF_Camera_GetTVSettings_t)(float* max_distance, float* prefab_uplift_x, float* prefab_uplift_y, float* prefab_uplift_z, float* road_uplift_x, float* road_uplift_y, float* road_uplift_z);
// DEPRECATED: typedef void (*SPF_Camera_SetTVSettings_t)(float max_distance, float prefab_uplift_x, float prefab_uplift_y, float prefab_uplift_z, float road_uplift_x, float road_uplift_y, float road_uplift_z);

/**
 * @brief Sets the maximum distance for the TV camera.
 * @param max_distance The new maximum distance value.
 */
typedef void (*SPF_Camera_SetTVMaxDistance_t)(float max_distance);

/**
 * @brief Sets the camera uplift offset when near a prefab.
 * @param x The new X-coordinate of the uplift.
 * @param y The new Y-coordinate of the uplift.
 * @param z The new Z-coordinate of the uplift.
 */
typedef void (*SPF_Camera_SetTVPrefabUplift_t)(float x, float y, float z);

/**
 * @brief Sets the camera uplift offset when on a regular road.
 * @param x The new X-coordinate of the uplift.
 * @param y The new Y-coordinate of the uplift.
 * @param z The new Z-coordinate of the uplift.
 */
typedef void (*SPF_Camera_SetTVRoadUplift_t)(float x, float y, float z);

/**
 * @brief Gets the base Field of View (FOV) for the TV camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTVFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the TV camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetTVFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the TV camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetTVFov_t)(float fov);



// ---Camera World Coordinates ---

/**
 * @brief Gets the world coordinates (position) of the currently active camera.
 * @param[out] x Pointer to store the X-coordinate.
 * @param[out] y Pointer to store the Y-coordinate.
 * @param[out] z Pointer to store the Z-coordinate.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetWorldCoordinates_t)(float* x, float* y, float* z);

// --- Free Camera Specific Functions ---

/**
 * @brief Gets the position of the free (developer) camera.
 * @param[out] x Pointer to store the X-coordinate.
 * @param[out] y Pointer to store the Y-coordinate.
 * @param[out] z Pointer to store the Z-coordinate.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetFreePosition_t)(float* x, float* y, float* z);

/**
 * @brief Sets the position of the free (developer) camera.
 * @param x The new X-coordinate.
 * @param y The new Y-coordinate.
 * @param z The new Z-coordinate.
 */
typedef void (*SPF_Camera_SetFreePosition_t)(float x, float y, float z);

/**
 * @brief Gets the orientation of the free camera as a quaternion.
 * @param[out] x Pointer to store the X component of the quaternion.
 * @param[out] y Pointer to store the Y component of the quaternion.
 * @param[out] z Pointer to store the Z component of the quaternion.
 * @param[out] w Pointer to store the W component of the quaternion.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetFreeQuaternion_t)(float* x, float* y, float* z, float* w);

/**
 * @brief Gets the orientation of the free camera in terms of mouse look and roll.
 * @param[out] mouse_x Pointer to store the horizontal mouse look value.
 * @param[out] mouse_y Pointer to store the vertical mouse look value.
 * @param[out] roll Pointer to store the camera's roll value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetFreeOrientation_t)(float* mouse_x, float* mouse_y, float* roll);

/**
 * @brief Sets the orientation of the free camera using mouse look and roll values.
 * @param mouse_x The new horizontal mouse look value.
 * @param mouse_y The new vertical mouse look value.
 * @param roll The new roll value.
 */
typedef void (*SPF_Camera_SetFreeOrientation_t)(float mouse_x, float mouse_y, float roll);

/**
 * @brief Gets the base Field of View (FOV) for the free camera.
 * @param[out] fov Pointer to a float to store the FOV value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetFreeFov_t)(float* fov);

/**
 * @brief Gets the final, calculated Field of View (FOV) for the free camera.
 * @param[out] out_horiz Pointer to a float to store the final horizontal FOV.
 * @param[out] out_vert Pointer to a float to store the final vertical FOV.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetFreeFinalFov_t)(float* out_horiz, float* out_vert);

/**
 * @brief Sets the base Field of View (FOV) for the free camera.
 * @param fov The new FOV value.
 */
typedef void (*SPF_Camera_SetFreeFov_t)(float fov);

/**
 * @brief Gets the movement speed of the free camera.
 * @param[out] speed Pointer to store the speed value.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetFreeSpeed_t)(float* speed);

/**
 * @brief Sets the movement speed of the free camera.
 * @param speed The new speed value.
 */
typedef void (*SPF_Camera_SetFreeSpeed_t)(float speed);



// --- Debug Camera Functions ---

/**
 * @brief Enables or disables the debug camera system.
 * @param enable True to enable, false to disable.
 */
typedef void (*SPF_Camera_EnableDebugCamera_t)(bool enable);

/**
 * @brief Checks if the debug camera system is currently enabled.
 * @param[out] out_isEnabled Pointer to a boolean to store the result.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetDebugCameraEnabled_t)(bool* out_isEnabled);

/**
 * @enum SPF_DebugCameraMode
 * @brief Defines the different modes for the debug camera.
 */
typedef enum {
    SPF_DEBUG_CAMERA_MODE_SIMPLE = 0,
    SPF_DEBUG_CAMERA_MODE_VIDEO = 1,
    SPF_DEBUG_CAMERA_MODE_TRAFFIC = 2,
    SPF_DEBUG_CAMERA_MODE_CINEMATIC = 3,
    SPF_DEBUG_CAMERA_MODE_ANIMATED = 4,
    SPF_DEBUG_CAMERA_MODE_OVERSIZE = 5
} SPF_DebugCameraMode;

/**
 * @brief Sets the current mode for the debug camera.
 * @param mode The desired mode from the SPF_DebugCameraMode enum.
 */
typedef void (*SPF_Camera_SetDebugCameraMode_t)(SPF_DebugCameraMode mode);

/**
 * @brief Gets the current mode of the debug camera.
 * @param[out] out_mode Pointer to an SPF_DebugCameraMode to store the result.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetDebugCameraMode_t)(SPF_DebugCameraMode* out_mode);

/**
 * @enum SPF_DebugHudPosition
 * @brief Defines the possible screen positions for the debug HUD.
 */
typedef enum {
    SPF_DEBUG_HUD_POSITION_TOP_LEFT = 0,
    SPF_DEBUG_HUD_POSITION_BOTTOM_LEFT = 1,
    SPF_DEBUG_HUD_POSITION_TOP_RIGHT = 2,
    SPF_DEBUG_HUD_POSITION_BOTTOM_RIGHT = 3
} SPF_DebugHudPosition;

/**
 * @brief Sets the visibility of the debug camera's Head-Up Display (HUD).
 * @param visible True to show the HUD, false to hide it.
 */
typedef void (*SPF_Camera_SetDebugHudVisible_t)(bool visible);

/**
 * @brief Checks if the debug camera's HUD is currently visible.
 * @param[out] out_isVisible Pointer to a boolean to store the result.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetDebugHudVisible_t)(bool* out_isVisible);

/**
 * @brief Sets the screen position of the debug camera's HUD.
 * @param position The desired position from the SPF_DebugHudPosition enum.
 */
typedef void (*SPF_Camera_SetDebugHudPosition_t)(SPF_DebugHudPosition position);

/**
 * @brief Gets the current screen position of the debug camera's HUD.
 * @param[out] out_position Pointer to an SPF_DebugHudPosition to store the result.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetDebugHudPosition_t)(SPF_DebugHudPosition* out_position);

/**
 * @brief Sets the visibility of the main game UI while the debug camera is active.
 * @param visible True to show the game UI, false to hide it.
 */
typedef void (*SPF_Camera_SetDebugGameUiVisible_t)(bool visible);

/**
 * @brief Checks if the main game UI is visible while the debug camera is active.
 * @param[out] out_isVisible Pointer to a boolean to store the result.
 * @return True on success, false otherwise.
 */
typedef bool (*SPF_Camera_GetDebugGameUiVisible_t)(bool* out_isVisible);


// --- Debug Camera State Functions ---

/**
 * @struct SPF_CameraState_t
 * @brief Represents a snapshot of a camera's state (position, orientation, FOV).
 */
typedef struct {
    float pos_x, pos_y, pos_z;
    float mystery_float; // An unknown value used by the game's internal state
    float q_x, q_y, q_z, q_w; // Quaternion for orientation
    float fov;
} SPF_CameraState_t;

/**
 * @brief Gets the total number of saved camera states.
 * @return The number of states.
 */
typedef int (*SPF_Camera_GetStateCount_t)();

/**
 * @brief Gets the index of the currently active camera state.
 * @return The zero-based index of the current state.
 */
typedef int (*SPF_Camera_GetCurrentStateIndex_t)();

/**
 * @brief Retrieves a specific camera state by its index.
 * @param index The index of the state to retrieve.
 * @param[out] out_state Pointer to an SPF_CameraState_t struct to store the data.
 * @return True on success, false if the index is out of bounds.
 */
typedef bool (*SPF_Camera_GetState_t)(int index, SPF_CameraState_t* out_state);

/**
 * @brief Applies a saved camera state by its index, moving the camera to that state.
 * @param index The index of the state to apply.
 */
typedef void (*SPF_Camera_ApplyState_t)(int index);

/**
 * @brief Cycles to the next or previous camera state.
 * @param direction A positive value moves to the next state, a negative value moves to the previous.
 */
typedef void (*SPF_Camera_CycleState_t)(int direction);

/**
 * @brief Saves the current camera's position, orientation, and FOV as a new state.
 */
typedef void (*SPF_Camera_SaveCurrentState_t)();

/**
 * @brief Reloads all camera states from the configuration file.
 */
typedef void (*SPF_Camera_ReloadStatesFromFile_t)();

// --- Debug Camera State (In-Memory) Functions ---

/**
 * @brief Clears all camera states currently held in memory.
 */
typedef void (*SPF_Camera_ClearAllStatesInMemory_t)();

/**
 * @brief Adds a new camera state to the in-memory list.
 * @param state Pointer to the SPF_CameraState_t to add.
 */
typedef void (*SPF_Camera_AddStateInMemory_t)(const SPF_CameraState_t* state);

/**
 * @brief Edits an existing in-memory camera state at a specific index.
 * @param index The index of the state to edit.
 * @param newState Pointer to the new state data to apply.
 * @return True on success, false if the index is out of bounds.
 */
typedef bool (*SPF_Camera_EditStateInMemory_t)(int index, const SPF_CameraState_t* newState);

/**
 * @brief Deletes an in-memory camera state at a specific index.
 * @param index The index of the state to delete.
 */
typedef void (*SPF_Camera_DeleteStateInMemory_t)(int index);


// --- Debug Camera Animation Control ---
/**
 * @enum SPF_AnimPlaybackState
 * @brief Defines the possible playback states of a camera animation.
 */
typedef enum {
    SPF_ANIM_STOPPED,
    SPF_ANIM_PLAYING,
    SPF_ANIM_PAUSED
} SPF_AnimPlaybackState;

/**
 * @brief Starts playing the camera animation sequence.
 * @param startIndex The index of the state from which to start the animation.
 */
typedef void (*SPF_Anim_Play_t)(int startIndex);

/**
 * @brief Pauses the currently playing camera animation.
 */
typedef void (*SPF_Anim_Pause_t)();

/**
 * @brief Stops the camera animation and resets it.
 */
typedef void (*SPF_Anim_Stop_t)();

/**
 * @brief Jumps directly to a specific frame (state) in the animation sequence.
 * @param frameIndex The index of the state to go to.
 */
typedef void (*SPF_Anim_GoToFrame_t)(int frameIndex);

/**
 * @brief Scrubs the animation to a specific position.
 * @param position A value from 0.0 to 1.0 representing the progress through the animation.
 */
typedef void (*SPF_Anim_ScrubTo_t)(float position);

/**
 * @brief Sets the animation to play in reverse.
 * @param isReversed True to play in reverse, false for forward.
 */
typedef void (*SPF_Anim_SetReverse_t)(bool isReversed);

/**
 * @brief Gets the current playback state of the animation (e.g., playing, paused).
 * @return The current SPF_AnimPlaybackState.
 */
typedef SPF_AnimPlaybackState (*SPF_Anim_GetPlaybackState_t)();

/**
 * @brief Gets the index of the current frame (state) in the animation.
 * @return The zero-based index of the current frame.
 */
typedef int (*SPF_Anim_GetCurrentFrame_t)();

/**
 * @brief Gets the progress through the current frame transition.
 * @return A value from 0.0 to 1.0 indicating the interpolation progress.
 */
typedef float (*SPF_Anim_GetCurrentFrameProgress_t)();

/**
 * @brief Checks if the animation is currently set to play in reverse.
 * @return True if reversed, false otherwise.
 */
typedef bool (*SPF_Anim_IsReversed_t)();



/**
 * @struct SPF_Camera_API
 * @brief The struct containing all function pointers for the Camera API.
 *
 * An instance of this struct will be provided to plugins by the framework.
 */
typedef struct SPF_Camera_API {
    /**
     * @brief Switches the active in-game camera. See `SPF_Camera_SwitchTo_t` for details.
     */
    SPF_Camera_SwitchTo_t Cam_SwitchTo;

    /**
     * @brief Gets a pointer to the raw camera object. See `SPF_Camera_GetCameraObject_t` for details.
     */
    SPF_Camera_GetCameraObject_t Cam_GetCameraObject;

    /**
     * @brief Gets the type of the currently active camera. See `SPF_Camera_GetCurrentCamera_t` for details.
     */
    SPF_Camera_GetCurrentCamera_t Cam_GetCurrentCamera;

    /**
     * @brief Resets a specific camera to its default values. See `SPF_Camera_ResetToDefaults_t` for details.
     */
    SPF_Camera_ResetToDefaults_t Cam_ResetToDefaults;

    // --- Interior Camera ---
    /** @brief Gets the interior camera's seat position. See `SPF_Camera_GetInteriorSeatPos_t`. */
    SPF_Camera_GetInteriorSeatPos_t Cam_GetInteriorSeatPos;
    /** @brief Sets the interior camera's seat position. See `SPF_Camera_SetInteriorSeatPos_t`. */
    SPF_Camera_SetInteriorSeatPos_t Cam_SetInteriorSeatPos;
    /** @brief Gets the interior camera's head rotation. See `SPF_Camera_GetInteriorHeadRot_t`. */
    SPF_Camera_GetInteriorHeadRot_t Cam_GetInteriorHeadRot;
    /** @brief Sets the interior camera's head rotation. See `SPF_Camera_SetInteriorHeadRot_t`. */
    SPF_Camera_SetInteriorHeadRot_t Cam_SetInteriorHeadRot;
    /** @brief Gets the interior camera's base FOV. See `SPF_Camera_GetInteriorFov_t`. */
    SPF_Camera_GetInteriorFov_t Cam_GetInteriorFov;
    /** @brief Gets the interior camera's final (calculated) FOV. See `SPF_Camera_GetInteriorFinalFov_t`. */
    SPF_Camera_GetInteriorFinalFov_t Cam_GetInteriorFinalFov;
    /** @brief Sets the interior camera's base FOV. See `SPF_Camera_SetInteriorFov_t`. */
    SPF_Camera_SetInteriorFov_t Cam_SetInteriorFov;
    /** @brief Gets the interior camera's rotation limits. See `SPF_Camera_GetInteriorRotationLimits_t`. */
    SPF_Camera_GetInteriorRotationLimits_t Cam_GetInteriorRotationLimits;
    /** @brief Sets the interior camera's rotation limits. See `SPF_Camera_SetInteriorRotationLimits_t`. */
    SPF_Camera_SetInteriorRotationLimits_t Cam_SetInteriorRotationLimits;
    /** @brief Gets the interior camera's default rotation. See `SPF_Camera_GetInteriorRotationDefaults_t`. */
    SPF_Camera_GetInteriorRotationDefaults_t Cam_GetInteriorRotationDefaults;
    /** @brief Sets the interior camera's default rotation. See `SPF_Camera_SetInteriorRotationDefaults_t`. */
    SPF_Camera_SetInteriorRotationDefaults_t Cam_SetInteriorRotationDefaults;

    // --- Behind Camera ---
    /** @brief Gets the behind camera's live pitch, yaw, and zoom. See `SPF_Camera_GetBehindLiveState_t`. */
    SPF_Camera_GetBehindLiveState_t Cam_GetBehindLiveState;
    /** @brief Sets the behind camera's live pitch, yaw, and zoom. See `SPF_Camera_SetBehindLiveState_t`. */
    SPF_Camera_SetBehindLiveState_t Cam_SetBehindLiveState;
    /** @brief Gets the behind camera's distance settings. See `SPF_Camera_GetBehindDistanceSettings_t`. */
    SPF_Camera_GetBehindDistanceSettings_t Cam_GetBehindDistanceSettings;
    /** @brief Sets the behind camera's distance settings. See `SPF_Camera_SetBehindDistanceSettings_t`. */
    SPF_Camera_SetBehindDistanceSettings_t Cam_SetBehindDistanceSettings;
    /** @brief Gets the behind camera's elevation settings. See `SPF_Camera_GetBehindElevationSettings_t`. */
    SPF_Camera_GetBehindElevationSettings_t Cam_GetBehindElevationSettings;
    /** @brief Sets the behind camera's elevation settings. See `SPF_Camera_SetBehindElevationSettings_t`. */
    SPF_Camera_SetBehindElevationSettings_t Cam_SetBehindElevationSettings;
    /** @brief Gets the behind camera's pivot offset. See `SPF_Camera_GetBehindPivot_t`. */
    SPF_Camera_GetBehindPivot_t Cam_GetBehindPivot;
    /** @brief Sets the behind camera's pivot offset. See `SPF_Camera_SetBehindPivot_t`. */
    SPF_Camera_SetBehindPivot_t Cam_SetBehindPivot;
    /** @brief Gets the behind camera's dynamic offset settings. See `SPF_Camera_GetBehindDynamicOffset_t`. */
    SPF_Camera_GetBehindDynamicOffset_t Cam_GetBehindDynamicOffset;
    /** @brief Sets the behind camera's dynamic offset settings. See `SPF_Camera_SetBehindDynamicOffset_t`. */
    SPF_Camera_SetBehindDynamicOffset_t Cam_SetBehindDynamicOffset;
    /** @brief Gets the behind camera's base FOV. See `SPF_Camera_GetBehindFov_t`. */
    SPF_Camera_GetBehindFov_t Cam_GetBehindFov;
    /** @brief Gets the behind camera's final (calculated) FOV. See `SPF_Camera_GetBehindFinalFov_t`. */
    SPF_Camera_GetBehindFinalFov_t Cam_GetBehindFinalFov;
    /** @brief Sets the behind camera's base FOV. See `SPF_Camera_SetBehindFov_t`. */
    SPF_Camera_SetBehindFov_t Cam_SetBehindFov;

    // --- Top Camera ---
    /** @brief Gets the top-down camera's height range. See `SPF_Camera_GetTopHeight_t`. */
    SPF_Camera_GetTopHeight_t Cam_GetTopHeight;
    /** @brief Gets the top-down camera's movement speed. See `SPF_Camera_GetTopSpeed_t`. */
    SPF_Camera_GetTopSpeed_t Cam_GetTopSpeed;
    /** @brief Gets the top-down camera's forward/backward offsets. See `SPF_Camera_GetTopOffsets_t`. */
    SPF_Camera_GetTopOffsets_t Cam_GetTopOffsets;
    /** @brief Sets the top-down camera's height range. See `SPF_Camera_SetTopHeight_t`. */
    SPF_Camera_SetTopHeight_t Cam_SetTopHeight;
    /** @brief Sets the top-down camera's movement speed. See `SPF_Camera_SetTopSpeed_t`. */
    SPF_Camera_SetTopSpeed_t Cam_SetTopSpeed;
    /** @brief Sets the top-down camera's forward/backward offsets. See `SPF_Camera_SetTopOffsets_t`. */
    SPF_Camera_SetTopOffsets_t Cam_SetTopOffsets;
    /** @brief Gets the top-down camera's base FOV. See `SPF_Camera_GetTopFov_t`. */
    SPF_Camera_GetTopFov_t Cam_GetTopFov;
    /** @brief Gets the top-down camera's final (calculated) FOV. See `SPF_Camera_GetTopFinalFov_t`. */
    SPF_Camera_GetTopFinalFov_t Cam_GetTopFinalFov;
    /** @brief Sets the top-down camera's base FOV. See `SPF_Camera_SetTopFov_t`. */
    SPF_Camera_SetTopFov_t Cam_SetTopFov;

    // --- Window Camera ---
    /** @brief Gets the window camera's head offset. See `SPF_Camera_GetWindowHeadOffset_t`. */
    SPF_Camera_GetWindowHeadOffset_t Cam_GetWindowHeadOffset;
    /** @brief Gets the window camera's live rotation. See `SPF_Camera_GetWindowLiveRotation_t`. */
    SPF_Camera_GetWindowLiveRotation_t Cam_GetWindowLiveRotation;
    /** @brief Gets the window camera's rotation limits. See `SPF_Camera_GetWindowRotationLimits_t`. */
    SPF_Camera_GetWindowRotationLimits_t Cam_GetWindowRotationLimits;
    /** @brief Gets the window camera's default rotation. See `SPF_Camera_GetWindowRotationDefaults_t`. */
    SPF_Camera_GetWindowRotationDefaults_t Cam_GetWindowRotationDefaults;
    /** @brief Sets the window camera's head offset. See `SPF_Camera_SetWindowHeadOffset_t`. */
    SPF_Camera_SetWindowHeadOffset_t Cam_SetWindowHeadOffset;
    /** @brief Sets the window camera's live rotation. See `SPF_Camera_SetWindowLiveRotation_t`. */
    SPF_Camera_SetWindowLiveRotation_t Cam_SetWindowLiveRotation;
    /** @brief Sets the window camera's rotation limits. See `SPF_Camera_SetWindowRotationLimits_t`. */
    SPF_Camera_SetWindowRotationLimits_t Cam_SetWindowRotationLimits;
    /** @brief Sets the window camera's default rotation. See `SPF_Camera_SetWindowRotationDefaults_t`. */
    SPF_Camera_SetWindowRotationDefaults_t Cam_SetWindowRotationDefaults;
    /** @brief Gets the window camera's base FOV. See `SPF_Camera_GetWindowFov_t`. */
    SPF_Camera_GetWindowFov_t Cam_GetWindowFov;
    /** @brief Gets the window camera's final (calculated) FOV. See `SPF_Camera_GetWindowFinalFov_t`. */
    SPF_Camera_GetWindowFinalFov_t Cam_GetWindowFinalFov;
    /** @brief Sets the window camera's base FOV. See `SPF_Camera_SetWindowFov_t`. */
    SPF_Camera_SetWindowFov_t Cam_SetWindowFov;

    // --- Bumper Camera ---
    /** @brief Gets the bumper camera's offset. See `SPF_Camera_GetBumperOffset_t`. */
    SPF_Camera_GetBumperOffset_t Cam_GetBumperOffset;
    /** @brief Sets the bumper camera's offset. See `SPF_Camera_SetBumperOffset_t`. */
    SPF_Camera_SetBumperOffset_t Cam_SetBumperOffset;
    /** @brief Gets the bumper camera's base FOV. See `SPF_Camera_GetBumperFov_t`. */
    SPF_Camera_GetBumperFov_t Cam_GetBumperFov;
    /** @brief Gets the bumper camera's final (calculated) FOV. See `SPF_Camera_GetBumperFinalFov_t`. */
    SPF_Camera_GetBumperFinalFov_t Cam_GetBumperFinalFov;
    /** @brief Sets the bumper camera's base FOV. See `SPF_Camera_SetBumperFov_t`. */
    SPF_Camera_SetBumperFov_t Cam_SetBumperFov;

    // --- Wheel Camera ---
    /** @brief Gets the wheel camera's offset. See `SPF_Camera_GetWheelOffset_t`. */
    SPF_Camera_GetWheelOffset_t Cam_GetWheelOffset;
    /** @brief Sets the wheel camera's offset. See `SPF_Camera_SetWheelOffset_t`. */
    SPF_Camera_SetWheelOffset_t Cam_SetWheelOffset;
    /** @brief Gets the wheel camera's base FOV. See `SPF_Camera_GetWheelFov_t`. */
    SPF_Camera_GetWheelFov_t Cam_GetWheelFov;
    /** @brief Gets the wheel camera's final (calculated) FOV. See `SPF_Camera_GetWheelFinalFov_t`. */
    SPF_Camera_GetWheelFinalFov_t Cam_GetWheelFinalFov;
    /** @brief Sets the wheel camera's base FOV. See `SPF_Camera_SetWheelFov_t`. */
    SPF_Camera_SetWheelFov_t Cam_SetWheelFov;

    // --- Cabin Camera ---
    /** @brief Gets the cabin camera's base FOV. See `SPF_Camera_GetCabinFov_t`. */
    SPF_Camera_GetCabinFov_t Cam_GetCabinFov;
    /** @brief Gets the cabin camera's final (calculated) FOV. See `SPF_Camera_GetCabinFinalFov_t`. */
    SPF_Camera_GetCabinFinalFov_t Cam_GetCabinFinalFov;
    /** @brief Sets the cabin camera's base FOV. See `SPF_Camera_SetCabinFov_t`. */
    SPF_Camera_SetCabinFov_t Cam_SetCabinFov;

    // --- TV Camera ---
    /** @brief Gets the TV camera's max distance. See `SPF_Camera_GetTVMaxDistance_t`. */
    SPF_Camera_GetTVMaxDistance_t Cam_GetTVMaxDistance;
    /** @brief Gets the TV camera's uplift near prefabs. See `SPF_Camera_GetTVPrefabUplift_t`. */
    SPF_Camera_GetTVPrefabUplift_t Cam_GetTVPrefabUplift;
    /** @brief Gets the TV camera's uplift on roads. See `SPF_Camera_GetTVRoadUplift_t`. */
    SPF_Camera_GetTVRoadUplift_t Cam_GetTVRoadUplift;
    /** @brief Sets the TV camera's max distance. See `SPF_Camera_SetTVMaxDistance_t`. */
    SPF_Camera_SetTVMaxDistance_t Cam_SetTVMaxDistance;
    /** @brief Sets the TV camera's uplift near prefabs. See `SPF_Camera_SetTVPrefabUplift_t`. */
    SPF_Camera_SetTVPrefabUplift_t Cam_SetTVPrefabUplift;
    /** @brief Sets the TV camera's uplift on roads. See `SPF_Camera_SetTVRoadUplift_t`. */
    SPF_Camera_SetTVRoadUplift_t Cam_SetTVRoadUplift;
    /** @brief Gets the TV camera's base FOV. See `SPF_Camera_GetTVFov_t`. */
    SPF_Camera_GetTVFov_t Cam_GetTVFov;
    /** @brief Gets the TV camera's final (calculated) FOV. See `SPF_Camera_GetTVFinalFov_t`. */
    SPF_Camera_GetTVFinalFov_t Cam_GetTVFinalFov;
    /** @brief Sets the TV camera's base FOV. See `SPF_Camera_SetTVFov_t`. */
    SPF_Camera_SetTVFov_t Cam_SetTVFov;

    // ---Camera World Coordinates ---
    /** @brief Gets the world coordinates of the active camera. See `SPF_Camera_GetWorldCoordinates_t`. */
    SPF_Camera_GetWorldCoordinates_t Cam_GetCameraWorldCoordinates;

    // --- Free Camera ---
    /** @brief Gets the free camera's position. See `SPF_Camera_GetFreePosition_t`. */
    SPF_Camera_GetFreePosition_t Cam_GetFreePosition;
    /** @brief Sets the free camera's position. See `SPF_Camera_SetFreePosition_t`. */
    SPF_Camera_SetFreePosition_t Cam_SetFreePosition;
    /** @brief Gets the free camera's orientation as a quaternion. See `SPF_Camera_GetFreeQuaternion_t`. */
    SPF_Camera_GetFreeQuaternion_t Cam_GetFreeQuaternion;
    /** @brief Gets the free camera's orientation (mouse look/roll). See `SPF_Camera_GetFreeOrientation_t`. */
    SPF_Camera_GetFreeOrientation_t Cam_GetFreeOrientation;
    /** @brief Sets the free camera's orientation (mouse look/roll). See `SPF_Camera_SetFreeOrientation_t`. */
    SPF_Camera_SetFreeOrientation_t Cam_SetFreeOrientation;
    /** @brief Gets the free camera's base FOV. See `SPF_Camera_GetFreeFov_t`. */
    SPF_Camera_GetFreeFov_t Cam_GetFreeFov;
    /** @brief Gets the free camera's final (calculated) FOV. See `SPF_Camera_GetFreeFinalFov_t`. */
    SPF_Camera_GetFreeFinalFov_t Cam_GetFreeFinalFov;
    /** @brief Sets the free camera's base FOV. See `SPF_Camera_SetFreeFov_t`. */
    SPF_Camera_SetFreeFov_t Cam_SetFreeFov;
    /** @brief Gets the free camera's movement speed. See `SPF_Camera_GetFreeSpeed_t`. */
    SPF_Camera_GetFreeSpeed_t Cam_GetFreeSpeed;
    /** @brief Sets the free camera's movement speed. See `SPF_Camera_SetFreeSpeed_t`. */
    SPF_Camera_SetFreeSpeed_t Cam_SetFreeSpeed;

    // --- Debug Camera ---
    /** @brief Enables or disables the debug camera system. See `SPF_Camera_EnableDebugCamera_t`. */
    SPF_Camera_EnableDebugCamera_t Cam_EnableDebugCamera;
    /** @brief Checks if the debug camera is enabled. See `SPF_Camera_GetDebugCameraEnabled_t`. */
    SPF_Camera_GetDebugCameraEnabled_t Cam_GetDebugCameraEnabled;
    /** @brief Sets the debug camera's mode. See `SPF_Camera_SetDebugCameraMode_t`. */
    SPF_Camera_SetDebugCameraMode_t Cam_SetDebugCameraMode;
    /** @brief Gets the debug camera's current mode. See `SPF_Camera_GetDebugCameraMode_t`. */
    SPF_Camera_GetDebugCameraMode_t Cam_GetDebugCameraMode;

    // Debug Camera HUD & UI
    /** @brief Sets the visibility of the debug HUD. See `SPF_Camera_SetDebugHudVisible_t`. */
    SPF_Camera_SetDebugHudVisible_t Cam_SetDebugHudVisible;
    /** @brief Checks if the debug HUD is visible. See `SPF_Camera_GetDebugHudVisible_t`. */
    SPF_Camera_GetDebugHudVisible_t Cam_GetDebugHudVisible;
    /** @brief Sets the position of the debug HUD. See `SPF_Camera_SetDebugHudPosition_t`. */
    SPF_Camera_SetDebugHudPosition_t Cam_SetDebugHudPosition;
    /** @brief Gets the position of the debug HUD. See `SPF_Camera_GetDebugHudPosition_t`. */
    SPF_Camera_GetDebugHudPosition_t Cam_GetDebugHudPosition;
    /** @brief Sets the visibility of the main game UI. See `SPF_Camera_SetDebugGameUiVisible_t`. */
    SPF_Camera_SetDebugGameUiVisible_t Cam_SetDebugGameUiVisible;
    /** @brief Checks if the main game UI is visible. See `SPF_Camera_GetDebugGameUiVisible_t`. */
    SPF_Camera_GetDebugGameUiVisible_t Cam_GetDebugGameUiVisible;

    // --- Debug Camera State Management ---
    /** @brief Gets the number of saved camera states. See `SPF_Camera_GetStateCount_t`. */
    SPF_Camera_GetStateCount_t Cam_GetStateCount;
    /** @brief Gets the index of the current camera state. See `SPF_Camera_GetCurrentStateIndex_t`. */
    SPF_Camera_GetCurrentStateIndex_t Cam_GetCurrentStateIndex;
    /** @brief Retrieves a camera state by index. See `SPF_Camera_GetState_t`. */
    SPF_Camera_GetState_t Cam_GetState;
    /** @brief Applies a camera state by index. See `SPF_Camera_ApplyState_t`. */
    SPF_Camera_ApplyState_t Cam_ApplyState;
    /** @brief Cycles to the next/previous camera state. See `SPF_Camera_CycleState_t`. */
    SPF_Camera_CycleState_t Cam_CycleState;
    /** @brief Saves the current camera view as a new state. See `SPF_Camera_SaveCurrentState_t`. */
    SPF_Camera_SaveCurrentState_t Cam_SaveCurrentState;
    /** @brief Reloads all camera states from the config file. See `SPF_Camera_ReloadStatesFromFile_t`. */
    SPF_Camera_ReloadStatesFromFile_t Cam_ReloadStatesFromFile;

    // New In-Memory State Functions
    /** @brief Clears all camera states from memory. See `SPF_Camera_ClearAllStatesInMemory_t`. */
    SPF_Camera_ClearAllStatesInMemory_t Cam_ClearAllStatesInMemory;
    /** @brief Adds a new camera state to memory. See `SPF_Camera_AddStateInMemory_t`. */
    SPF_Camera_AddStateInMemory_t Cam_AddStateInMemory;
    /** @brief Edits an in-memory camera state. See `SPF_Camera_EditStateInMemory_t`. */
    SPF_Camera_EditStateInMemory_t Cam_EditStateInMemory;
    /** @brief Deletes an in-memory camera state. See `SPF_Camera_DeleteStateInMemory_t`. */
    SPF_Camera_DeleteStateInMemory_t Cam_DeleteStateInMemory;

    // --- Debug Camera Animation Control ---
    /** @brief Starts the camera animation. See `SPF_Anim_Play_t`. */
    SPF_Anim_Play_t Cam_Anim_Play;
    /** @brief Pauses the camera animation. See `SPF_Anim_Pause_t`. */
    SPF_Anim_Pause_t Cam_Anim_Pause;
    /** @brief Stops the camera animation. See `SPF_Anim_Stop_t`. */
    SPF_Anim_Stop_t Cam_Anim_Stop;
    /** @brief Jumps to a specific frame in the animation. See `SPF_Anim_GoToFrame_t`. */
    SPF_Anim_GoToFrame_t Cam_Anim_GoToFrame;
    /** @brief Scrubs the animation to a specific position. See `SPF_Anim_ScrubTo_t`. */
    SPF_Anim_ScrubTo_t Cam_Anim_ScrubTo;
    /** @brief Toggles reverse playback for the animation. See `SPF_Anim_SetReverse_t`. */
    SPF_Anim_SetReverse_t Cam_Anim_SetReverse;
    /** @brief Gets the current animation playback state. See `SPF_Anim_GetPlaybackState_t`. */
    SPF_Anim_GetPlaybackState_t Cam_Anim_GetPlaybackState;
    /** @brief Gets the current animation frame index. See `SPF_Anim_GetCurrentFrame_t`. */
    SPF_Anim_GetCurrentFrame_t Cam_Anim_GetCurrentFrame;
    /** @brief Gets the progress within the current frame transition. See `SPF_Anim_GetCurrentFrameProgress_t`. */
    SPF_Anim_GetCurrentFrameProgress_t Cam_Anim_GetCurrentFrameProgress;
    /** @brief Checks if the animation is playing in reverse. See `SPF_Anim_IsReversed_t`. */
    SPF_Anim_IsReversed_t Cam_Anim_IsReversed;


} SPF_Camera_API;


#ifdef __cplusplus
}
#endif