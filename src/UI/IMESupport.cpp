#include "SPF/UI/IMESupport.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/LoggerFactory.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <imm.h>
#include <minwindef.h>
#include <stringapiset.h>
#include <windef.h>
#include <winnls.h>
#include <winnt.h>
#include <winuser.h>
#include <string>
#include <vector>

SPF_NS_BEGIN

namespace UI {

#if defined(_MSC_VER)
#pragma comment(lib, "imm32")
#endif

/**
 * @brief Cached native window handle used to resolve the IME context.
 */
static HWND s_imeHwnd = nullptr;

/**
 * @brief Tracks whether a system caret was created for the IME.
 */
static bool s_caretCreated = false;

static std::string s_compositionStrUTF8;
static std::vector<std::string> s_candidatesUTF8;
static uint32_t s_selectedCandidate = 0;
static uint32_t s_candidatePageStart = 0;
static uint32_t s_candidatePageSize = 9;
static bool s_isComposing = false;
static bool s_showCandidates = false;
static ImVec2 s_imeInputPos = ImVec2(0.0f, 0.0f);
static float s_imeInputLineHeight = 0.0f;

struct ClickableArea {
  ImVec2 min;
  ImVec2 max;
  int action; // -1 for Prev, -2 for Next, >=0 for candidate index
};

static std::vector<ClickableArea> s_clickableAreas;
static bool s_blockNextLButtonUp = false;
static int s_pendingAction = -3;
static ImVec2 s_windowMin = ImVec2(0.0f, 0.0f);
static ImVec2 s_windowMax = ImVec2(0.0f, 0.0f);

static std::string WideToUTF8(const std::wstring& wstr) {
  if (wstr.empty()) {
    return "";
  }
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &strTo[0], size_needed, nullptr, nullptr);
  return strTo;
}

static void UpdateCandidateList(HWND hwnd) {
  s_candidatesUTF8.clear();
  s_selectedCandidate = 0;
  s_candidatePageStart = 0;
  s_candidatePageSize = 9;

  HIMC himc = ImmGetContext(hwnd);
  if (himc) {
    DWORD size = ImmGetCandidateListW(himc, 0, nullptr, 0);
    if (size > 0) {
      std::vector<uint8_t> buffer(size);
      CANDIDATELIST* cl = reinterpret_cast<CANDIDATELIST*>(buffer.data());
      if (ImmGetCandidateListW(himc, 0, cl, size) > 0) {
        s_selectedCandidate = cl->dwSelection;
        s_candidatePageStart = cl->dwPageStart;
        s_candidatePageSize = cl->dwPageSize;
        if (s_candidatePageSize == 0) {
          s_candidatePageSize = 9;
        }
        for (DWORD i = 0; i < cl->dwCount; ++i) {
          const wchar_t* cand_w = reinterpret_cast<const wchar_t*>(buffer.data() + cl->dwOffset[i]);
          s_candidatesUTF8.push_back(WideToUTF8(cand_w));
        }
      }
    }
    ImmReleaseContext(hwnd, himc);
  }
}

static void SimulateKeyPress(WORD vk) {
  INPUT inputs[2] = {};
  UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);

  inputs[0].type = INPUT_KEYBOARD;
  inputs[0].ki.wVk = vk;
  inputs[0].ki.wScan = static_cast<WORD>(scan);
  inputs[0].ki.dwFlags = 0;

  inputs[1].type = INPUT_KEYBOARD;
  inputs[1].ki.wVk = vk;
  inputs[1].ki.wScan = static_cast<WORD>(scan);
  inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

  SendInput(2, inputs, sizeof(INPUT));
}

void IMESupport::Install(HWND hwnd) {
  s_imeHwnd = hwnd;

  ImGuiPlatformIO& platform = ImGui::GetPlatformIO();
  platform.Platform_SetImeDataFn = &IMESupport::SetImeDataFn;

  Logging::LoggerFactory::GetInstance().GetLogger("IMESupport")
      ->Info("IME support installed for HWND {0:p}.", static_cast<void*>(hwnd));
}

void IMESupport::SetImeDataFn(ImGuiContext* /*unused*/, ImGuiViewport* viewport, ImGuiPlatformImeData* data) {
  HWND hwnd = s_imeHwnd ? s_imeHwnd : static_cast<HWND>(viewport->PlatformHandleRaw);
  if (hwnd == nullptr) {
    return;
  }

  if (!data->WantVisible && s_blockNextLButtonUp) {
    return;
  }

  s_imeInputPos = data->InputPos;
  s_imeInputLineHeight = data->InputLineHeight;

  // Translate ImGui's screen coordinates to client coordinates. ImGui reports
  // InputPos in screen space while the IME expects client coordinates.
  POINT input_pos;
  input_pos.x = static_cast<LONG>(data->InputPos.x - viewport->Pos.x);
  input_pos.y = static_cast<LONG>(data->InputPos.y - viewport->Pos.y);

  HIMC context = ImmGetContext(hwnd);
  if (context == nullptr) {
    // SCS games disable IME by default on their window via ImmAssociateContext(hwnd, NULL).
    // When a text input field is focused, we re-associate the default context to enable typing.
    if (data->WantVisible) {
      ImmAssociateContextEx(hwnd, nullptr, IACE_DEFAULT);
      context = ImmGetContext(hwnd);
    }
    if (context == nullptr) {
      return;
    }
  }

  if (data->WantVisible) {
    ImmSetOpenStatus(context, TRUE);

    COMPOSITIONFORM composition_form = {};
    composition_form.dwStyle = CFS_FORCE_POSITION;
    composition_form.ptCurrentPos = input_pos;
    ImmSetCompositionWindow(context, &composition_form);

    CANDIDATEFORM candidate_form = {};
    candidate_form.dwStyle = CFS_CANDIDATEPOS;
    candidate_form.ptCurrentPos = input_pos;
    ImmSetCandidateWindow(context, &candidate_form);

    // Chinese IMEs (e.g. Microsoft Pinyin in TSF/CUAS mode) ignore the position
    // passed to ImmSetCandidateWindow() and instead anchor their candidate
    // window to the system caret (GetCaretPos()). Since ImGui never creates a
    // caret, we create a hidden caret at the ImGui cursor; GetCaretPos() is still
    // reported while the caret's blink is suppressed.
    if (!s_caretCreated) {
      s_caretCreated = CreateCaret(hwnd, nullptr, 1, 1) != 0;
    }
    if (s_caretCreated) {
      SetCaretPos(input_pos.x, input_pos.y + static_cast<LONG>(data->InputLineHeight));
      HideCaret(hwnd);
    }
  } else {
    // Input focus released: close the IME so it does not interfere with the game
    ImmSetOpenStatus(context, FALSE);
  }

  ImmReleaseContext(hwnd, context);

  if (!data->WantVisible && s_caretCreated) {
    if (DestroyCaret()) {
      s_caretCreated = false;
    }
  }
}

bool IMESupport::OnWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_LBUTTONDOWN: {
      ImVec2 mouse_pos(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));

      if (!s_isComposing) {
        break;
      }

      bool hit_window = (mouse_pos.x >= s_windowMin.x && mouse_pos.x <= s_windowMax.x &&
                         mouse_pos.y >= s_windowMin.y && mouse_pos.y <= s_windowMax.y);
      if (!hit_window) {
        break;
      }

      s_pendingAction = -3;
      s_blockNextLButtonUp = true;

      for (const auto& area : s_clickableAreas) {
        if (mouse_pos.x >= area.min.x && mouse_pos.x <= area.max.x &&
            mouse_pos.y >= area.min.y && mouse_pos.y <= area.max.y) {
          s_pendingAction = area.action;
          break;
        }
      }
      return true;
    }
    case WM_LBUTTONUP: {
      if (s_blockNextLButtonUp) {
        s_blockNextLButtonUp = false;

        if (s_pendingAction == -1) {
          SimulateKeyPress(VK_PRIOR);
        } else if (s_pendingAction == -2) {
          SimulateKeyPress(VK_NEXT);
        } else if (s_pendingAction >= 0) {
          size_t local_idx = static_cast<size_t>(s_pendingAction) - s_candidatePageStart;
          if (local_idx < s_candidatePageSize) {
            WORD vk = '1' + static_cast<WORD>(local_idx);
            SimulateKeyPress(vk);
          }
        }
        s_pendingAction = -3;
        return true;
      }
      break;
    }
    case WM_IME_STARTCOMPOSITION: {
      s_isComposing = true;
      s_compositionStrUTF8.clear();
      s_candidatesUTF8.clear();
      s_showCandidates = false;
      break;
    }
    case WM_IME_COMPOSITION: {
      HIMC himc = ImmGetContext(hwnd);
      if (himc) {
        if (lParam & GCS_COMPSTR) {
          LONG size = ImmGetCompositionStringW(himc, GCS_COMPSTR, nullptr, 0);
          if (size >= 0) {
            std::wstring comp(size / sizeof(wchar_t), L'\0');
            ImmGetCompositionStringW(himc, GCS_COMPSTR, &comp[0], size);
            s_compositionStrUTF8 = WideToUTF8(comp);
          }
        }
        ImmReleaseContext(hwnd, himc);
      }
      s_isComposing = true;
      break;
    }
    case WM_IME_ENDCOMPOSITION: {
      s_isComposing = false;
      s_showCandidates = false;
      s_compositionStrUTF8.clear();
      s_candidatesUTF8.clear();
      s_blockNextLButtonUp = false;
      s_pendingAction = -3;
      break;
    }
    case WM_IME_NOTIFY: {
      switch (wParam) {
        case IMN_OPENCANDIDATE:
          s_showCandidates = true;
          UpdateCandidateList(hwnd);
          break;
        case IMN_CLOSECANDIDATE:
          s_showCandidates = false;
          s_candidatesUTF8.clear();
          break;
        case IMN_CHANGECANDIDATE:
          s_showCandidates = true;
          UpdateCandidateList(hwnd);
          break;
      }
      break;
    }
    default:
      break;
  }
  return false;
}

void IMESupport::Render() {
  if (!s_isComposing || !ImGui::GetIO().WantTextInput) {
    return;
  }

  s_clickableAreas.clear();

  ImGui::SetNextWindowPos(ImVec2(s_imeInputPos.x, s_imeInputPos.y + s_imeInputLineHeight + 4.0f));

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                           ImGuiWindowFlags_NoNav | ImGuiWindowFlags_Tooltip;

  if (ImGui::Begin("##IME_Candidate_Window", nullptr, flags)) {
    if (!s_compositionStrUTF8.empty()) {
      ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "%s", s_compositionStrUTF8.c_str());
      if (s_showCandidates && !s_candidatesUTF8.empty()) {
        ImGui::Separator();
      }
    }

    if (s_showCandidates && !s_candidatesUTF8.empty()) {
      size_t start_idx = s_candidatePageStart;
      size_t end_idx = std::min(start_idx + s_candidatePageSize, s_candidatesUTF8.size());
      size_t total_pages = (s_candidatesUTF8.size() + s_candidatePageSize - 1) / s_candidatePageSize;
      size_t current_page = s_candidatePageStart / s_candidatePageSize;
      
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 6.0f));
      
      ImVec2 item_pad(6.0f, 3.0f);
      
      for (size_t i = start_idx; i < end_idx; ++i) {
        if (i > start_idx) {
          ImGui::SameLine(0.0f, 12.0f);
        }
        
        bool is_selected = (i == s_selectedCandidate);
        std::string cand_text = std::to_string((i - start_idx) + 1) + ". " + s_candidatesUTF8[i];
        
        ImVec2 text_size = ImGui::CalcTextSize(cand_text.c_str());
        ImVec2 size(text_size.x + item_pad.x * 2.0f, text_size.y + item_pad.y * 2.0f);
        
        ImGui::Dummy(size);
        
        ImVec2 min_p = ImGui::GetItemRectMin();
        ImVec2 max_p = ImGui::GetItemRectMax();
        
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        bool is_hovered = (mouse_pos.x >= min_p.x && mouse_pos.x <= max_p.x &&
                           mouse_pos.y >= min_p.y && mouse_pos.y <= max_p.y);
        
        ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
        if (is_selected) {
          ImGui::GetWindowDrawList()->AddRectFilled(min_p, max_p, ImGui::GetColorU32(ImGuiCol_HeaderActive), 3.0f);
          text_color = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else if (is_hovered) {
          ImGui::GetWindowDrawList()->AddRectFilled(min_p, max_p, ImGui::GetColorU32(ImGuiCol_HeaderHovered), 3.0f);
          text_color = ImGui::GetColorU32(ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        } else {
          text_color = ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        }
        
        ImGui::GetWindowDrawList()->AddText(ImVec2(min_p.x + item_pad.x, min_p.y + item_pad.y), text_color, cand_text.c_str());
        
        s_clickableAreas.push_back({ min_p, max_p, static_cast<int>(i) });
      }
      
      ImGui::PopStyleVar();
      
      if (total_pages > 1) {
        ImGui::Separator();
        
        bool has_prev = (current_page > 0);
        ImVec2 btn_pad(8.0f, 4.0f);
        ImVec2 prev_size = ImGui::CalcTextSize("<");
        ImVec2 prev_btn_size(prev_size.x + btn_pad.x * 2.0f, prev_size.y + btn_pad.y * 2.0f);
        
        ImGui::Dummy(prev_btn_size);
        ImVec2 prev_min = ImGui::GetItemRectMin();
        ImVec2 prev_max = ImGui::GetItemRectMax();
        
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        bool prev_hovered = has_prev && (mouse_pos.x >= prev_min.x && mouse_pos.x <= prev_max.x &&
                                         mouse_pos.y >= prev_min.y && mouse_pos.y <= prev_max.y);
        
        ImU32 prev_text_color;
        if (has_prev) {
          if (prev_hovered) {
            ImGui::GetWindowDrawList()->AddRectFilled(prev_min, prev_max, ImGui::GetColorU32(ImGuiCol_ButtonHovered), 3.0f);
            prev_text_color = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          } else {
            ImGui::GetWindowDrawList()->AddRectFilled(prev_min, prev_max, ImGui::GetColorU32(ImGuiCol_Button), 3.0f);
            prev_text_color = ImGui::GetColorU32(ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
          }
        } else {
          prev_text_color = ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        }
        
        ImGui::GetWindowDrawList()->AddText(ImVec2(prev_min.x + btn_pad.x, prev_min.y + btn_pad.y), prev_text_color, "<");
        s_clickableAreas.push_back({ prev_min, prev_max, -1 });
        
        ImGui::SameLine(0.0f, 12.0f);
        
        std::string page_str = "Page " + std::to_string(current_page + 1) + "/" + std::to_string(total_pages);
        ImVec2 page_size = ImGui::CalcTextSize(page_str.c_str());
        ImVec2 page_btn_size(page_size.x, page_size.y + btn_pad.y * 2.0f);
        ImGui::Dummy(page_btn_size);
        ImVec2 page_min = ImGui::GetItemRectMin();
        ImGui::GetWindowDrawList()->AddText(ImVec2(page_min.x, page_min.y + btn_pad.y), ImGui::GetColorU32(ImGuiCol_Text), page_str.c_str());
        
        ImGui::SameLine(0.0f, 12.0f);
        
        bool has_next = (current_page + 1 < total_pages);
        ImVec2 next_size = ImGui::CalcTextSize(">");
        ImVec2 next_btn_size(next_size.x + btn_pad.x * 2.0f, next_size.y + btn_pad.y * 2.0f);
        
        ImGui::Dummy(next_btn_size);
        ImVec2 next_min = ImGui::GetItemRectMin();
        ImVec2 next_max = ImGui::GetItemRectMax();
        
        bool next_hovered = has_next && (mouse_pos.x >= next_min.x && mouse_pos.x <= next_max.x &&
                                         mouse_pos.y >= next_min.y && mouse_pos.y <= next_max.y);
        
        ImU32 next_text_color;
        if (has_next) {
          if (next_hovered) {
            ImGui::GetWindowDrawList()->AddRectFilled(next_min, next_max, ImGui::GetColorU32(ImGuiCol_ButtonHovered), 3.0f);
            next_text_color = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
          } else {
            ImGui::GetWindowDrawList()->AddRectFilled(next_min, next_max, ImGui::GetColorU32(ImGuiCol_Button), 3.0f);
            next_text_color = ImGui::GetColorU32(ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
          }
        } else {
          next_text_color = ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        }
        
        ImGui::GetWindowDrawList()->AddText(ImVec2(next_min.x + btn_pad.x, next_min.y + btn_pad.y), next_text_color, ">");
        s_clickableAreas.push_back({ next_min, next_max, -2 });
      }
    }
    s_windowMin = ImGui::GetWindowPos();
    ImVec2 wnd_size = ImGui::GetWindowSize();
    s_windowMax = ImVec2(s_windowMin.x + wnd_size.x, s_windowMin.y + wnd_size.y);
    ImGui::End();
  }

  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar(2);
}

bool IMESupport::IsComposing() {
  return s_isComposing;
}

bool IMESupport::ShowingCandidates() {
  return s_showCandidates;
}

bool IMESupport::IsMouseHoveringWindow() {
  if (!s_isComposing || !s_showCandidates || s_imeHwnd == nullptr) {
    return false;
  }
  POINT pt;
  if (GetCursorPos(&pt)) {
    ScreenToClient(s_imeHwnd, &pt);
    ImVec2 mouse_pos(static_cast<float>(pt.x), static_cast<float>(pt.y));
    return (mouse_pos.x >= s_windowMin.x && mouse_pos.x <= s_windowMax.x &&
            mouse_pos.y >= s_windowMin.y && mouse_pos.y <= s_windowMax.y);
  }
  return false;
}

void IMESupport::PreFrame() {
  if (s_blockNextLButtonUp) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = false;
    io.MouseClicked[0] = false;

    // Filter out mouse button events from the input queue in ImGuiContext to prevent focus loss
    ImGuiContext& g = *GImGui;
    for (int n = 0; n < g.InputEventsQueue.Size; n++) {
      ImGuiInputEvent& e = g.InputEventsQueue[n];
      if (e.Type == ImGuiInputEventType_MouseButton) {
        e.Type = ImGuiInputEventType_None;
      }
    }
  }
}
}  // namespace UI

SPF_NS_END