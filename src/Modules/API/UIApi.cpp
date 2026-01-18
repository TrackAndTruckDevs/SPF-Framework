#include "SPF/Modules/API/UIApi.hpp"
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

void UIApi::FillUIApi(SPF_UI_API* ui_api) {
  if (!ui_api) return;

  // Window Management
  ui_api->RegisterDrawCallback = &UIApi::UI_RegisterDrawCallback;
  ui_api->RegisterDrawCallbackWithFlags = &UIApi::UI_RegisterDrawCallbackWithFlags;
  ui_api->GetWindowHandle = &UIApi::UI_GetWindowHandle;
  ui_api->SetVisibility = &UIApi::UI_SetVisibility;
  ui_api->IsVisible = &UIApi::UI_IsVisible;

  // Basic Widgets
  ui_api->Text = &UIApi::UI_Text;
  ui_api->TextColored = &UIApi::UI_TextColored;
  ui_api->TextDisabled = &UIApi::UI_TextDisabled;
  ui_api->TextWrapped = &UIApi::UI_TextWrapped;
  ui_api->LabelText = &UIApi::UI_LabelText;
  ui_api->BulletText = &UIApi::UI_BulletText;
  ui_api->Button = &UIApi::UI_Button;
  ui_api->SmallButton = &UIApi::UI_SmallButton;
  ui_api->InvisibleButton = &UIApi::UI_InvisibleButton;
  ui_api->Checkbox = &UIApi::UI_Checkbox;
  ui_api->RadioButton = &UIApi::UI_RadioButton;
  ui_api->ProgressBar = &UIApi::UI_ProgressBar;
  ui_api->Bullet = &UIApi::UI_Bullet;
  ui_api->Separator = &UIApi::UI_Separator;
  ui_api->Spacing = &UIApi::UI_Spacing;
  ui_api->Indent = &UIApi::UI_Indent;
  ui_api->Unindent = &UIApi::UI_Unindent;
  ui_api->SameLine = &UIApi::UI_SameLine;
  ui_api->InputText = &UIApi::UI_InputText;
  ui_api->InputInt = &UIApi::UI_InputInt;
  ui_api->InputFloat = &UIApi::UI_InputFloat;
  ui_api->InputDouble = &UIApi::UI_InputDouble;
  ui_api->SliderInt = &UIApi::UI_SliderInt;
  ui_api->SliderFloat = &UIApi::UI_SliderFloat;
  ui_api->PushStyleColor = &UIApi::UI_PushStyleColor;
  ui_api->PopStyleColor = &UIApi::UI_PopStyleColor;
  ui_api->PushStyleVarFloat = &UIApi::UI_PushStyleVarFloat;
  ui_api->PushStyleVarVec2 = &UIApi::UI_PushStyleVarVec2;
  ui_api->PopStyleVar = &UIApi::UI_PopStyleVar;
  ui_api->GetViewportSize = &UIApi::UI_GetViewportSize;
  ui_api->AddRectFilled = &UIApi::UI_AddRectFilled;
  ui_api->BeginCombo = &UIApi::UI_BeginCombo;
  ui_api->EndCombo = &UIApi::UI_EndCombo;
  ui_api->Selectable = &UIApi::UI_Selectable;
  ui_api->TreeNode = &UIApi::UI_TreeNode;
  ui_api->TreePush = &UIApi::UI_TreePush;
  ui_api->TreePop = &UIApi::UI_TreePop;
  ui_api->BeginTabBar = &UIApi::UI_BeginTabBar;
  ui_api->EndTabBar = &UIApi::UI_EndTabBar;
  ui_api->BeginTabItem = &UIApi::UI_BeginTabItem;
  ui_api->EndTabItem = &UIApi::UI_EndTabItem;
  ui_api->BeginTable = &UIApi::UI_BeginTable;
  ui_api->EndTable = &UIApi::UI_EndTable;
  ui_api->TableNextRow = &UIApi::UI_TableNextRow;
  ui_api->TableNextColumn = &UIApi::UI_TableNextColumn;
  ui_api->TableSetupColumn = &UIApi::UI_TableSetupColumn;
  ui_api->OpenPopup = &UIApi::UI_OpenPopup;
  ui_api->BeginPopup = &UIApi::UI_BeginPopup;
  ui_api->EndPopup = &UIApi::UI_EndPopup;
  ui_api->IsItemHovered = &UIApi::UI_IsItemHovered;
  ui_api->IsItemActive = &UIApi::UI_IsItemActive;
  ui_api->SetTooltip = &UIApi::UI_SetTooltip;
  ui_api->InputTextMultiline = &UIApi::UI_InputTextMultiline;
  ui_api->SliderFloat2 = &UIApi::UI_SliderFloat2;
  ui_api->SliderFloat3 = &UIApi::UI_SliderFloat3;
  ui_api->SliderFloat4 = &UIApi::UI_SliderFloat4;
  ui_api->SliderInt2 = &UIApi::UI_SliderInt2;
  ui_api->SliderInt3 = &UIApi::UI_SliderInt3;
  ui_api->SliderInt4 = &UIApi::UI_SliderInt4;
  ui_api->ColorEdit3 = &UIApi::UI_ColorEdit3;
  ui_api->ColorEdit4 = &UIApi::UI_ColorEdit4;
  ui_api->DragFloat = &UIApi::UI_DragFloat;
  ui_api->DragInt = &UIApi::UI_DragInt;

  // --- Text Styling API (v1.0 - SPF-377) ---
  ui_api->Style_Create = &UIApi::UI_Style_Create;
  ui_api->Style_Destroy = &UIApi::UI_Style_Destroy;
  ui_api->Style_SetFont = &UIApi::UI_Style_SetFont;
  ui_api->Style_SetColor = &UIApi::UI_Style_SetColor;
  ui_api->Style_SetAlign = &UIApi::UI_Style_SetAlign;
  ui_api->Style_SetWrap = &UIApi::UI_Style_SetWrap;
  ui_api->Style_SetPadding = &UIApi::UI_Style_SetPadding;
  ui_api->Style_SetSeparator = &UIApi::UI_Style_SetSeparator;
  ui_api->Style_SetUnderline = &UIApi::UI_Style_SetUnderline;
  ui_api->Style_SetStrikethrough = &UIApi::UI_Style_SetStrikethrough;
  ui_api->TextStyled = &UIApi::UI_TextStyled;
  ui_api->RenderMarkdown = &UIApi::UI_RenderMarkdown;

  // --- Custom Widget API (v1.1 - SPF-412) ---
  ui_api->ColorConvertFloat4ToU32 = &UIApi::UI_ColorConvertFloat4ToU32;
  ui_api->GetWindowDrawList = &UIApi::UI_GetWindowDrawList;
  ui_api->DrawList_AddLine = &UIApi::UI_DrawList_AddLine;
  ui_api->DrawList_AddRectFilled = &UIApi::UI_DrawList_AddRectFilled;
  ui_api->DrawList_AddCircleFilled = &UIApi::UI_DrawList_AddCircleFilled;
  ui_api->DrawList_AddText = &UIApi::UI_DrawList_AddText;
  ui_api->DrawList_AddRect = &UIApi::UI_DrawList_AddRect;
  ui_api->DrawList_AddQuadFilled = &UIApi::UI_DrawList_AddQuadFilled;
  ui_api->DrawList_AddTriangleFilled = &UIApi::UI_DrawList_AddTriangleFilled;
  ui_api->DrawList_AddBezierCubic = &UIApi::UI_DrawList_AddBezierCubic;
  ui_api->DrawList_AddPolyline = &UIApi::UI_DrawList_AddPolyline;
  ui_api->DrawList_PathClear = &UIApi::UI_DrawList_PathClear;
  ui_api->DrawList_PathLineTo = &UIApi::UI_DrawList_PathLineTo;
  ui_api->DrawList_PathStroke = &UIApi::UI_DrawList_PathStroke;
  ui_api->DrawList_PathFillConvex = &UIApi::UI_DrawList_PathFillConvex;
  ui_api->GetMousePos = &UIApi::UI_GetMousePos;
  ui_api->IsMouseDragging = &UIApi::UI_IsMouseDragging;
  ui_api->GetMouseDragDelta = &UIApi::UI_GetMouseDragDelta;
  ui_api->IsMouseDown = &UIApi::UI_IsMouseDown;
  ui_api->IsMouseClicked = &UIApi::UI_IsMouseClicked;
  ui_api->IsMouseReleased = &UIApi::UI_IsMouseReleased;
  ui_api->IsMouseDoubleClicked = &UIApi::UI_IsMouseDoubleClicked;
  ui_api->GetMouseWheel = &UIApi::UI_GetMouseWheel;
}
}  // namespace Modules::API
SPF_NS_END