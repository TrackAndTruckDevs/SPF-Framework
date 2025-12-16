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
#include "SPF/Utils/Signal.hpp"

#include <vector>
#include <memory>
#include <map>
#include <nlohmann/json.hpp>
#include <imgui.h>  // For ImFont

SPF_NS_BEGIN

namespace Events {
class EventManager;
}
namespace Input {
class InputManager;
}
namespace Modules {
class PluginManager;
}
namespace Config {
struct IConfigService;
}
namespace Rendering {
class Renderer;
}

namespace UI {
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
            Modules::PluginManager& pluginManager, Logging::LoggerFactory& loggerFactory, Modules::ITelemetryService& telemetryService);

  // Creates and registers all framework-defined UI windows.
  // This method centralizes UI window creation within UIManager.
  void CreateAndRegisterFrameworkWindows();

  Core::InitializationReport Initialize(const std::map<std::string, nlohmann::json>* allUIConfigs);
  void Shutdown();

  void RegisterWindow(std::shared_ptr<IWindow> window);
  IWindow* GetWindow(const std::string& componentName, const std::string& windowId) const;
  void RenderAll();

  // Renderer access
  void SetRenderer(Rendering::Renderer* renderer) { m_renderer = renderer; }
  Rendering::Renderer* GetRenderer() const { return m_renderer; }

  std::map<std::string, nlohmann::json> GetAllWindowSettings() const;

  void NotifyInputCaptured(const Input::InputCaptured& e);
  void NotifyInputCaptureCancelled(const Input::InputCaptureCancelled& e);
  void NotifyInputCaptureConflict(const Input::InputCaptureConflict& e);

  //  Update and Patrons notifications
  void NotifyUpdateCheckSucceeded(const Events::System::OnUpdateCheckSucceeded& e);
  void NotifyUpdateCheckFailed(const Events::System::OnUpdateCheckFailed& e);
  void NotifyPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e);

  void ToggleMouseOverridden();

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) override;

  ImFont* GetFont(const std::string& name) const;

  void CloseFocusedWindow();

 private:
  void OnPluginLoaded(const Events::OnPluginDidLoad& e);
  void OnPluginUnloaded(const Events::OnPluginWillBeUnloaded& e);

  void InitializeImGui();
  void ShutdownImGui();
  void DestroyWindowsForOwner(const std::string& owner);

 private:
  Events::EventManager* m_eventManager = nullptr;
  Input::InputManager* m_inputManager = nullptr;
  Config::IConfigService* m_configService = nullptr;
  Modules::KeyBindsManager* m_keyBindsManager = nullptr;
  Modules::PluginManager* m_pluginManager = nullptr;
  Logging::LoggerFactory* m_loggerFactory = nullptr;
  Modules::ITelemetryService* m_telemetryService = nullptr;
  Rendering::Renderer* m_renderer = nullptr;

  std::vector<std::shared_ptr<IWindow>> m_windows;
  std::map<std::string, ImFont*> m_fonts;
  const std::map<std::string, nlohmann::json>* m_allUIConfigs = nullptr;
  std::string m_windowToFocus;
  std::string m_lastFocusedDockedWindowId;
  bool m_wasShellVisibleLastFrame = false;
  bool m_isMouseControlOverridden = false;


  std::unique_ptr<Utils::Sink<void(const Events::OnPluginDidLoad&)>> m_onPluginDidLoadSink;
  std::unique_ptr<Utils::Sink<void(const Events::OnPluginWillBeUnloaded&)>> m_onPluginWillBeUnloadedSink;
};
}  // namespace UI

SPF_NS_END