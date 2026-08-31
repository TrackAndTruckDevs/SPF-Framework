#pragma once

#include "SPF/Events/EventProxyBase.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/Utils/Signal.hpp"

#include <memory>
#include <minwindef.h>
#include <windef.h>

namespace SPF::Events::Proxies {

class WndProcEventProxy : public EventProxyBase {
 public:
  WndProcEventProxy(EventManager& eventManager, Rendering::Renderer& renderer);
  ~WndProcEventProxy() override = default;

 private:
  void OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void SetBlockWndProc(bool block);

  std::shared_ptr<Logging::Logger> m_logger;
  Rendering::Renderer& m_renderer;

  // Sinks for different hooks
  Utils::Sink<void(HWND, UINT, WPARAM, LPARAM)> m_d3d11Sink;
  Utils::Sink<void(HWND, UINT, WPARAM, LPARAM)> m_d3d12Sink;
  Utils::Sink<void(HWND, UINT, WPARAM, LPARAM)> m_openGLSink;
};

}  // namespace SPF::Events::Proxies
