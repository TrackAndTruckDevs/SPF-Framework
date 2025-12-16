#pragma once

#include <memory>
#include <d3d11.h>
#include <windows.h>
#include <wrl/client.h> // For ComPtr

#include "SPF/Renderer/RendererBase.hpp"
#include "SPF/Utils/Signal.hpp"
#include "SPF/Namespace.hpp"


SPF_NS_BEGIN

// Use a template alias for ComPtr for consistency with the D3D12 implementation.
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace Logging { class Logger; }
namespace UI { class UIManager; }

namespace Rendering {

class Renderer;

class D3D11RendererImpl : public RendererBase {
 public:
  D3D11RendererImpl(Renderer& renderer, UI::UIManager& uiManager);
  ~D3D11RendererImpl() override;

  void Init() override;
  void Shutdown() override;

 private:
  // --- Hook Callbacks ---
  void OnInit(IDXGISwapChain* swapChain, ID3D11Device* device);
  void OnPresent(IDXGISwapChain* swapChain);
  void OnBeforeResize(IDXGISwapChain* swapChain, UINT width, UINT height);
  void OnResize(IDXGISwapChain* swapChain, UINT width, UINT height);

  // --- Helper Methods ---
  void CreateRenderTarget();
  void CleanupRenderTarget();

  // --- Dependencies & State ---
  UI::UIManager& m_uiManager;
  std::shared_ptr<Logging::Logger> m_logger;
  HWND m_hWnd = nullptr;

  // --- D3D11 Core Objects ---
  // Using ComPtr for automatic reference counting and resource management.
  ComPtr<IDXGISwapChain> m_swapChain;
  ComPtr<ID3D11Device> m_device;
  ComPtr<ID3D11DeviceContext> m_context;
  ComPtr<ID3D11RenderTargetView> m_mainRenderTargetView;

  // --- Sinks for Hook Signals ---
  Utils::Sink<void(IDXGISwapChain*, ID3D11Device*)> m_onInitSink;
  Utils::Sink<void(IDXGISwapChain*)> m_onPresentSink;
  Utils::Sink<void(IDXGISwapChain*, UINT, UINT)> m_onBeforeResizeSink;
  Utils::Sink<void(IDXGISwapChain*, UINT, UINT)> m_onResizeSink;
};

}  // namespace Rendering

SPF_NS_END