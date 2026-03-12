#include <SPF/Renderer/D3D11RendererImpl.hpp>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <SPF/Hooks/D3D11Hook.hpp>
#include <SPF/Logging/LoggerFactory.hpp>
#include <SPF/Renderer/Renderer.hpp>
#include <SPF/UI/UIManager.hpp>

SPF_NS_BEGIN

namespace Rendering {
using namespace SPF::Logging;
using namespace SPF::Hooks;
using namespace SPF::UI;

namespace {

class D3D11Texture : public ITexture {
public:
    D3D11Texture(ComPtr<ID3D11ShaderResourceView> srv, uint32_t width, uint32_t height)
        : m_srv(srv), m_width(width), m_height(height) {}

    void* GetHandle() const override { return static_cast<void*>(m_srv.Get()); }
    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }

private:
    ComPtr<ID3D11ShaderResourceView> m_srv;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace

D3D11RendererImpl::D3D11RendererImpl(Renderer& renderer, UI::UIManager& uiManager)
    : RendererBase(renderer),
      m_uiManager(uiManager),
      m_onInitSink(D3D11Hook::OnInit),
      m_onPresentSink(D3D11Hook::OnPresent),
      m_onBeforeResizeSink(D3D11Hook::OnBeforeResize),
      m_onResizeSink(D3D11Hook::OnResize) {
  m_logger = Logging::LoggerFactory::GetInstance().GetLogger("D3D11Impl");
  m_logger->Info("D3D11 Renderer Implementation created.");
}

D3D11RendererImpl::~D3D11RendererImpl() {
  Shutdown();
}

void D3D11RendererImpl::Init() {
  m_logger->Info("Connecting to D3D11Hook signals...");
  m_onInitSink.Connect<&D3D11RendererImpl::OnInit>(this);
  m_onPresentSink.Connect<&D3D11RendererImpl::OnPresent>(this);
  m_onBeforeResizeSink.Connect<&D3D11RendererImpl::OnBeforeResize>(this);
  m_onResizeSink.Connect<&D3D11RendererImpl::OnResize>(this);
}

void D3D11RendererImpl::Shutdown() {
  if (m_device == nullptr) {
    return;
  }
  m_logger->Info("Shutting down ImGui D3D11 implementation...");
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();

  // Release all COM pointers. ComPtr's Reset() is equivalent to releasing the pointer.
  m_mainRenderTargetView.Reset();
  m_context.Reset();
  m_device.Reset();
  m_swapChain.Reset();
  m_logger->Info("ImGui D3D11 implementation shut down.");
}

std::unique_ptr<ITexture> D3D11RendererImpl::CreateTextureFromMemory(const unsigned char* data, size_t size) {
    if (!m_device) {
        m_logger->Error("CreateTextureFromMemory failed: D3D11 device is not initialized.");
        return nullptr;
    }

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 4);
    if (!pixels) {
        m_logger->Error("stbi_load_from_memory failed: {}", stbi_failure_reason());
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = m_device->CreateTexture2D(&desc, &subResource, texture.GetAddressOf());
    stbi_image_free(pixels);

    if (FAILED(hr)) {
        m_logger->Error("Failed to create D3D11 texture. HRESULT: {:#x}", static_cast<unsigned int>(hr));
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;

    ComPtr<ID3D11ShaderResourceView> srv;
    hr = m_device->CreateShaderResourceView(texture.Get(), &srvDesc, srv.GetAddressOf());
    if (FAILED(hr)) {
        m_logger->Error("Failed to create D3D11 SRV. HRESULT: {:#x}", static_cast<unsigned int>(hr));
        return nullptr;
    }

    return std::make_unique<D3D11Texture>(srv, width, height);
}

void D3D11RendererImpl::RefreshFontAtlas() {
    if (m_device) {
        ImGui_ImplDX11_InvalidateDeviceObjects();
        ImGui_ImplDX11_CreateDeviceObjects();
    }
}

void D3D11RendererImpl::OnInit(IDXGISwapChain* swapChain, ID3D11Device* device) {
  m_logger->Info("OnInit signal received. Initializing ImGui D3D11 backend...");
  m_logger->Info("SwapChain: {0:p}, Device: {0:p}", static_cast<void*>(swapChain), static_cast<void*>(device));

  // Store the D3D11 objects provided by the hook.
  // ComPtr will handle reference counting.
  m_swapChain = swapChain;
  m_device = device;
  m_device->GetImmediateContext(m_context.GetAddressOf());
  m_hWnd = D3D11Hook::MainWindow;

  CreateRenderTarget();

  // Initialize ImGui backends.
  ImGui_ImplWin32_Init(m_hWnd);
  ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());

  m_renderer.OnRendererInit();
  m_logger->Info("ImGui D3D11 implementation initialized successfully.");
}

void D3D11RendererImpl::OnPresent(IDXGISwapChain* swapChain) {
  if (m_mainRenderTargetView == nullptr) {
    return;
  }

  // Start a new ImGui frame.
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  // Set our render target on the device context. All ImGui draw calls will render to this target.
  m_context->OMSetRenderTargets(1, m_mainRenderTargetView.GetAddressOf(), nullptr);

  // Ask the UIManager to render all registered windows.
  m_renderer.OnRendererRenderImGui();

  // Render the ImGui draw data.
  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void D3D11RendererImpl::OnBeforeResize(IDXGISwapChain* swapChain, UINT width, UINT height) {
  m_logger->Debug("OnBeforeResize received. Releasing render target before game resizes buffers.");
  // We must release our reference to the back buffer before the swap chain can be resized.
  CleanupRenderTarget();
}

void D3D11RendererImpl::OnResize(IDXGISwapChain* swapChain, UINT width, UINT height) {
  m_logger->Debug("OnResize received. Re-creating render target for new size {}x{}.", width, height);
  // The game has resized the swap chain, so we can now create a new render target view.
  CreateRenderTarget();
}

void D3D11RendererImpl::CreateRenderTarget() {
  CleanupRenderTarget();

  ComPtr<ID3D11Texture2D> backBuffer;
  HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer);
  if (FAILED(hr)) {
    m_logger->Error("Failed to get swap chain buffer. HRESULT: {:#x}", static_cast<unsigned int>(hr));
    return;
  }

  hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_mainRenderTargetView.GetAddressOf());
  if (FAILED(hr)) {
    m_logger->Error("Failed to create render target view. HRESULT: {:#x}", static_cast<unsigned int>(hr));
  } else {
    m_logger->Info("Render target created successfully.");
  }
}

void D3D11RendererImpl::CleanupRenderTarget() {
  if (m_mainRenderTargetView != nullptr) {
    // Unbind the render target before releasing it.
    ID3D11RenderTargetView* nullViews[] = {nullptr};
    m_context->OMSetRenderTargets(1, nullViews, nullptr);
    m_mainRenderTargetView.Reset();
    m_logger->Info("Render target cleaned up successfully.");
  }
}

}  // namespace Rendering

SPF_NS_END
