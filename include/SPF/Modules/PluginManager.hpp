#pragma once

#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Manifest_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_Localization_API.h"
#include "SPF/SPF_API/SPF_Config_API.h"
#include "SPF/SPF_API/SPF_KeyBinds_API.h"
#include "SPF/SPF_API/SPF_Hooks_API.h"
#include "SPF/SPF_API/SPF_Camera_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"
#include "SPF/SPF_API/SPF_Telemetry_API.h"
#include "SPF/SPF_API/SPF_VirtInput_API.h"
#include "SPF/SPF_API/SPF_GameConsole_API.h"
#include "SPF/SPF_API/SPF_JsonReader_API.h"
#include "SPF/SPF_API/SPF_Formatting_API.h"
#include "SPF/SPF_API/SPF_GameLog_API.h"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Namespace.hpp"
#include "SPF/Utils/Signal.hpp"

#include <Windows.h>  // For HMODULE
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <filesystem>


SPF_NS_BEGIN

// Forward declarations
namespace Events {
class EventManager;
}
namespace Modules {
class HandleManager;
class KeyBindsManager;
class ITelemetryService;
class IInputService;
}  // namespace Modules

namespace UI {
class UIManager;
}

namespace Modules {
class PluginManager {
 public:
  static PluginManager& GetInstance();
  void LoadAllDiscoveredPluginManifests();

  std::vector<std::string> GetDiscoveredPluginNames() const;
  bool IsPluginLoaded(const std::string& pluginName) const;

  PluginManager(const PluginManager&) = delete;
  void operator=(const PluginManager&) = delete;

  ~PluginManager();

  void Init(Events::EventManager& eventManager, HandleManager& handleManager, SPF::Config::IConfigService& configService, KeyBindsManager& keyBindsManager,
            SPF::UI::UIManager& uiManager, ITelemetryService& telemetryService, IInputService& inputService);

  void DiscoverPlugins();
  void InitializePlugins();
  void LoadPlugin(const std::string& pluginName);
  void UnloadPlugin(const std::string& pluginName);
  void QueuePluginForUnload(const std::string& pluginName);
  void ProcessUnloadQueue();
  void RegisterPluginUIs();
  void UnloadAllPlugins();
  void UpdateAllPlugins();

      void NotifyPluginOfSettingChange(const std::string& pluginName, const std::string& keyPath);
  SPF_UI_API* GetUIApi() { return &m_uiAPI; }
  SPF::UI::UIManager* GetUIManager() { return m_uiManager; }
  HandleManager* GetHandleManager() { return m_handleManager; }
  SPF::Config::IConfigService* GetConfigService() { return m_configService; }
  KeyBindsManager* GetKeyBindsManager() { return m_keyBindsManager; }
  ITelemetryService* GetTelemetryService() { return m_telemetryService; }
  IInputService* GetInputService() { return m_inputService; }

  std::vector<std::string>& GetL10nAvailableLanguagesCache() { return s_available_languages_cache; }
  std::vector<const char*>& GetL10nAvailableLanguagesCStrCache() { return s_available_languages_c_str_cache; }

 private:
  PluginManager();

  struct DiscoveredPlugin {
    std::filesystem::path dllPath;
  };

  struct LoadedPlugin {
    HMODULE handle = nullptr;
    std::string name;
    SPF_Plugin_Exports exports{};
    std::filesystem::path dllPath;
  };

  // --- Hooks Trampolines ---
  static SPF_Hook_Handle* T_Hooks_Register(const char* pluginName, const char* hookName, const char* displayName, void* pDetour, void** ppOriginal, const char* signature, bool isEnabled);

  void FillAPIs();

  void RegisterUIForPlugin(const LoadedPlugin& plugin);

  void OnGameWorldReady();

  // --- Member Variables ---
  Events::EventManager* m_eventManager = nullptr;
  HandleManager* m_handleManager = nullptr;
  SPF::Config::IConfigService* m_configService = nullptr;
  KeyBindsManager* m_keyBindsManager = nullptr;
  SPF::UI::UIManager* m_uiManager = nullptr;
  ITelemetryService* m_telemetryService = nullptr;
  IInputService* m_inputService = nullptr;
  bool m_isLateInitDone = false;

  std::map<std::string, DiscoveredPlugin> m_discoveredPlugins;
  std::map<std::string, std::unique_ptr<LoadedPlugin>> m_plugins;
  std::vector<std::string> m_unloadQueue;

  std::unique_ptr<Utils::Sink<void()>> m_onGameWorldReadySink;

  std::vector<std::unique_ptr<Hooks::IHook>> m_pluginHooks;

  SPF_Load_API m_loadAPI{};
  SPF_Core_API m_coreAPI{};
  SPF_Logger_API m_loggerAPI{};
  SPF_Localization_API m_localizationAPI{};
  SPF_Config_API m_configAPI{};
  SPF_KeyBinds_API m_keybindsAPI{};
  SPF_UI_API m_uiAPI{};
  SPF_Telemetry_API m_telemetryAPI{};
  SPF_Input_API m_inputAPI{};
  SPF_Hooks_API m_hooksAPI{};
  SPF_Camera_API m_cameraAPI{};
  SPF_GameConsole_API m_gameConsoleAPI{};
  SPF_JsonReader_API m_jsonReaderAPI{};
  SPF_Formatting_API m_formattingAPI{};
  SPF_GameLog_API m_gameLogAPI{};

  static std::vector<std::string> s_available_languages_cache;
  static std::vector<const char*> s_available_languages_c_str_cache;
};
}  // namespace Modules

SPF_NS_END
