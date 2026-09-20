#include "SPF/Renderer/Renderer.hpp"

#include "SPF/Core/Core.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Hooks/DXGIHook.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/PerformanceMonitor.hpp"
#include "SPF/Renderer/D3D11RendererImpl.hpp"
#include "SPF/Renderer/D3D12RendererImpl.hpp"
#include "SPF/Renderer/ITexture.hpp"
#include "SPF/Renderer/OpenGLRendererImpl.hpp"
#include "SPF/Renderer/RenderAPI.hpp"
#include "SPF/UI/UIManager.hpp"

#include <chrono>
#include <cstddef>
#include <libloaderapi.h>
#include <memory>
#include <minwindef.h>
#include <windef.h>
#include <winnt.h>

namespace SPF::Rendering {
using namespace SPF::Logging;
using namespace SPF::UI;
using namespace SPF::Hooks;
using namespace SPF::Modules;

Renderer::Renderer(Core::Core& core, Events::EventManager& eventManager, UIManager& uiManager)
    : m_core(core), m_uiManager(uiManager), m_impl(nullptr), m_lastFrameTime(std::chrono::steady_clock::now()), m_onAPIDetectedSink(Hooks::DXGIHook::OnAPIDetected) {
  m_logger = LoggerFactory::GetInstance().GetLogger("Renderer");
  m_logger->Info("Constructing Renderer...");
}

Renderer::~Renderer() {
  m_logger->Info("Shutting down...");

  if (m_impl) {
    m_impl->Shutdown();
  }
  m_impl.reset();
}

void Renderer::Init() {
  m_logger->Info("Connecting to DXGIHook::OnAPIDetected for deferred API detection...");
  m_onAPIDetectedSink.Connect<&Renderer::OnAPIDetected>(this);
}

void Renderer::OnAPIDetected(Rendering::RenderAPI api) {
  m_logger->Info("API detected by DXGIHook: {}. Creating renderer implementation...", static_cast<int>(api));
  m_detectedAPI = api;

  switch (api) {
    case RenderAPI::D3D11:
      m_logger->Info("Creating D3D11 renderer implementation.");
      m_impl = std::make_unique<D3D11RendererImpl>(*this, m_uiManager);
      break;
    case RenderAPI::D3D12:
      m_logger->Info("Creating D3D12 renderer implementation.");
      m_impl = std::make_unique<D3D12RendererImpl>(*this, m_uiManager);
      break;
    case RenderAPI::OpenGL:
      m_logger->Info("Creating OpenGL renderer implementation.");
      m_impl = std::make_unique<OpenGLRendererImpl>(*this, m_uiManager);
      break;
    case RenderAPI::Unknown:
    default:
      m_logger->Critical("Unknown API detected. No renderer implementation created.");
      return;
  }

  if (m_impl) {
    m_impl->Init();
  } else {
    m_logger->Error("Cannot initialize: renderer implementation creation failed.");
  }
}

RenderAPI Renderer::GetDetectedAPI() const { return m_detectedAPI; }

std::unique_ptr<ITexture> Renderer::CreateTextureFromMemory(const unsigned char* data, size_t size) {
  if (m_impl) {
    return m_impl->CreateTextureFromMemory(data, size);
  }
  return nullptr;
}

void Renderer::RefreshFontAtlas() {
  if (m_impl) {
    m_impl->RefreshFontAtlas();
  }
}

void Renderer::OnRendererInit() {
  m_logger->Info("Implementation initialized. Signaling Core for late-init...");
  m_core.LateInit();
}

void Renderer::OnRendererUpdate() {
  // --- Update Performance Monitor ---
  auto currentTime = std::chrono::steady_clock::now();
  std::chrono::duration<float> dt_duration = currentTime - m_lastFrameTime;
  m_lastFrameTime = currentTime;
  PerformanceMonitor::GetInstance().Update(dt_duration.count());

  // Perform core updates (including font processing in UIManager)
  // before the ImGui frame starts.
  m_core.Update();
}

void Renderer::OnRendererRenderImGui() {
  // Render UI windows
  m_core.ImGuiRender();
}

}  // namespace SPF::Rendering
