#pragma once

#include "SPF/Core/InitializationReport.hpp"
#include "SPF/UI/IWindow.hpp"
#include "SPF/Config/IConfigurable.hpp"

#include "SPF/Input/InputEvents.hpp"
#include "SPF/Namespace.hpp"

#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"    // Added include for LoggerFactory
#include "SPF/Modules/ITelemetryService.hpp" // Added include for ITelemetryService

#include "SPF/Events/PluginEvents.hpp"
#include "SPF/Events/SystemEvents.hpp"
#include "SPF/Utils/Signal.hpp"

#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <imgui.h>  // For ImFont

#include "SPF/Renderer/ITexture.hpp"

SPF_NS_BEGIN

namespace Events {
class EventManager;
}
namespace Input {
class InputManager;
}
namespace Modules {
class PluginManager;
class CommunicationManager;
}
namespace Config {
struct IConfigService;
}
namespace Rendering {
class Renderer;
}

namespace UI {
class IWindow;
class NotificationWindow; // Forward declaration

class UIManager : public Config::IConfigurable {
 private:  // Private constructor for Singleton pattern
  UIManager();

 public:
  ~UIManager();

  // Singleton access
  static UIManager& GetInstance();

  // Delete copy constructor and assignment operator for Singleton pattern
  UIManager(const UIManager&) = delete;
  void operator=(const UIManager&) = delete;

  // Initialize method to pass dependencies, replacing constructor parameters
  void Init(Events::EventManager& eventManager, Input::InputManager& inputManager, Config::IConfigService& configService, Modules::KeyBindsManager& keyBindsManager,
            Modules::PluginManager& pluginManager, Modules::CommunicationManager& communicationManager, Logging::LoggerFactory& loggerFactory, Modules::ITelemetryService& telemetryService);

  // Creates and registers all framework-defined UI windows.
  // This method centralizes UI window creation within UIManager.
  void CreateAndRegisterFrameworkWindows();

  Core::InitializationReport Initialize(const std::map<std::string, nlohmann::ordered_json>* allUIConfigs);
  void Shutdown();

  void RegisterWindow(std::shared_ptr<IWindow> window);
  IWindow* GetWindow(const std::string& componentName, const std::string& windowId) const;
  
  /**
   * @brief Updates internal UI state (processes queues, rebuilds font atlas if needed).
   * This MUST be called before ImGui::NewFrame().
   */
  void Update();

  void RenderAll();

  // Renderer access
  void SetRenderer(Rendering::Renderer* renderer) { m_renderer = renderer; }
  Rendering::Renderer* GetRenderer() const { return m_renderer; }

  void ApplyDeveloperMode(bool enabled);

  std::map<std::string, nlohmann::ordered_json> GetAllWindowSettings() const;

  void NotifyInputCaptured(const Input::InputCaptured& e);
  void NotifyInputCaptureCancelled(const Input::InputCaptureCancelled& e);
  void NotifyInputCaptureConflict(const Input::InputCaptureConflict& e);

  //  API related notifications
  void NotifyUpdateCheckCompleted(const Events::System::OnUpdateCheckCompleted& e);
  void NotifyPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e);
  void NotifyUsageTrackingCompleted(const Events::System::OnUsageTrackingCompleted& e);
  void NotifyPluginUpdateAvailable(const Events::System::OnPluginUpdateAvailable& e);

  const Events::System::OnPluginUpdateAvailable* GetPluginUpdate(const std::string& pluginId) const;

  void ToggleMouseOverridden();
  void SetMouseOverride(bool overridden) { m_isMouseControlOverridden = overridden; }
  bool IsMouseOverridden() const { return m_isMouseControlOverridden; }

  /**
   * @brief Shows a framework-level notification.
   */
  void ShowNotification(const std::string& message, int type, SPF_Notification_DisplayMode mode);

  /**
   * @brief Shows an extended notification using the provided parameters.
   */
  SPF_Notification_Handle ShowNotificationEx(const SPF_Notification_Params* params);

  /**
   * @brief Hides a notification by its handle.
   */
  void HideNotification(SPF_Notification_Handle handle);

  /**
   * @brief Starts a cinematic screen transition.
   */
  void PlayTransition(int type, float duration, bool reverse, int color);

  /**
   * @brief Checks if a transition is currently playing.
   */
  bool IsTransitionActive() const { return m_activeTransition.active; }

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& newValue) override;

  ImFont* GetFont(const std::string& name) const;

  void CloseFocusedWindow();

  // --- Manual Texture Management (API support) ---
  void* CreatePluginTexture(const void* data, size_t size, int* out_width, int* out_height);
  void* CreatePluginTextureFromFile(const char* file_path, int* out_width, int* out_height);
  void DestroyPluginTexture(void* texture_id);

  // --- Dynamic Font Management (API support) ---
  ImFont* LoadPluginFontFromMemory(const std::string& name, const void* data, size_t data_size, float size_pixels, bool merge, const uint16_t* ranges);
  ImFont* LoadPluginFontFromFile(const std::string& name, const std::string& path, float size_pixels, bool merge, const uint16_t* ranges);

 private:
  void OnPluginLoaded(const Events::OnPluginDidLoad& e);
  void OnPluginUnloaded(const Events::OnPluginWillBeUnloaded& e);
  void OnReleaseNotesReceived(const System::ChangelogData& data);

  void InitializeImGui();
  void ShutdownImGui();
  void DestroyWindowsForOwner(const std::string& owner);

  void ProcessTransitions();

 private:
  struct TransitionState {
    int type = 0;
    int colorPreset = 0;
    float duration = 0.0f;
    float startTime = 0.0f;
    bool reverse = false;
    bool active = false;
  };

  TransitionState m_activeTransition;

  Events::EventManager* m_eventManager = nullptr;
  Input::InputManager* m_inputManager = nullptr;
  Config::IConfigService* m_configService = nullptr;
  Modules::KeyBindsManager* m_keyBindsManager = nullptr;
  Modules::PluginManager* m_pluginManager = nullptr;
  Modules::CommunicationManager* m_communicationManager = nullptr;
  Logging::LoggerFactory* m_loggerFactory = nullptr;
  Modules::ITelemetryService* m_telemetryService = nullptr;
  Rendering::Renderer* m_renderer = nullptr;

  std::vector<std::shared_ptr<IWindow>> m_windows;
  std::map<std::string, ImFont*> m_fonts;
  std::unordered_map<void*, std::unique_ptr<Rendering::ITexture>> m_pluginTextures;

  // Dynamic Font Management internals
  struct FontRequest {
    std::string name;
    std::string path;
    std::vector<unsigned char> data;
    float size_pixels;
    bool merge;
    std::vector<uint16_t> ranges;
    bool isMemory;
    bool isCompressed;
  };
  std::vector<FontRequest> m_pendingFontRequests;
  std::vector<std::unique_ptr<std::vector<unsigned char>>> m_fontDataBuffers;
  bool m_fontAtlasRebuildPending = false;

  const std::map<std::string, nlohmann::ordered_json>* m_allUIConfigs = nullptr;
  std::string m_windowToFocus;
  std::string m_lastFocusedDockedWindowId;
  bool m_wasShellVisibleLastFrame = false;
  bool m_isMouseControlOverridden = false;

  std::shared_ptr<NotificationWindow> m_notificationWindow;

  std::unordered_map<std::string, Events::System::OnPluginUpdateAvailable> m_pluginUpdates;

  std::unique_ptr<Utils::Sink<void(const Events::OnPluginDidLoad&)>> m_onPluginDidLoadSink;
  std::unique_ptr<Utils::Sink<void(const Events::OnPluginWillBeUnloaded&)>> m_onPluginWillBeUnloadedSink;
  std::unique_ptr<Utils::Sink<void(const System::ChangelogData&)>> m_onReleaseNotesReceivedSink;
  std::unique_ptr<Utils::Sink<void(const Events::System::OnPluginUpdateAvailable&)>> m_onPluginUpdateAvailableSink;
};
}  // namespace UI

SPF_NS_END