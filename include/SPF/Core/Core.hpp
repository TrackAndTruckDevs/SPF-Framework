#pragma once

#include <Windows.h>
#include <memory>
#include <vector>
#include <set>

#include "SPF/Namespace.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Config/IConfigurable.hpp"

struct scs_telemetry_init_params_t;
struct scs_input_init_params_t;

SPF_NS_BEGIN

// Forward declarations for managers and components
namespace Events {
class EventManager;
struct OnPluginWillBeLoaded;
struct OnPluginDidLoad;
struct OnPluginWillBeUnloaded;
namespace UI {
struct FocusComponentInSettingsWindow;
struct RequestPluginStateChange;
struct RequestSettingChange;
struct OnSettingWasChanged;
struct RequestInputCapture;
struct RequestInputCaptureCancel;
struct RequestBindingUpdate;
struct RequestDeleteBinding;
struct RequestBindingPropertyUpdate;
struct RequestExecuteCommand;
struct RequestUpdateCheck;
struct RequestPatronsFetch;
}  // namespace UI
namespace Config {
struct OnKeybindsModified;  // Added for live keybind updates
}
namespace System {
struct OnUpdateCheckSucceeded;
struct OnUpdateCheckFailed;
struct OnPatronsFetchCompleted;
}  // namespace System
}  // namespace Events
namespace Utils {
template <typename>
class Sink;
}
namespace Rendering {
class Renderer;
}
namespace Logging {
class Logger;
}
namespace UI {
class LoggerWindowSink;
class ImGuiInputConsumer;
class UIManager;
}  // namespace UI
namespace Modules {
class KeyBindsManager;
class PluginManager;
class HandleManager;
class IInputService;
class UpdateManager;
}  // namespace Modules
namespace Input {
class InputManager;
struct InputCaptured;
struct InputCaptureCancelled;
struct InputCaptureConflict;
}  // namespace Input

namespace Config {
struct IConfigService;
}

namespace System {
class ApiService;
}  // namespace System

namespace Telemetry {
class GameContext;
class SCSTelemetryService;
}  // namespace Telemetry

namespace Core {

enum class LifecycleState { Stopped, Preloading, Preloaded, Initializing, Initialized, ShuttingDown };

/**
 * @class Core
 * @brief The central orchestrator of the framework.
 *
 * Owns all major components (managers) and controls the main
 * initialization and shutdown sequences.
 */
class Core {
 public:
  Core(HMODULE module);
  ~Core();

  Core(const Core&) = delete;
  Core& operator=(const Core&) = delete;

  /**
   * @brief First-stage loading phase, called once when the DLL is attached.
   * Use this to set up systems that do not depend on the game (paths, logger, config).
   */
  void Preload();

  /**
   * @brief Handles the telemetry subsystem initialization callback from the game.
   * @param params The SDK initialization parameters.
   */
  void OnTelemetryInit(const scs_telemetry_init_params_t* params);
  void OnInputInit(const scs_input_init_params_t* const params);

  /**
   * @brief Handles the telemetry subsystem shutdown callback from the game.
   */
  void OnTelemetryShutdown();
  void OnInputShutdown();

  /**
   * @brief Main shutdown function.
   */
  void FullShutdown();

  /**
   * @brief Resets all SDK-dependent components to allow for a safe re-initialization.
   * This is a partial shutdown that leaves core services like logging and config alive.
   */
  void Reset();

  /**
   * @brief Second-stage initialization, called by the Renderer when it's ready.
   * Use this to initialize components that depend on the renderer or UI.
   */
  void LateInit();

  /**
   * @brief Per-frame update function, called before rendering.
   * This should update all framework logic.
   */
  void Update();

  /**
   * @brief Per-frame render function, called by the Renderer.
   * This should delegate the render call to all active UI components.
   */
  void ImGuiRender();

  /**
   * @brief Gets the core logger instance.
   * @return A pointer to the core logger.
   */
  Logging::Logger* GetLogger() const { return m_logger.get(); }

  /**
   * @brief Executes a console command using the GameConsole component.
   * @param command The command string to execute.
   */
  void ExecuteCommand(const std::string& command);

 private:
  void TryStartInitialization();

  void BindEventHandlers();
  void LogInitializationReports(const std::vector<InitializationReport>& reports);
  std::set<std::string> HandleServiceInitialization(const std::vector<InitializationReport>& reports);

  // --- Event Handlers ---
  void OnGameWorldReady();
  void OnTelemetryFrameStart();
  void OnRequestPluginStateChange(const Events::UI::RequestPluginStateChange& e);
  void OnPluginWillBeUnloaded(const Events::OnPluginWillBeUnloaded& e);
  void OnRequestSettingChange(const Events::UI::RequestSettingChange& e);
  void OnSettingWasChanged(const Events::UI::OnSettingWasChanged& e);
  void OnRequestInputCapture(const Events::UI::RequestInputCapture& e);
  void OnInputCaptured(const Input::InputCaptured& e);
  void OnInputCaptureCancelled(const Input::InputCaptureCancelled& e);
  void OnInputCaptureConflict(const Input::InputCaptureConflict& e);
  void OnRequestInputCaptureCancel(const Events::UI::RequestInputCaptureCancel& e);
  void OnRequestBindingUpdate(const Events::UI::RequestBindingUpdate& e);
  void OnRequestDeleteBinding(const Events::UI::RequestDeleteBinding& e);
  void OnRequestBindingPropertyUpdate(const Events::UI::RequestBindingPropertyUpdate& e);
  void OnKeybindsModified(const Events::Config::OnKeybindsModified& e);
  void OnRequestExecuteCommand(const Events::UI::RequestExecuteCommand& e);

  //  Update and Patrons event handlers
  void OnRequestUpdateCheck(const Events::UI::RequestUpdateCheck& e);
  void OnRequestPatronsFetch(const Events::UI::RequestPatronsFetch& e);
  void OnUpdateCheckSucceeded(const Events::System::OnUpdateCheckSucceeded& e);
  void OnUpdateCheckFailed(const Events::System::OnUpdateCheckFailed& e);
  void OnPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e);

  void ProcessHookDependenciesForPlugin(const std::string& pluginName, bool isEnabled);

  // --- Init/Shutdown Helpers ---
  void InitFeatureHooks();
  void InitServices();
  void InitManagersAndPlugins();
  void InitUI();
  void InitHooks();
  void ShutdownUI();
  void ShutdownManagers();
  void ShutdownServices();

  void InitTelemetry(const scs_telemetry_init_params_t* params);
  void ShutdownTelemetry();
  void InitInputService(const scs_input_init_params_t* params);
  void ShutdownInputService();

  HMODULE m_module;
  LifecycleState m_lifecycleState;
  bool m_telemetryReady = false;
  bool m_inputReady = false;
  bool m_handlersBound = false;

  // --- Logging Components ---
  std::shared_ptr<Logging::Logger> m_logger;

  // --- Core Managers ---
  // Order of declaration matters for destruction!
  std::unique_ptr<Events::EventManager> m_eventManager;
  std::unique_ptr<SPF::Config::IConfigService> m_configService;
  std::unique_ptr<Input::InputManager> m_inputManager;
  std::unique_ptr<Modules::KeyBindsManager> m_keyBindsManager;
  std::unique_ptr<Modules::HandleManager> m_handleManager;
  std::unique_ptr<System::ApiService> m_apiService;
  std::unique_ptr<Modules::UpdateManager> m_updateManager;
  std::vector<Config::IConfigurable*> m_configurableServices;

  std::unique_ptr<Telemetry::GameContext> m_gameContext;
  std::unique_ptr<Telemetry::SCSTelemetryService> m_telemetryService;
  std::unique_ptr<Modules::IInputService> m_inputService;

  // --- Event Sinks ---
  // These must be declared after EventManager to ensure they are destroyed before it.
  std::unique_ptr<Utils::Sink<void(const Events::OnPluginWillBeLoaded&)>> m_onPluginWillBeLoadedSink;
  std::unique_ptr<Utils::Sink<void(const Events::OnPluginWillBeUnloaded&)>> m_onPluginWillBeUnloadedSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestPluginStateChange&)>> m_onRequestPluginStateChangeSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestSettingChange&)>> m_onRequestSettingChangeSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::OnSettingWasChanged&)>> m_onSettingWasChangedSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestInputCapture&)>> m_onRequestInputCaptureSink;
  std::unique_ptr<Utils::Sink<void(const Input::InputCaptured&)>> m_onInputCapturedSink;
  std::unique_ptr<Utils::Sink<void(const Input::InputCaptureCancelled&)>> m_onInputCaptureCancelledSink;
  std::unique_ptr<Utils::Sink<void(const Input::InputCaptureConflict&)>> m_onInputCaptureConflictSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestInputCaptureCancel&)>> m_onRequestInputCaptureCancelSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestBindingUpdate&)>> m_onRequestBindingUpdateSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestDeleteBinding&)>> m_onRequestDeleteBindingSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestBindingPropertyUpdate&)>> m_onRequestBindingPropertyUpdateSink;
  std::unique_ptr<Utils::Sink<void(const Events::Config::OnKeybindsModified&)>> m_onKeybindsModifiedSink;
  std::unique_ptr<Utils::Sink<void()>> m_onTelemetryFrameStartSink;
  std::unique_ptr<Utils::Sink<void()>> m_onGameWorldReadySink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestExecuteCommand&)>> m_onRequestExecuteCommandSink;
  //  Sinks for Update and Patrons
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestUpdateCheck&)>> m_onRequestUpdateCheckSink;
  std::unique_ptr<Utils::Sink<void(const Events::UI::RequestPatronsFetch&)>> m_onRequestPatronsFetchSink;
  std::unique_ptr<Utils::Sink<void(const Events::System::OnUpdateCheckSucceeded&)>> m_onUpdateCheckSucceededSink;
  std::unique_ptr<Utils::Sink<void(const Events::System::OnUpdateCheckFailed&)>> m_onUpdateCheckFailedSink;
  std::unique_ptr<Utils::Sink<void(const Events::System::OnPatronsFetchCompleted&)>> m_onPatronsFetchCompletedSink;

  // --- UI Components ---
  std::unique_ptr<UI::ImGuiInputConsumer> m_imguiInputConsumer;

  // --- Low-level Systems ---
  std::unique_ptr<Rendering::Renderer> m_renderer;
};

}  // namespace Core
SPF_NS_END