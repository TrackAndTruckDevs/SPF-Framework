#pragma once

#include <Windows.h>
#include <memory>

#include "SPF/Events/EventProxyBase.hpp"
#include "SPF/Namespace.hpp"
#include "SPF/Utils/Signal.hpp"

// Forward-declarations
SPF_NS_BEGIN
namespace Rendering {
class Renderer;
}
namespace Logging {
class Logger;
}
namespace Events {
class EventManager;
}
SPF_NS_END

SPF_NS_BEGIN

namespace Events::Proxies {

class WndProcEventProxy : public EventProxyBase {
 public:
  WndProcEventProxy(EventManager& eventManager, Rendering::Renderer& renderer);
  ~WndProcEventProxy() override = default;

 private:
  void OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  std::shared_ptr<Logging::Logger> m_logger;
  Rendering::Renderer& m_renderer;

  // Sinks for different hooks
  Utils::Sink<void(HWND, UINT, WPARAM, LPARAM)> m_d3d11Sink;
  Utils::Sink<void(HWND, UINT, WPARAM, LPARAM)> m_d3d12Sink;
  Utils::Sink<void(HWND, UINT, WPARAM, LPARAM)> m_openGLSink;
};

}  // namespace Events::Proxies

SPF_NS_END
