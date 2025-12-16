#pragma once

#include "SPF/Utils/Signal.hpp"
#include <Windows.h>

// Forward declare D3D12 types to avoid including the large header here
struct IDXGISwapChain3; // Or IDXGISwapChain4 for more advanced features if needed
struct ID3D12Device;
struct ID3D12CommandQueue;

SPF_NS_BEGIN

namespace Hooks {

/**
 * @class D3D12Hook
 * @brief Manages the D3D12 graphics API hooks. (System Hook)
 *
 * This singleton class is responsible for finding and hooking the D3D12 swap
 * chain to enable ImGui rendering. It provides signals that other parts of the
 * framework (like D3D12RendererImpl) can connect to.
 * NOTE: This is a system hook and does not inherit from IHook.
 */
class D3D12Hook {
 public:
  // --- Public API ---
  static bool Install();
  static void Uninstall();
  static void Remove();
  static bool IsInstalled();

  // --- Public Signals ---
  inline static SPF::Utils::Signal<void(IDXGISwapChain3* swapChain, ID3D12Device* device, ID3D12CommandQueue* commandQueue)> OnInit;
  inline static SPF::Utils::Signal<void(IDXGISwapChain3* swapChain)> OnPresent;
  inline static SPF::Utils::Signal<void(IDXGISwapChain3* swapChain, UINT width, UINT height)> OnBeforeResize;
  inline static SPF::Utils::Signal<void(IDXGISwapChain3* swapChain, UINT width, UINT height)> OnAfterResize;
  inline static SPF::Utils::Signal<void(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)> OnWndProc;

  // --- Public State ---
  inline static HWND MainWindow = nullptr;
  inline static bool block_wndproc_message = false;

 private:
  D3D12Hook() = default;
};

}  // namespace Hooks

SPF_NS_END
