#pragma once

#include <cstdint>
#include <windows.h>

#include "SPF/Utils/Signal.hpp"
#include "SPF/Namespace.hpp"

struct IDXGISwapChain;
struct ID3D11Device;

SPF_NS_BEGIN

namespace Hooks {

/**
 * @class D3D11Hook
 * @brief Manages the low-level hooking of the D3D11 render API.
 *
 * This is a fully static class that provides signals for key rendering events:
 * - Initialization (OnInit)
 * - Frame presentation (OnPresent)
 * - Buffer resizing (OnResize)
 * - Window messages (OnWndProc)
 *
 * It uses the "dummy device" method to reliably find the IDXGISwapChain vtable
 * before the game fully initializes.
 */
class D3D11Hook {
 public:
  // The game's main window handle, captured on initialization.
  inline static HWND MainWindow = nullptr;

  // A flag used by the WndProc hook to determine if a message should be blocked.
  // This is set by a signal handler (e.g., WndProcEventProxy) and read by the hook itself.
  inline static bool block_wndproc_message = false;

  // --- Signals ---
  // These signals are emitted when the corresponding event occurs.

  // Called once when the renderer is first initialized. Provides the swap chain and device.
  inline static SPF::Utils::Signal<void(IDXGISwapChain*, ID3D11Device*)> OnInit;

  // Called every frame when Present() is executed.
  inline static SPF::Utils::Signal<void(IDXGISwapChain*)> OnPresent;

  // Called before the swap chain buffers are resized.
  inline static SPF::Utils::Signal<void(IDXGISwapChain*, UINT, UINT)> OnBeforeResize;

  // Called after the swap chain buffers have been resized.
  inline static SPF::Utils::Signal<void(IDXGISwapChain*, UINT, UINT)> OnResize;

  // Called for every window message received by the game's main window.
  inline static SPF::Utils::Signal<void(HWND, UINT, WPARAM, LPARAM)> OnWndProc;

  /**
   * @brief Installs or re-enables all necessary D3D11 hooks.
   * @return True if successful, false otherwise.
   */
  static bool Install();

  /**
   * @brief Disables all D3D11 hooks for a framework reload.
   */
  static void Uninstall();

  /**
   * @brief Completely removes all D3D11 hooks on shutdown.
   */
  static void Remove();

  /**
   * @brief Checks if the D3D11 hooks have been successfully installed.
   * @return True if installed, false otherwise.
   */
  static bool IsInstalled();

 private:
  D3D11Hook() = default;
};

}  // namespace Hooks

SPF_NS_END