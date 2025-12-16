#pragma once

#include <memory>
#include <vector>

#include "SPF/Events/PluginEvents.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Events/ConfigEvents.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/Events/EventProxyBase.hpp"
#include "SPF/Events/SystemEvents.hpp"
#include "SPF/Namespace.hpp"
#include "SPF/Utils/Signal.hpp"

SPF_NS_BEGIN

namespace Rendering {
class Renderer; // Forward-declaration
}

namespace Events {
// Forward-declaration
class EventManager;

// Structure providing controlled access to EventManager signals
struct EventDispatcher {
  friend EventManager;

 public:
  Utils::Signal<void(const UI::ResizeEvent&)>& OnWindowResize;

  // Plugin lifecycle signals
  Utils::Signal<void(const OnPluginWillBeLoaded&)>& OnPluginWillBeLoaded;
  Utils::Signal<void(const OnPluginDidLoad&)>& OnPluginDidLoad;
  Utils::Signal<void(const OnPluginWillBeUnloaded&)>& OnPluginWillBeUnloaded;

  Utils::Signal<void(const UI::FocusComponentInSettingsWindow&)>& OnFocusComponentInSettingsWindow;
  Utils::Signal<void(const UI::RequestSettingChange&)>& OnRequestSettingChange;
  Utils::Signal<void(const UI::RequestPluginStateChange&)>& OnRequestPluginStateChange;
  Utils::Signal<void(const UI::OnSettingWasChanged&)>& OnSettingWasChanged;

  Utils::Signal<void(const UI::RequestInputCapture&)>& OnRequestInputCapture;
  Utils::Signal<void(const Input::InputCaptured&)>& OnInputCaptured;
  Utils::Signal<void(const Input::InputCaptureCancelled&)>& OnInputCaptureCancelled;
  Utils::Signal<void(const Input::InputCaptureConflict&)>& OnInputCaptureConflict;
  Utils::Signal<void(const UI::RequestBindingUpdate&)>& OnRequestBindingUpdate;
  Utils::Signal<void(const UI::RequestDeleteBinding&)>& OnRequestDeleteBinding;

 private:
  // Private constructor, so only EventManager can create it
  EventDispatcher(EventManager& manager);
};

class EventManager {
 public:
  // Events available to external systems
  struct SystemEvents {
    Utils::Signal<void(const UI::ResizeEvent&)> OnWindowResize;

    // --- Plugin Lifecycle Events ---
    Utils::Signal<void(const OnPluginWillBeLoaded&)> OnPluginWillBeLoaded;
    Utils::Signal<void(const OnPluginDidLoad&)> OnPluginDidLoad;
    Utils::Signal<void(const OnPluginWillBeUnloaded&)> OnPluginWillBeUnloaded;

    // --- UI Events ---
    Utils::Signal<void(const UI::FocusComponentInSettingsWindow&)> OnFocusComponentInSettingsWindow;
    Utils::Signal<void(const UI::RequestSettingChange&)> OnRequestSettingChange;
    Utils::Signal<void(const UI::RequestPluginStateChange&)> OnRequestPluginStateChange;
    Utils::Signal<void(const UI::OnSettingWasChanged&)> OnSettingWasChanged;

    // --- Key Capture Events ---
    Utils::Signal<void(const UI::RequestInputCapture&)> OnRequestInputCapture;
    Utils::Signal<void(const UI::RequestInputCaptureCancel&)> OnRequestInputCaptureCancel;
    Utils::Signal<void(const Input::InputCaptured&)> OnInputCaptured;
    Utils::Signal<void(const Input::InputCaptureCancelled&)> OnInputCaptureCancelled;
    Utils::Signal<void(const Input::InputCaptureConflict&)> OnInputCaptureConflict;
    Utils::Signal<void(const UI::RequestBindingUpdate&)> OnRequestBindingUpdate;
    Utils::Signal<void(const UI::RequestDeleteBinding&)> OnRequestDeleteBinding;
    Utils::Signal<void(const UI::RequestExecuteCommand&)> OnRequestExecuteCommand;
    Utils::Signal<void(const UI::RequestUpdateCheck&)> OnRequestUpdateCheck;
    Utils::Signal<void(const UI::RequestPatronsFetch&)> OnRequestPatronsFetch;
    Utils::Signal<void(const System::OnRequestTrackUsage&)> OnRequestTrackUsage;

    // --- System Events (Completion/Notification) ---
    Utils::Signal<void(const System::OnUpdateCheckSucceeded&)> OnUpdateCheckSucceeded;
    Utils::Signal<void(const System::OnUpdateCheckFailed&)> OnUpdateCheckFailed;
    Utils::Signal<void(const System::OnPatronsFetchCompleted&)> OnPatronsFetchCompleted;

    // --- SCS Input Events ---
    Utils::Signal<void(const Input::InputDeviceActivityChanged&)> OnInputDeviceActivityChanged;

    // --- Binding Property Update Event ---
    Utils::Signal<void(const UI::RequestBindingPropertyUpdate&)> OnRequestBindingPropertyUpdate;

    // --- Config Events ---
    Utils::Signal<void(const Config::OnKeybindsModified&)> OnKeybindsModified;

    // --- Telemetry Events ---
    Utils::Signal<void()> OnTelemetryFrameStart;
    Utils::Signal<void()> OnGameWorldReady;
  };

 public:
  SystemEvents System;

 private:
  struct InternalEvents {
    // TODO: Add internal events if needed
  };

  InternalEvents m_internal;

  std::vector<std::unique_ptr<EventProxyBase>> m_proxies;

 public:
  EventManager() = default;
  ~EventManager() = default;

  void Init(Rendering::Renderer& renderer);

  // Method to get a dispatcher through which other systems subscribe to events
  EventDispatcher CreateEventDispatcher();
};
}  // namespace Events

SPF_NS_END
