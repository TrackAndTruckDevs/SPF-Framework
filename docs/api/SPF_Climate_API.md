# SPF Climate API

The SPF Climate API provides plugins with full read/write access to the game's climate and weather systems. It covers climate selection, sun profiles, weather modes, bad weather factors, environment profile settings, and all visual attributes (colors, fog, rain, snow, bloom, depth of field, etc.).

## Core Concepts

### ProfileRef (Profile Reference)

Many climate attributes are organized per "sun profile." A `ProfileRef` identifies which sun profile to read/write, specified by:

- **index**: The zero-based position within the weather container
- **isBad**: `false` for nice weather container, `true` for bad weather container

Use `CL_ActiveProfile()` or `CL_NextProfile()` to get a valid `ProfileRef` for the current or next profile.

### Variations

Each sun profile attribute can have multiple variations. Functions with the `ByIndex` suffix access a specific variation. The base `Get`/`Set` functions (without `ByIndex`) operate on the **active** variation determined by the game.

### Blended Values

The game smoothly interpolates between the active and next sun profiles. `GetBlended*` functions return the current interpolated value. `SetBlended*` functions let you set a target blended value, and the API distributes the change across both profiles.

### String Handling

Functions returning strings use the buffer/size pattern:
- You provide a `char*` buffer and its size
- The function returns the actual string length (without null terminator)
- If the return value >= bufferSize, the string was truncated

## Getting the API

The Climate API is provided as part of the main `SPF_Core_API` struct that your plugin receives in its `OnActivated` lifecycle event.

```c
#include "SPF/SPF_API/SPF_Plugin.h"

const SPF_Core_API* s_coreAPI = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
}
```

Functions are accessed via the `climate` member:

```c
s_coreAPI->climate->CL_IsReady();
s_coreAPI->climate->CL_GetCurrentClimateName(buffer, sizeof(buffer));
```

## Data Types

### `SPF_Climate_ProfileRef`

Identifies a specific sun profile within the nice or bad weather container.

| Field | Type | Description |
| :--- | :--- | :--- |
| `index` | `uint64_t` | Zero-based sun profile index |
| `isBad` | `bool` | `false` = nice weather, `true` = bad weather |

### `SPF_Climate_Vector3`

A 3-component floating-point vector used for colors and lighting values.

| Field | Type | Description |
| :--- | :--- | :--- |
| `x` | `float` | First component (red) |
| `y` | `float` | Second component (green) |
| `z` | `float` | Third component (blue) |

### `SPF_Climate_Vector2`

A 2-component floating-point vector.

| Field | Type | Description |
| :--- | :--- | :--- |
| `x` | `float` | First component |
| `y` | `float` | Second component |

## Function Reference

### Section 1: Lifecycle

Before using any other functions, call `CL_IsReady()` to ensure the service is initialized.

---

### `bool CL_IsReady()`
Checks if the Climate Service is fully initialized and all memory offsets have been resolved.

---

### `bool CL_IsFinderReady(const char* finderName)`
Checks if a specific climate data finder has resolved its offsets.

- **Parameters:**
  - `finderName`: The finder name (e.g., `"ClimateDataFinder"`).

---

### `bool CL_AreAllOffsetsFound()`
Checks if ALL climate data finders have resolved their offsets.

---

### `bool CL_RefreshOffsets()`
Forces a re-scan of game memory to find all climate offsets. Call this if the service failed to initialize on the first attempt (e.g., game world wasn't fully loaded yet).

---

### Section 2: Climate Selection

### `int CL_GetCurrentClimateName(char* outBuffer, int bufferSize)`
Gets the human-readable name of the currently active climate.

- **Parameters:**
  - `outBuffer`: Buffer to receive the name string.
  - `bufferSize`: Size of the output buffer.
- **Returns:** The actual name length (excluding null terminator), or 0 if not ready.

---

### `int CL_GetAvailableClimateCount()`
Gets the number of available climate definitions.

---

### `bool CL_GetAvailableClimateByIndex(int index, char* outNameBuffer, int nameBufferSize, uint64_t* outToken)`
Gets the name and token of an available climate by index.

- **Parameters:**
  - `index`: Zero-based climate index (0..Count-1).
  - `outNameBuffer`: Buffer to receive the climate name.
  - `nameBufferSize`: Size of the name buffer.
  - `outToken`: Receives the unique numeric token for this climate.
- **Returns:** `true` if the index was valid.

---

### `void CL_SetClimate(uint64_t climateToken, bool instant)`
Switches to a different climate.

- **Parameters:**
  - `climateToken`: The unique token (from `CL_GetAvailableClimateByIndex`).
  - `instant`: `true` = immediate change, `false` = smooth transition.

---

### Section 3: Sun Profile

### `int32_t CL_GetActiveSunProfileIndex()`
Gets the index of the currently active sun profile. Returns -1 if not available.

---

### `int32_t CL_GetNextSunProfileIndex()`
Gets the index of the next (target) sun profile during a transition. Returns -1 if not available.

---

### `int32_t CL_GetSunProfileCount(bool isBad)`
Gets the number of sun profiles in a weather container.

- **Parameters:**
  - `isBad`: `false` = nice weather container, `true` = bad weather container.

---

### `int CL_GetSunProfileName(int32_t index, bool isBad, char* outBuffer, int bufferSize)`
Gets the display name of a specific sun profile.

---

### `float CL_GetSunProfileElevation(int32_t index)`
Gets the high elevation angle of a sun profile, in radians.

---

### `float CL_GetTransitionProgress()`
Gets the active→next sun profile transition progress (0.0 to 1.0).

---

### `float CL_GetSunAngle()`
Gets the current sun elevation angle in radians.

---

### `float CL_GetWeatherBlendProgress()`
Gets the current nice↔bad weather mixing progress. 0.0 = fully on current weather type, 1.0 = fully on target. Values > 1.0 mean no active transition.

---

### `void CL_SetTransitionDuration(int32_t minutes)`
Sets the weather transition duration in game minutes.

---

### Section 4: Weather Mode

### `int32_t CL_GetWeatherMode()`
Gets the current weather mode: 0 = nice, 1 = bad.

---

### `int32_t CL_GetNextWeatherMode()`
Gets the next (target) weather mode: 0 = nice, 1 = bad.

---

### `void CL_SetWeatherMode(int32_t mode, bool instant)`
Forces a weather mode switch.

- **Parameters:**
  - `mode`: 0 = nice weather, 1 = bad weather.
  - `instant`: `true` = immediate, `false` = smooth transition.

---

### Section 5: Bad Weather Factor & Timer

### `float CL_GetBadWeatherFactor()`
Gets the bad weather intensity factor (0.0 to 1.0). Returns ~0.07 default if not ready.

---

### `void CL_SetBadWeatherFactor(float factor)`
Sets the bad weather intensity and forces the weather mode accordingly. 0.0 = nice, 1.0 = full bad weather.

---

### `uint32_t CL_GetBadWeatherMode()`
Returns 1 if bad weather is active, 0 if nice weather is active.

---

### `float CL_GetRemainingBadWeatherTime()`
Gets the remaining real time (not game time) in seconds until the weather switches between nice and bad (either direction). Depends on `CL_SetBadWeatherFactor` and the last weather change time.

---

### Section 6: Environment Profile

### `float CL_GetLampsOnElevation()`
Gets the elevation angle at which lamps turn on, in degrees.

### `void CL_SetLampsOnElevation(float elevationDegrees)`
Sets the lamp-on elevation angle.

---

### `float CL_GetDayInYear()`
Gets the current day of year (0.0 = Jan 1, ~365.0 = Dec 31).

### `void CL_SetDayInYear(float dayValue)`
Sets the day of year.

---

### `float CL_GetSummerTime()`
Gets the daylight saving time offset (0.0 or 1.0).

### `void CL_SetSummerTime(float offsetHours)`
Sets the daylight saving time offset.

---

### `float CL_GetThunderstormProbability()`
Gets thunderstorm probability (0.0 to 1.0).

### `void CL_SetThunderstormProbability(float probability)`
Sets thunderstorm probability. Values outside 0..1 are clamped.

---

### Section 7: Profile Helpers, Elevation & Direction

### `SPF_Climate_ProfileRef CL_ActiveProfile()`
Gets the ProfileRef for the currently active sun profile.

### `SPF_Climate_ProfileRef CL_NextProfile()`
Gets the ProfileRef for the next (target) sun profile.

---

### `float CL_GetLowElevation(SPF_Climate_ProfileRef profile)`
Gets the low elevation angle for a sun profile, in degrees.

### `void CL_SetLowElevation(SPF_Climate_ProfileRef profile, float elevationDegrees)`
Sets the low elevation angle.

---

### `float CL_GetHighElevation(SPF_Climate_ProfileRef profile)`
Gets the high elevation angle for a sun profile, in degrees.

### `void CL_SetHighElevation(SPF_Climate_ProfileRef profile, float elevationDegrees)`
Sets the high elevation angle.

---

### `int32_t CL_GetSunDirection(SPF_Climate_ProfileRef profile)`
Gets the sun movement direction: -1 = rising, 0 = at peak, 1 = setting.

### `void CL_SetSunDirection(SPF_Climate_ProfileRef profile, int32_t direction)`
Sets the sun direction. Must be -1, 0, or 1.

---

### Section 8: Variation Index

### `uint64_t CL_GetActiveVariationIndex()`
Gets the active variation index for the current sun profile.

### `void CL_SetActiveVariationIndex(uint64_t variationIndex)`
Sets the active variation index.

### `uint64_t CL_GetNextVariationIndex()`
Gets the variation index for the next (target) sun profile.

### `void CL_SetNextVariationIndex(uint64_t variationIndex)`
Sets the variation index for the next profile.

---

### Sections 9–13: Profile Attributes

Each attribute provides a consistent set of accessor functions. The table below shows the pattern. Replace `Xxx` with the attribute name from the attribute list.

#### Float Attributes (Section 9)

Each float attribute provides 7 functions:

| Function | Signature | Description |
| :--- | :--- | :--- |
| `GetXxxCount` | `uint64_t(SPF_Climate_ProfileRef)` | Number of variations |
| `GetXxx` | `float(SPF_Climate_ProfileRef)` | Read active variation |
| `SetXxx` | `void(SPF_Climate_ProfileRef, float)` | Write active variation |
| `GetXxxByIndex` | `float(SPF_Climate_ProfileRef, uint64_t)` | Read specific variation |
| `SetXxxByIndex` | `void(SPF_Climate_ProfileRef, uint64_t, float)` | Write specific variation |
| `GetBlendedXxx` | `float()` | Interpolated value (active ↔ next) |
| `SetBlendedXxx` | `void(float, float, float)` | Set blended (value, min, max) |

**Float attribute list:**

| Attribute | Description |
| :--- | :--- |
| `Temperature` | Ambient temperature |
| `SunOpacity` | Sun disk opacity |
| `SunShadowStrength` | Shadow strength from sun |
| `MoonHaloScale` | Moon halo size |
| `FogVgradient` | Fog vertical gradient |
| `FogOffset` | Fog height offset |
| `FogDensity` | Fog density |
| `SpeedCoef` | Cloud speed coefficient |
| `CloudShadowWeight` | Cloud shadow opacity weight |
| `RainIntensity` | Rain intensity |
| `LightningIntensity` | Lightning flash intensity |
| `RainMaxWetness` | Maximum surface wetness from rain |
| `RainAdditionalAmbient` | Extra ambient light during rain |
| `SnowIntensity` | Snowfall intensity |
| `SnowChaosRate` | Snow chaos animation rate |
| `SnowChaosWeight` | Snow chaos animation weight |
| `SnowAdditionalAmbient` | Extra ambient light during snow |
| `DofStart` | Depth of field start distance |
| `DofTransition` | Depth of field transition range |
| `DofFilterSize` | Depth of field blur filter size |
| `ColorBalance` | Color balance adjustment |
| `ColorSaturation` | Color saturation level |
| `SunshaftSize` | Sun shaft (god ray) size |
| `LowIntensityMinimum` | Eye adaptation low intensity minimum |
| `LowIntensityMaximum` | Eye adaptation low intensity maximum |
| `DarkAdaptationSpeed` | Eye adaptation speed (darkening) |
| `BrightAdaptationSpeed` | Eye adaptation speed (brightening) |
| `TargetGray` | Auto-exposure target gray level |
| `MinScale` | Tonemapping minimum scale |
| `MaxScale` | Tonemapping maximum scale |
| `ScaleOverride` | Tonemapping scale override |
| `Contrast` | Contrast adjustment |
| `ShoulderLength` | Tonemapping shoulder length |
| `BloomThreshold` | Bloom brightness threshold |
| `BloomLimit` | Bloom brightness limit |
| `BloomIntensity` | Bloom intensity |
| `BloomStandardDeviation` | Bloom blur standard deviation |
| `Stability` | Temporal stability factor |
| `MirrorSkyTexture` | Mirror sky texture blend |
| `Env` | Environment map blend |
| `EnvStaticMod` | Static environment map modifier |

#### Int32 Attributes (Section 10)

Each provides 5 functions (no blended variants):

| Function | Signature | Description |
| :--- | :--- | :--- |
| `GetXxxCount` | `uint64_t(SPF_Climate_ProfileRef)` | Number of variations |
| `GetXxx` | `int32_t(SPF_Climate_ProfileRef)` | Read active variation |
| `SetXxx` | `void(SPF_Climate_ProfileRef, int32_t)` | Write active variation |
| `GetXxxByIndex` | `int32_t(SPF_Climate_ProfileRef, uint64_t)` | Read specific variation |
| `SetXxxByIndex` | `void(SPF_Climate_ProfileRef, uint64_t, int32_t)` | Write specific variation |

**Int32 attribute list:**

| Attribute | Description |
| :--- | :--- |
| `Weight` | Profile blending weight |
| `WindType` | Wind type identifier |

#### Vector3 Attributes (Section 11)

Each provides 7 functions. The Vector3 struct uses `SPF_Climate_Vector3`.

| Function | Signature | Description |
| :--- | :--- | :--- |
| `GetXxxCount` | `uint64_t(SPF_Climate_ProfileRef)` | Number of variations |
| `GetXxx` | `void(SPF_Climate_ProfileRef, SPF_Climate_Vector3*)` | Read active variation |
| `SetXxx` | `void(SPF_Climate_ProfileRef, SPF_Climate_Vector3)` | Write active variation |
| `GetXxxByIndex` | `void(SPF_Climate_ProfileRef, uint64_t, SPF_Climate_Vector3*)` | Read specific variation |
| `SetXxxByIndex` | `void(SPF_Climate_ProfileRef, uint64_t, SPF_Climate_Vector3)` | Write specific variation |
| `GetBlendedXxx` | `void(SPF_Climate_Vector3*)` | Interpolated value |
| `SetBlendedXxx` | `void(SPF_Climate_Vector3, float)` | Set blended (value, maxComponent) |

**Vector3 attribute list:**

| Attribute | Description |
| :--- | :--- |
| `Ambient` | Ambient light color (RGB) |
| `Diffuse` | Diffuse light color (RGB) |
| `Specular` | Specular light color (RGB) |
| `SkyColor` | Sky dome top color |
| `SkyBottomColor` | Sky dome bottom/horizon color |
| `StarmapColor` | Star field color |
| `StarsColor` | Individual star color |
| `SunColor` | Sun disk color |
| `SunHaloColor` | Sun halo glow color |
| `MoonColor` | Moon disk color |
| `MoonHaloColor` | Moon halo glow color |
| `FogColor` | Fog base color |
| `FogColor2` | Fog secondary color |
| `SunshaftColor` | Sun shaft (god ray) color |
| `LowIntensityColor` | Low-light vision color tint |

#### Vector2 Attributes (Section 12)

Each provides 7 functions. Uses `SPF_Climate_Vector2`.

**Vector2 attribute list:**

| Attribute | Description |
| :--- | :--- |
| `CloudShadowAreaSize` | Cloud shadow projection area size |
| `CloudShadowSpeed` | Cloud shadow movement speed |
| `SnowFlakeSizeRange` | Snowflake size range (min, max) |

#### Texture Attributes (Section 13)

Each provides 5 functions (no blended variants). Texture names are returned as strings via the buffer/size pattern. **Note:** `SetXxx` and `SetXxxByIndex` are stubs (not yet implemented).

| Function | Signature | Description |
| :--- | :--- | :--- |
| `GetXxxCount` | `uint64_t(SPF_Climate_ProfileRef)` | Number of variations |
| `GetXxx` | `int(SPF_Climate_ProfileRef, char*, int)` | Read active variation texture path |
| `SetXxx` | `void(SPF_Climate_ProfileRef, const char*)` | [STUB] |
| `GetXxxByIndex` | `int(SPF_Climate_ProfileRef, uint64_t, char*, int)` | Read specific variation texture path |
| `SetXxxByIndex` | `void(SPF_Climate_ProfileRef, uint64_t, const char*)` | [STUB] |

**Texture attribute list:**

| Attribute | Description |
| :--- | :--- |
| `SkyboxTexture` | Skybox cube map texture path |
| `SkycloudMaskTexture` | Sky-cloud mask texture path |
| `LightningMask` | Lightning flash mask texture path |
| `StarsTexture` | Star field texture path |
| `CloudShadowTexture` | Cloud shadow projection texture path |

## Complete Example

```cpp
#include "SPF/SPF_API/SPF_Plugin.h"

const SPF_Core_API* s_coreAPI = NULL;

void MyWeatherPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;

    if (!s_coreAPI || !s_coreAPI->climate) return;
    if (!s_coreAPI->climate->CL_IsReady()) return;

    // Read current climate name
    char climateName[64];
    s_coreAPI->climate->CL_GetCurrentClimateName(climateName, sizeof(climateName));

    // Get the current sun profile
    SPF_Climate_ProfileRef profile = s_coreAPI->climate->CL_ActiveProfile();

    // Read fog density for the current profile
    float fogDensity = s_coreAPI->climate->CL_GetFogDensity(profile);

    // Read blended fog color (interpolated between active and next)
    SPF_Climate_Vector3 fogColor;
    s_coreAPI->climate->CL_GetBlendedFogColor(&fogColor);

    // Set rain intensity
    s_coreAPI->climate->CL_SetRainIntensity(profile, 0.75f);

    // Force heavy bad weather
    s_coreAPI->climate->CL_SetBadWeatherFactor(1.0f);

    // List all available climates
    int count = s_coreAPI->climate->CL_GetAvailableClimateCount();
    for (int i = 0; i < count; ++i) {
        char name[64];
        uint64_t token;
        if (s_coreAPI->climate->CL_GetAvailableClimateByIndex(i, name, sizeof(name), &token)) {
            // Switch to this climate
            s_coreAPI->climate->CL_SetClimate(token, true);
            break;
        }
    }
}
```
