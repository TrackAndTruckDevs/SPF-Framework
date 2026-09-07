// --- THE STRATEGY ---
// Unified DXGI hook that works for both D3D11 and D3D12 games.
//
// 1. Create a dummy D3D11 device + swap chain to scrape the DXGI vtable.
//    vtable[8] = Present, vtable[13] = ResizeBuffers — identical for
//    both D3D11 and D3D12 swap chains (both derive from IDXGISwapChain).
//
// 2. Hook Present and ResizeBuffers.
//
// 3. On the FIRST real Present call from the game, probe GetDevice():
//    - Try GetDevice(ID3D11Device*) → D3D11 game
//    - If that fails, try GetDevice(ID3D12Device*) → D3D12 game
//
// 4. For D3D12 games: additionally hook ExecuteCommandLists (vtable[10]
//    of ID3D12CommandQueue) to capture the game's command queue pointer.
//    Once captured, disable the temporary hook.
//
// 5. Fire OnD3D11Init or OnD3D12Init with the resolved objects.
//    All subsequent Present calls fire OnPresent.
//
// This eliminates the unreliable GetModuleHandle-based detection — both
// d3d11.dll and d3d12.dll are present in both modes (DXGI dependencies).

#include "SPF/Hooks/DXGIHook.hpp"

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Renderer/RenderAPI.hpp"
#include "SPF/Utils/Windows.hpp"

#include <_mingw.h>
#include <cstddef>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <exception>
#include <libloaderapi.h>
#include <memory>
#include <MinHook.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <windef.h>
#include <winerror.h>
#include <winnt.h>
#include <wrl/client.h>

namespace SPF::Hooks {
using namespace SPF::Logging;
using Microsoft::WRL::ComPtr;

namespace {

auto GetLogger() {
  static auto logger = LoggerFactory::GetInstance().GetLogger("DXGIHook");
  return logger;
}

// --- Function pointer types ---
using FnPresent = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using FnResizeBuffers = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
using FnExecuteCommandLists = void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

// --- Hook function prototypes ---
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
void STDMETHODCALLTYPE new_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
LRESULT CALLBACK WndProcDXGI(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// --- Original function pointers ---
inline FnPresent o_Present = nullptr;
inline FnResizeBuffers o_ResizeBuffers = nullptr;
inline FnExecuteCommandLists o_ExecuteCommandLists = nullptr;

// --- Hook targets (vtable addresses) ---
inline LPVOID g_pPresentTarget = nullptr;
inline LPVOID g_pResizeBuffersTarget = nullptr;
inline LPVOID g_pExecuteCommandListsTarget = nullptr;

// --- State ---
inline WNDPROC g_originalWndProc = nullptr;
bool g_isCreated = false;
bool g_needUpdateInfo = true;
bool g_isInited = false;
Rendering::RenderAPI g_detectedAPI = Rendering::RenderAPI::Unknown;

// D3D12-specific: command queue capture
inline ID3D12CommandQueue* g_pGameCommandQueue = nullptr;
bool g_isExecuteHooked = false;

void RestoreWndProc() {
  if (g_originalWndProc != nullptr && DXGIHook::MainWindow != nullptr) {
    SetWindowLongPtr(DXGIHook::MainWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
    RemovePropW(DXGIHook::MainWindow, L"SPF_WndProcHook");
    RemovePropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc");
    GetLogger()->Info("Restored original WndProc.");
    g_originalWndProc = nullptr;
  }
}

}  // namespace

// --- Public API ---

bool DXGIHook::Install() {
  auto logger = GetLogger();
  if (g_isCreated) {
    logger->Info("DXGI hooks already created, ensuring they are enabled...");
    if (g_pPresentTarget && MH_EnableHook(g_pPresentTarget) != MH_OK) {
      logger->Error("Failed to re-enable Present hook.");
      return false;
    }
    if (g_pResizeBuffersTarget && MH_EnableHook(g_pResizeBuffersTarget) != MH_OK) {
      logger->Error("Failed to re-enable ResizeBuffers hook.");
      return false;
    }
    logger->Info("DXGI hooks successfully re-enabled.");
    return true;
  }

  logger->Info("Installing unified DXGI hooks (D3D11/D3D12)...");

  HWND hWnd = FindWindowA("prism3d", NULL);
  if (hWnd == NULL) {
    logger->Critical("Could not find game window with class 'prism3d'.");
    return false;
  }
  logger->Info("Found game window HWND: {0:p}", static_cast<void*>(hWnd));

  auto hD3D11 = GetModuleHandle(TEXT("d3d11.dll"));
  if (!hD3D11) {
    logger->Critical("d3d11.dll is not loaded — cannot create dummy swap chain for vtable scraping.");
    return false;
  }

  using FnD3D11CreateDeviceAndSwapChain =
      HRESULT(WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT,
                       const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*,
                       ID3D11DeviceContext**);

  auto pCreateDevice = reinterpret_cast<FnD3D11CreateDeviceAndSwapChain>(
      GetProcAddress(hD3D11, "D3D11CreateDeviceAndSwapChain"));
  if (!pCreateDevice) {
    logger->Critical("Failed to get D3D11CreateDeviceAndSwapChain address.");
    return false;
  }

  D3D_FEATURE_LEVEL featureLevel;
  DXGI_SWAP_CHAIN_DESC swapChainDesc{};
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

  HRESULT hr = pCreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
                              &swapChainDesc, &pSwapChain, &pDevice, &featureLevel, &pContext);
  if (FAILED(hr)) {
    logger->Critical("Failed to create dummy D3D11 device/swap chain. HRESULT: {:#x}", static_cast<unsigned>(hr));
    return false;
  }
  logger->Info("Dummy D3D11 device and swap chain created for vtable scraping.");

  void** vtable = *reinterpret_cast<void***>(pSwapChain.Get());
  g_pPresentTarget = vtable[8];    // IDXGISwapChain::Present
  g_pResizeBuffersTarget = vtable[13];  // IDXGISwapChain::ResizeBuffers

  // Dummy resources released by ComPtr here.

  if (MH_CreateHook(g_pPresentTarget, reinterpret_cast<LPVOID>(&new_IDXGISwapChain_Present),
                    reinterpret_cast<LPVOID*>(&o_Present)) != MH_OK) {
    logger->Critical("Failed to create hook for Present.");
    return false;
  }

  if (MH_CreateHook(g_pResizeBuffersTarget, reinterpret_cast<LPVOID>(&new_IDXGISwapChain_ResizeBuffers),
                    reinterpret_cast<LPVOID*>(&o_ResizeBuffers)) != MH_OK) {
    logger->Critical("Failed to create hook for ResizeBuffers.");
    MH_RemoveHook(g_pPresentTarget);
    return false;
  }

  if (MH_EnableHook(g_pPresentTarget) != MH_OK || MH_EnableHook(g_pResizeBuffersTarget) != MH_OK) {
    logger->Critical("Failed to enable Present/ResizeBuffers hooks.");
    MH_RemoveHook(g_pPresentTarget);
    MH_RemoveHook(g_pResizeBuffersTarget);
    return false;
  }

  logger->Info("Unified DXGI hooks installed. Waiting for first Present call to detect API...");
  g_isCreated = true;
  return true;
}

void DXGIHook::Uninstall() {
  auto logger = GetLogger();
  if (!g_isCreated) return;

  logger->Info("Disabling DXGI hooks for reload...");
  RestoreWndProc();

  if (g_pPresentTarget) MH_DisableHook(g_pPresentTarget);
  if (g_pResizeBuffersTarget) MH_DisableHook(g_pResizeBuffersTarget);
  if (g_pExecuteCommandListsTarget) MH_DisableHook(g_pExecuteCommandListsTarget);

  g_isInited = false;
  g_needUpdateInfo = true;
  g_isExecuteHooked = false;
  g_pGameCommandQueue = nullptr;
  g_detectedAPI = Rendering::RenderAPI::Unknown;

  logger->Info("DXGI hooks disabled.");
}

void DXGIHook::Remove() {
  auto logger = GetLogger();
  if (!g_isCreated) return;

  logger->Info("Removing DXGI hooks for shutdown...");
  RestoreWndProc();

  if (g_pPresentTarget) {
    MH_DisableHook(g_pPresentTarget);
    MH_RemoveHook(g_pPresentTarget);
  }
  if (g_pResizeBuffersTarget) {
    MH_DisableHook(g_pResizeBuffersTarget);
    MH_RemoveHook(g_pResizeBuffersTarget);
  }
  if (g_pExecuteCommandListsTarget) {
    MH_DisableHook(g_pExecuteCommandListsTarget);
    MH_RemoveHook(g_pExecuteCommandListsTarget);
  }

  g_pPresentTarget = nullptr;
  g_pResizeBuffersTarget = nullptr;
  g_pExecuteCommandListsTarget = nullptr;
  o_Present = nullptr;
  o_ResizeBuffers = nullptr;
  o_ExecuteCommandLists = nullptr;
  g_pGameCommandQueue = nullptr;

  g_isExecuteHooked = false;
  g_isInited = false;
  g_needUpdateInfo = true;
  g_isCreated = false;
  g_detectedAPI = Rendering::RenderAPI::Unknown;

  logger->Info("DXGI hooks completely removed.");
}

bool DXGIHook::IsInstalled() { return g_isCreated; }

Rendering::RenderAPI DXGIHook::GetDetectedAPI() { return g_detectedAPI; }

// --- Hook Implementations ---

namespace {

void STDMETHODCALLTYPE new_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists,
                                                ID3D12CommandList* const* ppCommandLists) {
  if (g_pGameCommandQueue == nullptr) {
    g_pGameCommandQueue = pQueue;
    GetLogger()->Info("Game's D3D12 Command Queue captured: {0:p}", static_cast<void*>(g_pGameCommandQueue));
  }
  MH_DisableHook(g_pExecuteCommandListsTarget);
  o_ExecuteCommandLists(pQueue, NumCommandLists, ppCommandLists);
}

HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
  if (g_needUpdateInfo) {
    g_needUpdateInfo = false;
    auto logger = GetLogger();
    logger->Debug("First Present call — detecting API via GetDevice...");

    // Try D3D11 first
    ComPtr<ID3D11Device> d3d11Device;
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), &d3d11Device))) {
      g_detectedAPI = Rendering::RenderAPI::D3D11;
      logger->Info("Detected D3D11 rendering API.");

      DXGI_SWAP_CHAIN_DESC desc;
      pSwapChain->GetDesc(&desc);
      DXGIHook::MainWindow = desc.OutputWindow;
      logger->Info("Game HWND captured: {0:p}", static_cast<void*>(DXGIHook::MainWindow));

      if (g_isInited) {
        DXGIHook::OnResize.Call(pSwapChain, desc.BufferDesc.Width, desc.BufferDesc.Height);
      } else {
        logger->Info("First-time D3D11 initialization. Hooking WndProc and firing OnInit...");

        WNDPROC oldWndProc = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook"));
        if (oldWndProc != nullptr) {
          WNDPROC prevOriginal = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc"));
          if (prevOriginal != nullptr) {
            SetWindowLongPtr(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(prevOriginal));
            logger->Info("Cleaned up previous WndProc hook.");
          }
        }

        g_originalWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcDXGI)));
        SetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook", reinterpret_cast<HANDLE>(WndProcDXGI));
        SetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc", reinterpret_cast<HANDLE>(g_originalWndProc));
        logger->Info("Original WndProc at {0:p}, hooked.", reinterpret_cast<void*>(g_originalWndProc));

        DXGIHook::OnAPIDetected.Call(Rendering::RenderAPI::D3D11);
        DXGIHook::OnD3D11Init.Call(pSwapChain, d3d11Device.Get());
        g_isInited = true;
      }
    } else {
      // Try D3D12
      ComPtr<IDXGISwapChain3> swapChain3;
      if (SUCCEEDED(pSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain3)))) {
        ComPtr<ID3D12Device> d3d12Device;
        if (SUCCEEDED(swapChain3->GetDevice(IID_PPV_ARGS(&d3d12Device)))) {
          g_detectedAPI = Rendering::RenderAPI::D3D12;
          logger->Info("Detected D3D12 rendering API.");

          DXGI_SWAP_CHAIN_DESC desc;
          pSwapChain->GetDesc(&desc);
          DXGIHook::MainWindow = desc.OutputWindow;
          logger->Info("Game HWND captured: {0:p}", static_cast<void*>(DXGIHook::MainWindow));

          if (g_isInited) {
            DXGIHook::OnResize.Call(pSwapChain, desc.BufferDesc.Width, desc.BufferDesc.Height);
          } else if (g_pGameCommandQueue != nullptr) {
            logger->Info("D3D12 init (command queue already captured). Hooking WndProc and firing OnInit...");

            WNDPROC oldWndProc = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook"));
            if (oldWndProc != nullptr) {
              WNDPROC prevOriginal = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc"));
              if (prevOriginal != nullptr) {
                SetWindowLongPtr(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(prevOriginal));
                logger->Info("Cleaned up previous WndProc hook.");
              }
            }

            g_originalWndProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtr(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcDXGI)));
            SetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook", reinterpret_cast<HANDLE>(WndProcDXGI));
            SetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc", reinterpret_cast<HANDLE>(g_originalWndProc));
            logger->Info("Original WndProc at {0:p}, hooked.", reinterpret_cast<void*>(g_originalWndProc));

            DXGIHook::OnAPIDetected.Call(Rendering::RenderAPI::D3D12);
            DXGIHook::OnD3D12Init.Call(swapChain3.Get(), d3d12Device.Get(), g_pGameCommandQueue);
            g_isInited = true;
          } else if (!g_isExecuteHooked) {
            logger->Info("D3D12 detected, hooking ExecuteCommandLists to capture command queue...");
            g_isExecuteHooked = true;

            ComPtr<ID3D12CommandQueue> pDummyQueue;
            D3D12_COMMAND_QUEUE_DESC queueDesc = {D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
            if (SUCCEEDED(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pDummyQueue)))) {
              void** pVTable = *reinterpret_cast<void***>(pDummyQueue.Get());
              g_pExecuteCommandListsTarget = pVTable[10];

              if (MH_CreateHook(g_pExecuteCommandListsTarget, reinterpret_cast<LPVOID>(&new_ExecuteCommandLists),
                                reinterpret_cast<LPVOID*>(&o_ExecuteCommandLists)) == MH_OK) {
                if (MH_EnableHook(g_pExecuteCommandListsTarget) == MH_OK) {
                  logger->Info("ExecuteCommandLists hook enabled. Waiting for command queue capture...");
                } else {
                  logger->Error("Failed to enable ExecuteCommandLists hook.");
                }
              } else {
                logger->Error("Failed to create ExecuteCommandLists hook.");
              }
            }
          }
        } else {
          GetLogger()->Error("Failed to get D3D12 device from swap chain.");
        }
      } else {
        GetLogger()->Error("Failed to get IDXGISwapChain3 interface.");
      }
    }
  }

  // Handle D3D12 deferred init: command queue captured after API was detected
  if (g_detectedAPI == Rendering::RenderAPI::D3D12 && g_pGameCommandQueue != nullptr && !g_isInited && DXGIHook::MainWindow != nullptr) {
    try {
      auto logger = GetLogger();
      g_isInited = true;
      logger->Info("D3D12 command queue captured. Initializing ImGui renderer...");

      WNDPROC oldWndProc = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook"));
      if (oldWndProc != nullptr) {
        WNDPROC prevOriginal = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc"));
        if (prevOriginal != nullptr) {
          SetWindowLongPtr(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(prevOriginal));
          logger->Info("Cleaned up previous WndProc hook.");
        }
      }

      g_originalWndProc = reinterpret_cast<WNDPROC>(
          SetWindowLongPtr(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcDXGI)));
      SetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook", reinterpret_cast<HANDLE>(WndProcDXGI));
      SetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc", reinterpret_cast<HANDLE>(g_originalWndProc));
      logger->Info("Original WndProc at {0:p}, hooked.", reinterpret_cast<void*>(g_originalWndProc));

      ComPtr<IDXGISwapChain3> swapChain3;
      if (SUCCEEDED(pSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain3)))) {
        ComPtr<ID3D12Device> d3d12Device;
        if (SUCCEEDED(swapChain3->GetDevice(IID_PPV_ARGS(&d3d12Device)))) {
          DXGIHook::OnAPIDetected.Call(Rendering::RenderAPI::D3D12);
          DXGIHook::OnD3D12Init.Call(swapChain3.Get(), d3d12Device.Get(), g_pGameCommandQueue);
        } else {
          logger->Error("Failed to get D3D12 device during deferred init.");
          g_isInited = false;
        }
      } else {
        logger->Error("Failed to QI for IDXGISwapChain3 during deferred init.");
        g_isInited = false;
      }
    } catch (const std::exception& e) {
      GetLogger()->Critical("Exception during D3D12 deferred init: {}", e.what());
      g_isInited = false;
    } catch (...) {
      GetLogger()->Critical("Unknown exception during D3D12 deferred init.");
      g_isInited = false;
    }
  }

  if (g_isInited) {
    try {
      DXGIHook::OnPresent.Call(pSwapChain);
    } catch (const std::exception& e) {
      GetLogger()->Critical("Exception during OnPresent: {}", e.what());
    } catch (...) {
      GetLogger()->Critical("Unknown exception during OnPresent.");
    }
  }

  return o_Present(pSwapChain, SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE new_IDXGISwapChain_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
                                                           UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                                                           UINT SwapChainFlags) {
  if (g_isInited) {
    try {
      GetLogger()->Debug("ResizeBuffers called. Firing OnBeforeResize.");
      DXGIHook::OnBeforeResize.Call(pSwapChain, Width, Height);
    } catch (const std::exception& e) {
      GetLogger()->Critical("Exception during OnBeforeResize: {}", e.what());
    } catch (...) {
      GetLogger()->Critical("Unknown exception during OnBeforeResize.");
    }
  }

  g_needUpdateInfo = true;
  HRESULT hr = o_ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

  if (g_isInited) {
    try {
      GetLogger()->Debug("ResizeBuffers done. Firing OnResize.");
      DXGIHook::OnResize.Call(pSwapChain, Width, Height);
    } catch (const std::exception& e) {
      GetLogger()->Critical("Exception during OnResize: {}", e.what());
    } catch (...) {
      GetLogger()->Critical("Unknown exception during OnResize.");
    }
  }

  return hr;
}

LRESULT CALLBACK WndProcDXGI(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  try {
    DXGIHook::block_wndproc_message = false;
    DXGIHook::OnWndProc.Call(hWnd, uMsg, wParam, lParam);

    if (DXGIHook::block_wndproc_message) {
      return 0;
    }
  } catch (const std::exception& e) {
    GetLogger()->Critical("Exception during WndProc: {}", e.what());
  } catch (...) {
    GetLogger()->Critical("Unknown exception during WndProc.");
  }
  return CallWindowProc(g_originalWndProc, hWnd, uMsg, wParam, lParam);
}

}  // namespace
}  // namespace SPF::Hooks
