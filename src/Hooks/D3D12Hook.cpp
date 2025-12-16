// --- THE STRATEGY ---
// This implementation uses a multi-stage process to capture the command queue pointer:
//
// 1.  **Hook `Present`**: We first hook `IDXGISwapChain::Present`. This is our entry point into the game's
//     render loop. We can reliably hook this method by creating a dummy device and swap chain to get the
//     virtual function table (v-table) address, a standard technique for late hooking.
//
// 2.  **Hook `ExecuteCommandLists`**: On the *first* call to our hooked `Present`, we know we have a valid
//     `ID3D12Device`. We use this device to create another dummy command queue just to get the v-table
//     address for `ID3D12CommandQueue::ExecuteCommandLists`. We then place a temporary hook on this function.
//
// 3.  **Capture the Queue**: The game, in its render loop, will call `ExecuteCommandLists` to submit its
//     rendering commands. Our temporary hook fires. The `this` pointer of that function call is a pointer
//     to the game's actual command queue. We capture this pointer into a global variable (`g_pGameCommandQueue`)
//     and immediately disable the temporary hook, as its job is done.
//
// 4.  **Initialize Renderer**: On the next call to `Present`, we see that the command queue pointer has been
//     captured. Now we have all the pieces we need: the device, the swap chain, and the correct command queue.
//     We can now safely initialize our renderer and ImGui.

#include <SPF/Hooks/D3D12Hook.hpp>

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <MinHook.h>
#include <SPF/Logging/LoggerFactory.hpp>
#include <SPF/UI/MainWindow.hpp>

SPF_NS_BEGIN

namespace Hooks {
using namespace SPF::Logging;
using namespace SPF::UI;
using Microsoft::WRL::ComPtr;

namespace {
// --- Logger for this module ---
auto GetLogger() {
    static auto logger = LoggerFactory::GetInstance().GetLogger("D3D12Hook");
    return logger;
}

// --- Function Prototypes for our hooks ---
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain3_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain3_ResizeBuffers(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
LRESULT CALLBACK WndProcD3D12(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void STDMETHODCALLTYPE new_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);

// --- Function pointer definitions for D3D12CreateDevice and CreateDXGIFactory
typedef HRESULT (WINAPI *PFN_D3D12_CREATE_DEVICE)(IUnknown* pAdapter, D3D_FEATURE_LEVEL MinimumFeatureLevel, REFIID riid, void** ppDevice);
typedef HRESULT (WINAPI *PFN_CREATE_DXGI_FACTORY)(REFIID riid, void** ppFactory); // Added this line

// --- Hook Management & Pointers to Original Functions ---
WNDPROC g_originalWndProcD3D12 = nullptr;

// For Present/ResizeBuffers
using FnPresentD3D12 = HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT);
using FnResizeBuffersD3D12 = HRESULT(WINAPI*)(IDXGISwapChain3*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
inline FnPresentD3D12 o_PresentD3D12 = nullptr;
inline FnResizeBuffersD3D12 o_ResizeBuffersD3D12 = nullptr;
inline LPVOID g_pPresentTargetD3D12 = nullptr;
inline LPVOID g_pResizeBuffersTargetD3D12 = nullptr;

// For ExecuteCommandLists
using FnExecuteCommandLists = void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
inline FnExecuteCommandLists o_ExecuteCommandLists = nullptr;
inline LPVOID pExecuteCommandListsTarget = nullptr; // Address in the V-table

// --- State ---
bool g_isCreatedD3D12 = false; // Are the Present/Resize hooks created?
bool g_isInitedD3D12 = false;
inline ID3D12CommandQueue* g_pGameCommandQueue = nullptr;
bool g_isExecuteHooked = false;

// Helper to restore WndProc safely
void RestoreWndProcD3D12() {
    if (g_originalWndProcD3D12 != nullptr && D3D12Hook::MainWindow != nullptr) {
        SetWindowLongPtr(D3D12Hook::MainWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProcD3D12);
        GetLogger()->Info("Restored original D3D12 WndProc.");
        g_originalWndProcD3D12 = nullptr;
    }
}

/**
 * @brief Creates dummy D3D12 objects to find the v-table addresses for Present and ResizeBuffers.
 * @return True on success, false on failure.
 *
 * This function is the first step of the hook. It doesn't activate any hooks on its own
 * but provides MinHook with the correct function pointers to hook, regardless of which
 * SwapChain instance the game is using.
 */
bool TryToHook() {
    auto logger = GetLogger();
    logger->Info("Attempting to hook D3D12 Present/ResizeBuffers...");

    HMODULE hD3D12 = GetModuleHandle(TEXT("d3d12.dll"));
    HMODULE hDXGI = GetModuleHandle(TEXT("dxgi.dll"));
    if (!hD3D12 || !hDXGI) {
        logger->Error("d3d12.dll or dxgi.dll is not loaded.");
        return false;
    }

    auto pD3D12CreateDevice = (decltype(&D3D12CreateDevice))GetProcAddress(hD3D12, "D3D12CreateDevice");
    auto pCreateDXGIFactory = (PFN_CREATE_DXGI_FACTORY)GetProcAddress(hDXGI, "CreateDXGIFactory");
    if (!pD3D12CreateDevice || !pCreateDXGIFactory) {
        logger->Error("Failed to get addresses of D3D12CreateDevice or CreateDXGIFactory.");
        return false;
    }
    
    ComPtr<IDXGIFactory4> pFactory;
    if (FAILED(pCreateDXGIFactory(IID_PPV_ARGS(&pFactory)))) return false;

    ComPtr<ID3D12Device> pDummyDevice;
    if (FAILED(pD3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDummyDevice)))) return false;

    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
    ComPtr<ID3D12CommandQueue> pDummyCommandQueue;
    if (FAILED(pDummyDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pDummyCommandQueue)))) return false;

    WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "SPF_DUMMY_WINDOW", NULL };
    ::RegisterClassExA(&wc);
    HWND temp_hwnd = ::CreateWindowA(wc.lpszClassName, NULL, WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = 1;
    swapChainDesc.Height = 1;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> pDummySwapChain1;
    if (FAILED(pFactory->CreateSwapChainForHwnd(pDummyCommandQueue.Get(), temp_hwnd, &swapChainDesc, NULL, NULL, &pDummySwapChain1))) {
        ::DestroyWindow(temp_hwnd);
        ::UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return false;
    }

    void** pSwapChainVTable = *(void***)pDummySwapChain1.Get();
    ::DestroyWindow(temp_hwnd);
    ::UnregisterClassA(wc.lpszClassName, wc.hInstance);

    g_pPresentTargetD3D12 = (LPVOID)pSwapChainVTable[8];
    g_pResizeBuffersTargetD3D12 = (LPVOID)pSwapChainVTable[13];

    if (MH_CreateHook(g_pPresentTargetD3D12, reinterpret_cast<LPVOID>(&new_IDXGISwapChain3_Present), reinterpret_cast<LPVOID*>(&o_PresentD3D12)) != MH_OK) return false;
    if (MH_CreateHook(g_pResizeBuffersTargetD3D12, reinterpret_cast<LPVOID>(&new_IDXGISwapChain3_ResizeBuffers), reinterpret_cast<LPVOID*>(&o_ResizeBuffersD3D12)) != MH_OK) {
        MH_RemoveHook(g_pPresentTargetD3D12);
        return false;
    }
    if (MH_EnableHook(g_pPresentTargetD3D12) != MH_OK || MH_EnableHook(g_pResizeBuffersTargetD3D12) != MH_OK) {
        MH_RemoveHook(g_pPresentTargetD3D12);
        MH_RemoveHook(g_pResizeBuffersTargetD3D12);
        return false;
    }
    
    logger->Info("Successfully prepared Present/ResizeBuffers hooks. Waiting for game to call them...");
    return true;
}

}  // namespace

// --- Public API ---

bool D3D12Hook::Install() {
    auto logger = GetLogger();
    if (g_isCreatedD3D12) {
        logger->Info("D3D12 hooks already created, ensuring they are enabled...");
        if (g_pPresentTargetD3D12 && MH_EnableHook(g_pPresentTargetD3D12) != MH_OK) {
             logger->Error("Failed to re-enable D3D12 Present hook.");
             return false;
        }
        if (g_pResizeBuffersTargetD3D12 && MH_EnableHook(g_pResizeBuffersTargetD3D12) != MH_OK) {
            logger->Error("Failed to re-enable D3D12 ResizeBuffers hook.");
            return false;
        }
        logger->Info("D3D12 hooks successfully re-enabled.");
        return true;
    }

    logger->Info("Installing D3D12 hooks for the first time...");
    if (TryToHook()) {
        logger->Info("Successfully created and enabled D3D12 hooks.");
        g_isCreatedD3D12 = true;
        return true;
    }
    
    logger->Critical("Failed to install D3D12 hooks.");
    return false;
}

void D3D12Hook::Uninstall() {
    auto logger = GetLogger();
    if (!g_isCreatedD3D12) {
        return;
    }
    logger->Info("Disabling D3D12 hooks for reload...");
    RestoreWndProcD3D12();
    
    if (g_pPresentTargetD3D12) MH_DisableHook(g_pPresentTargetD3D12);
    if (g_pResizeBuffersTargetD3D12) MH_DisableHook(g_pResizeBuffersTargetD3D12);
    
    g_isInitedD3D12 = false;
    logger->Info("D3D12 hooks disabled.");
}

void D3D12Hook::Remove() {
    auto logger = GetLogger();
    if (!g_isCreatedD3D12) {
        return;
    }
    logger->Info("Removing D3D12 hooks for shutdown...");
    RestoreWndProcD3D12();

    if (pExecuteCommandListsTarget) MH_RemoveHook(pExecuteCommandListsTarget);
    if (g_pPresentTargetD3D12) MH_RemoveHook(g_pPresentTargetD3D12);
    if (g_pResizeBuffersTargetD3D12) MH_RemoveHook(g_pResizeBuffersTargetD3D12);

    // Reset state
    g_pPresentTargetD3D12 = nullptr;
    g_pResizeBuffersTargetD3D12 = nullptr;
    o_PresentD3D12 = nullptr;
    o_ResizeBuffersD3D12 = nullptr;
    pExecuteCommandListsTarget = nullptr;
    o_ExecuteCommandLists = nullptr;
    g_pGameCommandQueue = nullptr;
    
    g_isExecuteHooked = false;
    g_isInitedD3D12 = false;
    g_isCreatedD3D12 = false;
    logger->Info("D3D12 hooks completely removed.");
}

bool D3D12Hook::IsInstalled() {
    // A hook is considered "installed" as soon as it has been successfully created via MH_CreateHook.
    // The g_isInitedD3D12 flag is for internal logic within the Present hook itself.
    return g_isCreatedD3D12;
}

// --- Hook Implementations ---

namespace {

/**
 * @brief A temporary, one-shot hook for ID3D12CommandQueue::ExecuteCommandLists.
 * @param pQueue The `this` pointer of the command queue object being used by the game.
 *
 * The sole purpose of this hook is to capture the game's command queue pointer.
 * Once the pointer is captured, the hook disables itself to avoid any performance overhead
 * on subsequent frames.
 */
void STDMETHODCALLTYPE new_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
    // This hook is temporary and its only job is to capture the command queue pointer.
    if (g_pGameCommandQueue == nullptr) {
        g_pGameCommandQueue = pQueue;
        GetLogger()->Info("Game's D3D12 Command Queue captured: {0:p}", (void*)g_pGameCommandQueue);
    }
    // Now that we have the pointer, we can disable this hook to reduce overhead.
    MH_DisableHook(pExecuteCommandListsTarget);
    
    // Call the original function.
    o_ExecuteCommandLists(pQueue, NumCommandLists, ppCommandLists);
}

/**
 * @brief The main hook for IDXGISwapChain::Present, which acts as a state machine.
 * @return The original HRESULT from the game's Present call.
 *
 * This function is the entry point for our rendering and initialization logic.
 * It follows a multi-stage process:
 * 1. On the first ever run, it sets up a temporary hook on `ExecuteCommandLists` to capture the command queue.
 * 2. On a subsequent run (after the queue is captured), it performs the main initialization for our renderer.
 * 3. On all future runs, it calls the OnPresent signal to render the ImGui UI.
 */
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain3_Present(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
    // State 1: We haven't hooked ExecuteCommandLists yet.
    if (!g_isExecuteHooked) {
        g_isExecuteHooked = true; // Prevent re-entry
        auto logger = GetLogger();
        logger->Info("First Present call. Attempting to hook ExecuteCommandLists to capture command queue...");
        
        ComPtr<ID3D12Device> device;
        if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&device)))) {
            // Create a dummy queue to get the v-table
            D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
            ComPtr<ID3D12CommandQueue> pDummyQueue;
            if (SUCCEEDED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pDummyQueue)))) {
                void** pVTable = *(void***)pDummyQueue.Get();
                pExecuteCommandListsTarget = pVTable[10]; // ID3D12CommandQueue::ExecuteCommandLists
                
                if (MH_CreateHook(pExecuteCommandListsTarget, reinterpret_cast<LPVOID>(&new_ExecuteCommandLists), reinterpret_cast<LPVOID*>(&o_ExecuteCommandLists)) == MH_OK) {
                    if (MH_EnableHook(pExecuteCommandListsTarget) == MH_OK) {
                        logger->Info("Temporary hook on ExecuteCommandLists enabled. Waiting for game to call it...");
                    } else {
                        logger->Error("Failed to enable hook on ExecuteCommandLists.");
                    }
                } else {
                    logger->Error("Failed to create hook for ExecuteCommandLists.");
                }
            } else {
                 logger->Error("Failed to create dummy command queue.");
            }
        } else {
             logger->Error("Failed to get D3D12 device from swap chain in first Present call.");
        }
    }
    
    // State 2: We have the command queue, but ImGui is not initialized.
    if (g_pGameCommandQueue != nullptr && !g_isInitedD3D12) {
        try {
            g_isInitedD3D12 = true; // Set flag immediately
            auto logger = GetLogger();
            logger->Info("Command queue captured. Initializing ImGui renderer...");

            ComPtr<ID3D12Device> device;
            if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(device.GetAddressOf())))) {
                DXGI_SWAP_CHAIN_DESC desc;
                pSwapChain->GetDesc(&desc);
                D3D12Hook::MainWindow = desc.OutputWindow;
                logger->Info("Game HWND captured: {0:p}", static_cast<void*>(D3D12Hook::MainWindow));

                logger->Info("First-time D3D12 initialization. Hooking WndProc and firing OnInit...");
                g_originalWndProcD3D12 = reinterpret_cast<WNDPROC>(SetWindowLongPtr(D3D12Hook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcD3D12)));
                logger->Info("Original D3D12 WndProc at {0:p}, hooked with ours.", reinterpret_cast<void*>(g_originalWndProcD3D12));
                
                D3D12Hook::OnInit.Call(pSwapChain, device.Get(), g_pGameCommandQueue);

            } else {
                logger->Error("Failed to get D3D12 device from swap chain during final initialization.");
                g_isInitedD3D12 = false; // Allow retry
            }
        } catch (const std::exception& e) {
            GetLogger()->Critical("An exception occurred during D3D12 initialization: {}", e.what());
            g_isInitedD3D12 = false; 
        } catch (...) {
            GetLogger()->Critical("An unknown exception occurred during D3D12 initialization.");
            g_isInitedD3D12 = false;
        }
    }

    // State 3: Fully initialized, render ImGui.
    if (g_isInitedD3D12) {
        try {
            D3D12Hook::OnPresent.Call(pSwapChain);
        } catch (const std::exception& e) {
            GetLogger()->Critical("An exception occurred during OnPresent: {}", e.what());
        } catch (...) {
            GetLogger()->Critical("An unknown exception occurred during OnPresent.");
        }
    }
    
    return o_PresentD3D12(pSwapChain, SyncInterval, Flags);
}

/**
 * @brief Hook for IDXGISwapChain::ResizeBuffers to handle window size changes.
 *
 * It's critical to handle resizes correctly. We fire a `OnBeforeResize` signal to allow
 * the renderer to release any references to the swap chain's back buffers *before* the
 * original function is called. After the game has resized the buffers, we fire `OnAfterResize`
 * to allow the renderer to re-create its resources.
 */
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain3_ResizeBuffers(IDXGISwapChain3* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    if (g_isInitedD3D12) {
        try {
            GetLogger()->Debug("new_IDXGISwapChain3_ResizeBuffers called. Firing OnBeforeResize.");
            D3D12Hook::OnBeforeResize.Call(pSwapChain, Width, Height);
        } catch (const std::exception& e) {
            GetLogger()->Critical("An exception occurred during OnBeforeResize: {}", e.what());
        } catch (...) {
            GetLogger()->Critical("An unknown exception occurred during OnBeforeResize.");
        }
    }

    HRESULT hr = o_ResizeBuffersD3D12(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    
    if (g_isInitedD3D12) {
        try {
            GetLogger()->Debug("new_IDXGISwapChain3_ResizeBuffers finished. Firing OnAfterResize.");
            D3D12Hook::OnAfterResize.Call(pSwapChain, Width, Height);
        } catch (const std::exception& e) {
            GetLogger()->Critical("An exception occurred during OnAfterResize: {}", e.what());
        } catch (...) {
            GetLogger()->Critical("An unknown exception occurred during OnAfterResize.");
        }
    }
    
    return hr;
}

LRESULT CALLBACK WndProcD3D12(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        D3D12Hook::block_wndproc_message = false;
        D3D12Hook::OnWndProc.Call(hWnd, uMsg, wParam, lParam);

        if (D3D12Hook::block_wndproc_message) {
            return 0;
        }
    } catch (const std::exception& e) {
        GetLogger()->Critical("An exception occurred during WndProc: {}", e.what());
    } catch (...) {
        GetLogger()->Critical("An unknown exception occurred during WndProc.");
    }
    return CallWindowProc(g_originalWndProcD3D12, hWnd, uMsg, wParam, lParam);
}

}  // namespace

}  // namespace Hooks

SPF_NS_END
