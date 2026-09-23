/**
 * @file ExamplePlugin.hpp
 * @brief Slim orchestrator header: PluginContext + lifecycle + UI entry points.
 *
 * @details Per-API examples (render + callbacks + update + docs) live in Example*API.*.
 * This header only owns:
 *   - PluginContext / g_ctx  (shared state bag)
 *   - Lifecycle exports      (OnLoad, OnActivated, OnUpdate, OnUnload, OnGameWorldReady)
 *   - UI entry               (OnRegisterUI, RenderMainWindow)
 *   - BuildManifest
 *
 * Framework callbacks that belong to a module (OnSettingChanged, OnLanguageChanged,
 * OnGameLogMessage, OnToggleMainWindow, OnCameraKeybind, telemetry On*Update) are
 * declared in their Example*API headers — include those where you need the pointer.
 */
#pragma once

// =================================================================================================
// SPF API Includes (shared by all Example*API translation units via this header)
// =================================================================================================
#include <SPF/SPF_API/SPF_Camera_API.h>
#include <SPF/SPF_API/SPF_Climate_API.h>
#include <SPF/SPF_API/SPF_Config_API.h>
#include <SPF/SPF_API/SPF_Environment_API.h>
#include <SPF/SPF_API/SPF_Formatting_API.h>
#include <SPF/SPF_API/SPF_GameConsole_API.h>
#include <SPF/SPF_API/SPF_GameLog_API.h>
#include <SPF/SPF_API/SPF_GameWorld_API.h>
#include <SPF/SPF_API/SPF_Hooks_API.h>
#include <SPF/SPF_API/SPF_Icons.h>
#include <SPF/SPF_API/SPF_JsonIO_API.h>
#include <SPF/SPF_API/SPF_JsonReader_API.h>
#include <SPF/SPF_API/SPF_JsonWriter_API.h>
#include <SPF/SPF_API/SPF_KeyBinds_API.h>
#include <SPF/SPF_API/SPF_Localization_API.h>
#include <SPF/SPF_API/SPF_Logger_API.h>
#include <SPF/SPF_API/SPF_Manifest_API.h>
#include <SPF/SPF_API/SPF_Plugin.h>
#include <SPF/SPF_API/SPF_Telemetry_API.h>
#include <SPF/SPF_API/SPF_TelemetryData.h>
#include <SPF/SPF_API/SPF_UI_API.h>
#include <SPF/SPF_API/SPF_Vehicle_API.h>
#include <SPF/SPF_API/SPF_VirtInput_API.h>
#include <SPF/SPF_API/SPF_Sound_API.h>

#include <cstdint>
#include <vector>

namespace ExamplePlugin {

// =================================================================================================
// Types
// =================================================================================================

/**
 * @brief Signature of the game's internal string formatting function (Hooks detour + trampoline).
 * @details Hook detours must match the original exactly; this alias is shared by
 * ExampleHooksAPI (detour body + trampoline storage on g_ctx).
 */
using GameStringFormatting_t = void* (*)(void* pOutput, const char** ppInput);

// =================================================================================================
// PluginContext — single global state bag (Context Object pattern)
// =================================================================================================

/**
 * @brief All plugin-wide state: API pointers, handles, UI flags, module-owned fields.
 * @details C-style framework callbacks cannot be member functions; consolidating state here
 * keeps one global (g_ctx) instead of scattered variables. Each Example*API documents which
 * fields it reads/writes. Module-local presentational state may live as file-scope statics
 * in the owning .cpp instead.
 */
struct PluginContext {
  // --- Primary API pointers (lifecycle-provided) ---
  const SPF_Load_API* loadAPI = nullptr;    ///< OnLoad
  const SPF_Core_API* coreAPI = nullptr;    ///< OnActivated

  // --- Cached service pointers (filled from core/load in lifecycle) ---
  SPF_Vehicle_API* vehicleAPI = nullptr;
  SPF_GameWorld_API* gameworldAPI = nullptr;
  const SPF_Climate_API* climateAPI = nullptr;
  SPF_Sound_API* soundAPI = nullptr;
  SPF_JsonWriter_API* jsonWriterAPI = nullptr;
  SPF_JsonIO_API* jsonIOAPI = nullptr;
  SPF_Environment_API* environmentAPI = nullptr;
  SPF_Environment_Handle* environmentHandle = nullptr;
  SPF_Config_Handle* customConfigHandle = nullptr;
  bool isCustomConfigAutoSave = true;

  // --- UI (OnRegisterUI) ---
  SPF_UI_API* uiAPI = nullptr;
  SPF_Window_Handle* mainWindowHandle = nullptr;

  // --- Virtual input (ExampleVirtInputAPI) ---
  SPF_VirtualDevice_Handle* virtualDevice = nullptr;

  // --- Telemetry (ExampleTelemetryAPI): callbacks write here; tabs only read ---
  struct EventDataCache {
    SPF_GameState gameState;
    SPF_Timestamps timestamps;
    SPF_CommonData commonData;
    SPF_TruckConstants truckConstants;
    SPF_TrailerConstants trailerConstants;
    SPF_TruckData truckData;
    std::vector<SPF_Trailer> trailers;
    SPF_JobConstants jobConstants;
    SPF_JobData jobData;
    SPF_NavigationData navigationData;
    SPF_Controls controls;
    SPF_SpecialEvents specialEvents;
    SPF_GameplayEvents gameplayEvents;
    SPF_GearboxConstants gearboxConstants;
    char lastGameplayEventId[256] = "N/A";
  } eventDataCache;

  SPF_Telemetry_Handle* telemetryHandle = nullptr;
  SPF_Telemetry_Callback_Handle* gameStateCallback = nullptr;
  SPF_Telemetry_Callback_Handle* timestampsCallback = nullptr;
  SPF_Telemetry_Callback_Handle* commonDataCallback = nullptr;
  SPF_Telemetry_Callback_Handle* truckConstantsCallback = nullptr;
  SPF_Telemetry_Callback_Handle* trailerConstantsCallback = nullptr;
  SPF_Telemetry_Callback_Handle* truckDataCallback = nullptr;
  SPF_Telemetry_Callback_Handle* trailersCallback = nullptr;
  SPF_Telemetry_Callback_Handle* jobConstantsCallback = nullptr;
  SPF_Telemetry_Callback_Handle* jobDataCallback = nullptr;
  SPF_Telemetry_Callback_Handle* navigationDataCallback = nullptr;
  SPF_Telemetry_Callback_Handle* controlsCallback = nullptr;
  SPF_Telemetry_Callback_Handle* specialEventsCallback = nullptr;
  SPF_Telemetry_Callback_Handle* gameplayEventsCallback = nullptr;
  SPF_Telemetry_Callback_Handle* gearboxConstantsCallback = nullptr;

  // --- Plugin / module state ---
  int32_t someNumber = 0;                         ///< ExampleConfigAPI
  char consoleCommand[256] = "g_traffic 1";       ///< ExampleConsoleAPI
  bool isHonkIntercepted = false;                 ///< ExampleKeybindsAPI (block Demo.honk)
  bool isModificationActive = false;              ///< ExampleHooksAPI (detour gate)
  bool weatherAutoToggle = false;                 ///< ExampleClimateAPI

  GameStringFormatting_t o_GameStringFormatting = nullptr;  ///< ExampleHooksAPI trampoline
  SPF_GameLog_Callback_Handle* gameLogCallbackHandle = nullptr;  ///< ExampleGameLogAPI
  SPF_KeyBinds_Handle* keybindsHandle = nullptr;             ///< ExampleKeybindsAPI / Camera

  SPF_VehicleHandle selectedVehicle = nullptr;     ///< ExampleVehicleAPI
  std::vector<SPF_VehicleHandle> vehicleHandles;   ///< ExampleVehicleAPI

  // --- Styling assets (ExampleStylingAPI) ---
  void* pluginTexture = nullptr;
  int textureWidth = 0;
  int textureHeight = 0;
  void* pluginFileTexture = nullptr;
  int fileTextureWidth = 0;
  int fileTextureHeight = 0;
  SPF_Font_Handle pluginFont = nullptr;
  SPF_Font_Handle memoryFont = nullptr;

  // --- Sound horn→bell replacement (ExampleSoundAPI) ---
  bool replaceHornEnabled = false;
  void* bellBank = nullptr;
  int bellEventIndex = -1;
  void* bellInstance = nullptr;
  int hornEventIndices[32] = {};
  int hornEventCount = 0;
  bool bellTestPlaying = false;
  bool bellReplacementActive = false;
};

/**
 * @brief Global plugin context instance (defined in ExamplePlugin.cpp).
 */
extern PluginContext g_ctx;

// =================================================================================================
// Lifecycle + UI + Manifest (implemented in ExamplePlugin.cpp)
// =================================================================================================

/**
 * @brief Builds plugin metadata (identity, policy, default settings, keybind defaults).
 * @param h Opaque manifest builder handle.
 * @param api Framework-provided builder function table (ABI-stable).
 */
void BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api);

/** @brief Earliest entry: cache loadAPI, read settings, create early handles (VirtInput, Environment). */
void OnLoad(const SPF_Load_API* load_api);

/** @brief Full activation: cache coreAPI, call each module's *_OnActivated (register callbacks/hooks/keybinds). */
void OnActivated(const SPF_Core_API* core_api);

/** @brief One-shot after the game world is loaded (safe point for world-dependent setup). */
void OnGameWorldReady();

/** @brief Per-frame tick: delegates to module *_OnUpdate (Climate, Sound, Telemetry). Keep cheap. */
void OnUpdate();

/** @brief Last chance cleanup: module *_OnUnload/Shutdown, then null cached pointers. */
void OnUnload();

/**
 * @brief Registers the MainWindow draw callback and caches the window handle.
 * @param ui_api UI API provided when the UI subsystem is ready.
 */
void OnRegisterUI(SPF_UI_API* ui_api);

/**
 * @brief Main window draw: tab bar only — each tab delegates to its Example*API render function.
 * @param ui UI API for widget calls.
 * @param user_data Passed through from registration (unused; state lives on g_ctx).
 */
void RenderMainWindow(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
