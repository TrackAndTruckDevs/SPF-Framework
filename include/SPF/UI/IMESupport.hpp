#pragma once

#include "SPF/Namespace.hpp"

#include <minwindef.h>
#include <windef.h>

struct ImGuiContext;
struct ImGuiViewport;
struct ImGuiPlatformImeData;

SPF_NS_BEGIN

namespace UI {
/**
 * @brief Installs a custom IME (Input Method Editor) data handler for ImGui.
 *
 * This module installs a replacement handler that:
 *   - re-associates a default IME context on the window when inset text input
 *     is wanted, so the IME composition window is actually displayed;
 *   - disables the IME once input focus is released, so it does not interfere
 *     with normal gameplay input;
 *   - keeps the composition and candidate windows positioned at the ImGui
 *     caret (InputPos) translated to client coordinates.
 *
 * Call Install(hwnd) once after ImGui_ImplWin32_Init().
 */
class IMESupport {
 public:
  /**
   * @brief Installs the IME data handler into ImGui.
   * @param hwnd Native window handle ImGui renders into.
   * @details Safe to call multiple times; only the first call configures the
   *          handler, later calls just refresh the cached HWND.
   */
  static void Install(HWND hwnd);

  /**
   * @brief Processes Windows IME messages to extract composition and candidate lists.
   * @param hwnd Native window handle.
   * @param msg Windows message ID.
   * @param wParam Message wParam.
   * @param lParam Message lParam.
   * @return True if the message should be blocked from further processing, false otherwise.
   */
  static bool OnWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  /**
   * @brief Renders the custom ImGui IME composition and candidate window.
   * @details Draws an overlay/tooltip window below the active input text box.
   */
  static void Render();

  /**
   * @brief Pre-frame hook called before ImGui::NewFrame() to mask clicks on candidate list.
   */
  static void PreFrame();

  /**
   * @brief Checks if the IME composition is currently active.
   * @return True if composing, false otherwise.
   */
  static bool IsComposing();

  /**
   * @brief Checks if the IME candidate list is currently shown.
   * @return True if showing candidates, false otherwise.
   */
  static bool ShowingCandidates();

  /**
   * @brief Checks if the mouse cursor is currently hovering over the candidate window.
   */
  static bool IsMouseHoveringWindow();

 private:
  static void SetImeDataFn(ImGuiContext* /*unused*/, ImGuiViewport* viewport, ImGuiPlatformImeData* data);
};
}  // namespace UI

SPF_NS_END