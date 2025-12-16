// --- THE STRATEGY ---
// This implementation uses a "dummy device" technique to achieve this:
//
// 1. **Find Game Window**: It first finds the game's main window, which is necessary for creating a swap chain.
//
// 2. **Create Dummy Objects**: It calls the original `D3D11CreateDeviceAndSwapChain` to create a temporary,
//    invisible "dummy" device and swap chain. These are not used for rendering.
//
// 3. **Get V-Table**: The dummy swap chain is a valid COM object. We can access its internal structure to get a
//    pointer to its v-table. Since all `IDXGISwapChain` objects of the same type share a v-table, this
//    v-table is the same one used by the game's *real* swap chain.
//
// 4. **Find Function Pointers**: From the v-table, we get the memory addresses of the `Present` (index 8)
//    and `ResizeBuffers` (index 13) functions.
//
// 5. **Hook and Cleanup**: With the function addresses, we use MinHook to place our hooks and then immediately
//    release all the dummy objects. Our hooks will now be called whenever the *game's* swap chain calls Present.

#include <SPF/Hooks/D3D11Hook.hpp>

#include <windows.h>
#include <d3d11.h>
#include <memory>
#include <wrl/client.h> // For ComPtr

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
  static auto logger = LoggerFactory::GetInstance().GetLogger("D3D11Hook");
  return logger;
}

// --- Function Prototypes for our hooks ---
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
LRESULT CALLBACK WndProcD3D(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// --- Hook Management & Pointers to Original Functions ---
WNDPROC g_originalWndProc = nullptr;

using FnPresent = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using FnResizeBuffers = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

inline FnPresent o_Present = nullptr;
inline FnResizeBuffers o_ResizeBuffers = nullptr;

// Store the original function pointers to allow for removal
inline LPVOID g_pPresentTarget = nullptr;
inline LPVOID g_pResizeBuffersTarget = nullptr;

// --- State ---
bool g_needUpdateInfo = true;
bool g_isInited = false;

// Helper to restore WndProc safely
void RestoreWndProc() {
  if (g_originalWndProc != nullptr && D3D11Hook::MainWindow != nullptr) {
    SetWindowLongPtr(D3D11Hook::MainWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
    GetLogger()->Info("Restored original WndProc.");
    g_originalWndProc = nullptr;
  }
}

bool TryToHookExistingDevice() {
    auto logger = GetLogger();
    logger->Info("Attempting to hook an already existing D3D11 device...");

    HWND hWnd = FindWindowA("prism3d", NULL);
    if (hWnd == NULL) {
        logger->Warn("Could not find game window with class 'prism3d'. Cannot hook existing device.");
        return false;
    }
    logger->Info("Found game window with HWND: {0:p}", (void*)hWnd);

    auto hD3D11 = GetModuleHandle(TEXT("d3d11.dll"));
    if (!hD3D11) {
        logger->Error("d3d11.dll is not loaded in this process.");
        return false;
    }
	
    using FnD3D11CreateDeviceAndSwapChainLocal = HRESULT(WINAPI*)(
        IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*,
        UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**,
        D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

    auto pD3D11CreateDeviceAndSwapChain = (FnD3D11CreateDeviceAndSwapChainLocal)GetProcAddress(hD3D11, "D3D11CreateDeviceAndSwapChain");
    if (!pD3D11CreateDeviceAndSwapChain) {
        logger->Error("Failed to get address of D3D11CreateDeviceAndSwapChain.");
        return false;
    }

    // 2. Create a dummy device and swapchain to get the vtable
    D3D_FEATURE_LEVEL featureLevel;
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> pContext;
    ComPtr<IDXGISwapChain> pSwapChain;
    
    HRESULT hr = pD3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice, &featureLevel, &pContext);

    if (FAILED(hr)) {
        logger->Error("Failed to create dummy D3D11 device and swapchain. HRESULT: {:#x}", static_cast<unsigned int>(hr));
        // ComPtr handles cleanup automatically
        return false;
    }
    logger->Debug("Dummy device and swapchain created successfully for v-table scraping.");

    // 3. Get vtable and function pointers
    void** vtable = *reinterpret_cast<void***>(pSwapChain.Get());
    g_pPresentTarget = vtable[8];
    g_pResizeBuffersTarget = vtable[13];
    
    // 4. Dummy resources are released automatically by ComPtr going out of scope here.
    logger->Debug("Dummy resources released.");

    // 5. Hook the functions using the pointers we found
    if (MH_CreateHook(g_pPresentTarget, reinterpret_cast<LPVOID>(&new_IDXGISwapChain_Present), reinterpret_cast<LPVOID*>(&o_Present)) != MH_OK) {
        logger->Critical("Failed to create hook for existing IDXGISwapChain::Present.");
        return false;
    }

    if (MH_CreateHook(g_pResizeBuffersTarget, reinterpret_cast<LPVOID>(&new_IDXGISwapChain_ResizeBuffers), reinterpret_cast<LPVOID*>(&o_ResizeBuffers)) != MH_OK) {
        logger->Critical("Failed to create hook for existing IDXGISwapChain::ResizeBuffers.");
        MH_RemoveHook(g_pPresentTarget);
        return false;
    }

    if (MH_EnableHook(g_pPresentTarget) != MH_OK || MH_EnableHook(g_pResizeBuffersTarget) != MH_OK) {
        logger->Critical("Failed to enable Present and/or ResizeBuffers hooks for existing device.");
        // Attempt to clean up hooks that might have been created
        if (g_pPresentTarget) MH_RemoveHook(g_pPresentTarget);
        if (g_pResizeBuffersTarget) MH_RemoveHook(g_pResizeBuffersTarget);
        return false;
    }
    
    logger->Info("Successfully hooked Present and ResizeBuffers of existing device. Waiting for game to call them...");
    return true;
}
}  // namespace

// --- State Management ---
namespace {
    bool g_isCreated = false; // Is the hook created via MH_CreateHook?
}

// --- Public API ---

bool D3D11Hook::Install() {
    auto logger = GetLogger();
    if (g_isCreated) {
        logger->Info("D3D11 hooks already created, ensuring they are enabled...");
        if (g_pPresentTarget && MH_EnableHook(g_pPresentTarget) != MH_OK) {
             logger->Error("Failed to re-enable D3D11 Present hook.");
             return false;
        }
        if (g_pResizeBuffersTarget && MH_EnableHook(g_pResizeBuffersTarget) != MH_OK) {
            logger->Error("Failed to re-enable D3D11 ResizeBuffers hook.");
            return false;
        }
        logger->Info("D3D11 hooks successfully re-enabled.");
        return true;
    }

    logger->Info("Installing D3D11 hooks for the first time...");
    if (TryToHookExistingDevice()) {
        logger->Info("Successfully created and enabled D3D11 hooks.");
        g_isCreated = true;
        return true;
    }
    
    logger->Critical("Failed to install D3D11 hooks. The game's D3D11 device could not be found or hooked. ImGui functionality will not be available.");
    return false;
}

void D3D11Hook::Uninstall() {
    auto logger = GetLogger();
    if (!g_isCreated) {
        // logger->Warn("Attempted to uninstall D3D11 hooks, but they were never created.");
        return;
    }
    logger->Info("Disabling D3D11 hooks...");
    RestoreWndProc();
    
    if (g_pPresentTarget) MH_DisableHook(g_pPresentTarget);
    if (g_pResizeBuffersTarget) MH_DisableHook(g_pResizeBuffersTarget);
    
    g_isInited = false;
    g_needUpdateInfo = true;

    logger->Info("D3D11 hooks disabled.");
}

void D3D11Hook::Remove() {
    auto logger = GetLogger();
    if (!g_isCreated) {
        // logger->Warn("Attempted to remove D3D11 hooks, but they were never created.");
        return;
    }
    logger->Info("Removing D3D11 hooks for shutdown...");
    RestoreWndProc();

    // Disable and remove hooks using the stored target pointers
    if (g_pPresentTarget) {
        MH_DisableHook(g_pPresentTarget);
        MH_RemoveHook(g_pPresentTarget);
    }
    if (g_pResizeBuffersTarget) {
        MH_DisableHook(g_pResizeBuffersTarget);
        MH_RemoveHook(g_pResizeBuffersTarget);
    }

    // Reset state
    g_pPresentTarget = nullptr;
    g_pResizeBuffersTarget = nullptr;
    o_Present = nullptr;
    o_ResizeBuffers = nullptr;

    g_isInited = false;
    g_needUpdateInfo = true;
    g_isCreated = false;
    logger->Info("D3D11 hooks completely removed.");
}

bool D3D11Hook::IsInstalled() {
    // A hook is considered "installed" as soon as it has been successfully created via MH_CreateHook.
    // The g_isInited flag is for internal logic within the Present hook itself.
    return g_isCreated;
}

// --- Hook Implementations ---

namespace {

HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
  if (g_needUpdateInfo) {
    g_needUpdateInfo = false;
    GetLogger()->Debug("new_IDXGISwapChain_Present: First call, initializing renderer...");

    ComPtr<ID3D11Device> device;
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), &device))) {
      DXGI_SWAP_CHAIN_DESC desc;
      pSwapChain->GetDesc(&desc);
      D3D11Hook::MainWindow = desc.OutputWindow;

      GetLogger()->Info("Game HWND captured: {0:p}", static_cast<void*>(D3D11Hook::MainWindow));

      if (g_isInited) {
        D3D11Hook::OnResize.Call(pSwapChain, desc.BufferDesc.Width, desc.BufferDesc.Height);
      } else {
        GetLogger()->Info("First-time initialization. Hooking WndProc and firing OnInit...");
        g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(D3D11Hook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcD3D)));
        GetLogger()->Info("Original WndProc at {0:p}, hooked with ours.", reinterpret_cast<void*>(g_originalWndProc));

        D3D11Hook::OnInit.Call(pSwapChain, device.Get());
        g_isInited = true;
      }
      // `device` ComPtr automatically releases the reference.
    } else {
      GetLogger()->Error("Failed to get D3D11 device from swap chain.");
    }
  }

  if(g_isInited) {
    D3D11Hook::OnPresent.Call(pSwapChain);
  }

  return o_Present(pSwapChain, SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
  GetLogger()->Debug("new_IDXGISwapChain_ResizeBuffers called. Firing OnBeforeResize and flagging for info update.");

  g_needUpdateInfo = true;
  if(g_isInited) {
    D3D11Hook::OnBeforeResize.Call(pSwapChain, Width, Height);
  }

  return o_ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

LRESULT CALLBACK WndProcD3D(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  D3D11Hook::block_wndproc_message = false;
  D3D11Hook::OnWndProc.Call(hWnd, uMsg, wParam, lParam);

  if (D3D11Hook::block_wndproc_message) {
    return 0;
  }

  return CallWindowProc(g_originalWndProc, hWnd, uMsg, wParam, lParam);
}

}  // namespace
}  // namespace Hooks
SPF_NS_END