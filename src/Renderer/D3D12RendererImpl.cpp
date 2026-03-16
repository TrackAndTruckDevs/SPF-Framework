#include <SPF/Renderer/D3D12RendererImpl.hpp>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <stb_image.h>

#include <SPF/Hooks/D3D12Hook.hpp>
#include <SPF/Logging/LoggerFactory.hpp>
#include <SPF/UI/UIManager.hpp>
#include <SPF/Renderer/Renderer.hpp>

SPF_NS_BEGIN
namespace Rendering {

using namespace SPF::Logging;
using namespace SPF::Hooks;

namespace {

class D3D12Texture : public ITexture {
public:
    D3D12Texture(ComPtr<ID3D12Resource> resource, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, uint32_t width, uint32_t height)
        : m_resource(resource), m_gpuHandle(gpuHandle), m_width(width), m_height(height) {}

    void* GetHandle() const override { return reinterpret_cast<void*>(m_gpuHandle.ptr); }
    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }

private:
    ComPtr<ID3D12Resource> m_resource;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace

D3D12RendererImpl::D3D12RendererImpl(Renderer& renderer, UI::UIManager& uiManager)
    : RendererBase(renderer),
      m_uiManager(uiManager),
      m_onInitSink(D3D12Hook::OnInit),
      m_onPresentSink(D3D12Hook::OnPresent),
      m_onBeforeResizeSink(D3D12Hook::OnBeforeResize),
      m_onAfterResizeSink(D3D12Hook::OnAfterResize),
      m_renderTargetsCreated(false)
{
    m_logger = LoggerFactory::GetInstance().GetLogger("D3D12Impl");
    m_logger->Info("D3D12 Renderer Implementation created.");
}

D3D12RendererImpl::~D3D12RendererImpl() {
    Shutdown();
}

void D3D12RendererImpl::Init() {
    m_logger->Info("Initializing ImGui for D3D12 and connecting to D3D12Hook signals...");
    m_onInitSink.Connect<&D3D12RendererImpl::OnD3D12Init>(this);
    m_onPresentSink.Connect<&D3D12RendererImpl::OnD3D12Present>(this);
    m_onBeforeResizeSink.Connect<&D3D12RendererImpl::OnD3D12BeforeResize>(this);
    m_onAfterResizeSink.Connect<&D3D12RendererImpl::OnD3D12AfterResize>(this);
}

void D3D12RendererImpl::Shutdown() {
    if (!m_isImGuiInitialized) {
        return;
    }
    m_logger->Info("Shutting down ImGui D3D12 implementation...");

    WaitForLastSubmittedFrame();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();

    CleanupRenderTarget();

    // ComPtrs will auto-release, just reset them to be sure
    m_pd3dCommandQueue.Reset();
    m_commandAllocator.Reset();
    m_commandList.Reset();
    m_pd3dSrvDescHeap.Reset();
    m_pd3dRtvDescHeap.Reset();
    m_fence.Reset();
    if (m_fenceEvent) { 
        CloseHandle(m_fenceEvent); 
        m_fenceEvent = nullptr; 
    }

    m_pd3dDevice.Reset();
    m_isImGuiInitialized = false;
    m_renderTargetsCreated = false;
    m_logger->Info("ImGui D3D12 implementation shut down.");
}

std::unique_ptr<ITexture> D3D12RendererImpl::CreateTextureFromMemory(const unsigned char* data, size_t size) {
    if (!m_pd3dDevice || !m_assetCommandList || !m_assetCommandAllocator) return nullptr;

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 4);
    if (!pixels) {
        m_logger->Error("D3D12 CreateTexture: stbi_load failed: {}", stbi_failure_reason());
        return nullptr;
    }

    // 1. Create the texture resource (Default Heap)
    D3D12_RESOURCE_DESC txtDesc = {};
    txtDesc.MipLevels = 1;
    txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    txtDesc.Width = width;
    txtDesc.Height = height;
    txtDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    txtDesc.DepthOrArraySize = 1;
    txtDesc.SampleDesc.Count = 1;
    txtDesc.SampleDesc.Quality = 0;
    txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ComPtr<ID3D12Resource> texture;
    D3D12_HEAP_PROPERTIES defaultHeapProps = {};
    defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_pd3dDevice->CreateCommittedResource(
        &defaultHeapProps, D3D12_HEAP_FLAG_NONE, &txtDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(texture.GetAddressOf()));

    if (FAILED(hr)) {
        stbi_image_free(pixels);
        m_logger->Error("D3D12: Failed to create texture resource (HRESULT: {:#x})", (unsigned int)hr);
        return nullptr;
    }

    // 2. Create upload buffer manually
    UINT rowPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UINT64 uploadBufferSize = static_cast<UINT64>(rowPitch) * height;

    ComPtr<ID3D12Resource> uploadBuffer;
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = uploadBufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = m_pd3dDevice->CreateCommittedResource(
        &uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf()));

    if (FAILED(hr)) {
        stbi_image_free(pixels);
        m_logger->Error("D3D12: Failed to create upload buffer (HRESULT: {:#x})", (unsigned int)hr);
        return nullptr;
    }

    // 3. Copy data to upload buffer
    void* mappedData = nullptr;
    if (SUCCEEDED(uploadBuffer->Map(0, nullptr, &mappedData))) {
        for (int y = 0; y < height; ++y) {
            memcpy(reinterpret_cast<unsigned char*>(mappedData) + (y * rowPitch), pixels + (y * width * 4), width * 4);
        }
        uploadBuffer->Unmap(0, nullptr);
    }
    stbi_image_free(pixels);

    // 4. Record and execute copy commands using the DEDICATED asset command list
    {
        std::lock_guard<std::mutex> lock(m_descHeapMutex);
        m_assetCommandAllocator->Reset();
        m_assetCommandList->Reset(m_assetCommandAllocator.Get(), nullptr);

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadBuffer.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.Width = width;
        src.PlacedFootprint.Footprint.Height = height;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.RowPitch = rowPitch;

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = texture.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        m_assetCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_assetCommandList->ResourceBarrier(1, &barrier);

        m_assetCommandList->Close();
        ID3D12CommandList* ppCommandLists[] = { m_assetCommandList.Get() };
        m_pd3dCommandQueue->ExecuteCommandLists(1, ppCommandLists);

        // Synchronize: Wait for this specific operation to complete
        UINT64 waitValue = ++m_fenceLastSignaledValue;
        m_pd3dCommandQueue->Signal(m_fence.Get(), waitValue);
        
        if (m_fence->GetCompletedValue() < waitValue) {
            m_fence->SetEventOnCompletion(waitValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    // 5. Create SRV
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGpu = {};
    {
        std::lock_guard<std::mutex> lock(m_descHeapMutex);
        if (m_nextSrvIndex >= MAX_SRV_DESCRIPTORS) {
            m_logger->Error("D3D12: SRV Descriptor Heap exhausted!");
            return nullptr;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCpu = m_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
        srvHandleCpu.ptr += (m_nextSrvIndex * m_srvDescriptorSize);

        srvHandleGpu = m_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
        srvHandleGpu.ptr += (m_nextSrvIndex * m_srvDescriptorSize);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        m_pd3dDevice->CreateShaderResourceView(texture.Get(), &srvDesc, srvHandleCpu);
        m_nextSrvIndex++;
    }

    return std::make_unique<D3D12Texture>(texture, srvHandleGpu, width, height);
}

void D3D12RendererImpl::RefreshFontAtlas() {
    if (m_pd3dDevice) {
        ImGui_ImplDX12_InvalidateDeviceObjects();
        ImGui_ImplDX12_CreateDeviceObjects();
    }
}

void D3D12RendererImpl::OnD3D12Init(IDXGISwapChain3* swapChain, ID3D12Device* device, ID3D12CommandQueue* commandQueue) {
    if (m_isImGuiInitialized) {
        return;
    }
    m_logger->Info("D3D12Hook OnInit received. Initializing ImGui D3D12 backend...");
    
    // Store the essential D3D12 objects provided by the hook.
    // We use ComPtr for automatic reference management, which is safer than manual Release().
    m_pd3dDevice = device;
    m_pd3dCommandQueue = commandQueue;

    HRESULT hr;
    DXGI_SWAP_CHAIN_DESC desc;
    hr = swapChain->GetDesc(&desc);
    if (FAILED(hr)) {
        m_logger->Critical("OnD3D12Init: swapChain->GetDesc failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }

    // Create a descriptor heap for the ImGui texture atlas and additional textures.
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = MAX_SRV_DESCRIPTORS;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_pd3dSrvDescHeap.GetAddressOf()));
    if (FAILED(hr)) {
        m_logger->Critical("OnD3D12Init: CreateDescriptorHeap (SRV) failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }

    m_srvDescriptorSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create a command allocator and a command list for our UI rendering.
    hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.GetAddressOf()));
    if (FAILED(hr)) {
        m_logger->Critical("OnD3D12Init: CreateCommandAllocator failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }
    hr = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), NULL, IID_PPV_ARGS(m_commandList.GetAddressOf()));
    if (FAILED(hr)) {
        m_logger->Critical("OnD3D12Init: CreateCommandList failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }
    hr = m_commandList->Close();
    if (FAILED(hr)) {
        m_logger->Critical("OnD3D12Init: commandList->Close failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }

    // NEW: Create dedicated command objects for ASSET LOADING (Texture creation etc)
    hr = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_assetCommandAllocator.GetAddressOf()));
    if (SUCCEEDED(hr)) {
        hr = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_assetCommandAllocator.Get(), NULL, IID_PPV_ARGS(m_assetCommandList.GetAddressOf()));
        if (SUCCEEDED(hr)) {
            m_assetCommandList->Close();
        } else {
            m_logger->Error("D3D12 Init: Failed to create asset command list.");
        }
    } else {
        m_logger->Error("D3D12 Init: Failed to create asset command allocator.");
    }

    // Create a fence for GPU-CPU synchronization.
    hr = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
     if (FAILED(hr)) {
        m_logger->Critical("OnD3D12Init: CreateFence failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }

    m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_fenceEvent == nullptr) {
        m_logger->Critical("OnD3D12Init: CreateEvent for fence failed.");
        return;
    }

    // Initialize the ImGui backends for Win32 and D3D12.
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = m_pd3dDevice.Get();
    initInfo.CommandQueue = m_pd3dCommandQueue.Get();
    initInfo.NumFramesInFlight = desc.BufferCount;
    initInfo.RTVFormat = desc.BufferDesc.Format;
    initInfo.SrvDescriptorHeap = m_pd3dSrvDescHeap.Get();
    initInfo.LegacySingleSrvCpuDescriptor = m_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
    initInfo.LegacySingleSrvGpuDescriptor = m_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
    
    if (ImGui_ImplWin32_Init(desc.OutputWindow) && ImGui_ImplDX12_Init(&initInfo)) {
        m_logger->Info("ImGui D3D12 backend initialized successfully.");
        ImGui_ImplDX12_CreateDeviceObjects();
        m_renderer.OnRendererInit(); // Signal to Core that we are ready for late-init tasks.
        m_isImGuiInitialized = true;
    } else {
        m_logger->Critical("Failed to initialize ImGui D3D12 backends.");
    }
}

void D3D12RendererImpl::OnD3D12Present(IDXGISwapChain3* swapChain) {
    if (!m_isImGuiInitialized || !m_pd3dCommandQueue) {
        return;
    }

    // Render targets are created here on the first Present call after initialization,
    // or after a resize event. This ensures they are always valid.
    if (!m_renderTargetsCreated) {
        CreateRenderTarget(swapChain);
        if (!m_renderTargetsCreated) {
             m_logger->Error("Render targets are not created, skipping frame.");
            return;
        }
    }

    // Ensure the GPU has finished with the command allocator before we reset and use it again.
    WaitForLastSubmittedFrame();

    // Ask the UIManager to update state (fonts, plugins, etc.) before the ImGui frame starts.
    m_renderer.OnRendererUpdate();

    // Start a new ImGui frame.
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Allow the UIManager to render all registered windows.
    m_renderer.OnRendererRenderImGui();

    // Record ImGui rendering commands into our command list.
    HRESULT hr;
    UINT backBufferIdx = swapChain->GetCurrentBackBufferIndex();
    
    hr = m_commandAllocator->Reset();
    if (FAILED(hr)) { m_logger->Error("OnD3D12Present: CommandAllocator->Reset failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr)); return; }
    
    hr = m_commandList->Reset(m_commandAllocator.Get(), NULL);
    if (FAILED(hr)) { m_logger->Error("OnD3D12Present: CommandList->Reset failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr)); return; }

    // Transition the back buffer from a "present" state to a "render target" state.
    // This is a requirement of D3D12 to ensure resources are in the correct state for a given operation.
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_mainRenderTargetResource[backBufferIdx].Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    m_commandList->ResourceBarrier(1, &barrier);

    // Set the back buffer as the render target for our command list.
    m_commandList->OMSetRenderTargets(1, &m_mainRenderTargetDescriptors[backBufferIdx], FALSE, NULL);
    ID3D12DescriptorHeap* heaps[] = { m_pd3dSrvDescHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
    
    // Render the ImGui draw data.
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

    // Transition the back buffer back to the "present" state, ready to be displayed.
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    m_commandList->ResourceBarrier(1, &barrier);

    hr = m_commandList->Close();
     if (FAILED(hr)) { m_logger->Error("OnD3D12Present: CommandList->Close failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr)); return; }
    
    // Execute the command list on the game's command queue.
    ID3D12CommandList* const commandLists[] = { m_commandList.Get() };
    m_pd3dCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // Signal the fence to mark that this frame's commands have been submitted.
    {
        std::lock_guard<std::mutex> lock(m_descHeapMutex);
        UINT64 fenceValue = ++m_fenceLastSignaledValue;
        hr = m_pd3dCommandQueue->Signal(m_fence.Get(), fenceValue);
        if (FAILED(hr)) { 
            m_logger->Error("OnD3D12Present: CommandQueue->Signal failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr)); 
            return; 
        }
    }
}

void D3D12RendererImpl::OnD3D12BeforeResize(IDXGISwapChain3* swapChain, UINT width, UINT height) {
    if (m_isImGuiInitialized) {
        m_logger->Info("D3D12 OnBeforeResize received. Invalidating render targets before recreation.");
        // We must wait for the GPU to be idle and then release our references to the
        // swap chain's back buffers before the game can resize them.
        WaitForLastSubmittedFrame();
        CleanupRenderTarget();
        m_renderTargetsCreated = false;
    }
}

void D3D12RendererImpl::OnD3D12AfterResize(IDXGISwapChain3* swapChain, UINT width, UINT height) {
    if (m_isImGuiInitialized) {
        m_logger->Info("D3D12 OnAfterResize received. Re-creating render targets.");
        // The game has resized the swap chain. We can now re-create our render target views
        // pointing to the new back buffers. This is done lazily on the next Present call.
        // For now, we just make sure ImGui's device objects are recreated.
        ImGui_ImplDX12_CreateDeviceObjects();
    }
}

void D3D12RendererImpl::CreateRenderTarget(IDXGISwapChain3* swapChain) {
    m_logger->Debug("Creating render target views for the D3D12 swap chain back buffers...");
    HRESULT hr;

    DXGI_SWAP_CHAIN_DESC desc;
    hr = swapChain->GetDesc(&desc);
    if (FAILED(hr)) { m_logger->Error("CreateRenderTarget: swapChain->GetDesc failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr)); return; }

    m_mainRenderTargetResource.resize(desc.BufferCount);
    m_mainRenderTargetDescriptors.resize(desc.BufferCount);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = desc.BufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = m_pd3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pd3dRtvDescHeap.GetAddressOf()));
    if (FAILED(hr)) { m_logger->Error("CreateRenderTarget: CreateDescriptorHeap (RTV) failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr)); return; }

    SIZE_T rtvDescriptorSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < desc.BufferCount; i++) {
        m_mainRenderTargetResource[i].Reset();
        hr = swapChain->GetBuffer(i, IID_PPV_ARGS(m_mainRenderTargetResource[i].GetAddressOf()));
        if (FAILED(hr)) { m_logger->Error("CreateRenderTarget: swapChain->GetBuffer failed for buffer {}. (HRESULT: {:#x})", i, static_cast<unsigned int>(hr)); return; }
        
        m_pd3dDevice->CreateRenderTargetView(m_mainRenderTargetResource[i].Get(), NULL, rtvHandle);
        m_mainRenderTargetDescriptors[i] = rtvHandle;
        rtvHandle.ptr += rtvDescriptorSize;
    }

    m_renderTargetsCreated = true;
    m_logger->Debug("Render target views created successfully.");
}

void D3D12RendererImpl::CleanupRenderTarget() {
    // This function must be called before the swap chain is resized.
    // It waits for the GPU to finish using the resources and then releases our references to them.
    WaitForLastSubmittedFrame();
    m_logger->Debug("Cleaning up render targets...");
    m_mainRenderTargetResource.clear();
    m_mainRenderTargetDescriptors.clear();
    m_pd3dRtvDescHeap.Reset();
}

void D3D12RendererImpl::WaitForLastSubmittedFrame() {
    if (!m_fence || !m_fenceEvent) return;

    // Check if the last signaled value has already been reached.
    if (m_fence->GetCompletedValue() >= m_fenceLastSignaledValue) {
        return;
    }

    // If not, schedule an event to be signaled when the fence reaches our value.
    HRESULT hr = m_fence->SetEventOnCompletion(m_fenceLastSignaledValue, m_fenceEvent);
    if (FAILED(hr)) {
        m_logger->Error("WaitForLastSubmittedFrame: SetEventOnCompletion failed (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }

    // Wait for the event to be signaled.
    WaitForSingleObject(m_fenceEvent, INFINITE);
}

}  // namespace Rendering
SPF_NS_END
