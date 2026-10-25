// --- THE STRATEGY ---
// Game-owned DXGI interception that never enters the Present chain. The
// swapchain Present slot is the most contested address in the stack (Steam
// overlay, OptiScaler, RTSS, driver frame generation); we stay out of it by
// hooking the GAME's own code instead of dxgi.dll:
//
// 1. FLIP HOOK — the game presents through one small backend flip function
//    that reads the swapchain pointer and calls [vtable+8] (Present). The
//    function is resolved from its unique error string, and MinHook detours
//    it inside the game's .text. No dxgi code is patched, no vtable is
//    wrapped, nothing another agent hooks (Present/Present1/ResizeBuffers)
//    is touched — the driver sees a vanilla Present call from the game.
//
// 2. ACQUISITION — the flip receives the backend object every frame; the
//    swapchain pointer lives in pattern-resolved slots on it (DXGI 1.3
//    preferred, classic fallback). Recreation swaps the pointer — the next
//    flip hands us the new instance. No poll thread, no import tables.
//
// 3. First flip probes GetDevice():
//    - GetDevice(ID3D11Device*) -> D3D11 game
//    - IDXGISwapChain3 + GetDevice(ID3D12Device*) -> D3D12 game
//    D3D12 additionally hooks ExecuteCommandLists (queue vtable[10], via the
//    game's own device) to capture the command queue, then disables itself.
//
// 4. RESIZE — the game's own resize function (string-anchored, entry in the
//    game's .text) is detoured as well: OnBeforeResize releases our
//    backbuffer references, the original runs ResizeBuffers unblocked, and
//    OnResize fires afterwards. Render targets persist across frames —
//    no shared dxgi.dll code is touched at any point.
//
// 5. Diagnostics: FinderLog walks every extraction step; detection, resize
//    changes and hook transitions are logged with pointers so a stall is
//    always attributable to a specific link.

#include "SPF/Hooks/DXGIHook.hpp"

#include "SPF/Hooks/OpenGLHook.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Renderer/RenderAPI.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/Windows.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <exception>
#include <libloaderapi.h>
#include <memory>
#include <MinHook.h>
#include <minwindef.h>
#include <windef.h>
#include <windows.h>
#include <winerror.h>
#include <winnt.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace SPF::Hooks {

using FnGameFlip = uint64_t (*)(void* backend);
using FnGameResize = void (*)(void* backend, uint64_t param2, uint8_t format, uint32_t width, uint32_t height);
using FnExecuteCommandLists = void(STDMETHODCALLTYPE*)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

// ID3D12CommandQueue inherits IUnknown (0-2) -> ID3D12Object (3-6) ->
// ID3D12DeviceChild (7) -> UpdateTileMappings (8), CopyTileMappings (9),
// ExecuteCommandLists (10).
constexpr int kExecuteCommandListsSlot = 10;

std::shared_ptr<SPF::Logging::Logger> GetLogger() { return SPF::Logging::LoggerFactory::GetInstance().GetLogger("DXGIHook"); }

// --- Game-owned DXGI layout (pattern-resolved, never hardcoded) ---
size_t g_offSwapChain1 = 0;  // backend+this: IDXGISwapChain1* (DXGI 1.3 path, preferred)
size_t g_offSwapChain = 0;   // backend+this: IDXGISwapChain* (classic path)
uintptr_t g_flipTarget = 0;   // game flip function entry (detour target)
uintptr_t g_resizeTarget = 0;  // game resize-backbuffers function entry (detour target)
FnGameFlip o_GameFlip = nullptr;
FnGameResize o_GameResize = nullptr;
std::atomic<bool> g_layoutResolved{false};

std::atomic<bool> g_hooksActive{false};
std::atomic<bool> g_isCreated{false};
std::atomic<Rendering::RenderAPI> g_detectedAPI{Rendering::RenderAPI::Unknown};
std::atomic<bool> g_isInited{false};
std::atomic<bool> g_needUpdateInfo{true};
std::atomic<bool> g_isExecuteHooked{false};

thread_local int g_presentDepth = 0;
thread_local bool g_loggedRecursionGuard = false;

ID3D12CommandQueue* g_pGameCommandQueue = nullptr;
FnExecuteCommandLists o_ExecuteCommandLists = nullptr;
void* g_pExecuteCommandListsTarget = nullptr;

WNDPROC g_originalWndProc = nullptr;

// Objects from the first successful init fire — used to replay signals for
// listeners that connect after detection (Renderer connects during Main Init,
// while the first flip lands within milliseconds of the detour).
ComPtr<IDXGISwapChain> g_replaySwapChain;
ComPtr<ID3D11Device> g_replayDevice11;
ComPtr<ID3D12Device> g_replayDevice12;

// ---------------------------------------------------------------------------
// Game DXGI layout resolution
// ---------------------------------------------------------------------------

namespace {

constexpr const char* kPresentFailedStr = "[dxgi] Present failed (0x%lx)";
constexpr const char* kResizingBackbuffersStr = "[dxgi] Resizing backbuffers: %u %u (flags: %lx)";
constexpr const char* kMovR64Off32Test = "[MOV r64, [r64+off32]] [TEST r64, r64]";
constexpr const char* kMovR64Off32 = "[MOV r64, [r64+off32]]";
constexpr size_t kFlipToMovWindow = 128;
constexpr size_t kMovToMovWindow = 64;

bool ResolveGameDxgiLayout() {
  if (g_layoutResolved.load(std::memory_order_acquire)) return true;

  Utils::FinderLog log("DXGIHook");
  auto phase = log.MakePhase("Game DXGI layout");

  // Flip function entry: unique error string -> function start.
  // --- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1402c1130[1402c1130]) ---
  // 1402c1130  48 83 EC 28                   SUB RSP,0x28
  uintptr_t flipEntry = Utils::PatternFinder::FindFunctionByString(kPresentFailedStr, true);
  if (!phase.Step(flipEntry, "Flip function entry (Present-failed string)", "CODE")) {
    log.Finish(false);
    return false;
  }

  // Preferred swapchain slot: MOV reads backend+0x130, TEST guards null.
  // --- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1402c1130[1402c1130]) ---
  // 1402c1169  4C 8B 89 30 01 00 00          MOV R9,qword ptr [RCX + 0x130]
  // 1402c1170  4D 85 C9                      TEST R9,R9
  uintptr_t movPreferred = Utils::PatternFinder::Find(flipEntry, kFlipToMovWindow, kMovR64Off32Test);
  if (!phase.Step(movPreferred, "MOV/TEST slot (IDXGISwapChain1)", "CODE")) {
    log.Finish(false);
    return false;
  }
  int movLen = 0;
  int32_t offPreferred = Utils::PatternFinder::ReadInstructionDisp(movPreferred, movLen);
  if (!phase.StepOffset(offPreferred, "IDXGISwapChain1 slot offset", "DATA")) {
    log.Finish(false);
    return false;
  }

  // Classic swapchain slot: next MOV past the first (movLen bytes —
  // self-exclusion).
  // --- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1402c1130[1402c1130]) ---
  // 1402c117d  48 8B 89 28 01 00 00          MOV RCX,qword ptr [RCX + 0x128]
  uintptr_t movClassic = Utils::PatternFinder::Find(movPreferred + static_cast<uintptr_t>(movLen), kMovToMovWindow, kMovR64Off32);
  if (!phase.Step(movClassic, "MOV slot (IDXGISwapChain)", "CODE")) {
    log.Finish(false);
    return false;
  }
  int movLen2 = 0;
  int32_t offClassic = Utils::PatternFinder::ReadInstructionDisp(movClassic, movLen2);
  if (!phase.StepOffset(offClassic, "IDXGISwapChain slot offset", "DATA")) {
    log.Finish(false);
    return false;
  }

  // Resize-backbuffers entry: unique resize log string -> function start.
  // --- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1402c0eb0[1402c0eb0]) ---
  // 1402c0eb0  40 53                         PUSH RBX
  uintptr_t resizeEntry = Utils::PatternFinder::FindFunctionByString(kResizingBackbuffersStr, true);
  if (!phase.Step(resizeEntry, "Resize function entry (Resizing-backbuffers string)", "CODE")) {
    log.Finish(false);
    return false;
  }

  g_offSwapChain1 = static_cast<size_t>(offPreferred);
  g_offSwapChain = static_cast<size_t>(offClassic);
  g_flipTarget = flipEntry;
  g_resizeTarget = resizeEntry;
  g_layoutResolved.store(true, std::memory_order_release);

  log.Finish(!phase.HasFailure());
  log.Info("[Layout] Flip function {} — resize function {} — swapchain slots +0x{:X} (preferred) / +0x{:X} (classic).", log.Rel(flipEntry), log.Rel(resizeEntry), offPreferred, offClassic);
  return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// WndProc + Present handling
// ---------------------------------------------------------------------------

namespace {

void STDMETHODCALLTYPE new_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);

LRESULT CALLBACK WndProcDXGI(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  DXGIHook::block_wndproc_message = false;
  try {
    DXGIHook::OnWndProc.Call(hwnd, msg, wParam, lParam);
  } catch (const std::exception& e) {
    GetLogger()->Critical("Exception in WndProc hook: {}", e.what());
  } catch (...) {
    GetLogger()->Critical("Unknown exception in WndProc hook.");
  }

  if (DXGIHook::block_wndproc_message) {
    return 0;
  }

  return CallWindowProcW(g_originalWndProc, hwnd, msg, wParam, lParam);
}

void InstallGameWndProc() {
  auto logger = GetLogger();

  WNDPROC oldWndProc = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook"));
  if (oldWndProc != nullptr) {
    WNDPROC prevOriginal = reinterpret_cast<WNDPROC>(GetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc"));
    if (prevOriginal != nullptr) {
      SetWindowLongPtrW(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(prevOriginal));
      logger->Info("Cleaned up previous WndProc hook.");
    }
  }

  g_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(DXGIHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcDXGI)));
  SetPropW(DXGIHook::MainWindow, L"SPF_WndProcHook", reinterpret_cast<HANDLE>(WndProcDXGI));
  SetPropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc", reinterpret_cast<HANDLE>(g_originalWndProc));
  logger->Info("Original WndProc at {}, hooked.", reinterpret_cast<void*>(g_originalWndProc));
}

void HandlePresent(IDXGISwapChain* pSwapChain) {
  auto logger = GetLogger();

  if (g_needUpdateInfo) {
    g_needUpdateInfo = false;
    logger->Debug("First flip call — detecting API via GetDevice...");

    ComPtr<ID3D11Device> d3d11Device;
    if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), &d3d11Device))) {
      g_detectedAPI = Rendering::RenderAPI::D3D11;
      logger->Info("Detected D3D11 rendering API.");

      DXGI_SWAP_CHAIN_DESC desc;
      pSwapChain->GetDesc(&desc);
      DXGIHook::MainWindow = desc.OutputWindow;
      logger->Info("Game HWND captured: {}", static_cast<void*>(DXGIHook::MainWindow));

      if (g_isInited) {
        DXGIHook::OnResize.Call(pSwapChain, desc.BufferDesc.Width, desc.BufferDesc.Height);
      } else {
        logger->Info("First-time D3D11 initialization. Hooking WndProc and firing OnInit...");
        InstallGameWndProc();
        g_replaySwapChain = pSwapChain;
        g_replayDevice11 = d3d11Device;
        DXGIHook::OnAPIDetected.Call(Rendering::RenderAPI::D3D11);
        DXGIHook::OnD3D11Init.Call(pSwapChain, d3d11Device.Get());
        g_isInited = true;
      }
    } else {
      ComPtr<IDXGISwapChain3> swapChain3;
      if (SUCCEEDED(pSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain3)))) {
        ComPtr<ID3D12Device> d3d12Device;
        if (SUCCEEDED(swapChain3->GetDevice(IID_PPV_ARGS(&d3d12Device)))) {
          g_detectedAPI = Rendering::RenderAPI::D3D12;
          logger->Info("Detected D3D12 rendering API.");

          DXGI_SWAP_CHAIN_DESC desc;
          pSwapChain->GetDesc(&desc);
          DXGIHook::MainWindow = desc.OutputWindow;
          logger->Info("Game HWND captured: {}", static_cast<void*>(DXGIHook::MainWindow));

          if (g_isInited) {
            DXGIHook::OnResize.Call(pSwapChain, desc.BufferDesc.Width, desc.BufferDesc.Height);
          } else if (g_pGameCommandQueue != nullptr) {
            logger->Info("D3D12 init (command queue already captured). Hooking WndProc and firing OnInit...");
            InstallGameWndProc();
            g_replaySwapChain = swapChain3;
            g_replayDevice12 = d3d12Device;
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
              g_pExecuteCommandListsTarget = pVTable[kExecuteCommandListsSlot];

              if (MH_CreateHook(g_pExecuteCommandListsTarget, reinterpret_cast<LPVOID>(&new_ExecuteCommandLists), reinterpret_cast<LPVOID*>(&o_ExecuteCommandLists)) == MH_OK) {
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
          logger->Error("Failed to get D3D12 device from swap chain.");
        }
      } else {
        logger->Error("Failed to get IDXGISwapChain3 interface.");
      }
    }
  }

  // D3D12 deferred init: command queue captured after API was detected.
  if (g_detectedAPI == Rendering::RenderAPI::D3D12 && g_pGameCommandQueue != nullptr && !g_isInited && DXGIHook::MainWindow != nullptr) {
    try {
      g_isInited = true;
      logger->Info("D3D12 command queue captured. Initializing ImGui renderer...");
      InstallGameWndProc();

      ComPtr<IDXGISwapChain3> swapChain3;
      if (SUCCEEDED(pSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain3)))) {
        ComPtr<ID3D12Device> d3d12Device;
        if (SUCCEEDED(swapChain3->GetDevice(IID_PPV_ARGS(&d3d12Device)))) {
          g_replaySwapChain = swapChain3;
          g_replayDevice12 = d3d12Device;
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
      logger->Critical("Exception during D3D12 deferred init: {}", e.what());
      g_isInited = false;
    } catch (...) {
      logger->Critical("Unknown exception during D3D12 deferred init.");
      g_isInited = false;
    }
  }

  if (g_isInited) {
    try {
      DXGIHook::OnPresent.Call(pSwapChain);
    } catch (const std::exception& e) {
      logger->Critical("Exception during OnPresent: {}", e.what());
    } catch (...) {
      logger->Critical("Unknown exception during OnPresent.");
    }
  }
}

void RestoreWndProcIfHooked() {
  if (g_originalWndProc != nullptr && DXGIHook::MainWindow != nullptr) {
    SetWindowLongPtrW(DXGIHook::MainWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProc);
    RemovePropW(DXGIHook::MainWindow, L"SPF_WndProcHook");
    RemovePropW(DXGIHook::MainWindow, L"SPF_OriginalWndProc");
    GetLogger()->Info("Restored original WndProc.");
    g_originalWndProc = nullptr;
  }
}

uintptr_t ReadBackendSwapChain(const void* backend) {
  const uintptr_t base = reinterpret_cast<uintptr_t>(backend);
  uintptr_t addr = 0;
  if (g_offSwapChain1 != 0) {
    addr = static_cast<uintptr_t>(Utils::PatternFinder::ReadInt64(base + g_offSwapChain1));
  }
  if (addr == 0 && g_offSwapChain != 0) {
    addr = static_cast<uintptr_t>(Utils::PatternFinder::ReadInt64(base + g_offSwapChain));
  }
  return addr;
}

uint64_t new_GameFlip(void* backend) {
  if (!g_hooksActive.load(std::memory_order_acquire) || o_GameFlip == nullptr) {
    return o_GameFlip(backend);
  }

  if (g_presentDepth > 0) {
    if (!g_loggedRecursionGuard) {
      g_loggedRecursionGuard = true;
      GetLogger()->Warn("Recursive flip detected (depth {}) — chaining directly to original.", g_presentDepth);
    }
    return o_GameFlip(backend);
  }
  ++g_presentDepth;

  // Swapchain pointer from the backend: DXGI 1.3 slot first (flip prefers
  // it), classic slot as fallback.
  const uintptr_t swapChainAddr = ReadBackendSwapChain(backend);
  if (swapChainAddr != 0) {
    try {
      HandlePresent(reinterpret_cast<IDXGISwapChain*>(swapChainAddr));
    } catch (const std::exception& e) {
      GetLogger()->Critical("Exception during flip handling: {}", e.what());
    } catch (...) {
      GetLogger()->Critical("Unknown exception during flip handling.");
    }
  }

  --g_presentDepth;
  return o_GameFlip(backend);
}

void new_GameResize(void* backend, uint64_t param2, uint8_t format, uint32_t width, uint32_t height) {
  if (o_GameResize == nullptr) {
    return;
  }
  if (!g_hooksActive.load(std::memory_order_acquire)) {
    o_GameResize(backend, param2, format, width, height);
    return;
  }

  auto logger = GetLogger();
  const uintptr_t swapChainAddr = ReadBackendSwapChain(backend);
  IDXGISwapChain* swapChain = reinterpret_cast<IDXGISwapChain*>(swapChainAddr);

  // Listeners release their backbuffer references first, the original
  // ResizeBuffers runs unblocked, then listeners rebuild. Both signal calls
  // are exception-guarded: a throwing listener must never skip the resize.
  if (g_isInited && swapChain != nullptr) {
    try {
      DXGIHook::OnBeforeResize.Call(swapChain, width, height);
    } catch (const std::exception& e) {
      logger->Critical("Exception during OnBeforeResize: {}", e.what());
    } catch (...) {
      logger->Critical("Unknown exception during OnBeforeResize.");
    }
  }

  o_GameResize(backend, param2, format, width, height);

  if (g_isInited && swapChain != nullptr) {
    try {
      logger->Debug("Game resize {}x{} — firing OnResize.", width, height);
      DXGIHook::OnResize.Call(swapChain, width, height);
    } catch (const std::exception& e) {
      logger->Critical("Exception during OnResize: {}", e.what());
    } catch (...) {
      logger->Critical("Unknown exception during OnResize.");
    }
  }
}

void STDMETHODCALLTYPE new_ExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
  if (g_pGameCommandQueue == nullptr) {
    g_pGameCommandQueue = pQueue;
    GetLogger()->Info("Game's D3D12 Command Queue captured: {}", static_cast<void*>(g_pGameCommandQueue));
  }
  MH_DisableHook(g_pExecuteCommandListsTarget);
  o_ExecuteCommandLists(pQueue, NumCommandLists, ppCommandLists);
}

}  // namespace

// --- Public API ---

bool DXGIHook::Install() {
  auto logger = GetLogger();

  const bool reactivating = g_isCreated;
  if (reactivating) {
    logger->Info("DXGI acquisition already active — re-enabling hooks.");
    g_hooksActive = true;
    if (g_flipTarget != 0) {
      MH_STATUS status = MH_EnableHook(reinterpret_cast<LPVOID>(g_flipTarget));
      if (status != MH_OK && status != MH_ERROR_ENABLED) {
        logger->Critical("Failed to re-enable flip hook (MinHook status {}).", static_cast<int>(status));
        return false;
      }
    }
    if (g_resizeTarget != 0) {
      MH_STATUS status = MH_EnableHook(reinterpret_cast<LPVOID>(g_resizeTarget));
      if (status != MH_OK && status != MH_ERROR_ENABLED) {
        logger->Critical("Failed to re-enable resize hook (MinHook status {}).", static_cast<int>(status));
        return false;
      }
    }
  } else {
    // OpenGLHook claims GL from its first wglSwapBuffers.
    logger->Info("Installing flip-function detour (game-owned code, no shared-DXGI patches)...");

    if (!ResolveGameDxgiLayout()) {
      logger->Critical("Game DXGI layout resolution failed — swapchain acquisition unavailable on this build.");
      return false;
    }

    if (MH_CreateHook(reinterpret_cast<LPVOID>(g_flipTarget), reinterpret_cast<LPVOID>(&new_GameFlip), reinterpret_cast<LPVOID*>(&o_GameFlip)) != MH_OK) {
      logger->Critical("Failed to create flip hook at {}.", reinterpret_cast<void*>(g_flipTarget));
      return false;
    }
    if (MH_EnableHook(reinterpret_cast<LPVOID>(g_flipTarget)) != MH_OK) {
      logger->Critical("Failed to enable flip hook at {}.", reinterpret_cast<void*>(g_flipTarget));
      return false;
    }

    if (MH_CreateHook(reinterpret_cast<LPVOID>(g_resizeTarget), reinterpret_cast<LPVOID>(&new_GameResize), reinterpret_cast<LPVOID*>(&o_GameResize)) != MH_OK) {
      logger->Critical("Failed to create resize hook at {}.", reinterpret_cast<void*>(g_resizeTarget));
      return false;
    }
    if (MH_EnableHook(reinterpret_cast<LPVOID>(g_resizeTarget)) != MH_OK) {
      logger->Critical("Failed to enable resize hook at {}.", reinterpret_cast<void*>(g_resizeTarget));
      return false;
    }

    g_needUpdateInfo = true;
    g_hooksActive = true;
    g_isCreated = true;
  }

  logger->Info("DXGI flip hook active at {}, resize hook active at {}.", reinterpret_cast<void*>(g_flipTarget), reinterpret_cast<void*>(g_resizeTarget));
  return true;
}

void DXGIHook::Uninstall() {
  auto logger = GetLogger();
  if (!g_isCreated) return;

  logger->Info("Disabling DXGI hooks for reload...");
  g_hooksActive = false;
  if (g_flipTarget != 0) {
    MH_DisableHook(reinterpret_cast<LPVOID>(g_flipTarget));
  }
  if (g_resizeTarget != 0) {
    MH_DisableHook(reinterpret_cast<LPVOID>(g_resizeTarget));
  }
  RestoreWndProcIfHooked();

  if (g_pExecuteCommandListsTarget != nullptr) {
    MH_DisableHook(g_pExecuteCommandListsTarget);
  }

  g_isInited = false;
  g_needUpdateInfo = true;
  g_isExecuteHooked = false;
  g_loggedRecursionGuard = false;
  g_pGameCommandQueue = nullptr;
  g_detectedAPI = Rendering::RenderAPI::Unknown;
  g_replaySwapChain.Reset();
  g_replayDevice11.Reset();
  g_replayDevice12.Reset();

  logger->Info("DXGI hooks disabled (flip hook registration retained for fast reload).");
}

void DXGIHook::Remove() {
  auto logger = GetLogger();
  if (!g_isCreated) return;

  logger->Info("Removing DXGI hooks for shutdown...");
  Uninstall();

  if (g_flipTarget != 0) {
    // Uninstall() above already disabled it — only the MinHook registration
    // needs to go away here.
    MH_RemoveHook(reinterpret_cast<LPVOID>(g_flipTarget));
    g_flipTarget = 0;
    o_GameFlip = nullptr;
  }

  if (g_resizeTarget != 0) {
    MH_RemoveHook(reinterpret_cast<LPVOID>(g_resizeTarget));
    g_resizeTarget = 0;
    o_GameResize = nullptr;
  }

  if (g_pExecuteCommandListsTarget != nullptr) {
    MH_RemoveHook(g_pExecuteCommandListsTarget);
    g_pExecuteCommandListsTarget = nullptr;
  }
  o_ExecuteCommandLists = nullptr;

  g_pGameCommandQueue = nullptr;
  g_isExecuteHooked = false;
  g_isInited = false;
  g_needUpdateInfo = true;
  g_isCreated = false;
  g_layoutResolved = false;
  g_offSwapChain1 = 0;
  g_offSwapChain = 0;
  g_detectedAPI = Rendering::RenderAPI::Unknown;

  logger->Info("DXGI hooks completely removed.");
}

bool DXGIHook::IsInstalled() { return g_isCreated; }

Rendering::RenderAPI DXGIHook::GetDetectedAPI() { return g_detectedAPI; }

void DXGIHook::NotifyOpenGLDetected() {
  auto logger = GetLogger();
  const Rendering::RenderAPI api = g_detectedAPI.load();
  if (api != Rendering::RenderAPI::Unknown && api != Rendering::RenderAPI::OpenGL) {
    logger->Critical("wglSwapBuffers fired but API already detected ({}) — ignoring stale OpenGL claim.", static_cast<int>(api));
    return;
  }

  g_detectedAPI = Rendering::RenderAPI::OpenGL;
  if (g_isInited) {
    return;
  }

  logger->Info("OpenGL backend proven by wglSwapBuffers. Firing API detection...");
  DXGIHook::OnAPIDetected.Call(Rendering::RenderAPI::OpenGL);
  g_isInited = true;
}

void DXGIHook::ReplayInitialization() {
  auto logger = GetLogger();
  if (!g_isInited || g_detectedAPI == Rendering::RenderAPI::Unknown) return;

  if (g_detectedAPI == Rendering::RenderAPI::D3D11) {
    if (!g_replaySwapChain || !g_replayDevice11) return;
    logger->Info("Replaying D3D11 init signals for late-connected listeners...");
    OnAPIDetected.Call(Rendering::RenderAPI::D3D11);
    OnD3D11Init.Call(g_replaySwapChain.Get(), g_replayDevice11.Get());
    return;
  }

  if (g_detectedAPI == Rendering::RenderAPI::D3D12) {
    if (!g_replaySwapChain || !g_replayDevice12 || g_pGameCommandQueue == nullptr) return;
    logger->Info("Replaying D3D12 init signals for late-connected listeners...");
    OnAPIDetected.Call(Rendering::RenderAPI::D3D12);
    OnD3D12Init.Call(g_replaySwapChain.Get(), g_replayDevice12.Get(), g_pGameCommandQueue);
    return;
  }

  if (g_detectedAPI == Rendering::RenderAPI::OpenGL) {
    logger->Info("Replaying OpenGL init signal for late-connected listeners...");
    OnAPIDetected.Call(Rendering::RenderAPI::OpenGL);
    // OnInit fired on the first wglSwapBuffers, before any listener existed —
    // re-fire it now (same semantics as the D3D11/D3D12 init replays above).
    if (Hooks::OpenGLHook::InitHDC != nullptr) {
      Hooks::OpenGLHook::OnInit.Call(Hooks::OpenGLHook::InitHDC);
    }
  }
}

}  // namespace SPF::Hooks
