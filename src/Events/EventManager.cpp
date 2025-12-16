#include "SPF/Events/EventManager.hpp"

#include "SPF/Events/Proxies/WndProcEventProxy.hpp"
#include "SPF/Renderer/Renderer.hpp"

SPF_NS_BEGIN

namespace Events {
// --- EventDispatcher Implementation ---
EventDispatcher::EventDispatcher(EventManager& manager)
    : OnWindowResize(manager.System.OnWindowResize),
      OnPluginWillBeLoaded(manager.System.OnPluginWillBeLoaded),
      OnPluginDidLoad(manager.System.OnPluginDidLoad),
      OnPluginWillBeUnloaded(manager.System.OnPluginWillBeUnloaded),
      OnFocusComponentInSettingsWindow(manager.System.OnFocusComponentInSettingsWindow),
      OnRequestSettingChange(manager.System.OnRequestSettingChange),
      OnRequestPluginStateChange(manager.System.OnRequestPluginStateChange),
      OnSettingWasChanged(manager.System.OnSettingWasChanged),
      OnRequestInputCapture(manager.System.OnRequestInputCapture),
      OnInputCaptured(manager.System.OnInputCaptured),
      OnInputCaptureCancelled(manager.System.OnInputCaptureCancelled),
      OnInputCaptureConflict(manager.System.OnInputCaptureConflict),
      OnRequestBindingUpdate(manager.System.OnRequestBindingUpdate),
      OnRequestDeleteBinding(manager.System.OnRequestDeleteBinding) {}

// --- EventManager Implementation ---
void EventManager::Init(Rendering::Renderer& renderer) { m_proxies.emplace_back(std::make_unique<Proxies::WndProcEventProxy>(*this, renderer)); }

EventDispatcher EventManager::CreateEventDispatcher() { return EventDispatcher(*this); }
}  // namespace Events

SPF_NS_END
