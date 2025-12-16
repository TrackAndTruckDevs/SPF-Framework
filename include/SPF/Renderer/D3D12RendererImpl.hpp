#pragma once

#include <memory>
#include <d3d12.h> // For ID3D12Device, ID3D12CommandQueue, ID3D12DescriptorHeap
#include <dxgi1_4.h> // For IDXGISwapChain3
#include <windows.h> // For UINT
#include <wrl/client.h> // For ComPtr

#include <vector>

#include <SPF/Renderer/RendererBase.hpp>
#include "SPF/Utils/Signal.hpp"

SPF_NS_BEGIN

// Use ComPtr for managing COM object lifetimes
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

// Forward declarations are already handled by the includes
namespace UI {
class UIManager;
}
namespace Logging { class Logger; } // Already included Logger.hpp

namespace Rendering {

/**
 * @class D3D12RendererImpl
 * @brief D3D12-specific implementation of the renderer.
 *
 * Handles ImGui initialization and rendering using the D3D12 backend.
 */
class D3D12RendererImpl : public RendererBase {
 public:
  D3D12RendererImpl(Renderer& renderer, UI::UIManager& uiManager);
  ~D3D12RendererImpl();

  void Init() override;
  void Shutdown() override;

 private:
  void OnD3D12Init(IDXGISwapChain3* swapChain, ID3D12Device* device, ID3D12CommandQueue* commandQueue);
  void OnD3D12Present(IDXGISwapChain3* swapChain);
  void OnD3D12BeforeResize(IDXGISwapChain3* swapChain, UINT width, UINT height);
  void OnD3D12AfterResize(IDXGISwapChain3* swapChain, UINT width, UINT height);

  UI::UIManager& m_uiManager;
  std::shared_ptr<Logging::Logger> m_logger;

  // D3D12-specific members
  ComPtr<ID3D12Device> m_pd3dDevice;
  ComPtr<ID3D12DescriptorHeap> m_pd3dSrvDescHeap;
  ComPtr<ID3D12DescriptorHeap> m_pd3dRtvDescHeap;
  ComPtr<ID3D12CommandQueue> m_pd3dCommandQueue;
  ComPtr<ID3D12GraphicsCommandList> m_commandList;
  ComPtr<ID3D12CommandAllocator> m_commandAllocator;

  // Render Target
  std::vector<ComPtr<ID3D12Resource>> m_mainRenderTargetResource;
  std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_mainRenderTargetDescriptors;

  // Synchronization
  ComPtr<ID3D12Fence> m_fence;
  UINT64 m_fenceLastSignaledValue = 0;
  HANDLE m_fenceEvent = nullptr;

  bool m_isImGuiInitialized = false;
  bool m_renderTargetsCreated = false;

  void CreateRenderTarget(IDXGISwapChain3* swapChain);
  void CleanupRenderTarget();
  void WaitForLastSubmittedFrame();

  // Sinks for D3D12Hook signals
  Utils::Sink<void(IDXGISwapChain3* swapChain, ID3D12Device* device, ID3D12CommandQueue* commandQueue)> m_onInitSink;
  Utils::Sink<void(IDXGISwapChain3* swapChain)> m_onPresentSink;
  Utils::Sink<void(IDXGISwapChain3* swapChain, UINT width, UINT height)> m_onBeforeResizeSink;
  Utils::Sink<void(IDXGISwapChain3* swapChain, UINT width, UINT height)> m_onAfterResizeSink;
};

}  // namespace Rendering

SPF_NS_END
