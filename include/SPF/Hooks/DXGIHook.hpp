#pragma once

#include "SPF/Renderer/RenderAPI.hpp"
#include "SPF/Utils/Signal.hpp"

#include <minwindef.h>
#include <windef.h>

struct IDXGISwapChain;
struct IDXGISwapChain3;
struct ID3D11Device;
struct ID3D12Device;
struct ID3D12CommandQueue;

namespace SPF::Hooks {

class DXGIHook {
 public:
  static bool Install();
  static void Uninstall();
  static void Remove();
  static bool IsInstalled();

  static Rendering::RenderAPI GetDetectedAPI();

  inline static HWND MainWindow = nullptr;
  inline static bool block_wndproc_message = false;

  inline static SPF::Utils::Signal<void(HWND, UINT, WPARAM, LPARAM)> OnWndProc;
  inline static SPF::Utils::Signal<void(Rendering::RenderAPI)> OnAPIDetected;

  inline static SPF::Utils::Signal<void(IDXGISwapChain*, ID3D11Device*)> OnD3D11Init;
  inline static SPF::Utils::Signal<void(IDXGISwapChain*, ID3D12Device*, ID3D12CommandQueue*)> OnD3D12Init;
  inline static SPF::Utils::Signal<void(IDXGISwapChain*)> OnPresent;
  inline static SPF::Utils::Signal<void(IDXGISwapChain*, UINT, UINT)> OnBeforeResize;
  inline static SPF::Utils::Signal<void(IDXGISwapChain*, UINT, UINT)> OnResize;

 private:
  DXGIHook() = default;
};

}  // namespace SPF::Hooks
