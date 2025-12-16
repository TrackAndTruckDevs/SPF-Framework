#include "SPF/Events/Proxies/WndProcEventProxy.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>

#include "SPF/Events/EventManager.hpp"
#include "SPF/Hooks/D3D11Hook.hpp"
#include "SPF/Hooks/D3D12Hook.hpp"
#include "SPF/Hooks/OpenGLHook.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/Renderer/RenderAPI.hpp"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

SPF_NS_BEGIN

namespace Events::Proxies {
using namespace SPF::Logging;
using namespace SPF::Rendering;

WndProcEventProxy::WndProcEventProxy(EventManager& eventManager, Renderer& renderer) 
    : EventProxyBase(eventManager), 
      m_renderer(renderer),
      m_d3d11Sink(Hooks::D3D11Hook::OnWndProc),
      m_d3d12Sink(Hooks::D3D12Hook::OnWndProc),
      m_openGLSink(Hooks::OpenGLHook::OnWndProc)
{
    m_logger = LoggerFactory::GetInstance().GetLogger("WndProcEventProxy");

    RenderAPI api = m_renderer.GetDetectedAPI();
    switch (api) {
        case RenderAPI::D3D11:
            m_d3d11Sink.Connect<&WndProcEventProxy::OnWndProc>(this);
            m_logger->Info("Proxy created and connected to D3D11Hook::OnWndProc.");
            break;
        case RenderAPI::D3D12:
            m_d3d12Sink.Connect<&WndProcEventProxy::OnWndProc>(this);
            m_logger->Info("Proxy created and connected to D3D12Hook::OnWndProc.");
            break;
        case RenderAPI::OpenGL:
             m_openGLSink.Connect<&WndProcEventProxy::OnWndProc>(this);
            m_logger->Info("Proxy created and connected to OpenGLHook::OnWndProc.");
            break;
        default:
            m_logger->Warn("WndProcEventProxy created, but no compatible graphics hook was detected to connect to.");
            break;
    }
}

void WndProcEventProxy::OnWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  // Let ImGui have the first chance to process the message.
  if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
    // If ImGui consumes the message, block it from the game and stop further processing here.
     RenderAPI api = m_renderer.GetDetectedAPI();
    switch (api) {
        case RenderAPI::D3D11:  SPF::Hooks::D3D11Hook::block_wndproc_message = true; break;
        case RenderAPI::D3D12:  SPF::Hooks::D3D12Hook::block_wndproc_message = true; break;
        case RenderAPI::OpenGL: SPF::Hooks::OpenGLHook::block_wndproc_message = true; break;
        default: break;
    }
    return;
  }

  bool blockMessage = false;

  // If ImGui did NOT consume the event, we can process it for our own framework's logic (e.g., keybinds).
  switch (uMsg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
      auto& keyMapper = SPF::System::VirtualKeyMapping::GetInstance();
      bool isPressed = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);
      SPF::Input::KeyboardEvent event{keyMapper.FromWinAPI(wParam), isPressed};

      // Publish the event to our InputManager. If it returns true, it means the event
      // was consumed (e.g., by a keybind) and should be blocked from the game.
      if (SPF::Input::InputManager::GetInstance().PublishKeyboardEvent(event)) {
        blockMessage = true;
      }
      break;
    }

    // --- Mouse Button Messages ---
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP: {
      auto& inputManager = SPF::Input::InputManager::GetInstance();
      int button = -1;
      if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP) button = 0;
      if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP) button = 1;
      if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP) button = 2;
      if (uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP) button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;

      if (button != -1) {
        bool isPressed = (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN);
        if (inputManager.PublishMouseButton({button, isPressed})) {
          blockMessage = true;
        }
      }
      break;
    }

    // --- Mouse Wheel Message ---
    case WM_MOUSEWHEEL: {
      auto& inputManager = SPF::Input::InputManager::GetInstance();
      float wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
      if (inputManager.PublishMouseWheel({wheelDelta})) {
        blockMessage = true;
      }
      break;
    }

    // Handle non-input messages we care about.
    case WM_SIZE: {
      UI::ResizeEvent event{.width = LOWORD(lParam), .height = HIWORD(lParam)};
      m_logger->Trace("WM_SIZE detected. Calling OnWindowResize with {}x{}", event.width, event.height);
      m_eventManager.System.OnWindowResize.Call(event);
      break;
    }
  }

  if (blockMessage) {
    RenderAPI api = m_renderer.GetDetectedAPI();
    switch (api) {
        case RenderAPI::D3D11:  SPF::Hooks::D3D11Hook::block_wndproc_message = true; break;
        case RenderAPI::D3D12:  SPF::Hooks::D3D12Hook::block_wndproc_message = true; break;
        case RenderAPI::OpenGL: SPF::Hooks::OpenGLHook::block_wndproc_message = true; break;
        default: break;
    }
  }
}
}  // namespace Events::Proxies

SPF_NS_END
