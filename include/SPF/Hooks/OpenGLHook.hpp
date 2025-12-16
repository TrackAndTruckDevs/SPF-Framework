#pragma once

#include "SPF/Utils/Signal.hpp"
#include <Windows.h>

SPF_NS_BEGIN

namespace Hooks {

/**
 * @class OpenGLHook
 * @brief Manages the OpenGL graphics API hooks. (System Hook)
 *
 * This singleton class is responsible for finding and hooking OpenGL functions
 * (e.g., wglSwapBuffers) to enable ImGui rendering.
 * NOTE: This is a system hook and does not inherit from IHook.
 */
class OpenGLHook {
 public:
  // --- Public API ---
  static bool Install();
  static void Uninstall();
  static void Remove();
  static bool IsInstalled();

  // --- Public Signals ---
  inline static SPF::Utils::Signal<void(HDC hdc)> OnInit;
  inline static SPF::Utils::Signal<void(HDC hdc)> OnPresent;
  inline static SPF::Utils::Signal<void(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)> OnWndProc;

  // --- Public State ---
  inline static HWND MainWindow = nullptr;
  inline static bool block_wndproc_message = false;

 private:
  OpenGLHook() = default;
};

}  // namespace Hooks

SPF_NS_END
