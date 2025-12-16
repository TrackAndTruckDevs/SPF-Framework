#include <SPF/Renderer/D3D12RendererImpl.hpp>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <SPF/Hooks/D3D12Hook.hpp>
#include <SPF/Logging/LoggerFactory.hpp>
#include <SPF/UI/UIManager.hpp>
#include <SPF/Renderer/Renderer.hpp>

SPF_NS_BEGIN
namespace Rendering {

using namespace SPF::Logging;
using namespace SPF::Hooks;

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

    // Create a descriptor heap for the ImGui texture atlas.
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_pd3dSrvDescHeap.GetAddressOf()));
    if (FAILED(hr)) {
        m_logger->Critical("OnD3D12Init: CreateDescriptorHeap (SRV) failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr));
        return;
    }

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
    UINT64 fenceValue = m_fenceLastSignaledValue + 1;
    hr = m_pd3dCommandQueue->Signal(m_fence.Get(), fenceValue);
    if (FAILED(hr)) { m_logger->Error("OnD3D12Present: CommandQueue->Signal failed. (HRESULT: {:#x})", static_cast<unsigned int>(hr)); return; }
    m_fenceLastSignaledValue = fenceValue;
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
