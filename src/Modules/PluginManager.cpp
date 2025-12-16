#include "SPF/Modules/PluginManager.hpp"
#include "imgui.h"

#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/Vec3.hpp"
#include "SPF/Hooks/HookManager.hpp"        // For registering hooks
#include "SPF/Hooks/PluginHook.hpp"         // For creating proxy objects
#include "SPF/GameConsole/GameConsole.hpp"  // For ExecuteCommand trampoline
#include "SPF/Hooks/CameraHooks.hpp"        // For Camera API trampolines
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/GameCamera/GameCameraManager.hpp"  // For Camera API trampolines
#include "SPF/GameCamera/GameCameraInterior.hpp"
#include "SPF/GameCamera/GameCameraBehind.hpp"
#include "SPF/GameCamera/GameCameraTop.hpp"
#include "SPF/GameCamera/GameCameraCabin.hpp"
#include "SPF/GameCamera/GameCameraWindow.hpp"
#include "SPF/GameCamera/GameCameraBumper.hpp"
#include "SPF/GameCamera/GameCameraWheel.hpp"
#include "SPF/GameCamera/GameCameraTV.hpp"
#include "SPF/GameCamera/GameCameraFree.hpp"
#include "SPF/Modules/API/CameraApi.hpp"
#include "SPF/Modules/API/UIApi.hpp"
#include "SPF/Modules/API/LoggerApi.hpp"
#include "SPF/Modules/API/LocalizationApi.hpp"
#include "SPF/Modules/API/ConfigApi.hpp"
#include "SPF/Modules/API/KeyBindsApi.hpp"
#include "SPF/Modules/API/TelemetryApi.hpp"
#include "SPF/Modules/API/VirtualInputApi.hpp"
#include "SPF/Modules/API/ManifestApi.hpp"
#include "SPF/Modules/API/GameConsoleApi.hpp"
#include "SPF/Modules/API/JsonReaderApi.hpp"
#include "SPF/Modules/API/HooksApi.hpp"
#include "SPF/Modules/API/FormattingApi.hpp"
#include "SPF/Modules/API/GameLogApi.hpp"
#include "SPF/Hooks/IHook.hpp"

#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/PluginProxyWindow.hpp"
#include "SPF/Handles/LocalizationHandle.hpp"
#include "SPF/Handles/ConfigHandle.hpp"
#include "SPF/Handles/LoggerHandle.hpp"
#include "SPF/Handles/KeyBindsHandle.hpp"
#include "SPF/Handles/WindowHandle.hpp"
#include "SPF/Handles/TelemetryHandle.hpp"
#include "SPF/Handles/InputDeviceHandle.hpp"
#include "SPF/Modules/ITelemetryService.hpp"
#include "SPF/Modules/IInputService.hpp"
#include "SPF/Input/SCS/VirtualDevice.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/PluginEvents.hpp"
#include "SPF/Config/IConfigService.hpp"

#include <SPF/Telemetry/SCS/Common.hpp>
#include <SPF/Telemetry/SCS/Truck.hpp>
#include <SPF/Telemetry/SCS/Trailer.hpp>
#include <SPF/Telemetry/SCS/Job.hpp>
#include <SPF/Telemetry/SCS/Navigation.hpp>
#include <SPF/Telemetry/SCS/Controls.hpp>
#include <SPF/Telemetry/SCS/Events.hpp>
#include <SPF/Telemetry/SCS/Gearbox.hpp>

#include <algorithm>
#include <cstring>
#include <cctype>

#include <filesystem>
#include <fstream>
#include <Windows.h>
#include <vector>
#include <map>
#include <fmt/core.h>
#include <nlohmann/json.hpp>

using SPF_GetPlugin_t = bool (*)(SPF_Plugin_Exports*);

SPF_NS_BEGIN
namespace Modules {
using namespace SPF::Localization;
using namespace SPF::Handles;
using namespace SPF::System;
using namespace SPF::Config;
using namespace SPF::UI;
using namespace SPF::Events;
using namespace SPF::GameCamera;
using namespace SPF::Utils;
using namespace SPF::Data::GameData;
using namespace SPF::Modules::API;
using namespace SPF::Hooks;

// --- Static Member Variable Definitions ---
std::vector<std::string> PluginManager::s_available_languages_cache;
std::vector<const char*> PluginManager::s_available_languages_c_str_cache;

// --- Singleton Access ---
PluginManager& PluginManager::GetInstance() {
  static PluginManager instance;
  return instance;
}

// --- Lifecycle ---
PluginManager::PluginManager() = default;
PluginManager::~PluginManager() = default;

std::vector<std::string> PluginManager::GetDiscoveredPluginNames() const {
  std::vector<std::string> names;
  names.reserve(m_discoveredPlugins.size());
  for (const auto& [name, discoveredPlugin] : m_discoveredPlugins) {
    names.push_back(name);
  }
  return names;
}

void PluginManager::LoadAllDiscoveredPluginManifests() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
    if (!logger) return;

    logger->Info("--- Loading all discovered plugin manifests (C-API Method) ---");

    for (const auto& [pluginName, discoveredPlugin] : m_discoveredPlugins) {
        logger->Debug("  -> Attempting to load manifest for plugin '{}' from '{}'", pluginName, discoveredPlugin.dllPath.string());

        HMODULE handle = LoadLibraryW(discoveredPlugin.dllPath.c_str());
        if (!handle) {
            logger->Error("  -> Failed to temporarily load library for manifest extraction. Win32 Error: {}", GetLastError());
            continue;
        }

        auto getManifestApiFunc = reinterpret_cast<SPF_GetManifestAPI_t>(GetProcAddress(handle, "SPF_GetManifestAPI"));
        if (getManifestApiFunc) {
            SPF_Manifest_API manifestApi{};
            if (getManifestApiFunc(&manifestApi)) {
                if (manifestApi.GetManifestData) {
                    auto cManifest = std::make_unique<SPF_ManifestData_C>();
                    manifestApi.GetManifestData(*cManifest.get());

                    // Convert C manifest to C++ manifest
                    SPF::Config::ManifestData cppManifest = Modules::API::ManifestApi::ConvertCManifestToCppManifest(*cManifest, pluginName);

                    // Register the C++ manifest
                    m_configService->RegisterPluginManifest(pluginName, cppManifest);
                    logger->Info("    -> Successfully registered manifest for plugin '{}' from new C-API.", pluginName);

                } else {
                    logger->Error("    -> Manifest API struct for plugin '{}' does not contain a valid GetManifestData function pointer.", pluginName);
                }
            } else {
                logger->Error("    -> SPF_GetManifestAPI function returned false for plugin '{}'. Manifest not registered.", pluginName);
            }
        } else {
            logger->Info("    -> SPF_GetManifestAPI not found for plugin '{}'. This plugin does not provide an in-code manifest via the new C-API.", pluginName);
        }

        FreeLibrary(handle);
        logger->Debug("  -> Unloaded library for plugin '{}'", pluginName);
    }
    logger->Info("--- Finished loading all discovered plugin manifests ---");
}

void PluginManager::Init(EventManager& eventManager, HandleManager& handleManager, IConfigService& configService, KeyBindsManager& keyBindsManager, UIManager& uiManager,
                         ITelemetryService& telemetryService, IInputService& inputService) {
  m_eventManager = &eventManager;
  m_handleManager = &handleManager;
  m_configService = &configService;
  m_keyBindsManager = &keyBindsManager;
  m_uiManager = &uiManager;
  m_telemetryService = &telemetryService;
  m_inputService = &inputService;

  m_onGameWorldReadySink = std::make_unique<Utils::Sink<void()>>(m_eventManager->System.OnGameWorldReady);
  m_onGameWorldReadySink->Connect<&PluginManager::OnGameWorldReady>(this);

  API::JsonReaderApi::FillJsonReaderApi(&m_jsonReaderAPI);
  FillAPIs();
}

void PluginManager::DiscoverPlugins() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
  if (!logger) return;

  logger->Info("--- Discovering Plugins ---");
  const auto& pluginsDir = PathManager::GetPluginsPath();
  logger->Info("Searching for plugins in: {}", pluginsDir.string());

  m_discoveredPlugins.clear();

  try {
    if (!std::filesystem::exists(pluginsDir) || !std::filesystem::is_directory(pluginsDir)) {
      logger->Warn("Plugins directory does not exist. Skipping plugin discovery.");
      return;
    }

    for (const auto& pluginDirEntry : std::filesystem::directory_iterator(pluginsDir)) {
      if (!pluginDirEntry.is_directory()) continue;

      const auto& pluginPath = pluginDirEntry.path();
      const auto& pluginDirName = pluginPath.filename();
      auto dllPath = pluginPath / (pluginDirName.native() + L".dll");

      if (std::filesystem::exists(dllPath) && std::filesystem::is_regular_file(dllPath)) {
        std::string name = pluginDirName.string();
        m_discoveredPlugins[name] = {dllPath};
        logger->Info("Discovered plugin '{}' at {}", name, dllPath.string());
      }
    }
    logger->Info("--- Finished discovering plugins ---");
  } catch (const std::exception& e) {
    logger->Error("An unexpected error occurred during plugin discovery: {}", e.what());
  }
}

void PluginManager::InitializePlugins() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
  if (!logger) return;

  logger->Info("--- Initializing Enabled Plugins ---");
  const auto& componentInfoMap = m_configService->GetAllComponentInfo();

  for (const auto& [name, info] : componentInfoMap) {
    if (info.isFramework) {
      continue;
    }
    if (info.isEnabled) {
      LoadPlugin(name);
    }
  }
  logger->Info("--- Finished Initializing Plugins ---");
}

void PluginManager::LoadPlugin(const std::string& pluginName) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
  if (!logger) return;

  if (m_plugins.count(pluginName)) {
    logger->Warn("Plugin '{}' is already loaded.", pluginName);
    return;
  }

  auto discoveredIt = m_discoveredPlugins.find(pluginName);
  if (discoveredIt == m_discoveredPlugins.end()) {
    logger->Error("Cannot load plugin '{}': Not discovered.", pluginName);
    return;
  }

  const auto& dllPath = discoveredIt->second.dllPath;
  logger->Info("  -> Attempting to load library: {}", dllPath.string());
  HMODULE handle = LoadLibraryW(dllPath.c_str());
  if (!handle) {
    logger->Error("  -> Failed to load library. Win32 Error: {}", GetLastError());
    return;
  }

  auto getPluginFunc = reinterpret_cast<SPF_GetPlugin_t>(GetProcAddress(handle, "SPF_GetPlugin"));
  if (!getPluginFunc) {
    logger->Error("  -> Failed to find exported function 'SPF_GetPlugin'.");
    FreeLibrary(handle);
    return;
  }

  auto plugin = std::make_unique<LoadedPlugin>();
  plugin->handle = handle;
  plugin->dllPath = dllPath;
  plugin->name = pluginName;

  if (!getPluginFunc(&plugin->exports)) {
    logger->Error("  -> SPF_GetPlugin function returned false.");
    FreeLibrary(handle);
    return;
  }

  if (!plugin->exports.OnLoad || !plugin->exports.OnUnload) {
    logger->Error("  -> Plugin is missing required OnLoad or OnUnload functions.");
    FreeLibrary(handle);
    return;
  }

  m_eventManager->System.OnPluginWillBeLoaded.Call({plugin->name});
  if (plugin->exports.OnLoad) {
    logger->Debug("    -> Calling OnLoad() for plugin '{}'...", plugin->name);
    plugin->exports.OnLoad(&m_loadAPI);
  }

  auto& insertedPlugin = (m_plugins[pluginName] = std::move(plugin));
  m_eventManager->System.OnPluginDidLoad.Call({pluginName});
  logger->Info("Successfully loaded and initialized plugin '{}'.", pluginName);

  if (insertedPlugin->exports.OnActivated) {
    logger->Debug("    -> Calling OnActivated() for plugin '{}'...", insertedPlugin->name);
    insertedPlugin->exports.OnActivated(&m_coreAPI);
  }

  // If late init has already run, register UI for this single plugin immediately
  if (m_isLateInitDone) {
    logger->Info("Registering UI for dynamically loaded plugin '{}'...", pluginName);
    RegisterUIForPlugin(*insertedPlugin);
  }
}

void PluginManager::UnloadPlugin(const std::string& pluginName) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
  if (!logger) return;

  auto it = m_plugins.find(pluginName);
  if (it == m_plugins.end()) {
    logger->Warn("Cannot unload plugin '{}': Not loaded.", pluginName);
    return;
  }

  logger->Info("Unloading plugin: '{}'", pluginName);
  auto& plugin = it->second;

  m_eventManager->System.OnPluginWillBeUnloaded.Call({plugin->name});
  if (plugin->exports.OnUnload) {
    logger->Debug("-> Calling OnUnload...");
    plugin->exports.OnUnload();
  }

  // Clean up all hooks registered by this plugin
  auto& hookManager = Hooks::HookManager::GetInstance();
  m_pluginHooks.erase(std::remove_if(m_pluginHooks.begin(),
                                     m_pluginHooks.end(),
                                     [&](const std::unique_ptr<Hooks::IHook>& hook) {
                                       if (hook->GetOwnerName() == pluginName) {
                                         logger->Info("--> [1/3] Found hook '''{}''' to remove.", hook->GetDisplayName());
                                         hookManager.UnregisterFeatureHook(hook.get());
                                         logger->Info("--> [2/3] Unregistered from HookManager.");
                                         return true;  // Mark for erasure
                                       }
                                       return false;
                                     }),
                      m_pluginHooks.end());
  logger->Info("--> [3/3] Erased from PluginManager vector, destructors should have run.");

  m_handleManager->ReleaseHandlesFor(plugin->name);

  if (plugin->handle) {
    if (!FreeLibrary(plugin->handle)) {
      logger->Error("-> Failed to free library '{}'. Win32 Error: {}", plugin->dllPath.filename().string(), GetLastError());
    }
  }

  m_plugins.erase(it);
  logger->Info("Successfully unloaded plugin '{}'.", pluginName);
}

void PluginManager::RegisterPluginUIs() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
  if (!logger) return;

  logger->Info("--- Registering Plugin UIs and Keybind Actions ---");
  for (const auto& [name, plugin] : m_plugins) {
    RegisterUIForPlugin(*plugin);
  }
  m_isLateInitDone = true;  // Set the flag that late init is done
  logger->Info("--- Finished Registering Plugin UIs and Keybind Actions ---");
}

void PluginManager::RegisterUIForPlugin(const LoadedPlugin& plugin) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");

  // 1. Call the plugin's own UI registration function if it exists
  if (plugin.exports.OnRegisterUI) {
    logger->Debug("    -> Calling OnRegisterUI() for plugin '{}'...", plugin.name);
    plugin.exports.OnRegisterUI(&m_uiAPI);
  }


}

void PluginManager::UnloadAllPlugins() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
  logger->Info("--- Unloading All Plugins ---");

  // Create a copy of keys since UnloadPlugin modifies the map
  std::vector<std::string> pluginNames;
  for (const auto& [name, plugin] : m_plugins) {
    pluginNames.push_back(name);
  }

  for (const auto& name : pluginNames) {
    UnloadPlugin(name);
  }
  logger->Info("--- All plugins unloaded. ---");
}

void PluginManager::QueuePluginForUnload(const std::string& pluginName) {
  // To avoid duplicates
  if (std::find(m_unloadQueue.begin(), m_unloadQueue.end(), pluginName) == m_unloadQueue.end()) {
    m_unloadQueue.push_back(pluginName);
  }
}

void PluginManager::ProcessUnloadQueue() {
  if (m_unloadQueue.empty()) return;

  for (const auto& name : m_unloadQueue) {
    UnloadPlugin(name);
  }
  m_unloadQueue.clear();
}

void PluginManager::UpdateAllPlugins() {
  for (const auto& [name, plugin] : m_plugins) {
    if (plugin->exports.OnUpdate) {
      plugin->exports.OnUpdate();
    }
  }
}

void PluginManager::OnGameWorldReady() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
  if (!logger) return;

  logger->Info("--- Firing OnGameWorldReady for all loaded plugins ---");
  for (const auto& [name, plugin] : m_plugins) {
    if (plugin->exports.OnGameWorldReady) {
      logger->Debug("  -> Calling OnGameWorldReady() for plugin '{}'...", name);
      plugin->exports.OnGameWorldReady();
    }
  }
}

void PluginManager::NotifyPluginOfSettingChange(const std::string& pluginName, const std::string& keyPath, const void* newValue) {
  auto it = m_plugins.find(pluginName);
  if (it != m_plugins.end()) {
    auto& plugin = it->second;
    if (plugin->exports.OnSettingChanged) {
      plugin->exports.OnSettingChanged(keyPath.c_str(), reinterpret_cast<const SPF_JsonValue_Handle*>(newValue), &m_jsonReaderAPI);
    }
  }
}
bool PluginManager::IsPluginLoaded(const std::string& pluginName) const { return m_plugins.count(pluginName) > 0; }

// --- Hooks Trampolines ---

SPF_Hook_Handle* PluginManager::T_Hooks_Register(const char* pluginName, const char* hookName, const char* displayName, void* pDetour, void** ppOriginal, const char* signature, bool isEnabled) {
  auto& self = PluginManager::GetInstance();
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");

  if (!pluginName || !hookName || !displayName || !pDetour || !ppOriginal || !signature) {
    logger->Error("T_Hooks_Register failed: one or more arguments are null.");
    return nullptr;
  }

  logger->Info("Plugin '{}' is registering a new hook: '{}'", pluginName, hookName);

  // Create the proxy object
  auto hook = std::make_unique<PluginHook>(pluginName, hookName, displayName, pDetour, ppOriginal, signature, isEnabled);

  // Load configuration for this new hook (enabled state only)
  const auto* hooksSystems = self.m_configService->GetAllComponentSettings("hooks");
  if (hooksSystems && hooksSystems->count(pluginName)) {
    const auto& pluginHookSettings = hooksSystems->at(pluginName);
    if (pluginHookSettings.contains(hookName)) {
      const auto& specificHookSettings = pluginHookSettings.at(hookName);
      bool isEnabled = specificHookSettings.value("enabled", hook->IsEnabled());
      hook->SetEnabled(isEnabled);
      logger->Info("  -> Loaded config for hook '{}'. Enabled: {}.", hookName, isEnabled);
    }
  }

  // Register with the manager
  auto* hookPtr = hook.get();
  Hooks::HookManager::GetInstance().RegisterFeatureHook(hookPtr);

  // If the main hook installation has already run, install this new hook immediately.
  if (self.m_isLateInitDone) {
      Hooks::HookManager::GetInstance().InstallFeatureHook(hookPtr);
  }

  // Store it for lifetime management and return its raw pointer as a handle
  self.m_pluginHooks.push_back(std::move(hook));
  return reinterpret_cast<SPF_Hook_Handle*>(hookPtr);
}

void PluginManager::FillAPIs() {
  // Fill individual API structs
  API::LoggerApi::FillLoggerApi(&m_loggerAPI);
  API::LocalizationApi::FillLocalizationApi(&m_localizationAPI);
  API::ConfigApi::FillConfigApi(&m_configAPI);
  API::KeyBindsApi::FillKeyBindsApi(&m_keybindsAPI);
  API::UIApi::FillUIApi(&m_uiAPI);
  API::TelemetryApi::FillTelemetryApi(&m_telemetryAPI);
  API::VirtualInputApi::FillVirtualInputApi(&m_inputAPI);
  API::HooksApi::FillHooksApi(&m_hooksAPI, &PluginManager::T_Hooks_Register);
  API::CameraApi::FillCameraAPI(&m_cameraAPI);
  API::GameConsoleApi::FillGameConsoleApi(&m_gameConsoleAPI);
  API::FormattingApi::FillFormattingApi(&m_formattingAPI);
  API::GameLogApi::FillGameLogApi(&m_gameLogAPI);

  // --- Fill Load-Time API ---
  m_loadAPI.logger = &m_loggerAPI;
  m_loadAPI.localization = &m_localizationAPI;
  m_loadAPI.config = &m_configAPI;
  m_loadAPI.formatting = &m_formattingAPI;

  // --- Fill Core API (all services) ---
  m_coreAPI.logger = &m_loggerAPI;
  m_coreAPI.localization = &m_localizationAPI;
  m_coreAPI.config = &m_configAPI;
  m_coreAPI.keybinds = &m_keybindsAPI;
  m_coreAPI.ui = &m_uiAPI;
  m_coreAPI.telemetry = &m_telemetryAPI;
  m_coreAPI.input = &m_inputAPI;
  m_coreAPI.hooks = &m_hooksAPI;
  m_coreAPI.camera = &m_cameraAPI;
  m_coreAPI.console = &m_gameConsoleAPI;
  m_coreAPI.formatting = &m_formattingAPI;
  m_coreAPI.gamelog = &m_gameLogAPI;
}
}  // namespace Modules
SPF_NS_END  // namespace Modules