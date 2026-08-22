#include "SPF/Core/Core.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/ConfigService.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Data/GameData/GameObjectVehicleService.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Events/ConfigEvents.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/Proxies/WndProcEventProxy.hpp"
#include "SPF/Events/SystemEvents.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/GameCamera/GameCameraManager.hpp"
#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Hooks/GameLogHook.hpp"
#include "SPF/Hooks/GameTools/ScsNameResolver.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Input/SCS/SCSInputService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/CommunicationManager.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Modules/IInputService.hpp"
#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/System/ApiService.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Telemetry/SCSTelemetryService.hpp"
#include "SPF/UI/ImGuiInputConsumer.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/SEHGuard.hpp"
#include "SPF/Utils/Signal.hpp"

#include "nlohmann/json_fwd.hpp"
#include "scssdk_telemetry.h"

#include <chrono>
#include <debugapi.h>
#include <exception>
#include <functional>
#include <memory>
#include <minwindef.h>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace SPF::Logging;
using namespace SPF::Events;
using namespace SPF::Rendering;
using namespace SPF::UI;
using namespace SPF::Hooks;
using namespace SPF::Modules;
using namespace SPF::System;
using namespace SPF::Config;
using namespace SPF::Localization;
using namespace SPF::Input;
using namespace SPF::GameCamera;
using namespace SPF::Data::GameData;

SPF_NS_BEGIN
namespace Core {
Core::Core(HMODULE module)
    : m_module(module),
      m_lifecycleState(LifecycleState::Stopped),
      m_telemetryReady(false),
      m_inputReady(false),
      m_eventManager(std::make_unique<EventManager>()),
      m_configService(std::make_unique<Config::ConfigService>(*m_eventManager)),
      m_inputManager(std::make_unique<Input::InputManager>(*m_eventManager)),
      m_onPluginWillBeLoadedSink(std::make_unique<Utils::Sink<void(const Events::OnPluginWillBeLoaded&)>>(m_eventManager->System.OnPluginWillBeLoaded)),
      m_onPluginWillBeUnloadedSink(std::make_unique<Utils::Sink<void(const Events::OnPluginWillBeUnloaded&)>>(m_eventManager->System.OnPluginWillBeUnloaded)),
      m_onRequestPluginStateChangeSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestPluginStateChange&)>>(m_eventManager->System.OnRequestPluginStateChange)),
      m_onRequestSettingChangeSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestSettingChange&)>>(m_eventManager->System.OnRequestSettingChange)),
      m_onSettingWasChangedSink(std::make_unique<Utils::Sink<void(const Events::UI::OnSettingWasChanged&)>>(m_eventManager->System.OnSettingWasChanged)),
      m_onRequestInputCaptureSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestInputCapture&)>>(m_eventManager->System.OnRequestInputCapture)),
      m_onInputCapturedSink(std::make_unique<Utils::Sink<void(const Input::InputCaptured&)>>(m_eventManager->System.OnInputCaptured)),
      m_onInputCaptureCancelledSink(std::make_unique<Utils::Sink<void(const Input::InputCaptureCancelled&)>>(m_eventManager->System.OnInputCaptureCancelled)),
      m_onInputCaptureConflictSink(std::make_unique<Utils::Sink<void(const Input::InputCaptureConflict&)>>(m_eventManager->System.OnInputCaptureConflict)),
      m_onRequestInputCaptureCancelSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestInputCaptureCancel&)>>(m_eventManager->System.OnRequestInputCaptureCancel)),
      m_onRequestBindingUpdateSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestBindingUpdate&)>>(m_eventManager->System.OnRequestBindingUpdate)),
      m_onRequestDeleteBindingSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestDeleteBinding&)>>(m_eventManager->System.OnRequestDeleteBinding)),
      m_onRequestBindingPropertyUpdateSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestBindingPropertyUpdate&)>>(m_eventManager->System.OnRequestBindingPropertyUpdate)),
      m_onKeybindsModifiedSink(std::make_unique<Utils::Sink<void(const Events::Config::OnKeybindsModified&)>>(m_eventManager->System.OnKeybindsModified)),
      m_onTelemetryFrameStartSink(std::make_unique<Utils::Sink<void()>>(m_eventManager->System.OnTelemetryFrameStart)),
      m_onRequestExecuteCommandSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestExecuteCommand&)>>(m_eventManager->System.OnRequestExecuteCommand))
      //  Initialize new Sinks
      ,
      m_onRequestUpdateCheckSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestUpdateCheck&)>>(m_eventManager->System.OnRequestUpdateCheck)),
      m_onRequestPatronsFetchSink(std::make_unique<Utils::Sink<void(const Events::UI::RequestPatronsFetch&)>>(m_eventManager->System.OnRequestPatronsFetch)),
      m_onUpdateCheckCompletedSink(std::make_unique<Utils::Sink<void(const Events::System::OnUpdateCheckCompleted&)>>(m_eventManager->System.OnUpdateCheckCompleted)),
      m_onPatronsFetchCompletedSink(std::make_unique<Utils::Sink<void(const Events::System::OnPatronsFetchCompleted&)>>(m_eventManager->System.OnPatronsFetchCompleted)),
      m_onUsageTrackingCompletedSink(std::make_unique<Utils::Sink<void(const Events::System::OnUsageTrackingCompleted&)>>(m_eventManager->System.OnUsageTrackingCompleted)) {}

Core::~Core() { FullShutdown(); }

void Core::Preload() {
  if (m_lifecycleState != LifecycleState::Stopped) {
    OutputDebugStringA("SPF WARNING: Core::Preload called in an unexpected state. Aborting.");
    return;
  }
  m_preloadStartTime = std::chrono::steady_clock::now();
  m_lifecycleState = LifecycleState::Preloading;

  InitializationReport preloadReport;
  preloadReport.ServiceName = "Preload";
  preloadReport.InfoMessages.push_back("Core Preload sequence started.");

  // This is the earliest possible point for initialization.
  // If this fails, we cannot continue, so we use a low-level message and exit.
  try {
    PathManager::Init(m_module);
    preloadReport.InfoMessages.push_back("PathManager initialized successfully.");
  } catch (const std::exception& e) {
    OutputDebugStringA("FATAL: Failed to initialize PathManager: ");
    OutputDebugStringA(e.what());
    m_lifecycleState = LifecycleState::Stopped;  // Reset state on failure
    return;
  }

  // Initialize EnvironmentManager early so framework information is available during UI initialization.
  EnvironmentManager::GetInstance().Initialize(m_module);

  // Initialize services that do not depend on the game SDK.
  InitServices();

  // Set the installation status detected by ConfigService into EnvironmentManager
  EnvironmentManager::GetInstance().SetInstallationStatus(m_configService->GetInstallationStatus());

  // Now that the logger is initialized in InitServices, we can log the report from the preload phase.
  LogInitializationReports({preloadReport});

  m_preloadMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_preloadStartTime).count();
  m_logger->Info("--- Core Preload sequence finished ({} ms) ---", m_preloadMs);
  m_lifecycleState = LifecycleState::Preloaded;
}

void Core::OnTelemetryInit(const scs_telemetry_init_params_t* params) {
  if (m_telemetryReady) {
    m_logger->Warn("Core::OnTelemetryInit called more than once. Ignoring.");
    return;
  }

  if (m_lifecycleState != LifecycleState::Preloaded) {
    m_logger->Error("Core::OnTelemetryInit called in unexpected state: {}. Aborting.", static_cast<int>(m_lifecycleState));
    return;
  }

  auto startTime = std::chrono::steady_clock::now();
  if (m_mainInitStartTime.time_since_epoch().count() == 0) {
    m_mainInitStartTime = startTime;
  }

  m_logger->Info("Core::OnTelemetryInit called.");
  InitTelemetry(params);

  m_telemetryMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
  m_telemetryReady = true;
  TryStartInitialization();
}

void Core::OnInputInit(const scs_input_init_params_t* const params) {
  if (m_inputReady) {
    m_logger->Warn("Core::OnInputInit called more than once. Ignoring.");
    return;
  }

  if (m_lifecycleState != LifecycleState::Preloaded) {
    m_logger->Error("Core::OnInputInit called in unexpected state: {}. Aborting.", static_cast<int>(m_lifecycleState));
    return;
  }

  auto startTime = std::chrono::steady_clock::now();
  if (m_mainInitStartTime.time_since_epoch().count() == 0) {
    m_mainInitStartTime = startTime;
  }

  m_logger->Info("Core::OnInputInit called.");
  InitInputService(params);

  m_inputMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
  m_inputReady = true;
  TryStartInitialization();
}

void Core::OnTelemetryShutdown() {
  m_logger->Info("SDK Telemetry is shutting down, triggering framework reset...");
  m_shutdownStartTime = std::chrono::steady_clock::now();
  Reset();
}

void Core::OnInputShutdown() {
  m_logger->Info("SDK Input is shutting down, triggering framework reset...");
  if (m_shutdownStartTime.time_since_epoch().count() == 0) {
    m_shutdownStartTime = std::chrono::steady_clock::now();
  }
  Reset();
}

void Core::TryStartInitialization() {
  // Check if we are in the right state and if all SDKs are ready.
  if (m_lifecycleState != LifecycleState::Preloaded || !m_telemetryReady || !m_inputReady) {
    return;
  }

  m_lifecycleState = LifecycleState::Initializing;
  m_logger->Info("--- All SDK services are ready. Initializing framework... ---");

  // Clear the list of configurable services to remove any dangling pointers from a previous session (after a Reset).
  m_configurableServices.clear();
  // Re-add the persistent services that are not re-created in the functions below.
  m_configurableServices.push_back(&Logging::LoggerFactory::GetInstance());

  auto managersStartTime = std::chrono::steady_clock::now();
  InitManagersAndPlugins();

  // 1. Bind core event handlers immediately after managers are initialized.
  // This ensures that any events fired during UI or plugin initialization (like update checks) are handled.
  m_logger->Info("-> [Init] Binding core event handlers...");
  BindEventHandlers();

  // Preload plugins so they can create virtual input devices during their OnLoad phase.
  // This must happen before RegisterCreatedDevices().
  PluginManager::GetInstance().InitializePlugins();

  // Automatically trigger analytics session with a 10-second delay to avoid startup stutters.
  ScheduleTask(std::chrono::seconds(10), [this]() {
    m_logger->Info("Triggering deferred analytics session...");
    m_eventManager->System.OnRequestTrackUsage.Call({});
  });

  // Now that plugins are loaded and have created their devices, register them.
  // This MUST be done before scs_input_init returns.
  m_inputService->RegisterCreatedDevices();
  m_managersMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - managersStartTime).count();

  auto uiStartTime = std::chrono::steady_clock::now();
  InitUI();
  m_uiMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - uiStartTime).count();

  auto hooksStartTime = std::chrono::steady_clock::now();
  InitHooks();
  m_hooksMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - hooksStartTime).count();

  m_lifecycleState = LifecycleState::Initialized;
  m_logger->Info("--- Framework initialization complete. ---");
}

void Core::Reset() {
  if (m_lifecycleState != LifecycleState::Initialized) {
    // If we are not initialized, there is nothing to reset.
    // We might be called twice by the two SDK shutdown events, this check prevents double execution.
    return;
  }

  m_lifecycleState = LifecycleState::ShuttingDown;
  m_logger->Info("--- Core Reset sequence started (partial shutdown) ---");

  // Step 1: Disable all hooks before shutting down the renderer.
  m_logger->Info("-> [Reset] Step 1/5: Disabling all hooks...");
  HookManager::GetInstance().UninstallAllHooks();

  // Step 2: Unload all plugins (their draw callbacks can no longer run).
  m_logger->Info("-> [Reset] Step 2/5: Unloading plugins...");
  PluginManager::GetInstance().UnloadAllPlugins();

  // Full world-scoped teardown: data services drop cached world state and the
  // camera stack uninstalls. The world may change while the framework is down
  // (exit to menu, another save), so nothing world-scoped survives a Reset.
  ResetWorldScopedServices();

  // Step 3: Shutdown renderer backend and then UI.
  // UIManager is a singleton that outlives this cycle; without this it keeps
  // a dangling pointer into the destroyed renderer until SetRenderer() runs
  // again next session. Plugin API calls in between would use freed memory.
  UIManager::GetInstance().SetRenderer(nullptr);
  m_logger->Info("-> [Reset] Step 3/5: Shutting down Renderer and UI...");
  m_renderer.reset();
  ShutdownUI();

  // Save any pending configuration changes after subsystems are shut down (including UI window states).
  m_logger->Info("-> [Reset] Saving dirty configurations before managers shutdown...");
  m_configService->SaveAllDirty();

  // Step 4: Shutdown session-based managers.
  m_logger->Info("-> [Reset] Step 4/5: Shutting down session managers...");
  ShutdownManagers();

  // Step 5: Shutdown game-specific services provided by the SDK.
  m_logger->Info("-> [Reset] Step 5/5: Shutting down SDK services...");
  ShutdownTelemetry();
  ShutdownInputService();

  // --- DO NOT SHUTDOWN CORE SERVICES LIKE LOGGER/CONFIG/EVENTMANAGER ---
  // These services must persist to allow for a framework reload.

  // Reset SDK readiness flags to allow for a full restart.
  m_telemetryReady = false;
  m_inputReady = false;

  // The framework is now in a clean state, ready to be re-initialized by new SDK callbacks.
  m_lifecycleState = LifecycleState::Preloaded;
  m_logger->Info("--- Core Reset sequence finished. Framework is now in Preloaded state. ---");

  // Ensure background thread is finished before allowing a reload
  if (m_deferredInitThread.joinable()) {
    m_deferredInitThread.join();
  }

  // Reset one-shot state for the new telemetry session.
  m_deferredInitDone.store(false);
  m_worldLoadedOnce = false;

  // Drop any deferred tasks that never executed (they may capture freed manager state).
  {
    std::lock_guard<std::mutex> lock(m_deferredTasksMutex);
    m_deferredTasks.clear();
  }
}

void Core::FullShutdown() {
  if (m_lifecycleState == LifecycleState::Stopped || m_lifecycleState == LifecycleState::ShuttingDown) {
    return;
  }
  m_lifecycleState = LifecycleState::ShuttingDown;

  // From this point nothing may query the plugin manager (its config service
  // dies in step 6): logging falls back to cached results instead.
  PluginManager::MarkNotAlive();

  // The MinGW SEH guard registers a process-wide vectored handler; if it
  // outlives the DLL, the next exception anywhere in the game crashes in
  // unloaded-module memory.
  Utils::detail::RemoveHandler();

  m_logger->Info("--- Core Full Shutdown sequence started ---");

  // Step 1: Completely remove all hooks
  m_logger->Info("-> [Shutdown] Step 1/7: Removing all hooks...");
  HookManager::GetInstance().RemoveAllHooks();

  // Step 2: Unload all plugins (their draw callbacks can no longer run).
  m_logger->Info("-> [Shutdown] Step 2/7: Unloading plugins...");
  PluginManager::GetInstance().UnloadAllPlugins();

  // Step 3: Shutdown session-based managers.
  m_logger->Info("-> [Shutdown] Step 3/7: Shutting down session managers...");
  ShutdownManagers();

  // Step 4: Shutdown renderer backend and UI.
  // Same dangling-pointer contract as in Reset(): clear it before the
  // renderer dies, the singleton outlives the object.
  UIManager::GetInstance().SetRenderer(nullptr);
  m_logger->Info("-> [Shutdown] Step 4/7: Shutting down Renderer and UI...");
  m_renderer.reset();
  ShutdownUI();

  // Step 5: Shutdown game-specific services provided by the SDK.
  m_logger->Info("-> [Shutdown] Step 5/7: Shutting down SDK services...");
  ShutdownTelemetry();
  ShutdownInputService();

  // Step 6: Shutdown core services, saving configuration to disk.
  m_logger->Info("-> [Shutdown] Step 6/7: Shutting down core services...");
  ShutdownServices();

  // Reset SDK readiness flags.
  m_telemetryReady = false;
  m_inputReady = false;

  m_lifecycleState = LifecycleState::Stopped;

  if (m_shutdownStartTime.time_since_epoch().count() != 0) {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_shutdownStartTime).count();
    m_logger->Info("--- Core Full Shutdown sequence finished. ---");
    m_logger->Debug("<<<---------- The total time to unload the framework is: {} ms ---------->>>", duration);
  } else {
    m_logger->Info("--- Core Full Shutdown sequence finished. ---");
  }

  // Ensure background thread is finished before exiting
  if (m_deferredInitThread.joinable()) {
    m_deferredInitThread.join();
  }

  // Drop any deferred tasks that never executed (they may capture freed manager state).
  {
    std::lock_guard<std::mutex> lock(m_deferredTasksMutex);
    m_deferredTasks.clear();
  }

  // Release pattern-finder caches (xref/string/pointer lookups) before unloading the game.
  Utils::PatternFinder::ClearXrefCache();

  // Step 7: The logger factory is the very last thing to be shut down.
  // This is called last because all previous steps may want to log messages.
  LoggerFactory::GetInstance().Shutdown();
}

void Core::InitFeatureHooks() {
  m_logger->Info("--- Applying Feature Hook Configurations ---");

  const auto* frameworkSettingsNode = m_configService->GetAllComponentSettings("settings");
  if (!frameworkSettingsNode || !frameworkSettingsNode->count("framework")) {
    m_logger->Error("Could not retrieve framework settings to apply hook states.");
    return;
  }

  const auto& frameworkSettings = frameworkSettingsNode->at("framework");
  const bool isNewInstall = (System::EnvironmentManager::GetInstance().GetFrameworkInfo().installStatus == System::InstallationStatus::NewInstall);

  if (!frameworkSettings.contains("hook_states")) {
    if (isNewInstall) {
      m_logger->Debug("'hook_states' not found in framework settings (expected for new install). Hooks will use default values.");
    } else {
      m_logger->Warn("'hook_states' not found in framework settings. Hooks will use default values.");
    }
    return;
  }

  const auto& hookStates = frameworkSettings.at("hook_states");
  auto& hookManager = Hooks::HookManager::GetInstance();

  for (auto* hook : hookManager.GetFeatureHooks()) {
    const std::string& hookName = hook->GetName();
    if (hookStates.contains(hookName)) {
      bool isEnabled = hookStates.at(hookName).value("enabled", hook->IsEnabled());
      hookManager.ReconcileHookState(hook, isEnabled);  // Use ReconcileHookState
      m_logger->Info("Applied config for hook: {}. Enabled: {}.", hookName, isEnabled);
    } else {
      // If no config entry, use the hook's default enabled state
      hookManager.ReconcileHookState(hook, hook->IsEnabled());

      if (isNewInstall) {
        m_logger->Debug("No config entry found for hook: {}. Using default state (Enabled: {}).", hookName, hook->IsEnabled());
      } else {
        m_logger->Warn("No config entry found for hook: {}. Using default state (Enabled: {}).", hookName, hook->IsEnabled());
      }
    }
  }
}

void Core::InitServices() {
  // This function is called once during Preload.
  // It bootstraps services that have no dependencies other than the filesystem.

  // Phase 1: Create services.
  m_handleManager = std::make_unique<HandleManager>();

  // Phase 2: Finalize ConfigService so manifests are loaded.
  InitializationReport configReport;
  m_configService->Finalize(&configReport);

  // Phase 3: Initialize the logger factory now that config is available.
  const auto* loggingConfigs = m_configService->GetAllComponentSettings("logging");
  auto loggerReport = LoggerFactory::GetInstance().Initialize(PathManager::GetLogsPath(), loggingConfigs ? loggingConfigs->at("framework") : nlohmann::ordered_json{});
  m_configurableServices.push_back(&LoggerFactory::GetInstance());
  m_logger = LoggerFactory::GetInstance().GetLogger("Core");  // Logger is assigned here.

  // --- LOGGING CAN ONLY HAPPEN AFTER THIS POINT ---
  m_logger->Info("--- Initializing Core Services ---");
  m_logger->Info("-> [Init] Core service instances created.");
  m_logger->Info("-> [Init] ConfigService finalized.");
  m_logger->Info("-> [Init] LoggerFactory initialized.");

  LogInitializationReports({configReport, loggerReport});

  // Phase 4: Register and initialize feature hooks from config.
  m_logger->Info("-> [Init] Registering and configuring feature hooks...");
  auto& hookManager = HookManager::GetInstance();
  hookManager.RegisterFeatureHook(&GameLogHook::GetInstance());
  hookManager.RegisterFeatureHook(&GameConsole::GetInstance());
  hookManager.RegisterFeatureHook(&GameTools::ScsNameResolver::GetInstance());
  InitFeatureHooks();
  m_logger->Info("--- Core Services Initialized ---");
}

void Core::InitManagersAndPlugins() {
  m_logger->Info("--- Initializing Managers and Plugins ---");
  std::vector<InitializationReport> reports;

  // Phase 1: Create session-based managers.
  m_logger->Info("-> [Init] Creating session manager instances (KeyBinds, UI)...");
  m_apiService = std::make_unique<System::ApiService>();
  m_communicationManager = std::make_unique<Modules::CommunicationManager>(*m_eventManager, *m_apiService, *m_configService);
  m_keyBindsManager = std::make_unique<KeyBindsManager>(*m_inputManager, *m_eventManager);
  m_configurableServices.push_back(m_keyBindsManager.get());
  // Initialize the UIManager singleton
  UIManager::GetInstance().Init(*m_eventManager, *m_inputManager, *m_configService, *m_keyBindsManager, PluginManager::GetInstance(), *m_communicationManager, LoggerFactory::GetInstance(), *m_telemetryService);
  m_configurableServices.push_back(&UIManager::GetInstance());  // Add the singleton to configurable services
  //  Initialize the CommunicationManager
  reports.push_back(m_communicationManager->Initialize());

  // Phase 2: Initialize PluginManager and discover plugins on disk.
  m_logger->Info("-> [Init] Initializing PluginManager and discovering plugins...");
  PluginManager::GetInstance().Init(*m_eventManager, *m_handleManager, *m_configService, *m_keyBindsManager, UIManager::GetInstance(), *m_telemetryService, *m_inputService);
  PluginManager::GetInstance().DiscoverPlugins();

  //  Load all discovered plugin manifests and then re-process all system configurations.
  m_logger->Info("-> [Init] Loading all discovered plugin manifests...");
  PluginManager::GetInstance().LoadAllDiscoveredPluginManifests();

  m_logger->Info("-> [Init] Re-processing all system configurations with plugin manifests...");
  InitializationReport configReaggregationReport;
  configReaggregationReport.ServiceName = "ConfigServiceReaggregation";
  m_configService->ProcessAllSystemConfigurations(configReaggregationReport);
  if (configReaggregationReport.HasIssues()) {
    reports.push_back(configReaggregationReport);
  }
  LogInitializationReports({configReaggregationReport});  // Log this report immediately

  // --- Apply logging configurations for all loaded components ---
  m_logger->Info("-> [Init] Applying logging configurations for all components...");
  if (const auto* loggingConfigs = m_configService->GetAllComponentSettings("logging")) {
    for (const auto& [componentName, componentConfig] : *loggingConfigs) {
      LoggerFactory::GetInstance().ApplyConfigurationFor(componentName, componentConfig);
    }
  }

  // Phase 3: Reconcile config state with discovered plugins (e.g., add new plugins to config).
  m_logger->Info("-> [Init] Reconciling plugin states...");
  InitializationReport reconReport;
  m_configService->ReconcilePluginStates(PluginManager::GetInstance().GetDiscoveredPluginNames(), &reconReport);
  m_configService->ReconcileHookStates(HookManager::GetInstance().GetFeatureHooks(), &reconReport);
  if (reconReport.HasIssues()) {
    reports.push_back(reconReport);
  }

  // Refresh installation identity and its registry mirror (needs reconciled component info)
  m_configService->GetFrameworkInstanceId();

  // Phase 4: Initialize managers that depend on plugin manifests.
  m_logger->Info("-> [Init] Initializing Localization, KeyBinds, and UI managers...");
  reports.push_back(LocalizationManager::GetInstance().Initialize(m_configService->GetAllComponentSettings("localization")));
  m_configurableServices.push_back(&LocalizationManager::GetInstance());
  reports.push_back(m_keyBindsManager->Initialize(m_configService->GetMergedConfig("keybinds"), m_configService->GetAllComponentInfo()));
  reports.push_back(UIManager::GetInstance().Initialize(m_configService->GetAllComponentSettings("ui")));
  LogInitializationReports(reports);

  // Phase 5: Handle any initialization issues and re-initialize if necessary.
  auto resetServices = HandleServiceInitialization(reports);
  if (!resetServices.empty()) {
    m_logger->Info("--- Re-initializing services after configuration reset ---");
    std::vector<InitializationReport> reinitReports;
    if (resetServices.count("keybinds")) {
      reinitReports.push_back(m_keyBindsManager->Initialize(m_configService->GetMergedConfig("keybinds"), m_configService->GetAllComponentInfo()));
    }
    if (resetServices.count("localization")) {
      reinitReports.push_back(LocalizationManager::GetInstance().Initialize(m_configService->GetAllComponentSettings("localization")));
    }
    if (!reinitReports.empty()) LogInitializationReports(reinitReports);
  }

  // Phase 6: Call OnLoad for all plugins now that the core is fully configured.
  // This is now moved to LateInit to speed up initial framework startup.
  // m_logger->Info("-> [Init] Loading enabled plugins...");
  // PluginManager::GetInstance().InitializePlugins();

  // Phase 7: Process hook dependencies for all initially enabled plugins.
  // This is now moved to LateInit to speed up initial framework startup.
  // m_logger->Info("-> [Init] Processing initial hook dependencies...");
  // const auto& pluginInfoMap = m_configService->GetPluginInfo();
  // for (const auto& [name, info] : pluginInfoMap) {
  //   if (info.isEnabled) {
  //     ProcessHookDependenciesForPlugin(name, true);
  //   }
  // }
  m_logger->Info("--- Managers and Plugins Initialized ---");
}

void Core::InitUI() {
  m_logger->Info("--- Initializing UI Components ---");

  // Create and register framework-owned windows via UIManager.
  m_logger->Info("-> [Init] Creating and registering framework windows via UIManager...");
  UIManager::GetInstance().CreateAndRegisterFrameworkWindows();

  // The ImGui consumer should be registered last to give it priority over other input consumers.
  m_logger->Info("-> [Init] Creating and registering ImGui input consumer...");
  m_imguiInputConsumer = std::make_unique<ImGuiInputConsumer>();
  m_inputManager->RegisterConsumer(m_imguiInputConsumer.get());
}

void Core::InitHooks() {
  m_logger->Info("--- Initializing Low-Level Systems (Renderer and Hooks) ---");

  // 1. Create the renderer. Its constructor will perform API detection.
  m_logger->Info("-> [Init] Creating Renderer and detecting API...");
  m_renderer = std::make_unique<Renderer>(*this, *m_eventManager, UIManager::GetInstance());
  UIManager::GetInstance().SetRenderer(m_renderer.get());
  auto detectedAPI = m_renderer->GetDetectedAPI();

  // 2. Initialize standalone services that don't depend on hooks.
  m_logger->Info("-> [Init] Initializing standalone services...");
  GameDataCameraService::GetInstance().Initialize();
  GameObjectVehicleService::GetInstance().Initialize();
  GameWorldService::GetInstance().Initialize();
  ClimateService::GetInstance().Initialize();
  ManagerCoreService::GetInstance().Initialize();
  GameObjectSessionService::GetInstance().Initialize();
  GameObjectFileSystemService::GetInstance().Initialize();

  // 4. Initialize core systems that may be used by hooks.
  m_logger->Info("-> [Init] Initializing EventManager and InputManager...");
  m_eventManager->Init(*m_renderer);
  m_inputManager->Initialize();

  // 5. Install hooks based on detected API.
  auto& hookManager = HookManager::GetInstance();

  m_logger->Info("-> [Init] Installing graphics hooks for detected API...");
  if (!hookManager.InstallGraphicsHooks(detectedAPI)) {
    // If this fails, a critical error is already logged by the manager.
    // We can't proceed with rendering.
    m_logger->Error("Graphics hook installation failed. UI will not be available.");
  }

  m_logger->Info("-> [Init] Installing other system and feature hooks...");
  if (!hookManager.InstallSystemAndFeatureHooks()) {
    m_logger->Warn("Failed to install one or more system/feature hooks.");
  }

  // 6. Finalize renderer initialization. This will create the specific implementation
  // and connect to the now-installed graphics hook signals.
  m_logger->Info("-> [Init] Initializing Renderer backend...");
  if (m_renderer) {
    m_renderer->Init();
  }
  m_logger->Info("--- Low-Level Systems Initialized ---");
}

void Core::ShutdownUI() {
  m_logger->Info("--> Shutting down UI...");
  // Save all window settings before destroying the UI.
  // UIManager is a singleton, so we just check if it's initialized (e.g., has eventManager)
  // or rely on its own shutdown. For now, directly call GetInstance().
  if (m_configService) {  // UIManager::GetInstance() is always available, check configService instead.
    const auto allWindowSettings = UIManager::GetInstance().GetAllWindowSettings();
    for (const auto& [componentName, componentSettings] : allWindowSettings) {
      if (componentSettings.contains("windows")) {
        m_configService->SetValue(componentName, "ui.windows", componentSettings.at("windows"));
      }
    }
  }
  // UIManager is a singleton, so its context will be destroyed by ShutdownImGui, not reset here.
  // The UIManager itself is a static object.
  if (m_imguiInputConsumer) m_inputManager->UnregisterConsumer(m_imguiInputConsumer.get());
  m_imguiInputConsumer.reset();

  // Explicitly call Shutdown on the UIManager singleton.
  UIManager::GetInstance().Shutdown();
}

void Core::ShutdownManagers() {
  m_logger->Info("--> Shutting down managers...");
  m_keyBindsManager.reset();
  if (m_communicationManager) {
    // Cancel in-flight API requests and disconnect sinks before destruction.
    m_communicationManager->Shutdown();
  }
  m_communicationManager.reset();  //  Reset CommunicationManager
  m_apiService.reset();            //  Reset ApiService
  // m_inputManager.reset();
  // m_handleManager.reset();
  // UIManager is now a singleton, no need to reset unique_ptr
}

void Core::ShutdownServices() {
  m_logger->Info("--> Shutting down core services...");

  // Unbind all event handlers and reset the sinks.
  m_logger->Info("    -> Unbinding event handlers and resetting sinks...");
  m_handlersBound = false;
  m_onPluginWillBeLoadedSink.reset();
  m_onPluginWillBeUnloadedSink.reset();
  m_onRequestPluginStateChangeSink.reset();
  m_onRequestSettingChangeSink.reset();
  m_onSettingWasChangedSink.reset();
  m_onRequestInputCaptureSink.reset();
  m_onInputCapturedSink.reset();
  m_onInputCaptureCancelledSink.reset();
  m_onInputCaptureConflictSink.reset();
  m_onRequestInputCaptureCancelSink.reset();
  m_onRequestBindingUpdateSink.reset();
  m_onRequestDeleteBindingSink.reset();
  m_onRequestBindingPropertyUpdateSink.reset();
  m_onKeybindsModifiedSink.reset();
  m_onTelemetryFrameStartSink.reset();
  m_onRequestExecuteCommandSink.reset();

  //  Update and Patrons sinks
  m_onRequestUpdateCheckSink.reset();
  m_onRequestPatronsFetchSink.reset();
  m_onUpdateCheckCompletedSink.reset();
  m_onPatronsFetchCompletedSink.reset();
  m_onUsageTrackingCompletedSink.reset();

  // Config service is last, saving all pending changes to disk.
  m_logger->Info("    -> Saving configuration and shutting down ConfigService...");
  if (m_configService) {
    m_configService->SaveAllDirty();
  }
  m_configService.reset();
}

void Core::LateInit() {
  m_logger->Info("LateInit called. Initializing UI-dependent components...");
  // This is called by the renderer once the graphics device is ready.
  m_logger->Info("-> [LateInit] Activating preloaded plugins...");
  PluginManager::GetInstance().ActivatePlugins();

  m_logger->Info("-> [LateInit] Processing initial hook dependencies for enabled plugins...");
  const auto& componentInfoMap = m_configService->GetAllComponentInfo();
  for (const auto& [name, info] : componentInfoMap) {
    if (info.isEnabled) {
      ProcessHookDependenciesForPlugin(name, true);
    }
  }

  PluginManager::GetInstance().RegisterPluginUIs();

  // Start heavy background initialization
  m_logger->Info("-> [LateInit] Starting deferred background initialization thread...");
  m_deferredInitThread = std::thread(&Core::PerformDeferredInitialization, this);
}

void Core::PerformDeferredInitialization() {
  /**
   * BACKGROUND THREAD: Heavy tasks like memory pattern scanning.
   */
  auto startTime = std::chrono::steady_clock::now();
  auto logger = LoggerFactory::GetInstance().GetLogger("AsyncInit");
  logger->Info("Starting background tasks (memory scanning, build hashing)...");

  // 1. Resolve Session and Profile offsets
  auto& sessionService = GameObjectSessionService::GetInstance();
  if (sessionService.TryFindAllOffsets()) {
    logger->Debug("Session and Profile offsets resolved.");
  }

  // 2. Resolve FileSystem (UFS) offsets
  auto& fileSystemService = GameObjectFileSystemService::GetInstance();
  if (fileSystemService.TryFindAllOffsets()) {
    logger->Debug("FileSystem (UFS) offsets resolved.");
  }

  // 3. Resolve Core Manager addresses (GameplayManager, ...)
  auto& managerService = Data::GameData::ManagerCoreService::GetInstance();
  if (managerService.TryFindAllOffsets()) {
    logger->Debug("Manager Core addresses resolved.");
  }

  // 4. Resolve GameWorld (Environment) offsets
  auto& worldService = GameWorldService::GetInstance();
  if (worldService.TryFindAllOffsets()) {
    logger->Debug("GameWorld (Environment) offsets resolved.");
  }

  // 5. Calculate framework build hash
  EnvironmentManager::GetInstance().CalculateBuildHash();

  m_deferredInitDone.store(true);

  m_deferredMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
  logger->Info("Background tasks completed successfully ({} ms).", m_deferredMs);

  // --- FINAL DETAILED REPORT ---
  auto now = std::chrono::steady_clock::now();
  auto mainInitTotal = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_mainInitStartTime).count();

  m_logger->Info("<<<---------- FRAMEWORK LOADING REPORT ---------->>>");
  m_logger->Info("  -> Preload phase (DLL attach):     {:>5} ms", m_preloadMs);
  m_logger->Info("  -> [Main Init] (Click to ready):   {:>5} ms", mainInitTotal);
  m_logger->Info("     |-- Telemetry Module:           {:>5} ms", m_telemetryMs);
  m_logger->Info("     |-- Input Module:               {:>5} ms", m_inputMs);
  m_logger->Info("     |-- Managers & Plugins:         {:>5} ms", m_managersMs);
  m_logger->Info("     |-- UI Components:              {:>5} ms", m_uiMs);
  m_logger->Info("     |-- Low-level Hooks:            {:>5} ms", m_hooksMs);
  m_logger->Info("     |-- Deferred Background Tasks:  {:>5} ms", m_deferredMs);
  m_logger->Info("<<<---------------------------------------------->>>");
}

void Core::Update() {
  // This is the main update loop for logic that is not tied to telemetry frames.
  // It's called by the renderer on every visual frame.

  // 1. Process deferred tasks
  {
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_deferredTasksMutex);
    for (auto it = m_deferredTasks.begin(); it != m_deferredTasks.end();) {
      if (now >= it->triggerTime) {
        it->action();
        it = m_deferredTasks.erase(it);
      } else {
        ++it;
      }
    }
  }

  if (m_inputManager) {
    m_inputManager->ProcessButtonActions();
    m_inputManager->ProcessKeyboardActions();
    m_inputManager->ProcessMouseActions();
    m_inputManager->ProcessJoystickActions();
  }

  // Update UI Manager (processes font requests and other pre-frame tasks)
  UI::UIManager::GetInstance().Update();

  PluginManager::GetInstance().UpdateAllPlugins();
  //  Update CommunicationManager to process async results
  if (m_communicationManager) {
    m_communicationManager->Update();
  }
}

void Core::ImGuiRender() {
  // Update() is no longer called from here.

  // The UIManager is a singleton, so it's always available after Init.
  UIManager::GetInstance().RenderAll();
}
void Core::BindEventHandlers() {
  if (m_handlersBound) {
    return;
  }
  m_logger->Info("Binding event handlers...");
  m_onPluginWillBeUnloadedSink->Connect<&Core::OnPluginWillBeUnloaded>(this);
  m_onRequestPluginStateChangeSink->Connect<&Core::OnRequestPluginStateChange>(this);
  m_onRequestSettingChangeSink->Connect<&Core::OnRequestSettingChange>(this);
  m_onSettingWasChangedSink->Connect<&Core::OnSettingWasChanged>(this);
  m_onRequestInputCaptureSink->Connect<&Core::OnRequestInputCapture>(this);
  m_onInputCapturedSink->Connect<&Core::OnInputCaptured>(this);
  m_onInputCaptureCancelledSink->Connect<&Core::OnInputCaptureCancelled>(this);
  m_onInputCaptureConflictSink->Connect<&Core::OnInputCaptureConflict>(this);
  m_onRequestInputCaptureCancelSink->Connect<&Core::OnRequestInputCaptureCancel>(this);
  m_onRequestBindingUpdateSink->Connect<&Core::OnRequestBindingUpdate>(this);
  m_onRequestDeleteBindingSink->Connect<&Core::OnRequestDeleteBinding>(this);
  m_onRequestBindingPropertyUpdateSink->Connect<&Core::OnRequestBindingPropertyUpdate>(this);
  m_onKeybindsModifiedSink->Connect<&Core::OnKeybindsModified>(this);
  m_onTelemetryFrameStartSink->Connect<&Core::OnTelemetryFrameStart>(this);
  m_onRequestExecuteCommandSink->Connect<&Core::OnRequestExecuteCommand>(this);
  //  Connect new Sinks
  m_onRequestUpdateCheckSink->Connect<&Core::OnRequestUpdateCheck>(this);
  m_onRequestPatronsFetchSink->Connect<&Core::OnRequestPatronsFetch>(this);
  m_onUpdateCheckCompletedSink->Connect<&Core::OnUpdateCheckCompleted>(this);
  m_onPatronsFetchCompletedSink->Connect<&Core::OnPatronsFetchCompleted>(this);
  m_onUsageTrackingCompletedSink->Connect<&Core::OnUsageTrackingCompleted>(this);
  m_handlersBound = true;
}

void Core::OnRequestExecuteCommand(const Events::UI::RequestExecuteCommand& e) { GameConsole::GetInstance().Execute(e.command); }

void Core::OnRequestUpdateCheck(const Events::UI::RequestUpdateCheck& e) { m_communicationManager->RequestUpdateCheck(e.force); }

void Core::OnRequestPatronsFetch(const Events::UI::RequestPatronsFetch& e) { m_communicationManager->RequestPatronsFetch(e.force); }

void Core::OnUpdateCheckCompleted(const Events::System::OnUpdateCheckCompleted& e) { UIManager::GetInstance().NotifyUpdateCheckCompleted(e); }

void Core::OnPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e) { UIManager::GetInstance().NotifyPatronsFetchCompleted(e); }

void Core::OnUsageTrackingCompleted(const Events::System::OnUsageTrackingCompleted& e) {}

void Core::FinalizeWorldInitialization() {
  m_logger->Info("Finalizing world initialization...");

  // Lazy-install CameraHooks now that they are needed for the GameCameraManager.
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  if (!cameraHooks.IsInstalled()) {
    m_logger->Info("Performing deferred installation of CameraHooks...");
    Hooks::HookManager::GetInstance().RegisterFeatureHook(&cameraHooks);
    Hooks::HookManager::GetInstance().InstallFeatureHook(&cameraHooks);
  }

  // Re-resolve every registered world-scoped data service BEFORE installing
  // the camera manager: it depends on camera data being ready. The registry
  // orders services by priority, so dependency roots resolve first.
  for (auto* service : Data::GameData::WorldServiceRegistry::Get().All()) {
    if (service->TryFinalizeWorldInit()) {
      m_logger->Info("{} is now ready.", service->GetName());
    } else {
      m_logger->Warn("{} is not ready yet (waiting for dependencies or game data).", service->GetName());
    }
  }

  // Install the camera manager last: all finders are ready at this point.
  if (!GameCameraManager::GetInstance().IsInstalled()) {
    GameCameraManager::GetInstance().Install();
  }
}
void Core::LogInitializationReports(const std::vector<InitializationReport>& reports) {
  m_logger->Info("--- System Initialization Report ---");
  bool hasAnyIssues = false;

  for (const auto& report : reports) {
    auto serviceLogger = LoggerFactory::GetInstance().GetLogger(report.ServiceName);
    if (report.HasIssues()) {
      hasAnyIssues = true;
      serviceLogger->Info("Initialized with issues:");
      for (const auto& msg : report.InfoMessages) {
        serviceLogger->Info("  -> {}", msg);
      }
      for (const auto& warning : report.Warnings) {
        serviceLogger->Warn("  -> {} (Key: {})", warning.Message, warning.ConfigKeyPath);
      }
      for (const auto& error : report.Errors) {
        serviceLogger->Error("  -> {} (Key: {})", error.Message, error.ConfigKeyPath);
      }
    } else {
      serviceLogger->Info("Initialized successfully.");
      for (const auto& msg : report.InfoMessages) {
        serviceLogger->Info("  -> {}", msg);
      }
    }
  }

  if (hasAnyIssues) {
    m_logger->Info("--- End of Report (with issues) ---");
  } else {
    m_logger->Info("--- End of Report (all services OK) ---");
  }
}

std::set<std::string> Core::HandleServiceInitialization(const std::vector<InitializationReport>& reports) {
  InitializationReport resetReport;
  resetReport.ServiceName = "ConfigServiceReset";
  bool wasResetNeeded = false;
  std::set<std::string> resetServices;

  for (const auto& report : reports) {
    if (!report.HasIssues()) continue;

    for (const auto& error : report.Errors) {
      if (!error.ConfigKeyPath.empty()) {
        m_logger->Warn("Service '{}' reported an issue with key '{}'. Attempting to reset to default.", report.ServiceName, error.ConfigKeyPath);
        m_configService->ResetToDefault(report.ServiceName, error.ConfigKeyPath, &resetReport);
        wasResetNeeded = true;
        resetServices.insert(report.ServiceName);
      }
    }
  }

  if (wasResetNeeded) {
    m_logger->Info("--- Config Service Reset Report ---");
    LogInitializationReports({resetReport});
    m_logger->Warn("Configuration was reset. Some changes may require a restart to take full effect.");
  }
  return resetServices;
}

void Core::OnRequestPluginStateChange(const Events::UI::RequestPluginStateChange& e) {
  if (e.enable) {
    // Process dependencies before loading the plugin
    ProcessHookDependenciesForPlugin(e.pluginName, true);
    Modules::PluginManager::GetInstance().LoadPlugin(e.pluginName);
  } else {
    // Dependencies are processed on the OnPluginWillBeUnloaded event.
    Modules::PluginManager::GetInstance().QueuePluginForUnload(e.pluginName);

    // Ensure all dirty settings are saved to disk before/during unloading
    if (m_configService) {
      m_configService->SaveAllDirty();
    }
  }

  // Also update the configuration to persist the state
  m_configService->SetValue("framework", "settings.plugin_states." + e.pluginName + ".enabled", e.enable);

  // Trigger update check after enabling (isEnabled must be set first)
  if (e.enable && m_communicationManager) {
    m_communicationManager->RequestPluginUpdateChecks();
  }
}

void Core::OnPluginWillBeUnloaded(const Events::OnPluginWillBeUnloaded& e) {
  // This event is fired just before a plugin is unloaded.
  // We process its dependencies to release any hooks it required.
  ProcessHookDependenciesForPlugin(e.pluginName, false);
}

void Core::OnRequestSettingChange(const Events::UI::RequestSettingChange& e) { m_configService->SetValue(e.componentName, e.keyPath, e.newValue); }

void Core::OnSettingWasChanged(const Events::UI::OnSettingWasChanged& e) {
  // m_logger->Debug("Setting changed: System='{}', Component='{}', Path='{}', Value='{}'", e.systemName, e.componentName, e.keyPath, e.newValue.dump());

  bool wasHandledByFramework = false;
  // --- POLL FRAMEWORK SERVICES ---
  for (auto* service : m_configurableServices) {
    if (service->OnSettingChanged(e.systemName, e.componentName, e.keyPath, e.newValue)) {
      wasHandledByFramework = true;
    }
  }

  // --- Handle hook settings changes ---
  if (!wasHandledByFramework && e.systemName == "settings" && e.componentName == "framework") {
    static const std::string prefix = "hook_states.";
    static const std::string suffix = ".enabled";
    if (e.keyPath.rfind(prefix, 0) == 0 && e.keyPath.size() > prefix.size() + suffix.size() && e.keyPath.substr(e.keyPath.size() - suffix.size()) == suffix) {
      std::string hookName = e.keyPath.substr(prefix.size(), e.keyPath.size() - prefix.size() - suffix.size());

      auto& hookManager = Hooks::HookManager::GetInstance();
      if (auto* hook = hookManager.GetHook(hookName)) {
        if (e.newValue.is_boolean()) {
          bool shouldBeEnabled = e.newValue.get<bool>();
          // Do NOT call hook->SetEnabled(shouldBeEnabled) directly here.
          // ReconcileHookState will handle both setting the internal state and MinHook state.
          m_logger->Info("Runtime change for hook '{}': user set enabled state to {}.", hook->GetName(), shouldBeEnabled);

          // Centralize all install/uninstall decisions in one place.
          hookManager.ReconcileHookState(hook, shouldBeEnabled);

          wasHandledByFramework = true;
        }
      }
    }
  }

  std::string fullKeyPath = e.systemName + "." + e.keyPath;

  // --- GLOBAL BROADCASTS ---
  // If the framework's language changes, we notify ALL plugins via the dedicated OnLanguageChanged callback.
  // This broadcast only happens if the 'sync_plugin_languages' setting is enabled in the framework config.
  if (e.componentName == "framework" && fullKeyPath == "localization.language") {
    bool shouldSync = m_configService->GetValue("framework", "localization.sync_plugin_languages", false).get<bool>();

    if (shouldSync) {
      std::string newLangCode;
      if (e.newValue.is_object() && e.newValue.contains("_value")) {
        newLangCode = e.newValue["_value"].get<std::string>();
      } else if (e.newValue.is_string()) {
        newLangCode = e.newValue.get<std::string>();
      }

      if (!newLangCode.empty()) {
        m_logger->Info("Broadcasting framework language change to all plugins: '{}'", newLangCode);
        PluginManager::GetInstance().NotifyAllPluginsOfLanguageChange(newLangCode);
      }
    } else {
      m_logger->Debug("Framework language changed, but plugin synchronization is disabled.");
    }
  }

  // --- DISPATCH TO PLUGINS IF NOT HANDLED ---
  if (!wasHandledByFramework) {
    PluginManager::GetInstance().NotifyPluginOfSettingChange(e.componentName, fullKeyPath);
  }
}

void Core::OnRequestInputCapture(const Events::UI::RequestInputCapture& e) { m_inputManager->StartInputCapture(e.actionFullName, e.originalBinding); }

void Core::OnInputCaptured(const Input::InputCaptured& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("Core");
  logger->Debug("Core received InputCaptured event for input: {}", e.capturedInput->GetDisplayName());

  // Check for conflict using KeyBindsManager
  auto conflicts = m_keyBindsManager->GetBindingsForInput(*e.capturedInput);

  if (!conflicts.empty()) {
    // Conflict detected, fire InputCaptureConflict event
    logger->Info("Input conflict detected: Input {} is already bound to {} action(s).", e.capturedInput->GetDisplayName(), conflicts.size());

    // Re-create the input object to pass ownership to the new event, as the original event is const.
    auto newCapturedInput = Modules::InputFactory::CreateFromJson(e.capturedInput->ToJson());

    InputCaptureConflict conflict_data{e.actionFullName, std::move(newCapturedInput), std::move(conflicts), e.originalBinding};
    m_eventManager->System.OnInputCaptureConflict.Call(conflict_data);
  } else {
    // No conflict, forward the original InputCaptured event
    logger->Info("Input {} captured for action {}. No conflict detected.", e.capturedInput->GetDisplayName(), e.actionFullName);
    UIManager::GetInstance().NotifyInputCaptured(e);
  }
}

void Core::OnInputCaptureCancelled(const Input::InputCaptureCancelled& e) {
  // Forward to UI
  UIManager::GetInstance().NotifyInputCaptureCancelled(e);
}

void Core::OnInputCaptureConflict(const Input::InputCaptureConflict& e) {
  // Forward to UI
  UIManager::GetInstance().NotifyInputCaptureConflict(e);
}

void Core::OnRequestInputCaptureCancel(const Events::UI::RequestInputCaptureCancel& e) { m_inputManager->CancelInputCapture(); }

void Core::OnRequestBindingUpdate(const Events::UI::RequestBindingUpdate& e) {
  nlohmann::ordered_json newBindingWithDefaults = e.newBinding;

  // If originalBinding is empty, it's a new binding, so we should add defaults.
  if (e.originalBinding.empty()) {
    // Add default press_type if not present
    if (!newBindingWithDefaults.contains("press_type")) {
      newBindingWithDefaults["press_type"] = "short";
    }
    // Add default consume policy if not present
    if (!newBindingWithDefaults.contains("consume")) {
      newBindingWithDefaults["consume"] = "never";
    }
  }

  m_configService->UpdateBinding(e.actionFullName, e.originalBinding, newBindingWithDefaults, e.bindingToClear);
  m_inputManager->CancelInputCapture();
}

void Core::OnRequestDeleteBinding(const Events::UI::RequestDeleteBinding& e) { m_configService->DeleteBinding(e.actionFullName, e.bindingToDelete); }

void Core::OnRequestBindingPropertyUpdate(const Events::UI::RequestBindingPropertyUpdate& e) { m_configService->UpdateBindingProperty(e.actionFullName, e.originalBinding, e.propertyName, e.newValue); }

void Core::OnKeybindsModified(const Events::Config::OnKeybindsModified& e) {
  m_logger->Info("Keybindings were modified. Updating KeyBindsManager...");
  m_keyBindsManager->UpdateKeybindings(m_configService->GetMergedConfig("keybinds"));
}

void Core::ProcessHookDependenciesForPlugin(const std::string& pluginName, bool isEnabled) {
  const auto& componentInfoMap = m_configService->GetAllComponentInfo();
  auto it = componentInfoMap.find(pluginName);
  if (it == componentInfoMap.end()) return;

  const auto& info = it->second;
  if (info.required_hooks.empty()) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("Core");
  auto& hookManager = Hooks::HookManager::GetInstance();

  for (const auto& hookName : info.required_hooks) {
    if (isEnabled) {
      logger->Info("Processing requirement: Plugin '{}' requires hook '{}'.", pluginName, hookName);
      hookManager.RequestEnableHook(hookName, pluginName);
    } else {
      logger->Info("Processing release: Plugin '{}' no longer requires hook '{}'.", pluginName, hookName);
      hookManager.ReleaseEnableRequest(hookName, pluginName);
    }

    // Immediately reconcile the state of the affected hook.
    if (auto* hook = hookManager.GetHook(hookName)) {
      // Get the configured enabled state for this hook from ConfigService.
      // If not found in config, use the hook's current enabled state as default.
      bool configuredEnabledState = m_configService->GetValue("framework", "settings.hook_states." + hookName + ".enabled", hook->IsEnabled()).get<bool>();
      hookManager.ReconcileHookState(hook, configuredEnabledState);
    }
  }
}

void Core::OnTelemetryFrameStart() {
  // Fire world-ready only when the world is loaded AND background init finished.
  if (m_deferredInitDone && m_telemetryService) {
    // SCS sends timer_restart only when a world STARTS loading. A framework
    // reset + re-init can happen while a world is already loaded (the player
    // can toggle the framework mid-session), so a fresh session also detects
    // an in-progress world via the simulation timestamp.
    const bool timerRestarted = m_telemetryService->ConsumeTimerRestart();
    const bool worldAlreadyLoaded = !m_worldLoadedOnce && m_telemetryService->GetTimestamps().simulation > 0;
    if (timerRestarted || worldAlreadyLoaded) {
      if (m_worldLoadedOnce) {
        m_logger->Info("World reload detected. Firing OnWorldUnloading, resetting services, then finalizing.");
        // Notify plugins to unhook from the old world
        m_eventManager->System.OnWorldUnloading.Call();
        // Reset world-scoped services so they re-find offsets in the new world
        ResetWorldScopedServices();
      } else {
        m_logger->Info("First world load detected. Finalizing world initialization.");
        m_worldLoadedOnce = true;
      }
      // Core finalizes its own world-scoped state directly, then notifies external consumers.
      FinalizeWorldInitialization();
      m_eventManager->System.OnGameWorldReady.Call();
    }
  }

  const float dt = m_telemetryService ? m_telemetryService->GetDeltaTime() : 0.0f;
  if (GameCameraManager::GetInstance().IsInstalled()) {
    GameCameraManager::GetInstance().Update(dt);
  }
}

void Core::ResetWorldScopedServices() {
  m_logger->Info("Resetting world-scoped services for reload...");

  // Every registered world-scoped service clears its own state; new services
  // join automatically via WorldServiceRegistry self-registration.
  for (auto* service : Data::GameData::WorldServiceRegistry::Get().All()) {
    service->ResetForWorldReload();
  }

  // Camera stack: full uninstall so it re-installs on new world
  GameCameraManager::GetInstance().Uninstall();
  Hooks::CameraHooks::GetInstance().Uninstall();

  m_logger->Info("World-scoped services reset complete.");
}
void Core::InitTelemetry(const scs_telemetry_init_params_t* params) {
  m_logger->Info("--- Initializing Telemetry Module ---");

  // The `params` struct is a base type. We need to cast it to a versioned
  // type to access its members. The header shows v100 and v101 are the same.
  const auto* versioned_params = static_cast<const scs_telemetry_init_params_v100_t*>(params);

  // The `common` member is a struct, not a pointer. We access its members directly.
  m_gameContext = std::make_unique<Telemetry::GameContext>(versioned_params->common.game_id, versioned_params->common.game_version);

  m_telemetryService = std::make_unique<Telemetry::SCSTelemetryService>(*m_logger, *m_gameContext, *m_eventManager);

  // Initialize the telemetry service, which will register for SDK events.
  m_telemetryService->Initialize(params);
}

void Core::ShutdownTelemetry() {
  m_logger->Info("--- Shutting Down Telemetry Module ---");
  if (m_telemetryService) {
    m_telemetryService->Shutdown();
  }
  m_telemetryService.reset();
  m_gameContext.reset();
}

void Core::InitInputService(const scs_input_init_params_t* params) {
  m_logger->Info("--- Initializing Input Service ---");
  m_inputService = std::make_unique<Input::SCS::SCSInputService>(*m_logger, *m_eventManager);
  m_inputService->Initialize(params);
}

void Core::ShutdownInputService() {
  m_logger->Info("--- Shutting Down Input Service ---");
  if (m_inputService) {
    m_inputService->Shutdown();
  }
  m_inputService.reset();
}

void Core::ExecuteCommand(const std::string& command) { GameConsole::GetInstance().Execute(command); }

void Core::ScheduleTask(std::chrono::milliseconds delay, std::function<void()> action) {
  auto triggerTime = std::chrono::steady_clock::now() + delay;
  std::lock_guard<std::mutex> lock(m_deferredTasksMutex);
  m_deferredTasks.push_back({triggerTime, std::move(action)});
}

}  // namespace Core
SPF_NS_END