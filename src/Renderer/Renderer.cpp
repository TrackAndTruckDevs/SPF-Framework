#include <SPF/Renderer/Renderer.hpp>

#include <Windows.h>
#include <imgui.h>
#include <chrono>

#include <SPF/Core/Core.hpp>
#include <SPF/Events/EventManager.hpp>
#include <SPF/Logging/LoggerFactory.hpp>
#include <SPF/UI/UIManager.hpp>
#include <SPF/Modules/PerformanceMonitor.hpp>

// Implementations
#include <SPF/Renderer/D3D11RendererImpl.hpp>
#include <SPF/Renderer/D3D12RendererImpl.hpp>
#include "SPF/Renderer/OpenGLRendererImpl.hpp" // For the future

// Hooks for detection
#include <SPF/Hooks/D3D11Hook.hpp>
#include <SPF/Hooks/D3D12Hook.hpp>
#include <SPF/Hooks/OpenGLHook.hpp> // For the future

SPF_NS_BEGIN
namespace Rendering {
using namespace SPF::Logging;
using namespace SPF::UI;
using namespace SPF::Hooks;
using namespace SPF::Modules;

Renderer::Renderer(Core::Core& core, Events::EventManager& eventManager, UIManager& uiManager)
    : m_core(core), m_uiManager(uiManager), m_impl(nullptr), m_lastFrameTime(std::chrono::steady_clock::now()) {
  m_logger = LoggerFactory::GetInstance().GetLogger("Renderer");
  m_logger->Info("Constructing Renderer...");

  m_detectedAPI = DetectRenderAPI();
  m_logger->Info("Detected Render API: {}", static_cast<int>(m_detectedAPI));
}

Renderer::~Renderer() {
  m_logger->Info("Shutting down...");

  if (m_impl) {
    m_impl->Shutdown();
  }
  m_impl.reset();
}

RenderAPI Renderer::DetectRenderAPI() {
    m_logger->Info("Detecting game's graphics API using verification chain...");

    // Step 1: Check for an active OpenGL context. This is the most reliable check.
    HMODULE hOpenGL = GetModuleHandle(TEXT("opengl32.dll"));
    if (hOpenGL) {
        using wglGetCurrentContext_t = HGLRC(WINAPI*)();
        auto wglGetCurrentContext_ptr = reinterpret_cast<wglGetCurrentContext_t>(GetProcAddress(hOpenGL, "wglGetCurrentContext"));
        if (wglGetCurrentContext_ptr && wglGetCurrentContext_ptr() != NULL) {
            m_logger->Info("Active OpenGL context found. Selecting OpenGL.");
            return RenderAPI::OpenGL;
        }
        m_logger->Info("opengl32.dll is loaded, but no active context was found on this thread.");
    }

    // Step 2: Differentiate between DirectX versions if OpenGL is not active.
    m_logger->Info("No active OpenGL context found. Checking for DirectX versions...");

    // Check for D3D11 first
    HMODULE hD3D11 = GetModuleHandle(TEXT("d3d11.dll"));
    if (hD3D11 && GetProcAddress(hD3D11, "D3D11CreateDeviceAndSwapChain")) {
        m_logger->Info("Found d3d11.dll with required functions. Selecting D3D11.");
        return RenderAPI::D3D11;
    }    

    // Check for D3D12 if D3D11 is not detected.    
    HMODULE hD3D12 = GetModuleHandle(TEXT("d3d12.dll"));
    if (hD3D12 && GetProcAddress(hD3D12, "D3D12CreateDevice")) {
        m_logger->Info("Found d3d12.dll with required functions. Selecting D3D12.");
        return RenderAPI::D3D12;
    }

    m_logger->Warn("Could not detect any supported graphics API (OpenGL, D3D11, D3D12).");
    return RenderAPI::Unknown;
}

void Renderer::Init() {
  m_logger->Info("Initializing renderer implementation based on detected API...");
  switch (m_detectedAPI) {
    case RenderAPI::D3D11:
      if (D3D11Hook::IsInstalled()) {
        m_logger->Info("D3D11 hook is active. Creating D3D11 renderer implementation.");
        m_impl = std::move(std::make_unique<D3D11RendererImpl>(*this, m_uiManager));
      } else {
        m_logger->Error("D3D11 was detected, but D3D11Hook failed to install.");
      }
      break;
    case RenderAPI::D3D12:
       if (D3D12Hook::IsInstalled()) {
         m_logger->Info("D3D12 hook is active. Creating D3D12 renderer implementation.");
         m_impl = std::move(std::make_unique<D3D12RendererImpl>(*this, m_uiManager));
       } else {
         m_logger->Error("D3D12 was detected, but D3D12Hook failed to install.");
       }
      break;
    case RenderAPI::OpenGL:
      if (OpenGLHook::IsInstalled()) {
        m_logger->Info("OpenGL hook is active. Creating OpenGL renderer implementation.");
        m_impl = std::move(std::make_unique<OpenGLRendererImpl>(*this, m_uiManager));
      } else {
      m_logger->Warn("OpenGL was detected, but the OpenGL hook/renderer is not yet implemented.");
      }
      break;
    case RenderAPI::Unknown:
    default:
      m_logger->Critical("No renderer implementation could be created for the detected API.");
      break;
  }

  if (m_impl) {
    m_impl->Init();
  } else {
    m_logger->Error("Cannot initialize: no renderer implementation was created.");
  }
}

RenderAPI Renderer::GetDetectedAPI() const {
  return m_detectedAPI;
}

void Renderer::OnRendererInit() {
  m_logger->Info("Implementation initialized. Signaling Core for late-init...");
  m_core.LateInit();
}

void Renderer::OnRendererRenderImGui() {
  // --- Update Performance Monitor ---
  auto currentTime = std::chrono::steady_clock::now();
  std::chrono::duration<float> dt_duration = currentTime - m_lastFrameTime;
  m_lastFrameTime = currentTime;
  PerformanceMonitor::GetInstance().Update(dt_duration.count());

  m_core.Update();
  m_core.ImGuiRender();
}

}  // namespace Rendering
SPF_NS_END
