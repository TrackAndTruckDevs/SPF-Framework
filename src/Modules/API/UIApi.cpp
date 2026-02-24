#include <Windows.h>
#include "SPF/Modules/API/UIApi.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Modules/PluginManager.hpp"  // For PluginManager::GetInstance()
#include "SPF/Modules/HandleManager.hpp"  // For HandleManager
#include "SPF/UI/UIManager.hpp"           // For UIManager::GetWindow
#include "SPF/UI/PluginProxyWindow.hpp"   // For PluginProxyWindow
#include "SPF/UI/BaseWindow.hpp"          // For BaseWindow
#include "SPF/Handles/WindowHandle.hpp"   // For WindowHandle
#include "SPF/UI/UITypographyHelper.hpp"
#include <cstdarg>

#include "imgui.h"

// Define the concrete type for the opaque handle.
struct SPF_TextStyle_Handle_t {
    SPF::UI::TextStyle style;
};

SPF_NS_BEGIN
namespace Modules::API {
using namespace SPF::UI;
using namespace SPF::Handles;

// --- UI Window Management Trampolines ---

void UIApi::UI_RegisterDrawCallback(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data) {
  // Call the new extended function with default flags for backward compatibility
  UI_RegisterDrawCallbackWithFlags(pluginName, windowId, drawCallback, user_data, SPF_WINDOW_FLAG_NONE);
}

void UIApi::UI_RegisterDrawCallbackWithFlags(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_Window_Flags flags) {
  auto& self = PluginManager::GetInstance();
  auto* uiManager = self.GetUIManager();
  if (!uiManager || !pluginName || !windowId || !drawCallback) return;

  IWindow* window = uiManager->GetWindow(pluginName, windowId);
  if (auto* proxyWindow = dynamic_cast<PluginProxyWindow*>(window)) {
    proxyWindow->SetDrawCallback(drawCallback, user_data);
    proxyWindow->SetWindowFlags(flags);
  }
}

SPF_Window_Handle* UIApi::UI_GetWindowHandle(const char* pluginName, const char* windowId) {
  auto& self = PluginManager::GetInstance();
  auto* uiManager = self.GetUIManager();
  auto* handleManager = self.GetHandleManager();
  if (!uiManager || !handleManager || !pluginName || !windowId) return nullptr;

  IWindow* window = uiManager->GetWindow(pluginName, windowId);
  if (!window) return nullptr;

  auto handle = std::make_unique<WindowHandle>(window);
  return reinterpret_cast<SPF_Window_Handle*>(handleManager->RegisterHandle(pluginName, std::move(handle)));
}

void UIApi::UI_SetVisibility(SPF_Window_Handle* handle, bool isVisible) {
  auto* windowHandle = reinterpret_cast<WindowHandle*>(handle);
  if (windowHandle && windowHandle->window) {
    if (auto* baseWindow = dynamic_cast<BaseWindow*>(windowHandle->window)) {
      baseWindow->SetVisibility(isVisible);
    }
  }
}

bool UIApi::UI_IsVisible(SPF_Window_Handle* handle) {
  auto* windowHandle = reinterpret_cast<WindowHandle*>(handle);
  if (windowHandle && windowHandle->window) {
    return windowHandle->window->IsVisible();
  }
  return false;
}

// --- UI Builder Trampolines ---

void UIApi::UI_Text(const char* text) {
  if (text) ImGui::TextUnformatted(text);
}
void UIApi::UI_TextColored(float r, float g, float b, float a, const char* text) {
  if (text) ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
}
void UIApi::UI_TextDisabled(const char* text) {
  if (text) ImGui::TextDisabled("%s", text);
}
void UIApi::UI_TextWrapped(const char* text) {
  if (text) ImGui::TextWrapped("%s", text);
}
void UIApi::UI_LabelText(const char* label, const char* text) {
  if (label && text) ImGui::LabelText(label, "%s", text);
}
void UIApi::UI_BulletText(const char* text) {
  if (text) ImGui::BulletText("%s", text);
}
bool UIApi::UI_Button(const char* label, float width, float height) { return label ? ImGui::Button(label, ImVec2(width, height)) : false; }
bool UIApi::UI_SmallButton(const char* label) { return label ? ImGui::SmallButton(label) : false; }
bool UIApi::UI_InvisibleButton(const char* str_id, float width, float height) { return str_id ? ImGui::InvisibleButton(str_id, ImVec2(width, height)) : false; }
bool UIApi::UI_Checkbox(const char* label, bool* v) { return label && v ? ImGui::Checkbox(label, v) : false; }
bool UIApi::UI_RadioButton(const char* label, bool active) { return label ? ImGui::RadioButton(label, active) : false; }
void UIApi::UI_ProgressBar(float fraction, float width, float height, const char* overlay) { ImGui::ProgressBar(fraction, ImVec2(width, height), overlay); }
void UIApi::UI_Bullet() { ImGui::Bullet(); }
void UIApi::UI_Separator() { ImGui::Separator(); }
void UIApi::UI_Spacing() { ImGui::Spacing(); }
void UIApi::UI_Indent(float indent_w) { ImGui::Indent(indent_w); }
void UIApi::UI_Unindent(float indent_w) { ImGui::Unindent(indent_w); }
void UIApi::UI_SameLine(float offset_from_start_x, float spacing) { ImGui::SameLine(offset_from_start_x, spacing); }
bool UIApi::UI_InputText(const char* label, char* buf, size_t buf_size) { return label && buf ? ImGui::InputText(label, buf, buf_size) : false; }
bool UIApi::UI_InputInt(const char* label, int* v, int step, int step_fast, int flags) { return label && v ? ImGui::InputInt(label, v, step, step_fast, flags) : false; }
bool UIApi::UI_InputFloat(const char* label, float* v, float step, float step_fast, const char* format, int flags) {
  return label && v && format ? ImGui::InputFloat(label, v, step, step_fast, format, flags) : false;
}
bool UIApi::UI_InputDouble(const char* label, double* v, double step, double step_fast, const char* format) {
  return label && v && format ? ImGui::InputDouble(label, v, step, step_fast, format) : false;
}
bool UIApi::UI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format) {
  return label && v && format ? ImGui::SliderInt(label, v, v_min, v_max, format) : false;
}
bool UIApi::UI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format) {
  return label && v && format ? ImGui::SliderFloat(label, v, v_min, v_max, format) : false;
}
void UIApi::UI_PushStyleColor(int im_gui_color_idx, float r, float g, float b, float a) { ImGui::PushStyleColor(im_gui_color_idx, ImVec4(r, g, b, a)); }
void UIApi::UI_PopStyleColor(int count) { ImGui::PopStyleColor(count); }
void UIApi::UI_PushStyleVarFloat(int im_gui_stylevar_idx, float val) { ImGui::PushStyleVar(im_gui_stylevar_idx, val); }
void UIApi::UI_PushStyleVarVec2(int im_gui_stylevar_idx, float val_x, float val_y) { ImGui::PushStyleVar(im_gui_stylevar_idx, ImVec2(val_x, val_y)); }
void UIApi::UI_PopStyleVar(int count) { ImGui::PopStyleVar(count); }

void UIApi::UI_GetViewportSize(float* out_width, float* out_height) {
  if (out_width && out_height) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    *out_width = viewport->Size.x;
    *out_height = viewport->Size.y;
  }
}

void UIApi::UI_AddRectFilled(float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  if (drawList) {
    drawList->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a)));
  }
}

bool UIApi::UI_BeginCombo(const char* label, const char* preview_value) { return label && preview_value ? ImGui::BeginCombo(label, preview_value) : false; }
void UIApi::UI_EndCombo() { ImGui::EndCombo(); }
bool UIApi::UI_Selectable(const char* label, bool selected) { return label ? ImGui::Selectable(label, selected) : false; }

bool UIApi::UI_TreeNode(const char* label) { return label ? ImGui::TreeNode(label) : false; }
void UIApi::UI_TreePush(const char* str_id) {
  if (str_id) ImGui::TreePush(str_id);
}
void UIApi::UI_TreePop() { ImGui::TreePop(); }

bool UIApi::UI_BeginTabBar(const char* str_id) { return str_id ? ImGui::BeginTabBar(str_id, ImGuiTabBarFlags_None) : false; }
void UIApi::UI_EndTabBar() { ImGui::EndTabBar(); }
bool UIApi::UI_BeginTabItem(const char* label) { return label ? ImGui::BeginTabItem(label) : false; }
void UIApi::UI_EndTabItem() { ImGui::EndTabItem(); }

bool UIApi::UI_BeginTable(const char* str_id, int column) { return str_id ? ImGui::BeginTable(str_id, column) : false; }
void UIApi::UI_EndTable() { ImGui::EndTable(); }
void UIApi::UI_TableNextRow() { ImGui::TableNextRow(); }
bool UIApi::UI_TableNextColumn() { return ImGui::TableNextColumn(); }
void UIApi::UI_TableSetupColumn(const char* label) {
  if (label) ImGui::TableSetupColumn(label);
}

void UIApi::UI_OpenPopup(const char* str_id) {
  if (str_id) ImGui::OpenPopup(str_id);
}
bool UIApi::UI_BeginPopup(const char* str_id) { return str_id ? ImGui::BeginPopup(str_id) : false; }
void UIApi::UI_EndPopup() { ImGui::EndPopup(); }
bool UIApi::UI_IsItemHovered() { return ImGui::IsItemHovered(); }
bool UIApi::UI_IsItemActive() { return ImGui::IsItemActive(); }
void UIApi::UI_SetTooltip(const char* text) {
  if (text) ImGui::SetTooltip("%s", text);
}

bool UIApi::UI_InputTextMultiline(const char* label, char* buf, size_t buf_size) { return label && buf ? ImGui::InputTextMultiline(label, buf, buf_size) : false; }
bool UIApi::UI_SliderFloat2(const char* label, float v[2], float v_min, float v_max) { return label && v ? ImGui::SliderFloat2(label, v, v_min, v_max) : false; }
bool UIApi::UI_SliderFloat3(const char* label, float v[3], float v_min, float v_max) { return label && v ? ImGui::SliderFloat3(label, v, v_min, v_max) : false; }
bool UIApi::UI_SliderFloat4(const char* label, float v[4], float v_min, float v_max) { return label && v ? ImGui::SliderFloat4(label, v, v_min, v_max) : false; }
bool UIApi::UI_SliderInt2(const char* label, int v[2], int v_min, int v_max) { return label && v ? ImGui::SliderInt2(label, v, v_min, v_max) : false; }
bool UIApi::UI_SliderInt3(const char* label, int v[3], int v_min, int v_max) { return label && v ? ImGui::SliderInt3(label, v, v_min, v_max) : false; }
bool UIApi::UI_SliderInt4(const char* label, int v[4], int v_min, int v_max) { return label && v ? ImGui::SliderInt4(label, v, v_min, v_max) : false; }
bool UIApi::UI_ColorEdit3(const char* label, float col[3]) { return label && col ? ImGui::ColorEdit3(label, col) : false; }
bool UIApi::UI_ColorEdit4(const char* label, float col[4]) { return label && col ? ImGui::ColorEdit4(label, col) : false; }
bool UIApi::UI_DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max) { return label && v ? ImGui::DragFloat(label, v, v_speed, v_min, v_max) : false; }
bool UIApi::UI_DragInt(const char* label, int* v, float v_speed, int v_min, int v_max) { return label && v ? ImGui::DragInt(label, v, v_speed, v_min, v_max) : false; }

// --- Text Styling API Implementation ---

SPF_TextStyle_Handle UIApi::UI_Style_Create() {
    return new SPF_TextStyle_Handle_t();
}

void UIApi::UI_Style_Destroy(SPF_TextStyle_Handle handle) {
    delete handle;
}

void UIApi::UI_Style_SetFont(SPF_TextStyle_Handle handle, SPF_Font font) {
    if (!handle) return;
    const char* fontKey = "regular"; // Default
    switch (font) {
        case SPF_FONT_REGULAR:       fontKey = "regular"; break;
        case SPF_FONT_BOLD:          fontKey = "bold"; break;
        case SPF_FONT_ITALIC:        fontKey = "italic"; break;
        case SPF_FONT_BOLD_ITALIC:   fontKey = "bold_italic"; break;
        case SPF_FONT_MEDIUM:        fontKey = "medium"; break;
        case SPF_FONT_MEDIUM_ITALIC: fontKey = "medium_italic"; break;
        case SPF_FONT_MONOSPACE:     fontKey = "monospace"; break;
        case SPF_FONT_H1:            fontKey = "h1"; break;
        case SPF_FONT_H2:            fontKey = "h2"; break;
        case SPF_FONT_H3:            fontKey = "h3"; break;
    }
    handle->style.Font(fontKey);
}

void UIApi::UI_Style_SetColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a) {
    if (handle) handle->style.Color(ImVec4(r, g, b, a));
}

void UIApi::UI_Style_SetAlign(SPF_TextStyle_Handle handle, SPF_TextAlign align) {
    if (!handle) return;
    // The C++ enum has the same values as the C enum.
    handle->style.Align(static_cast<SPF::UI::TextAlign>(align));
}

void UIApi::UI_Style_SetWrap(SPF_TextStyle_Handle handle, bool wrap) {
    if (handle) handle->style.Wrapped(wrap);
}

void UIApi::UI_Style_SetPadding(SPF_TextStyle_Handle handle, float pad_x, float pad_y) {
    if (handle) handle->style.Padding({pad_x, pad_y});
}

void UIApi::UI_Style_SetSeparator(SPF_TextStyle_Handle handle, bool is_separator) {
    if (handle) handle->style.Separator(is_separator);
}

void UIApi::UI_Style_SetUnderline(SPF_TextStyle_Handle handle, bool is_underline) {
    if (handle) handle->style.Underline(is_underline);
}

void UIApi::UI_Style_SetStrikethrough(SPF_TextStyle_Handle handle, bool is_strikethrough) {
    if (handle) handle->style.Strikethrough(is_strikethrough);
}

void UIApi::UI_TextStyled(SPF_TextStyle_Handle handle, const char* fmt, ...) {
    if (!fmt) return;
    
    va_list args;
    va_start(args, fmt);

    if (handle) {
        SPF::UI::Typography::TextV(handle->style, fmt, args);
    } else {
        // Fallback to default if no style handle is provided.
        SPF::UI::Typography::TextV(SPF::UI::TextStyle::Regular(), fmt, args);
    }
    
    va_end(args);
}

void UIApi::UI_RenderMarkdown(const char* markdown_text, SPF_TextStyle_Handle base_style_handle) {
    if (!markdown_text) return;
    
    if (base_style_handle) {
        SPF::UI::Typography::RenderMarkdownText(markdown_text, base_style_handle->style);
    } else {
        SPF::UI::Typography::RenderMarkdownText(markdown_text, SPF::UI::TextStyle::Regular());
    }
}

// --- Custom Widget API Implementation ---

uint32_t UIApi::UI_ColorConvertFloat4ToU32(float r, float g, float b, float a) {
    return ImGui::ColorConvertFloat4ToU32({r, g, b, a});
}

SPF_DrawList_Handle UIApi::UI_GetWindowDrawList() {
    return reinterpret_cast<SPF_DrawList_Handle>(ImGui::GetWindowDrawList());
}

void UIApi::UI_DrawList_AddLine(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->AddLine({p1_x, p1_y}, {p2_x, p2_y}, col, thickness);
}

void UIApi::UI_DrawList_AddRectFilled(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->AddRectFilled({p_min_x, p_min_y}, {p_max_x, p_max_y}, col, rounding);
}

void UIApi::UI_DrawList_AddCircleFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->AddCircleFilled({center_x, center_y}, radius, col, num_segments);
}

void UIApi::UI_DrawList_AddText(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text) {
    if (dl && text) reinterpret_cast<ImDrawList*>(dl)->AddText({pos_x, pos_y}, col, text);
}

void UIApi::UI_DrawList_AddRect(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, float thickness) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->AddRect({p_min_x, p_min_y}, {p_max_x, p_max_y}, col, rounding, 0, thickness);
}

void UIApi::UI_DrawList_AddQuadFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->AddQuadFilled({p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, {p4_x, p4_y}, col);
}

void UIApi::UI_DrawList_AddTriangleFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->AddTriangleFilled({p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, col);
}

void UIApi::UI_DrawList_AddBezierCubic(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col, float thickness, int num_segments) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->AddBezierCubic({p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, {p4_x, p4_y}, col, thickness, num_segments);
}

void UIApi::UI_DrawList_AddPolyline(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col, bool closed, float thickness) {
    if (!dl || !points_x || !points_y || num_points <= 1) return;

    // ImGui takes an array of ImVec2, so we need to construct it from our separate x/y arrays.
    // Using a temporary std::vector is a safe way to handle the allocation.
    std::vector<ImVec2> points;
    points.reserve(num_points);
    for (int i = 0; i < num_points; ++i) {
        points.emplace_back(points_x[i], points_y[i]);
    }

    reinterpret_cast<ImDrawList*>(dl)->AddPolyline(points.data(), num_points, col, closed, thickness);
}

void UIApi::UI_DrawList_PathClear(SPF_DrawList_Handle dl) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->PathClear();
}

void UIApi::UI_DrawList_PathLineTo(SPF_DrawList_Handle dl, float pos_x, float pos_y) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->PathLineTo({pos_x, pos_y});
}

void UIApi::UI_DrawList_PathStroke(SPF_DrawList_Handle dl, uint32_t col, bool closed, float thickness) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->PathStroke(col, closed, thickness);
}

void UIApi::UI_DrawList_PathFillConvex(SPF_DrawList_Handle dl, uint32_t col) {
    if (dl) reinterpret_cast<ImDrawList*>(dl)->PathFillConvex(col);
}

void UIApi::UI_GetMousePos(float* out_x, float* out_y) {
    if (out_x && out_y) {
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        *out_x = mouse_pos.x;
        *out_y = mouse_pos.y;
    }
}

bool UIApi::UI_IsMouseDragging(int mouse_button_index) {
    return ImGui::IsMouseDragging(mouse_button_index);
}

void UIApi::UI_GetMouseDragDelta(int mouse_button_index, float* out_dx, float* out_dy) {
    if (out_dx && out_dy) {
        const ImVec2 drag_delta = ImGui::GetMouseDragDelta(mouse_button_index);
        *out_dx = drag_delta.x;
        *out_dy = drag_delta.y;
    }
}

bool UIApi::UI_IsMouseDown(int mouse_button_index) {
    return ImGui::IsMouseDown(mouse_button_index);
}

bool UIApi::UI_IsMouseClicked(int mouse_button_index) {
    return ImGui::IsMouseClicked(mouse_button_index);
}

bool UIApi::UI_IsMouseReleased(int mouse_button_index) {
    return ImGui::IsMouseReleased(mouse_button_index);
}

bool UIApi::UI_IsMouseDoubleClicked(int mouse_button_index) {
    return ImGui::IsMouseDoubleClicked(mouse_button_index);
}

float UIApi::UI_GetMouseWheel() {
  return ImGui::GetIO().MouseWheel;
}

void UIApi::UI_SetMouseBlockState(bool blockAxes, bool blockButtons, bool blockWheel) {
    Input::InputManager::GetInstance().SetProgrammaticMouseBlock(blockAxes, blockButtons, blockWheel);
}

void UIApi::UI_GetContentRegionAvail(float* out_x, float* out_y) {

    if (out_x && out_y) {
        const ImVec2 content_region = ImGui::GetContentRegionAvail();
        *out_x = content_region.x;
        *out_y = content_region.y;
    }
}

void UIApi::UI_GetWindowPos(float* out_x, float* out_y) {
    if (out_x && out_y) {
        const ImVec2 pos = ImGui::GetWindowPos();
        *out_x = pos.x;
        *out_y = pos.y;
    }
}

void UIApi::UI_GetWindowSize(float* out_x, float* out_y) {
    if (out_x && out_y) {
        const ImVec2 size = ImGui::GetWindowSize();
        *out_x = size.x;
        *out_y = size.y;
    }
}

void UIApi::UI_GetCursorScreenPos(float* out_x, float* out_y) {
    if (out_x && out_y) {
        const ImVec2 pos = ImGui::GetCursorScreenPos();
        *out_x = pos.x;
        *out_y = pos.y;
    }
}

void UIApi::UI_SetCursorScreenPos(float x, float y) {
    ImGui::SetCursorScreenPos({x, y});
}

void UIApi::UI_GetItemRectMin(float* out_x, float* out_y) {
    if (out_x && out_y) {
        const ImVec2 min = ImGui::GetItemRectMin();
        *out_x = min.x;
        *out_y = min.y;
    }
}

void UIApi::UI_GetItemRectMax(float* out_x, float* out_y) {
    if (out_x && out_y) {
        const ImVec2 max = ImGui::GetItemRectMax();
        *out_x = max.x;
        *out_y = max.y;
    }
}

void UIApi::UI_GetItemRectSize(float* out_x, float* out_y) {
    if (out_x && out_y) {
        const ImVec2 size = ImGui::GetItemRectSize();
        *out_x = size.x;
        *out_y = size.y;
    }
}

// --- Miscellaneous Utilities API Implementation ---

const char* UIApi::UI_GetClipboardText() {
    return ImGui::GetClipboardText();
}

void UIApi::UI_SetClipboardText(const char* text) {
    ImGui::SetClipboardText(text);
}

SPF_Font_Handle* UIApi::UI_GetFont(const char* font_key) {
    if (!font_key) return nullptr;
    auto& self = PluginManager::GetInstance();
    auto* uiManager = self.GetUIManager();
    if (!uiManager) return nullptr;
    return reinterpret_cast<SPF_Font_Handle*>(uiManager->GetFont(font_key));
}

void UIApi::UI_PushFont(SPF_Font_Handle* font_handle) {
    if (font_handle) ImGui::PushFont(reinterpret_cast<ImFont*>(font_handle));
}

void UIApi::UI_PopFont() {
    ImGui::PopFont();
}

// --- Global Style Access API Implementation ---

SPF_Style_Handle* UIApi::UI_GetStyle() {
    return reinterpret_cast<SPF_Style_Handle*>(&ImGui::GetStyle());
}

void UIApi::UI_Style_GetWindowPadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y) {
    if (style_handle && out_x && out_y) {
        *out_x = reinterpret_cast<ImGuiStyle*>(style_handle)->WindowPadding.x;
        *out_y = reinterpret_cast<ImGuiStyle*>(style_handle)->WindowPadding.y;
    }
}

void UIApi::UI_Style_GetItemSpacing(SPF_Style_Handle* style_handle, float* out_x, float* out_y) {
    if (style_handle && out_x && out_y) {
        *out_x = reinterpret_cast<ImGuiStyle*>(style_handle)->ItemSpacing.x;
        *out_y = reinterpret_cast<ImGuiStyle*>(style_handle)->ItemSpacing.y;
    }
}

void UIApi::UI_Style_GetFramePadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y) {
    if (style_handle && out_x && out_y) {
        *out_x = reinterpret_cast<ImGuiStyle*>(style_handle)->FramePadding.x;
        *out_y = reinterpret_cast<ImGuiStyle*>(style_handle)->FramePadding.y;
    }
}

// --- ID Management API Implementation ---

void UIApi::UI_PushID_Str(const char* str_id) {
    ImGui::PushID(str_id);
}

void UIApi::UI_PushID_Int(int int_id) {
    ImGui::PushID(int_id);
}

void UIApi::UI_PushID_Ptr(void* ptr_id) {
    ImGui::PushID(ptr_id);
}

void UIApi::UI_PopID() {
    ImGui::PopID();
}

uint32_t UIApi::UI_GetID_Str(const char* str_id) {
    return ImGui::GetID(str_id);
}

// --- Drag and Drop API Implementation ---

bool UIApi::UI_BeginDragDropSource() {
    return ImGui::BeginDragDropSource(ImGuiDragDropFlags_None);
}

bool UIApi::UI_SetDragDropPayload(const char* type, const void* data, size_t size) {
    return ImGui::SetDragDropPayload(type, data, size);
}

void UIApi::UI_EndDragDropSource() {
    ImGui::EndDragDropSource();
}

bool UIApi::UI_BeginDragDropTarget() {
    return ImGui::BeginDragDropTarget();
}

const SPF_Payload_Handle* UIApi::UI_AcceptDragDropPayload(const char* type) {
    return reinterpret_cast<const SPF_Payload_Handle*>(ImGui::AcceptDragDropPayload(type));
}

void UIApi::UI_EndDragDropTarget() {
    ImGui::EndDragDropTarget();
}

void UIApi::UI_ShowNotification(SPF_NotificationType type, const char* message) {
    if (!message) return;
    UIManager::GetInstance().ShowNotification(message, static_cast<int>(type));
}

void UIApi::FillUIApi(SPF_UI_API* ui_api) {
  if (!ui_api) return;

  // Window Management
  ui_api->UI_RegisterDrawCallback = &UIApi::UI_RegisterDrawCallback;
  ui_api->UI_RegisterDrawCallbackWithFlags = &UIApi::UI_RegisterDrawCallbackWithFlags;
  ui_api->UI_GetWindowHandle = &UIApi::UI_GetWindowHandle;
  ui_api->UI_SetVisibility = &UIApi::UI_SetVisibility;
  ui_api->UI_IsVisible = &UIApi::UI_IsVisible;

  // Basic Widgets
  ui_api->UI_Text = &UIApi::UI_Text;
  ui_api->UI_TextColored = &UIApi::UI_TextColored;
  ui_api->UI_TextDisabled = &UIApi::UI_TextDisabled;
  ui_api->UI_TextWrapped = &UIApi::UI_TextWrapped;
  ui_api->UI_LabelText = &UIApi::UI_LabelText;
  ui_api->UI_BulletText = &UIApi::UI_BulletText;
  ui_api->UI_Button = &UIApi::UI_Button;
  ui_api->UI_SmallButton = &UIApi::UI_SmallButton;
  ui_api->UI_InvisibleButton = &UIApi::UI_InvisibleButton;
  ui_api->UI_Checkbox = &UIApi::UI_Checkbox;
  ui_api->UI_RadioButton = &UIApi::UI_RadioButton;
  ui_api->UI_ProgressBar = &UIApi::UI_ProgressBar;
  ui_api->UI_Bullet = &UIApi::UI_Bullet;
  ui_api->UI_Separator = &UIApi::UI_Separator;
  ui_api->UI_Spacing = &UIApi::UI_Spacing;
  ui_api->UI_Indent = &UIApi::UI_Indent;
  ui_api->UI_Unindent = &UIApi::UI_Unindent;
  ui_api->UI_SameLine = &UIApi::UI_SameLine;
  ui_api->UI_InputText = &UIApi::UI_InputText;
  ui_api->UI_InputInt = &UIApi::UI_InputInt;
  ui_api->UI_InputFloat = &UIApi::UI_InputFloat;
  ui_api->UI_InputDouble = &UIApi::UI_InputDouble;
  ui_api->UI_SliderInt = &UIApi::UI_SliderInt;
  ui_api->UI_SliderFloat = &UIApi::UI_SliderFloat;
  ui_api->UI_PushStyleColor = &UIApi::UI_PushStyleColor;
  ui_api->UI_PopStyleColor = &UIApi::UI_PopStyleColor;
  ui_api->UI_PushStyleVarFloat = &UIApi::UI_PushStyleVarFloat;
  ui_api->UI_PushStyleVarVec2 = &UIApi::UI_PushStyleVarVec2;
  ui_api->UI_PopStyleVar = &UIApi::UI_PopStyleVar;
  ui_api->UI_GetViewportSize = &UIApi::UI_GetViewportSize;
  ui_api->UI_AddRectFilled = &UIApi::UI_AddRectFilled;
  ui_api->UI_BeginCombo = &UIApi::UI_BeginCombo;
  ui_api->UI_EndCombo = &UIApi::UI_EndCombo;
  ui_api->UI_Selectable = &UIApi::UI_Selectable;
  ui_api->UI_TreeNode = &UIApi::UI_TreeNode;
  ui_api->UI_TreePush = &UIApi::UI_TreePush;
  ui_api->UI_TreePop = &UIApi::UI_TreePop;
  ui_api->UI_BeginTabBar = &UIApi::UI_BeginTabBar;
  ui_api->UI_EndTabBar = &UIApi::UI_EndTabBar;
  ui_api->UI_BeginTabItem = &UIApi::UI_BeginTabItem;
  ui_api->UI_EndTabItem = &UIApi::UI_EndTabItem;
  ui_api->UI_BeginTable = &UIApi::UI_BeginTable;
  ui_api->UI_EndTable = &UIApi::UI_EndTable;
  ui_api->UI_TableNextRow = &UIApi::UI_TableNextRow;
  ui_api->UI_TableNextColumn = &UIApi::UI_TableNextColumn;
  ui_api->UI_TableSetupColumn = &UIApi::UI_TableSetupColumn;
  ui_api->UI_OpenPopup = &UIApi::UI_OpenPopup;
  ui_api->UI_BeginPopup = &UIApi::UI_BeginPopup;
  ui_api->UI_EndPopup = &UIApi::UI_EndPopup;
  ui_api->UI_IsItemHovered = &UIApi::UI_IsItemHovered;
  ui_api->UI_IsItemActive = &UIApi::UI_IsItemActive;
  ui_api->UI_SetTooltip = &UIApi::UI_SetTooltip;
  ui_api->UI_InputTextMultiline = &UIApi::UI_InputTextMultiline;
  ui_api->UI_SliderFloat2 = &UIApi::UI_SliderFloat2;
  ui_api->UI_SliderFloat3 = &UIApi::UI_SliderFloat3;
  ui_api->UI_SliderFloat4 = &UIApi::UI_SliderFloat4;
  ui_api->UI_SliderInt2 = &UIApi::UI_SliderInt2;
  ui_api->UI_SliderInt3 = &UIApi::UI_SliderInt3;
  ui_api->UI_SliderInt4 = &UIApi::UI_SliderInt4;
  ui_api->UI_ColorEdit3 = &UIApi::UI_ColorEdit3;
  ui_api->UI_ColorEdit4 = &UIApi::UI_ColorEdit4;
  ui_api->UI_DragFloat = &UIApi::UI_DragFloat;
  ui_api->UI_DragInt = &UIApi::UI_DragInt;

  // --- Text Styling API (v1.0 - SPF-377) ---
  ui_api->UI_Style_Create = &UIApi::UI_Style_Create;
  ui_api->UI_Style_Destroy = &UIApi::UI_Style_Destroy;
  ui_api->UI_Style_SetFont = &UIApi::UI_Style_SetFont;
  ui_api->UI_Style_SetColor = &UIApi::UI_Style_SetColor;
  ui_api->UI_Style_SetAlign = &UIApi::UI_Style_SetAlign;
  ui_api->UI_Style_SetWrap = &UIApi::UI_Style_SetWrap;
  ui_api->UI_Style_SetPadding = &UIApi::UI_Style_SetPadding;
  ui_api->UI_Style_SetSeparator = &UIApi::UI_Style_SetSeparator;
  ui_api->UI_Style_SetUnderline = &UIApi::UI_Style_SetUnderline;
  ui_api->UI_Style_SetStrikethrough = &UIApi::UI_Style_SetStrikethrough;
  ui_api->UI_TextStyled = &UIApi::UI_TextStyled;
  ui_api->UI_RenderMarkdown = &UIApi::UI_RenderMarkdown;

  // --- Custom Widget API (v1.1 - SPF-412) ---
  ui_api->UI_ColorConvertFloat4ToU32 = &UIApi::UI_ColorConvertFloat4ToU32;
  ui_api->UI_GetWindowDrawList = &UIApi::UI_GetWindowDrawList;
  ui_api->UI_DrawList_AddLine = &UIApi::UI_DrawList_AddLine;
  ui_api->UI_DrawList_AddRectFilled = &UIApi::UI_DrawList_AddRectFilled;
  ui_api->UI_DrawList_AddCircleFilled = &UIApi::UI_DrawList_AddCircleFilled;
  ui_api->UI_DrawList_AddText = &UIApi::UI_DrawList_AddText;
  ui_api->UI_DrawList_AddRect = &UIApi::UI_DrawList_AddRect;
  ui_api->UI_DrawList_AddQuadFilled = &UIApi::UI_DrawList_AddQuadFilled;
  ui_api->UI_DrawList_AddTriangleFilled = &UIApi::UI_DrawList_AddTriangleFilled;
  ui_api->UI_DrawList_AddBezierCubic = &UIApi::UI_DrawList_AddBezierCubic;
  ui_api->UI_DrawList_AddPolyline = &UIApi::UI_DrawList_AddPolyline;
  ui_api->UI_DrawList_PathClear = &UIApi::UI_DrawList_PathClear;
  ui_api->UI_DrawList_PathLineTo = &UIApi::UI_DrawList_PathLineTo;
  ui_api->UI_DrawList_PathStroke = &UIApi::UI_DrawList_PathStroke;
  ui_api->UI_DrawList_PathFillConvex = &UIApi::UI_DrawList_PathFillConvex;
  ui_api->UI_GetMousePos = &UIApi::UI_GetMousePos;
  ui_api->UI_IsMouseDragging = &UIApi::UI_IsMouseDragging;
  ui_api->UI_GetMouseDragDelta = &UIApi::UI_GetMouseDragDelta;
  ui_api->UI_IsMouseDown = &UIApi::UI_IsMouseDown;
  ui_api->UI_IsMouseClicked = &UIApi::UI_IsMouseClicked;
  ui_api->UI_IsMouseReleased = &UIApi::UI_IsMouseReleased;
  ui_api->UI_IsMouseDoubleClicked = &UIApi::UI_IsMouseDoubleClicked;
  ui_api->UI_GetMouseWheel = &UIApi::UI_GetMouseWheel;
  ui_api->UI_SetMouseBlockState = &UIApi::UI_SetMouseBlockState;

  ui_api->UI_GetContentRegionAvail = &UIApi::UI_GetContentRegionAvail;
  ui_api->UI_GetWindowPos = &UIApi::UI_GetWindowPos;
  ui_api->UI_GetWindowSize = &UIApi::UI_GetWindowSize;
  ui_api->UI_GetCursorScreenPos = &UIApi::UI_GetCursorScreenPos;
  ui_api->UI_SetCursorScreenPos = &UIApi::UI_SetCursorScreenPos;
  ui_api->UI_GetItemRectMin = &UIApi::UI_GetItemRectMin;
  ui_api->UI_GetItemRectMax = &UIApi::UI_GetItemRectMax;
  ui_api->UI_GetItemRectSize = &UIApi::UI_GetItemRectSize;

  // --- Miscellaneous Utilities API ---
  ui_api->UI_GetClipboardText = &UIApi::UI_GetClipboardText;
  ui_api->UI_SetClipboardText = &UIApi::UI_SetClipboardText;
  ui_api->UI_GetFont = &UIApi::UI_GetFont;
  ui_api->UI_PushFont = &UIApi::UI_PushFont;
  ui_api->UI_PopFont = &UIApi::UI_PopFont;
  ui_api->UI_GetStyle = &UIApi::UI_GetStyle;
  ui_api->UI_Style_GetWindowPadding = &UIApi::UI_Style_GetWindowPadding;
  ui_api->UI_Style_GetItemSpacing = &UIApi::UI_Style_GetItemSpacing;
  ui_api->UI_Style_GetFramePadding = &UIApi::UI_Style_GetFramePadding;
  ui_api->UI_PushID_Str = &UIApi::UI_PushID_Str;
  ui_api->UI_PushID_Int = &UIApi::UI_PushID_Int;
  ui_api->UI_PushID_Ptr = &UIApi::UI_PushID_Ptr;
  ui_api->UI_PopID = &UIApi::UI_PopID;
  ui_api->UI_GetID_Str = &UIApi::UI_GetID_Str;

  // --- Drag and Drop API ---
  ui_api->UI_BeginDragDropSource = &UIApi::UI_BeginDragDropSource;
  ui_api->UI_SetDragDropPayload = &UIApi::UI_SetDragDropPayload;
  ui_api->UI_EndDragDropSource = &UIApi::UI_EndDragDropSource;
  ui_api->UI_BeginDragDropTarget = &UIApi::UI_BeginDragDropTarget;
  ui_api->UI_AcceptDragDropPayload = &UIApi::UI_AcceptDragDropPayload;
  ui_api->UI_EndDragDropTarget = &UIApi::UI_EndDragDropTarget;

  // Notifications
  ui_api->UI_ShowNotification = &UIApi::UI_ShowNotification;
}
}  // namespace Modules::API
SPF_NS_END