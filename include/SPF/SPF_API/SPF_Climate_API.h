/**
 * @file SPF_Climate_API.h
 * @brief API for reading and controlling the game's climate and weather systems.
 *
 * @details This API provides plugins with full access to the game's climate and weather
 *          simulation. It covers climate selection, sun profiles, weather modes, bad weather
 *          factors, environment profile settings, and all visual attributes (colors, fog,
 *          rain, snow, bloom, depth of field, etc.) organized by sun profile — each with
 *          support for multiple variations and smooth blending between active and next profiles.
 *
 * ================================================================================================
 * KEY CONCEPTS
 * ================================================================================================
 *
 * 1. **ProfileRef (Profile Reference)** — Many climate attributes are organized per "sun profile"
 *    (a set of visual parameters for a specific sun position). A ProfileRef specifies which
 *    sun profile you want to read from or write to, identified by its index and whether it
 *    belongs to the "nice weather" (isBad = false) or "bad weather" (isBad = true) container.
 *    Use `CL_ActiveProfile()` or `CL_NextProfile()` to get the current/next active profile.
 *
 * 2. **Variations** — Each sun profile attribute can have multiple "variations" (e.g., slightly
 *    different fog colors for the same sun position). Use the `ByIndex` variants to access a
 *    specific variation, or the base `Get`/`Set` functions to access the *active* variation
 *    (determined by the game engine).
 *
 * 3. **Blended Values** — The game smoothly transitions between the "active" and "next" sun
 *    profiles. The `GetBlended*` functions return the current interpolated value between the
 *    two profiles at the current transition progress. The `SetBlended*` functions allow you
 *    to set a desired blended value, and the API will distribute the change across both
 *    profiles while respecting the transition progress.
 *
 * ================================================================================================
 * STRING HANDLING
 * ================================================================================================
 * Functions returning strings use the buffer/size pattern:
 *   - You provide a pointer to a char array and its size.
 *   - The function returns the actual length of the string (excluding null terminator).
 *   - If the return value >= bufferSize, the string was truncated.
 *
 * ================================================================================================
 * USAGE EXAMPLE (C++)
 * ================================================================================================
 * @code
 * void MyPlugin_OnActivated(const SPF_Core_API* api) {
 *     // Check if climate service is ready
 *     if (!api->climate->CL_IsReady()) return;
 *
 *     // Read current climate info
 *     char name[64];
 *     api->climate->CL_GetCurrentClimateName(name, sizeof(name));
 *
 *     // Read a blended attribute (fog color at current transition point)
 *     SPF_Climate_Vector3 fogColor;
 *     api->climate->CL_GetBlendedFogColor(&fogColor);
 *
 *     // Set rain intensity for the current active profile
 *     SPF_Climate_ProfileRef profile = api->climate->CL_ActiveProfile();
 *     api->climate->CL_SetRainIntensity(profile, 0.5f);
 *
 *     // Force bad weather
 *     api->climate->CL_SetBadWeatherFactor(1.0f);
 *
 *     // Change climate
 *     api->climate->CL_SetClimate(someToken, true);
 * }
 * @endcode
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// DATA TYPES
// =================================================================================================

/**
 * @brief Identifies a specific sun profile within the nice or bad weather container.
 *
 * @details Many climate attributes (fog, colors, rain, etc.) are stored per "sun profile".
 *          A profile reference tells the API which sun profile entry to read from or write to.
 *          Obtain a valid ProfileRef via CL_ActiveProfile() or CL_NextProfile().
 *
 * @param index The zero-based index of the sun profile within its container.
 * @param isBad When false, selects the "nice weather" container. When true, selects the
 *              "bad weather" container. Bad weather profiles typically contain overcast,
 *              rain, snow, and storm visual settings.
 */
typedef struct {
  uint64_t index;
  bool isBad;
} SPF_Climate_ProfileRef;

/**
 * @brief A 3-component floating-point vector.
 *
 * @details Used throughout the API for colors (ambient, diffuse, specular, sky, fog, etc.)
 *          and other three-component values.
 */
typedef struct {
  float x;
  float y;
  float z;
} SPF_Climate_Vector3;

/**
 * @brief A 2-component floating-point vector.
 *
 * @details Used for attributes like cloud shadow area size, cloud shadow speed,
 *          and snow flake size range.
 */
typedef struct {
  float x;
  float y;
} SPF_Climate_Vector2;

// =================================================================================================
// REUSABLE FUNCTION SHAPE TYPEDEFS
// =================================================================================================
// These define the calling convention for groups of similar functions so that every
// individual attribute (temperature, fog color, etc.) does not require its own typedef.

// --- Float variant shapes ---

/**
 * @brief Shape: Gets the number of variations available for a float attribute on a given profile.
 * @param profile The sun profile to query.
 * @return The number of variations.
 */
typedef uint64_t (*SPF_CL_Profile_GetCount_t)(SPF_Climate_ProfileRef profile);

/**
 * @brief Shape: Gets a float attribute for a given profile (active variation).
 * @param profile The sun profile to read from.
 * @return The float value.
 */
typedef float (*SPF_CL_Profile_GetFloat_t)(SPF_Climate_ProfileRef profile);

/**
 * @brief Shape: Sets a float attribute for a given profile (active variation).
 * @param profile The sun profile to write to.
 * @param value The float value to set.
 */
typedef void (*SPF_CL_Profile_SetFloat_t)(SPF_Climate_ProfileRef profile, float value);

/**
 * @brief Shape: Gets a float attribute for a specific variation index on a given profile.
 * @param profile The sun profile to read from.
 * @param variationIndex The zero-based variation index.
 * @return The float value at the given variation.
 */
typedef float (*SPF_CL_Profile_GetFloatByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex);

/**
 * @brief Shape: Sets a float attribute for a specific variation index on a given profile.
 * @param profile The sun profile to write to.
 * @param variationIndex The zero-based variation index.
 * @param value The float value to set.
 */
typedef void (*SPF_CL_Profile_SetFloatByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, float value);

/**
 * @brief Shape: Gets the interpolated (blended) float value between the active and next profile.
 * @return The blended float value at the current transition progress.
 */
typedef float (*SPF_CL_GetBlendedFloat_t)();

/**
 * @brief Shape: Sets a desired blended float value by distributing the change across both
 *               active and next profiles, respecting the current transition progress.
 * @param blendedValue The desired blended (interpolated) value.
 * @param minimumValue The minimum possible value (used to extrapolate when blendedValue is below current).
 * @param maximumValue The maximum possible value (used to extrapolate when blendedValue is above current).
 */
typedef void (*SPF_CL_SetBlendedFloat_t)(float blendedValue, float minimumValue, float maximumValue);

// --- Int32 variant shapes ---

/**
 * @brief Shape: Gets the number of variations for an int32 attribute on a given profile.
 */
typedef uint64_t (*SPF_CL_Profile_GetInt32Count_t)(SPF_Climate_ProfileRef profile);

/**
 * @brief Shape: Gets an int32 attribute for a given profile (active variation).
 */
typedef int32_t (*SPF_CL_Profile_GetInt32_t)(SPF_Climate_ProfileRef profile);

/**
 * @brief Shape: Sets an int32 attribute for a given profile (active variation).
 */
typedef void (*SPF_CL_Profile_SetInt32_t)(SPF_Climate_ProfileRef profile, int32_t value);

/**
 * @brief Shape: Gets an int32 attribute for a specific variation index on a given profile.
 */
typedef int32_t (*SPF_CL_Profile_GetInt32ByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex);

/**
 * @brief Shape: Sets an int32 attribute for a specific variation index on a given profile.
 */
typedef void (*SPF_CL_Profile_SetInt32ByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, int32_t value);

// --- Vector3 variant shapes ---

/**
 * @brief Shape: Gets the number of variations for a Vector3 attribute on a given profile.
 */
typedef uint64_t (*SPF_CL_Profile_GetVector3Count_t)(SPF_Climate_ProfileRef profile);

/**
 * @brief Shape: Gets a Vector3 attribute for a given profile (active variation).
 * @param profile The sun profile to read from.
 * @param[out] outResult Pointer to a Vector3 that will receive the result.
 */
typedef void (*SPF_CL_Profile_GetVector3_t)(SPF_Climate_ProfileRef profile, SPF_Climate_Vector3* outResult);

/**
 * @brief Shape: Sets a Vector3 attribute for a given profile (active variation).
 * @param profile The sun profile to write to.
 * @param value The Vector3 value to set.
 */
typedef void (*SPF_CL_Profile_SetVector3_t)(SPF_Climate_ProfileRef profile, SPF_Climate_Vector3 value);

/**
 * @brief Shape: Gets a Vector3 attribute for a specific variation index on a given profile.
 */
typedef void (*SPF_CL_Profile_GetVector3ByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, SPF_Climate_Vector3* outResult);

/**
 * @brief Shape: Sets a Vector3 attribute for a specific variation index on a given profile.
 */
typedef void (*SPF_CL_Profile_SetVector3ByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, SPF_Climate_Vector3 value);

/**
 * @brief Shape: Gets the interpolated (blended) Vector3 value between active and next profile.
 */
typedef void (*SPF_CL_GetBlendedVector3_t)(SPF_Climate_Vector3* outResult);

/**
 * @brief Shape: Sets a desired blended Vector3 value by distributing across both profiles.
 * @param blendedValue The desired blended (interpolated) value.
 * @param maximumComponent The maximum possible value for any component (used for extrapolation).
 */
typedef void (*SPF_CL_SetBlendedVector3_t)(SPF_Climate_Vector3 blendedValue, float maximumComponent);

// --- Vector2 variant shapes ---

/**
 * @brief Shape: Gets the number of variations for a Vector2 attribute on a given profile.
 */
typedef uint64_t (*SPF_CL_Profile_GetVector2Count_t)(SPF_Climate_ProfileRef profile);

/**
 * @brief Shape: Gets a Vector2 attribute for a given profile (active variation).
 */
typedef void (*SPF_CL_Profile_GetVector2_t)(SPF_Climate_ProfileRef profile, SPF_Climate_Vector2* outResult);

/**
 * @brief Shape: Sets a Vector2 attribute for a given profile (active variation).
 */
typedef void (*SPF_CL_Profile_SetVector2_t)(SPF_Climate_ProfileRef profile, SPF_Climate_Vector2 value);

/**
 * @brief Shape: Gets a Vector2 attribute for a specific variation index on a given profile.
 */
typedef void (*SPF_CL_Profile_GetVector2ByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, SPF_Climate_Vector2* outResult);

/**
 * @brief Shape: Sets a Vector2 attribute for a specific variation index on a given profile.
 */
typedef void (*SPF_CL_Profile_SetVector2ByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, SPF_Climate_Vector2 value);

/**
 * @brief Shape: Gets the interpolated (blended) Vector2 value between active and next profile.
 */
typedef void (*SPF_CL_GetBlendedVector2_t)(SPF_Climate_Vector2* outResult);

/**
 * @brief Shape: Sets a desired blended Vector2 value by distributing across both profiles.
 */
typedef void (*SPF_CL_SetBlendedVector2_t)(SPF_Climate_Vector2 blendedValue, float maximumComponent);

// --- Texture string variant shapes ---

/**
 * @brief Shape: Gets the number of variations for a texture attribute on a given profile.
 */
typedef uint64_t (*SPF_CL_Profile_GetTextureCount_t)(SPF_Climate_ProfileRef profile);

/**
 * @brief Shape: Gets a texture name string for a given profile (active variation).
 * @param profile The sun profile to read from.
 * @param[out] outBuffer Buffer to receive the texture name.
 * @param bufferSize Size of the output buffer.
 * @return The actual length of the texture name string (excluding null terminator).
 */
typedef int (*SPF_CL_Profile_GetTexture_t)(SPF_Climate_ProfileRef profile, char* outBuffer, int bufferSize);

/**
 * @brief Shape: Sets a texture name string for a given profile (active variation).
 */
typedef void (*SPF_CL_Profile_SetTexture_t)(SPF_Climate_ProfileRef profile, const char* textureName);

/**
 * @brief Shape: Gets a texture name string for a specific variation index on a given profile.
 */
typedef int (*SPF_CL_Profile_GetTextureByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, char* outBuffer, int bufferSize);

/**
 * @brief Shape: Sets a texture name string for a specific variation index on a given profile.
 */
typedef void (*SPF_CL_Profile_SetTextureByIndex_t)(SPF_Climate_ProfileRef profile, uint64_t variationIndex, const char* textureName);

// =================================================================================================
// MACROS FOR EXPANDING ATTRIBUTE API FIELDS
// =================================================================================================
// These macros expand to named fields in the API struct below, avoiding repetitive manual
// declarations while keeping individual function names descriptive and self-documenting.

/**
 * @brief Expands to 7 fields for a float-profile attribute (e.g., Temperature, FogDensity).
 * Produces: GetXCount, GetX, SetX, GetXByIndex, SetXByIndex, GetBlendedX, SetBlendedX
 */
#define SPF_CL_FLOAT_PROFILE_API(name) \
  /** @brief Gets the number of variations for `name`. */ \
  SPF_CL_Profile_GetCount_t Get##name##Count; \
  /** @brief Gets the current `name` value for the specified profile (active variation). */ \
  SPF_CL_Profile_GetFloat_t Get##name; \
  /** @brief Sets the `name` value for the specified profile (active variation). */ \
  SPF_CL_Profile_SetFloat_t Set##name; \
  /** @brief Gets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_GetFloatByIndex_t Get##name##ByIndex; \
  /** @brief Sets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_SetFloatByIndex_t Set##name##ByIndex; \
  /** @brief Gets the blended (interpolated) `name` between active and next profile. */ \
  SPF_CL_GetBlendedFloat_t GetBlended##name; \
  /** @brief Sets a desired blended `name` value, distributing across both profiles. */ \
  SPF_CL_SetBlendedFloat_t SetBlended##name

/**
 * @brief Expands to 5 fields for an int32-profile attribute (e.g., Weight, WindType).
 * Produces: GetXCount, GetX, SetX, GetXByIndex, SetXByIndex
 */
#define SPF_CL_INT32_PROFILE_API(name) \
  /** @brief Gets the number of variations for `name`. */ \
  SPF_CL_Profile_GetInt32Count_t Get##name##Count; \
  /** @brief Gets the current `name` value for the specified profile (active variation). */ \
  SPF_CL_Profile_GetInt32_t Get##name; \
  /** @brief Sets the `name` value for the specified profile (active variation). */ \
  SPF_CL_Profile_SetInt32_t Set##name; \
  /** @brief Gets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_GetInt32ByIndex_t Get##name##ByIndex; \
  /** @brief Sets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_SetInt32ByIndex_t Set##name##ByIndex

/**
 * @brief Expands to 7 fields for a Vector3-profile attribute (e.g., Ambient, FogColor).
 * Produces: GetXCount, GetX, SetX, GetXByIndex, SetXByIndex, GetBlendedX, SetBlendedX
 */
#define SPF_CL_VECTOR3_PROFILE_API(name) \
  /** @brief Gets the number of variations for `name`. */ \
  SPF_CL_Profile_GetVector3Count_t Get##name##Count; \
  /** @brief Gets the current `name` for the specified profile (active variation). */ \
  SPF_CL_Profile_GetVector3_t Get##name; \
  /** @brief Sets the `name` for the specified profile (active variation). */ \
  SPF_CL_Profile_SetVector3_t Set##name; \
  /** @brief Gets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_GetVector3ByIndex_t Get##name##ByIndex; \
  /** @brief Sets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_SetVector3ByIndex_t Set##name##ByIndex; \
  /** @brief Gets the blended (interpolated) `name` between active and next profile. */ \
  SPF_CL_GetBlendedVector3_t GetBlended##name; \
  /** @brief Sets a desired blended `name`, distributing across both profiles. */ \
  SPF_CL_SetBlendedVector3_t SetBlended##name

/**
 * @brief Expands to 7 fields for a Vector2-profile attribute (e.g., CloudShadowAreaSize).
 * Produces: GetXCount, GetX, SetX, GetXByIndex, SetXByIndex, GetBlendedX, SetBlendedX
 */
#define SPF_CL_VECTOR2_PROFILE_API(name) \
  /** @brief Gets the number of variations for `name`. */ \
  SPF_CL_Profile_GetVector2Count_t Get##name##Count; \
  /** @brief Gets the current `name` for the specified profile (active variation). */ \
  SPF_CL_Profile_GetVector2_t Get##name; \
  /** @brief Sets the `name` for the specified profile (active variation). */ \
  SPF_CL_Profile_SetVector2_t Set##name; \
  /** @brief Gets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_GetVector2ByIndex_t Get##name##ByIndex; \
  /** @brief Sets `name` at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_SetVector2ByIndex_t Set##name##ByIndex; \
  /** @brief Gets the blended (interpolated) `name` between active and next profile. */ \
  SPF_CL_GetBlendedVector2_t GetBlended##name; \
  /** @brief Sets a desired blended `name`, distributing across both profiles. */ \
  SPF_CL_SetBlendedVector2_t SetBlended##name

/**
 * @brief Expands to 5 fields for a texture-profile attribute (e.g., SkyboxTexture).
 * Produces: GetXCount, GetX, SetX, GetXByIndex, SetXByIndex
 */
#define SPF_CL_TEXTURE_PROFILE_API(name) \
  /** @brief Gets the number of variations for `name`. */ \
  SPF_CL_Profile_GetTextureCount_t Get##name##Count; \
  /** @brief Gets the current `name` texture path for the specified profile (active variation). */ \
  SPF_CL_Profile_GetTexture_t Get##name; \
  /** @brief [STUB] Sets the `name` texture path for the specified profile (active variation). Not yet implemented. */ \
  SPF_CL_Profile_SetTexture_t Set##name; \
  /** @brief Gets the `name` texture path at a specific variation index for the specified profile. */ \
  SPF_CL_Profile_GetTextureByIndex_t Get##name##ByIndex; \
  /** @brief [STUB] Sets the `name` texture path at a specific variation index for the specified profile. Not yet implemented. */ \
  SPF_CL_Profile_SetTextureByIndex_t Set##name##ByIndex

// =================================================================================================
// MAIN CLIMATE API STRUCT
// =================================================================================================

/**
 * @struct SPF_Climate_API
 * @brief Complete API for reading and controlling the game's climate and weather systems.
 *
 * @details Provides functions to query and modify all aspects of the game environment:
 *          climate selection, sun profiles, weather modes, bad weather, and all visual
 *          attributes including fog, colors, precipitation, bloom, depth of field, and more.
 *
 *          **ABI Stability**: New functions will only be added to the end of this structure.
 *          Existing fields will never be removed or reordered.
 *
 * @section Lifecycle Before using any other functions, call CL_IsReady() to ensure the
 *          climate service is fully initialized and all memory offsets have been resolved.
 */
typedef struct SPF_Climate_API {
  // ===============================================================================================
  // SECTION 1: LIFECYCLE
  // ===============================================================================================
  // These functions check whether the climate service is ready for use. Call CL_IsReady()
  // before accessing any other climate API functions.

  /**
   * @brief Checks if the Climate Service is fully initialized and all memory offsets have
   *        been successfully resolved.
   * @return true if the service is ready, false if offsets are still being resolved.
   */
  bool (*CL_IsReady)();

  /**
   * @brief Checks if a specific climate data finder has resolved its memory offsets.
   * @param finderName The name of the finder to check (e.g., "ClimateDataFinder").
   * @return true if the finder's offsets are resolved and valid.
   */
  bool (*CL_IsFinderReady)(const char* finderName);

  /**
   * @brief Checks if ALL climate data finders have successfully resolved their offsets.
   * @return true if all required memory patterns were found.
   */
  bool (*CL_AreAllOffsetsFound)();

  /**
   * @brief Forces a re-scan of game memory to find all climate-related offsets.
   * @details Call this if the climate service failed to initialize on the first attempt
   *          (e.g., the game world was not fully loaded). Repeated calls are safe.
   * @return true if all offsets were successfully resolved after this attempt.
   */
  bool (*CL_RefreshOffsets)();

  // ===============================================================================================
  // SECTION 2: CLIMATE SELECTION
  // ===============================================================================================
  // Functions for querying the current climate, listing available climates, and switching
  // between them. A "climate" defines the overall weather profile (e.g., "desert", "arctic").

  /**
   * @brief Gets the human-readable name of the currently active climate.
   * @param[out] outBuffer Buffer to receive the climate name string.
   * @param bufferSize Size of the output buffer in bytes.
   * @return The actual length of the climate name (excluding null terminator).
   *         Returns 0 if the service is not ready.
   */
  int (*CL_GetCurrentClimateName)(char* outBuffer, int bufferSize);

  /**
   * @brief Gets the total number of available climate definitions loaded by the game.
   * @return The number of climates, or 0 if not ready or no climates are registered.
   */
  int (*CL_GetAvailableClimateCount)();

  /**
   * @brief Gets the name and token of an available climate by its index.
   * @param index The zero-based index of the climate to query (0..Count-1).
   * @param[out] outNameBuffer Buffer to receive the climate name.
   * @param nameBufferSize Size of the name buffer in bytes.
   * @param[out] outToken Receives the unique numeric token for this climate.
   *                      Use this token with CL_SetClimate().
   * @return true if the index is valid and data was written.
   */
  bool (*CL_GetAvailableClimateByIndex)(int index, char* outNameBuffer, int nameBufferSize, uint64_t* outToken);

  /**
   * @brief Switches the game to a different climate.
   * @details The climate is identified by its unique token (obtainable via
   *          CL_GetAvailableClimateByIndex()). The change can be instant or gradual.
   * @param climateToken The unique token of the target climate.
   * @param instant When true, the transition is immediate. When false, the game
   *                performs a smooth visual transition.
   */
  void (*CL_SetClimate)(uint64_t climateToken, bool instant);

  // ===============================================================================================
  // SECTION 3: SUN PROFILE
  // ===============================================================================================
  // Sun profiles define the visual parameters for different sun positions (elevation angles)
  // during the day. The game transitions between adjacent sun profiles as the sun moves.

  /**
   * @brief Gets the index of the currently active (current) sun profile.
   * @return The sun profile index, or -1 if not available.
   */
  int32_t (*CL_GetActiveSunProfileIndex)();

  /**
   * @brief Gets the index of the next (target) sun profile that the game is transitioning to.
   * @return The next sun profile index, or -1 if not available.
   */
  int32_t (*CL_GetNextSunProfileIndex)();

  /**
   * @brief Gets the total number of sun profiles in a weather container.
   * @param isBad When false, queries the "nice weather" container. When true, queries
   *              the "bad weather" container (overcast/rain/snow profiles).
   * @return The number of sun profiles in the specified container.
   */
  int32_t (*CL_GetSunProfileCount)(bool isBad);

  /**
   * @brief Gets the display name of a specific sun profile.
   * @param index The zero-based sun profile index.
   * @param isBad Which weather container to query (false = nice, true = bad).
   * @param[out] outBuffer Buffer to receive the profile name.
   * @param bufferSize Size of the output buffer.
   * @return The actual length of the name string (excluding null terminator).
   *         Returns 0 if the index is out of range or the name is empty.
   */
  int (*CL_GetSunProfileName)(int32_t index, bool isBad, char* outBuffer, int bufferSize);

  /**
   * @brief Gets the high (upper) elevation angle of a sun profile, in radians.
   * @details Each sun profile covers a range of sun elevations. This function returns
   *          the top of that range as a raw game value in radians.
   * @param index The zero-based sun profile index (from the current weather container).
   * @return The elevation angle in radians, or 0.0 if the index is invalid.
   */
  float (*CL_GetSunProfileElevation)(int32_t index);

  /**
   * @brief Gets the current sun-to-profile transition progress as a 0..1 factor.
   * @details Returns how far the game has progressed in transitioning from the active
   *          sun profile to the next one. 0.0 = fully on active, 1.0 = fully on next.
   * @return Transition progress from 0.0 to 1.0.
   */
  float (*CL_GetTransitionProgress)();

  /**
   * @brief Gets the current sun elevation angle in radians.
   * @return The sun angle in radians (0 = horizon, positive = above horizon).
   */
  float (*CL_GetSunAngle)();

  /**
   * @brief Gets the current weather mixing progress (nice↔bad weather transition).
   * @details Returns how far along the blend between nice and bad weather has progressed.
   *          0.0 = fully on the current weather type, 1.0 = fully on the target weather type.
   *          Values > 1.0 indicate no active transition is in progress.
   * @return Progress from 0.0 to 1.0 during a transition, or > 1.0 if idle.
   */
  float (*CL_GetWeatherBlendProgress)();

  /**
   * @brief Sets the duration of the weather transition.
   * @param transitionDurationMinutes The desired transition time in game minutes.
   */
  void (*CL_SetTransitionDuration)(int32_t transitionDurationMinutes);

  // ===============================================================================================
  // SECTION 4: WEATHER MODE
  // ===============================================================================================
  // Weather modes control whether the game uses "nice" (clear/partly cloudy) or "bad"
  // (overcast/rain/snow/storm) weather profiles.

  /**
   * @brief Gets the current weather mode.
   * @return 0 for nice weather, 1 for bad weather.
   */
  int32_t (*CL_GetWeatherMode)();

  /**
   * @brief Gets the next (target) weather mode during a transition.
   * @return 0 for nice weather, 1 for bad weather.
   */
  int32_t (*CL_GetNextWeatherMode)();

  /**
   * @brief Forces a specific weather mode.
   * @details Switches between nice (0) and bad (1) weather. When set to 1, the game
   *          activates its overcast/precipitation profiles. The transition can be
   *          instant or gradual depending on the instant parameter.
   * @param mode 0 for nice weather, 1 for bad weather.
   * @param instant When true, the switch is immediate; when false, a smooth transition occurs.
   */
  void (*CL_SetWeatherMode)(int32_t mode, bool instant);

  // ===============================================================================================
  // SECTION 5: BAD WEATHER FACTOR & TIMER
  // ===============================================================================================
  // The "bad weather factor" controls the severity/intensity when the game is in bad
  // weather mode. 0.0 = minimal, 1.0 = maximum intensity.

  /**
   * @brief Gets the current bad weather intensity factor.
   * @return The factor from 0.0 (minimal bad weather) to 1.0 (maximum intensity).
   *         Returns a default of 0.07 if the service is not ready.
   */
  float (*CL_GetBadWeatherFactor)();

  /**
   * @brief Sets the bad weather intensity factor and forces the weather mode accordingly.
   * @details When set to 0.0, forces nice weather. When set to 1.0, forces full bad weather.
   *          Intermediate values toggle between modes as appropriate.
   * @param factor The desired factor from 0.0 to 1.0.
   */
  void (*CL_SetBadWeatherFactor)(float factor);

  /**
   * @brief Gets whether bad weather mode is active.
   * @return 1 if bad weather is active, 0 if nice weather is active.
   */
  uint32_t (*CL_GetBadWeatherMode)();

  /**
   * @brief Gets the remaining real time (not game time) in seconds until the weather
   *        switches between nice and bad (in either direction).
   * @details Depends on CL_SetBadWeatherFactor and the last weather change time.
   * @return Remaining real time in seconds, or 0.0 if not ready.
   */
  float (*CL_GetRemainingBadWeatherTime)();

  // ===============================================================================================
  // SECTION 6: ENVIRONMENT PROFILE
  // ===============================================================================================
  // These settings are part of the global environment profile (not per-sun-profile).
  // They control global lighting behavior, time of year, and storm probability.

  /**
   * @brief Gets the elevation angle at which street/vehicle lamps turn on, in degrees.
   * @return The lamp-on elevation in degrees.
   */
  float (*CL_GetLampsOnElevation)();

  /**
   * @brief Sets the elevation angle for automatic lamp activation.
   * @param elevationDegrees The desired angle in degrees.
   */
  void (*CL_SetLampsOnElevation)(float elevationDegrees);

  /**
   * @brief Gets the current day-of-year for the environment profile.
   * @details Affects the sun's position, seasonal lighting, and vegetation appearance.
   * @return The day of year as a float (0.0 = Jan 1, 365.0 = Dec 31).
   */
  float (*CL_GetDayInYear)();

  /**
   * @brief Sets the current day-of-year.
   * @param dayValue The desired day (0.0 to ~365.0).
   */
  void (*CL_SetDayInYear)(float dayValue);

  /**
   * @brief Gets the daylight saving time offset.
   * @return The summer time offset in hours (typically 0.0 or 1.0).
   */
  float (*CL_GetSummerTime)();

  /**
   * @brief Sets the daylight saving time offset.
   * @param offsetHours The desired offset (0.0 for no DST, 1.0 for DST).
   */
  void (*CL_SetSummerTime)(float offsetHours);

  /**
   * @brief Gets the probability of thunderstorms during bad weather.
   * @return Probability from 0.0 (never) to 1.0 (always).
   */
  float (*CL_GetThunderstormProbability)();

  /**
   * @brief Sets the probability of thunderstorms during bad weather.
   * @param probability Value from 0.0 to 1.0. Values outside this range are clamped.
   */
  void (*CL_SetThunderstormProbability)(float probability);

  // ===============================================================================================
  // SECTION 7: PROFILE HELPERS, ELEVATION & SUN DIRECTION
  // ===============================================================================================
  // ProfileRef helpers return the currently active or next ProfileRef for use with
  // attribute accessors. Elevation and sun direction are per-profile settings
  // (in degrees, converted from radians automatically).

  /**
   * @brief Gets the ProfileRef for the currently active sun profile.
   * @details Use the returned ProfileRef with any Get/Set attribute function to operate
   *          on the current profile. The index and isBad fields are filled automatically.
   * @return An SPF_Climate_ProfileRef describing the active profile.
   *         Returns {0, false} if the service is not ready.
   */
  SPF_Climate_ProfileRef (*CL_ActiveProfile)();

  /**
   * @brief Gets the ProfileRef for the next (target) sun profile during a transition.
   * @return An SPF_Climate_ProfileRef describing the next profile.
   *         Returns {0, false} if the service is not ready.
   */
  SPF_Climate_ProfileRef (*CL_NextProfile)();

  /**
   * @brief Gets the low (lower bound) elevation angle for a sun profile, in degrees.
   * @param profile The target sun profile.
   * @return The elevation in degrees.
   */
  float (*CL_GetLowElevation)(SPF_Climate_ProfileRef profile);

  /**
   * @brief Sets the low elevation angle for a sun profile.
   * @param profile The target sun profile.
   * @param elevationDegrees The desired elevation in degrees.
   */
  void (*CL_SetLowElevation)(SPF_Climate_ProfileRef profile, float elevationDegrees);

  /**
   * @brief Gets the high (upper bound) elevation angle for a sun profile, in degrees.
   * @param profile The target sun profile.
   * @return The elevation in degrees.
   */
  float (*CL_GetHighElevation)(SPF_Climate_ProfileRef profile);

  /**
   * @brief Sets the high elevation angle for a sun profile.
   * @param profile The target sun profile.
   * @param elevationDegrees The desired elevation in degrees.
   */
  void (*CL_SetHighElevation)(SPF_Climate_ProfileRef profile, float elevationDegrees);

  /**
   * @brief Gets the sun direction for a sun profile.
   * @details Controls how the sun moves: -1 = rising, 0 = at peak, 1 = setting.
   * @param profile The target sun profile.
   * @return -1, 0, or 1 representing the sun's movement direction.
   */
  int32_t (*CL_GetSunDirection)(SPF_Climate_ProfileRef profile);

  /**
   * @brief Sets the sun direction for a sun profile.
   * @param profile The target sun profile.
   * @param direction Must be -1, 0, or 1. Values outside this range are ignored.
   */
  void (*CL_SetSunDirection)(SPF_Climate_ProfileRef profile, int32_t direction);

  // ===============================================================================================
  // SECTION 8: VARIATION INDEX
  // ===============================================================================================
  // Each sun profile attribute can have multiple variations. The active variation index
  // determines which variation is currently used for reading/writing via the non-ByIndex
  // accessors. There are separate indices for the active and next profiles.

  /**
   * @brief Gets the active variation index for the current sun profile.
   * @return The zero-based variation index.
   */
  uint64_t (*CL_GetActiveVariationIndex)();

  /**
   * @brief Sets the active variation index for the current sun profile.
   * @param variationIndex The desired zero-based variation index.
   */
  void (*CL_SetActiveVariationIndex)(uint64_t variationIndex);

  /**
   * @brief Gets the variation index for the next (target) sun profile.
   * @return The zero-based variation index.
   */
  uint64_t (*CL_GetNextVariationIndex)();

  /**
   * @brief Sets the variation index for the next (target) sun profile.
   * @param variationIndex The desired zero-based variation index.
   */
  void (*CL_SetNextVariationIndex)(uint64_t variationIndex);

  // ===============================================================================================
  // SECTION 9: FLOAT PROFILE ATTRIBUTES
  // ===============================================================================================
  // Each attribute provides 7 accessors:
  //   GetXxxCount(ProfileRef)       — number of variations
  //   GetXxx(ProfileRef)            — read active variation
  //   SetXxx(ProfileRef, float)     — write active variation
  //   GetXxxByIndex(ProfileRef, uint64_t) — read specific variation
  //   SetXxxByIndex(ProfileRef, uint64_t, float) — write specific variation
  //   GetBlendedXxx()               — read interpolated value (active ↔ next)
  //   SetBlendedXxx(float, float, float) — write blended value (distributed across both profiles)

  // ---------------------------------------------------------------------------
  // Temperature
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(Temperature);

  // ---------------------------------------------------------------------------
  // Sun visual attributes
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(SunOpacity);
  SPF_CL_FLOAT_PROFILE_API(SunShadowStrength);

  // ---------------------------------------------------------------------------
  // Moon visual attributes
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(MoonHaloScale);

  // ---------------------------------------------------------------------------
  // Fog attributes
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(FogVgradient);
  SPF_CL_FLOAT_PROFILE_API(FogOffset);
  SPF_CL_FLOAT_PROFILE_API(FogDensity);

  // ---------------------------------------------------------------------------
  // Cloud shadow attributes
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(SpeedCoef);
  SPF_CL_FLOAT_PROFILE_API(CloudShadowWeight);

  // ---------------------------------------------------------------------------
  // Rain attributes
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(RainIntensity);
  SPF_CL_FLOAT_PROFILE_API(LightningIntensity);
  SPF_CL_FLOAT_PROFILE_API(RainMaxWetness);
  SPF_CL_FLOAT_PROFILE_API(RainAdditionalAmbient);

  // ---------------------------------------------------------------------------
  // Snow attributes
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(SnowIntensity);
  SPF_CL_FLOAT_PROFILE_API(SnowChaosRate);
  SPF_CL_FLOAT_PROFILE_API(SnowChaosWeight);
  SPF_CL_FLOAT_PROFILE_API(SnowAdditionalAmbient);

  // ---------------------------------------------------------------------------
  // Depth of Field
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(DofStart);
  SPF_CL_FLOAT_PROFILE_API(DofTransition);
  SPF_CL_FLOAT_PROFILE_API(DofFilterSize);

  // ---------------------------------------------------------------------------
  // Color grading
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(ColorBalance);
  SPF_CL_FLOAT_PROFILE_API(ColorSaturation);

  // ---------------------------------------------------------------------------
  // Sun shafts (god rays)
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(SunshaftSize);

  // ---------------------------------------------------------------------------
  // Eye adaptation (auto-exposure)
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(LowIntensityMinimum);
  SPF_CL_FLOAT_PROFILE_API(LowIntensityMaximum);
  SPF_CL_FLOAT_PROFILE_API(DarkAdaptationSpeed);
  SPF_CL_FLOAT_PROFILE_API(BrightAdaptationSpeed);
  SPF_CL_FLOAT_PROFILE_API(TargetGray);

  // ---------------------------------------------------------------------------
  // Tonemapping
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(MinScale);
  SPF_CL_FLOAT_PROFILE_API(MaxScale);
  SPF_CL_FLOAT_PROFILE_API(ScaleOverride);
  SPF_CL_FLOAT_PROFILE_API(Contrast);
  SPF_CL_FLOAT_PROFILE_API(ShoulderLength);

  // ---------------------------------------------------------------------------
  // Bloom
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(BloomThreshold);
  SPF_CL_FLOAT_PROFILE_API(BloomLimit);
  SPF_CL_FLOAT_PROFILE_API(BloomIntensity);
  SPF_CL_FLOAT_PROFILE_API(BloomStandardDeviation);
  SPF_CL_FLOAT_PROFILE_API(Stability);

  // ---------------------------------------------------------------------------
  // Misc profile attributes
  // ---------------------------------------------------------------------------
  SPF_CL_FLOAT_PROFILE_API(MirrorSkyTexture);
  SPF_CL_FLOAT_PROFILE_API(Env);
  SPF_CL_FLOAT_PROFILE_API(EnvStaticMod);

  // ===============================================================================================
  // SECTION 10: INT32 PROFILE ATTRIBUTES
  // ===============================================================================================
  // Each provides 5 accessors (no blended variants):
  //   GetXxxCount, GetXxx, SetXxx, GetXxxByIndex, SetXxxByIndex

  SPF_CL_INT32_PROFILE_API(Weight);
  SPF_CL_INT32_PROFILE_API(WindType);

  // ===============================================================================================
  // SECTION 11: VECTOR3 PROFILE ATTRIBUTES
  // ===============================================================================================
  // Each provides 7 accessors:
  //   GetXxxCount, GetXxx, SetXxx, GetXxxByIndex, SetXxxByIndex,
  //   GetBlendedXxx, SetBlendedXxx
  // All Vector3 values are passed/returned via SPF_Climate_Vector3.

  // ---------------------------------------------------------------------------
  // Lighting colors
  // ---------------------------------------------------------------------------
  SPF_CL_VECTOR3_PROFILE_API(Ambient);
  SPF_CL_VECTOR3_PROFILE_API(Diffuse);
  SPF_CL_VECTOR3_PROFILE_API(Specular);

  // ---------------------------------------------------------------------------
  // Sky colors
  // ---------------------------------------------------------------------------
  SPF_CL_VECTOR3_PROFILE_API(SkyColor);
  SPF_CL_VECTOR3_PROFILE_API(SkyBottomColor);
  SPF_CL_VECTOR3_PROFILE_API(StarmapColor);
  SPF_CL_VECTOR3_PROFILE_API(StarsColor);

  // ---------------------------------------------------------------------------
  // Sun colors
  // ---------------------------------------------------------------------------
  SPF_CL_VECTOR3_PROFILE_API(SunColor);
  SPF_CL_VECTOR3_PROFILE_API(SunHaloColor);

  // ---------------------------------------------------------------------------
  // Moon colors
  // ---------------------------------------------------------------------------
  SPF_CL_VECTOR3_PROFILE_API(MoonColor);
  SPF_CL_VECTOR3_PROFILE_API(MoonHaloColor);

  // ---------------------------------------------------------------------------
  // Fog colors
  // ---------------------------------------------------------------------------
  SPF_CL_VECTOR3_PROFILE_API(FogColor);
  SPF_CL_VECTOR3_PROFILE_API(FogColor2);

  // ---------------------------------------------------------------------------
  // Sun shaft color
  // ---------------------------------------------------------------------------
  SPF_CL_VECTOR3_PROFILE_API(SunshaftColor);

  // ---------------------------------------------------------------------------
  // Eye adaptation low intensity color
  // ---------------------------------------------------------------------------
  SPF_CL_VECTOR3_PROFILE_API(LowIntensityColor);

  // ===============================================================================================
  // SECTION 12: VECTOR2 PROFILE ATTRIBUTES
  // ===============================================================================================
  // Each provides 7 accessors: Count, Get, Set, ByIndex (get/set), GetBlended, SetBlended.

  SPF_CL_VECTOR2_PROFILE_API(CloudShadowAreaSize);
  SPF_CL_VECTOR2_PROFILE_API(CloudShadowSpeed);
  SPF_CL_VECTOR2_PROFILE_API(SnowFlakeSizeRange);

  // ===============================================================================================
  // SECTION 13: TEXTURE PROFILE ATTRIBUTES
  // ===============================================================================================
  // Each provides 5 accessors (no blended variants):
  //   GetXxxCount, GetXxx(buf, size), SetXxx, GetXxxByIndex(buf, size), SetXxxByIndex

  SPF_CL_TEXTURE_PROFILE_API(SkyboxTexture);
  SPF_CL_TEXTURE_PROFILE_API(SkycloudMaskTexture);
  SPF_CL_TEXTURE_PROFILE_API(LightningMask);
  SPF_CL_TEXTURE_PROFILE_API(StarsTexture);
  SPF_CL_TEXTURE_PROFILE_API(CloudShadowTexture);

} SPF_Climate_API;

#ifdef __cplusplus
}
#endif
