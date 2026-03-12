#include <Windows.h>
#include "SPF/Modules/API/UIApi.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/PluginProxyWindow.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Handles/WindowHandle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/UIElements.hpp"
#include <cstdarg>
#include <vector>
#include <cmath>

#include "imgui.h"
#include "imgui_internal.h"

// Define the concrete type for the opaque handle.
struct SPF_TextStyle_Handle_t {
    SPF::UI::TextStyle style;
};

SPF_NS_BEGIN
namespace Modules::API {
using namespace SPF::UI;
using namespace SPF::Handles;

// --- I. Plugin Registration & Window Lifecycle ---

void UIApi::UI_RegisterDrawCallback(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data) {
    UI_RegisterDrawCallbackWithFlags(pluginName, windowId, drawCallback, user_data, SPF_WINDOW_FLAG_NONE);
}

void UIApi::UI_RegisterDrawCallbackWithFlags(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_WindowFlags flags) {
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
    return (windowHandle && windowHandle->window) ? windowHandle->window->IsVisible() : false;
}

void UIApi::UI_SetFocus(SPF_Window_Handle* handle) {
    auto* windowHandle = reinterpret_cast<WindowHandle*>(handle);
    if (windowHandle && windowHandle->window) {
        if (auto* baseWindow = dynamic_cast<BaseWindow*>(windowHandle->window)) {
            baseWindow->Focus();
        }
    }
}

// --- II. Main Loop, Context & IO ---

void* UIApi::UI_GetCurrentContext() { return (void*)ImGui::GetCurrentContext(); }
void UIApi::UI_SetCurrentContext(void* ctx) { ImGui::SetCurrentContext((ImGuiContext*)ctx); }

void UIApi::UI_SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data) {
    ImGui::SetAllocatorFunctions(alloc_func, free_func, user_data);
}

void* UIApi::UI_MemAlloc(size_t sz) { return ImGui::MemAlloc(sz); }
void UIApi::UI_MemFree(void* ptr) { ImGui::MemFree(ptr); }

float UIApi::UI_GetDeltaTime() { return ImGui::GetIO().DeltaTime; }
double UIApi::UI_GetTime() { return ImGui::GetTime(); }
int UIApi::UI_GetFrameCount() { return ImGui::GetFrameCount(); }
float UIApi::UI_GetFramerate() { return ImGui::GetIO().Framerate; }
int UIApi::UI_GetIO_ConfigFlags() { return (int)ImGui::GetIO().ConfigFlags; }
int UIApi::UI_GetIO_BackendFlags() { return (int)ImGui::GetIO().BackendFlags; }

void UIApi::UI_GetMousePos(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetMousePos();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

bool UIApi::UI_IsMouseDown(SPF_MouseButton button) { return ImGui::IsMouseDown((ImGuiMouseButton)button); }
bool UIApi::UI_IsMouseClicked(SPF_MouseButton button) { return ImGui::IsMouseClicked((ImGuiMouseButton)button); }
bool UIApi::UI_IsMouseReleased(SPF_MouseButton button) { return ImGui::IsMouseReleased((ImGuiMouseButton)button); }
bool UIApi::UI_IsMouseDoubleClicked(SPF_MouseButton button) { return ImGui::IsMouseDoubleClicked((ImGuiMouseButton)button); }

bool UIApi::UI_IsMouseHoveringRect(float min_x, float min_y, float max_x, float max_y, bool clip) {
    return ImGui::IsMouseHoveringRect({min_x, min_y}, {max_x, max_y}, clip);
}

bool UIApi::UI_IsMousePosValid() { return ImGui::IsMousePosValid(); }

void UIApi::UI_GetMousePosOnOpeningCurrentPopup(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetMousePosOnOpeningCurrentPopup();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

float UIApi::UI_GetMouseWheel() { return ImGui::GetIO().MouseWheel; }
float UIApi::UI_GetMouseWheelH() { return ImGui::GetIO().MouseWheelH; }

bool UIApi::UI_Shortcut(int key_chord, SPF_InputFlags flags) { return ImGui::Shortcut((ImGuiKeyChord)key_chord, (ImGuiInputFlags)flags); }
void UIApi::UI_SetNextItemShortcut(int key_chord, SPF_InputFlags flags) { ImGui::SetNextItemShortcut((ImGuiKeyChord)key_chord, (ImGuiInputFlags)flags); }
void UIApi::UI_SetItemKeyOwner(SPF_Key key) { ImGui::SetItemKeyOwner((ImGuiKey)key); }

bool UIApi::UI_IsMouseDragging(SPF_MouseButton button) { return ImGui::IsMouseDragging((ImGuiMouseButton)button); }

void UIApi::UI_GetMouseDragDelta(SPF_MouseButton button, float* out_dx, float* out_dy) {
    ImVec2 delta = ImGui::GetMouseDragDelta((ImGuiMouseButton)button);
    if (out_dx) *out_dx = delta.x;
    if (out_dy) *out_dy = delta.y;
}

void UIApi::UI_ResetMouseDragDelta(SPF_MouseButton button) { ImGui::ResetMouseDragDelta((ImGuiMouseButton)button); }
void UIApi::UI_SetMouseCursor(SPF_MouseCursor cursor) { ImGui::SetMouseCursor((ImGuiMouseCursor)cursor); }
SPF_MouseCursor UIApi::UI_GetMouseCursor() { return (SPF_MouseCursor)ImGui::GetMouseCursor(); }

bool UIApi::UI_IsKeyDown(int key_index) { return ImGui::IsKeyDown((ImGuiKey)key_index); }
bool UIApi::UI_IsKeyPressed(int key_index) { return ImGui::IsKeyPressed((ImGuiKey)key_index); }
bool UIApi::UI_IsKeyReleased(int key_index) { return ImGui::IsKeyReleased((ImGuiKey)key_index); }
int UIApi::UI_GetKeyPressedAmount(int key_index, float repeat_delay, float rate) { return ImGui::GetKeyPressedAmount((ImGuiKey)key_index, repeat_delay, rate); }
bool UIApi::UI_IsMouseReleasedWithDelay(SPF_MouseButton button, float delay) { return ImGui::IsMouseReleasedWithDelay((ImGuiMouseButton)button, delay); }
const char* UIApi::UI_GetKeyName(int key_index) { return ImGui::GetKeyName((ImGuiKey)key_index); }

const char* UIApi::UI_GetClipboardText() { return ImGui::GetClipboardText(); }
void UIApi::UI_SetClipboardText(const char* text) { ImGui::SetClipboardText(text); }

// --- III. Windows, Layout & Positioning ---

bool UIApi::UI_IsWindowAppearing() { return ImGui::IsWindowAppearing(); }
bool UIApi::UI_IsWindowCollapsed() { return ImGui::IsWindowCollapsed(); }
bool UIApi::UI_IsWindowFocused(SPF_FocusedFlags flags) { return ImGui::IsWindowFocused((ImGuiFocusedFlags)flags); }
bool UIApi::UI_IsWindowHovered(SPF_HoveredFlags flags) { return ImGui::IsWindowHovered((ImGuiHoveredFlags)flags); }

SPF_Storage_Handle UIApi::UI_GetStateStorage() { return (SPF_Storage_Handle)ImGui::GetStateStorage(); }
void UIApi::UI_SetStateStorage(SPF_Storage_Handle storage) { ImGui::SetStateStorage((ImGuiStorage*)storage); }
void* UIApi::UI_GetWindowViewport() { return (void*)ImGui::GetWindowViewport(); }

SPF_DrawList_Handle UIApi::UI_GetWindowDrawList() { return (SPF_DrawList_Handle)ImGui::GetWindowDrawList(); }
SPF_DrawList_Handle UIApi::UI_GetBackgroundDrawList() { return (SPF_DrawList_Handle)ImGui::GetBackgroundDrawList(); }
SPF_DrawList_Handle UIApi::UI_GetForegroundDrawList() { return (SPF_DrawList_Handle)ImGui::GetForegroundDrawList(); }

void UIApi::UI_GetWindowPos(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetWindowPos();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

void UIApi::UI_GetWindowSize(float* out_x, float* out_y) {
    ImVec2 size = ImGui::GetWindowSize();
    if (out_x) *out_x = size.x;
    if (out_y) *out_y = size.y;
}

float UIApi::UI_GetWindowWidth() { return ImGui::GetWindowWidth(); }
float UIApi::UI_GetWindowHeight() { return ImGui::GetWindowHeight(); }
float UIApi::UI_GetWindowDpiScale() { return ImGui::GetWindowDpiScale(); }

void UIApi::UI_SetWindowPos(float x, float y, SPF_Cond cond) { ImGui::SetWindowPos({x, y}, (ImGuiCond)cond); }
void UIApi::UI_SetWindowSize(float x, float y, SPF_Cond cond) { ImGui::SetWindowSize({x, y}, (ImGuiCond)cond); }
void UIApi::UI_SetNextWindowPos(float x, float y, SPF_Cond cond, float pivot_x, float pivot_y) { ImGui::SetNextWindowPos({x, y}, (ImGuiCond)cond, {pivot_x, pivot_y}); }
void UIApi::UI_SetNextWindowSize(float x, float y, SPF_Cond cond) { ImGui::SetNextWindowSize({x, y}, (ImGuiCond)cond); }
void UIApi::UI_SetNextWindowViewport(uint32_t viewport_id) { ImGui::SetNextWindowViewport(viewport_id); }
void UIApi::UI_SetNextWindowScroll(float scroll_x, float scroll_y) { ImGui::SetNextWindowScroll({scroll_x, scroll_y}); }

float UIApi::UI_GetScrollX() { return ImGui::GetScrollX(); }
float UIApi::UI_GetScrollY() { return ImGui::GetScrollY(); }
float UIApi::UI_GetScrollMaxX() { return ImGui::GetScrollMaxX(); }
float UIApi::UI_GetScrollMaxY() { return ImGui::GetScrollMaxY(); }
void UIApi::UI_SetScrollX(float scroll_x) { ImGui::SetScrollX(scroll_x); }
void UIApi::UI_SetScrollY(float scroll_y) { ImGui::SetScrollY(scroll_y); }
void UIApi::UI_SetScrollHereY(float center_y_ratio) { ImGui::SetScrollHereY(center_y_ratio); }

void UIApi::UI_SetNextWindowFocus() { ImGui::SetNextWindowFocus(); }
void UIApi::UI_SetNextWindowCollapsed(bool collapsed, SPF_Cond cond) { ImGui::SetNextWindowCollapsed(collapsed, (ImGuiCond)cond); }
void UIApi::UI_SetWindowCollapsed(bool collapsed, SPF_Cond cond) { ImGui::SetWindowCollapsed(collapsed, (ImGuiCond)cond); }
void UIApi::UI_SetWindowFocus() { ImGui::SetWindowFocus(); }
void UIApi::UI_FocusWindow(void* window) { ImGui::FocusWindow((ImGuiWindow*)window); }
void UIApi::UI_BringWindowToFocusFront(void* window) { ImGui::BringWindowToFocusFront((ImGuiWindow*)window); }

void UIApi::UI_SetWindowFontScale(float scale) { ImGui::SetWindowFontScale(scale); }
void UIApi::UI_SetNextWindowSizeConstraints(float min_x, float min_y, float max_x, float max_y) { ImGui::SetNextWindowSizeConstraints({min_x, min_y}, {max_x, max_y}); }
void UIApi::UI_SetNextWindowBgAlpha(float alpha) { ImGui::SetNextWindowBgAlpha(alpha); }
void UIApi::UI_SetNextWindowContentSize(float size_x, float size_y) { ImGui::SetNextWindowContentSize({size_x, size_y}); }

void UIApi::UI_BringWindowToDisplayFront() { ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow()); }
void UIApi::UI_BringWindowToDisplayBack() { ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow()); }

void UIApi::UI_GetMainViewportPos(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetMainViewport()->Pos;
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

void UIApi::UI_GetMainViewportSize(float* out_x, float* out_y) {
    ImVec2 size = ImGui::GetMainViewport()->Size;
    if (out_x) *out_x = size.x;
    if (out_y) *out_y = size.y;
}

bool UIApi::UI_BeginChild(const char* str_id, float size_x, float size_y, bool border, SPF_WindowFlags flags) {
    return ImGui::BeginChild(str_id, {size_x, size_y}, border, (ImGuiWindowFlags)flags);
}
void UIApi::UI_EndChild() { ImGui::EndChild(); }

void UIApi::UI_GetContentRegionAvail(float* out_x, float* out_y) {
    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (out_x) *out_x = sz.x;
    if (out_y) *out_y = sz.y;
}

void UIApi::UI_GetContentRegionMax(float* out_x, float* out_y) {
    ImVec2 sz = ImGui::GetContentRegionMax();
    if (out_x) *out_x = sz.x;
    if (out_y) *out_y = sz.y;
}

void UIApi::UI_GetWindowContentRegionMin(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetWindowContentRegionMin();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

void UIApi::UI_GetWindowContentRegionMax(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetWindowContentRegionMax();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

void UIApi::UI_GetCursorPos(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetCursorPos();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

float UIApi::UI_GetCursorPosX() { return ImGui::GetCursorPosX(); }
float UIApi::UI_GetCursorPosY() { return ImGui::GetCursorPosY(); }
void UIApi::UI_SetCursorPos(float x, float y) { ImGui::SetCursorPos({x, y}); }
void UIApi::UI_SetCursorPosX(float x) { ImGui::SetCursorPosX(x); }
void UIApi::UI_SetCursorPosY(float y) { ImGui::SetCursorPosY(y); }

void UIApi::UI_GetCursorScreenPos(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

void UIApi::UI_SetCursorScreenPos(float x, float y) { ImGui::SetCursorScreenPos({x, y}); }

void UIApi::UI_GetCursorStartPos(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetCursorStartPos();
    if (out_x) *out_x = pos.x;
    if (out_y) *out_y = pos.y;
}

void UIApi::UI_Separator() { ImGui::Separator(); }
void UIApi::UI_SeparatorText(const char* label) { ImGui::SeparatorText(label); }
void UIApi::UI_AlignTextToFramePadding() { ImGui::AlignTextToFramePadding(); }
void UIApi::UI_SameLine(float offset_from_start_x, float spacing) { ImGui::SameLine(offset_from_start_x, spacing); }
void UIApi::UI_NewLine() { ImGui::NewLine(); }
void UIApi::UI_Spacing() { ImGui::Spacing(); }
void UIApi::UI_Dummy(float size_x, float size_y) { ImGui::Dummy({size_x, size_y}); }
void UIApi::UI_Indent(float indent_w) { ImGui::Indent(indent_w); }
void UIApi::UI_Unindent(float indent_w) { ImGui::Unindent(indent_w); }
void UIApi::UI_BeginGroup() { ImGui::BeginGroup(); }
void UIApi::UI_EndGroup() { ImGui::EndGroup(); }

void UIApi::UI_PushID_Str(const char* str_id) { ImGui::PushID(str_id); }
void UIApi::UI_PushID_Int(int int_id) { ImGui::PushID(int_id); }
void UIApi::UI_PushID_Ptr(const void* ptr_id) { ImGui::PushID(ptr_id); }
void UIApi::UI_PopID() { ImGui::PopID(); }

void UIApi::UI_PushItemFlag(int flags, bool enabled) { ImGui::PushItemFlag((ImGuiItemFlags)flags, enabled); }
void UIApi::UI_PopItemFlag() { ImGui::PopItemFlag(); }
void UIApi::UI_BeginDisabled(bool disabled) { ImGui::BeginDisabled(disabled); }
void UIApi::UI_EndDisabled() { ImGui::EndDisabled(); }
uint32_t UIApi::UI_GetID_Str(const char* str_id) { return ImGui::GetID(str_id); }

void UIApi::UI_PushItemWidth(float item_width) { ImGui::PushItemWidth(item_width); }
void UIApi::UI_PopItemWidth() { ImGui::PopItemWidth(); }
void UIApi::UI_SetNextItemWidth(float item_width) { ImGui::SetNextItemWidth(item_width); }
float UIApi::UI_CalcItemWidth() { return ImGui::CalcItemWidth(); }
void UIApi::UI_PushTextWrapPos(float wrap_local_pos_x) { ImGui::PushTextWrapPos(wrap_local_pos_x); }
void UIApi::UI_PopTextWrapPos() { ImGui::PopTextWrapPos(); }

float UIApi::UI_GetTextLineHeight() { return ImGui::GetTextLineHeight(); }
float UIApi::UI_GetTextLineHeightWithSpacing() { return ImGui::GetTextLineHeightWithSpacing(); }
float UIApi::UI_GetFrameHeight() { return ImGui::GetFrameHeight(); }
float UIApi::UI_GetFrameHeightWithSpacing() { return ImGui::GetFrameHeightWithSpacing(); }

// --- IV. Basic Widgets ---

void UIApi::UI_Text(const char* text) { if (text) ImGui::TextUnformatted(text); }
void UIApi::UI_TextUnformatted(const char* text) { if (text) ImGui::TextUnformatted(text); }
bool UIApi::UI_TextLink(const char* label) { return ImGui::TextLink(label); }
void UIApi::UI_TextLinkOpenURL(const char* label, const char* url) { ImGui::TextLinkOpenURL(label, url); }
void UIApi::UI_TextColored(float r, float g, float b, float a, const char* text) { if (text) ImGui::TextColored({r, g, b, a}, "%s", text); }
void UIApi::UI_TextDisabled(const char* text) { if (text) ImGui::TextDisabled("%s", text); }
void UIApi::UI_TextWrapped(const char* text) { if (text) ImGui::TextWrapped("%s", text); }
void UIApi::UI_LabelText(const char* label, const char* text) { if (label && text) ImGui::LabelText(label, "%s", text); }
void UIApi::UI_BulletText(const char* text) { if (text) ImGui::BulletText("%s", text); }

bool UIApi::UI_Button(const char* label, float width, float height) { return ImGui::Button(label, {width, height}); }

bool UIApi::UI_ButtonEx(const char* label, float width, float height, const char* tooltip, SPF_TextStyle_Handle style) {
    if (style) {
        return SPF::UI::Button(label, style->style, {width, height}, tooltip);
    }
    return SPF::UI::Button(label, SPF::UI::TextStyle::DefaultButton(), {width, height}, tooltip);
}

bool UIApi::UI_SmallButton(const char* label) { return ImGui::SmallButton(label); }
bool UIApi::UI_InvisibleButton(const char* str_id, float width, float height) { return ImGui::InvisibleButton(str_id, {width, height}); }
bool UIApi::UI_ArrowButton(const char* str_id, SPF_Dir dir) { return ImGui::ArrowButton(str_id, (ImGuiDir)dir); }
bool UIApi::UI_Checkbox(const char* label, bool* v) { return ImGui::Checkbox(label, v); }
bool UIApi::UI_CheckboxFlags(const char* label, int* flags, int flags_value) { return ImGui::CheckboxFlags(label, flags, flags_value); }
bool UIApi::UI_RadioButton(const char* label, bool active) { return ImGui::RadioButton(label, active); }
bool UIApi::UI_RadioButtonFlags(const char* label, int* v, int v_button) { return ImGui::RadioButton(label, v, v_button); }
void UIApi::UI_ProgressBar(float fraction, float width, float height, const char* overlay) { ImGui::ProgressBar(fraction, {width, height}, overlay); }
void UIApi::UI_Bullet() { ImGui::Bullet(); }

void UIApi::UI_Image(void* user_texture_id, float width, float height) { ImGui::Image(user_texture_id, {width, height}); }
void UIApi::UI_ImageWithBg(void* user_texture_id, float width, float height, float bg_col[4], float tint_col[4]) {
    ImGui::Image(user_texture_id, {width, height}, {0,0}, {1,1}, {tint_col[0], tint_col[1], tint_col[2], tint_col[3]}, {bg_col[0], bg_col[1], bg_col[2], bg_col[3]});
}
bool UIApi::UI_ImageButton(const char* str_id, void* user_texture_id, float width, float height) {
    return ImGui::ImageButton(str_id, user_texture_id, {width, height});
}

// --- V. Advanced Inputs ---

bool UIApi::UI_BeginCombo(const char* label, const char* preview_value, SPF_ComboFlags flags) { return ImGui::BeginCombo(label, preview_value, (ImGuiComboFlags)flags); }
void UIApi::UI_EndCombo() { ImGui::EndCombo(); }
bool UIApi::UI_Combo(const char* label, int* current_item, const char* const items[], int items_count) { return ImGui::Combo(label, current_item, items, items_count); }

bool UIApi::UI_DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragFloat2(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragFloat3(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragFloat4(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, SPF_SliderFlags flags) { return ImGui::DragFloatRange2(label, v_current_min, v_current_max, v_speed, v_min, v_max, format, format_max, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragInt(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragInt2(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragInt3(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragInt4(label, v, v_speed, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max, SPF_SliderFlags flags) { return ImGui::DragIntRange2(label, v_current_min, v_current_max, v_speed, v_min, v_max, format, format_max, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragScalar(const char* label, SPF_DataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragScalar(label, (ImGuiDataType)data_type, p_data, v_speed, p_min, p_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_DragScalarN(const char* label, SPF_DataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragScalarN(label, (ImGuiDataType)data_type, p_data, components, v_speed, p_min, p_max, format, (ImGuiSliderFlags)flags); }

bool UIApi::UI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderFloat(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderFloat2(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderFloat3(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderFloat4(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderAngle(label, v_rad, v_degrees_min, v_degrees_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderInt(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderInt2(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderInt3(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderInt4(label, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderScalar(const char* label, SPF_DataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderScalar(label, (ImGuiDataType)data_type, p_data, p_min, p_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderScalarN(const char* label, SPF_DataType data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags) { return ImGui::SliderScalarN(label, (ImGuiDataType)data_type, p_data, components, p_min, p_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_VSliderFloat(const char* label, float size_x, float size_y, float* v, float v_min, float v_max, const char* format, SPF_SliderFlags flags) { return ImGui::VSliderFloat(label, {size_x, size_y}, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_VSliderInt(const char* label, float size_x, float size_y, int* v, int v_min, int v_max, const char* format, SPF_SliderFlags flags) { return ImGui::VSliderInt(label, {size_x, size_y}, v, v_min, v_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_VSliderScalar(const char* label, float size_x, float size_y, SPF_DataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags) { return ImGui::VSliderScalar(label, {size_x, size_y}, (ImGuiDataType)data_type, p_data, p_min, p_max, format, (ImGuiSliderFlags)flags); }

bool UIApi::UI_InputText(const char* label, char* buf, size_t buf_size, SPF_InputTextFlags flags) { return ImGui::InputText(label, buf, buf_size, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputTextMultiline(const char* label, char* buf, size_t buf_size, float size_x, float size_y, SPF_InputTextFlags flags) { return ImGui::InputTextMultiline(label, buf, buf_size, {size_x, size_y}, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, SPF_InputTextFlags flags) { return ImGui::InputTextWithHint(label, hint, buf, buf_size, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputFloat(const char* label, float* v, float step, float step_fast, const char* format, SPF_InputTextFlags flags) { return ImGui::InputFloat(label, v, step, step_fast, format, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputFloat2(const char* label, float v[2], const char* format, SPF_InputTextFlags flags) { return ImGui::InputFloat2(label, v, format, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputFloat3(const char* label, float v[3], const char* format, SPF_InputTextFlags flags) { return ImGui::InputFloat3(label, v, format, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputFloat4(const char* label, float v[4], const char* format, SPF_InputTextFlags flags) { return ImGui::InputFloat4(label, v, format, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputInt(const char* label, int* v, int step, int step_fast, SPF_InputTextFlags flags) { return ImGui::InputInt(label, v, step, step_fast, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputInt2(const char* label, int v[2], SPF_InputTextFlags flags) { return ImGui::InputInt2(label, v, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputInt3(const char* label, int v[3], SPF_InputTextFlags flags) { return ImGui::InputInt3(label, v, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputInt4(const char* label, int v[4], SPF_InputTextFlags flags) { return ImGui::InputInt4(label, v, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputDouble(const char* label, double* v, double step, double step_fast, const char* format, SPF_InputTextFlags flags) { return ImGui::InputDouble(label, v, step, step_fast, format, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputScalar(const char* label, SPF_DataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, SPF_InputTextFlags flags) { return ImGui::InputScalar(label, (ImGuiDataType)data_type, p_data, p_step, p_step_fast, format, (ImGuiInputTextFlags)flags); }
bool UIApi::UI_InputScalarN(const char* label, SPF_DataType data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, SPF_InputTextFlags flags) { return ImGui::InputScalarN(label, (ImGuiDataType)data_type, p_data, components, p_step, p_step_fast, format, (ImGuiInputTextFlags)flags); }

bool UIApi::UI_ColorEdit3(const char* label, float col[3], SPF_ColorEditFlags flags) { return ImGui::ColorEdit3(label, col, (ImGuiColorEditFlags)flags); }
bool UIApi::UI_ColorEdit4(const char* label, float col[4], SPF_ColorEditFlags flags) { return ImGui::ColorEdit4(label, col, (ImGuiColorEditFlags)flags); }
bool UIApi::UI_ColorPicker3(const char* label, float col[3], SPF_ColorEditFlags flags) { return ImGui::ColorPicker3(label, col, (ImGuiColorEditFlags)flags); }
bool UIApi::UI_ColorPicker4(const char* label, float col[4], SPF_ColorEditFlags flags) { return ImGui::ColorPicker4(label, col, (ImGuiColorEditFlags)flags); }
bool UIApi::UI_ColorButton(const char* desc_id, float r, float g, float b, float a, SPF_ColorEditFlags flags, float size_x, float size_y) { return ImGui::ColorButton(desc_id, {r, g, b, a}, (ImGuiColorEditFlags)flags, {size_x, size_y}); }
void UIApi::UI_SetColorEditOptions(SPF_ColorEditFlags flags) { ImGui::SetColorEditOptions((ImGuiColorEditFlags)flags); }

// --- VI. Advanced Widgets ---

bool UIApi::UI_TreeNode(const char* label) { return ImGui::TreeNode(label); }
bool UIApi::UI_TreeNodeEx(const char* str_id, const char* label, SPF_TreeNodeFlags flags) { return ImGui::TreeNodeEx(str_id, (ImGuiTreeNodeFlags)flags, "%s", label); }
void UIApi::UI_TreePush(const char* str_id) { ImGui::TreePush(str_id); }
void UIApi::UI_TreePop() { ImGui::TreePop(); }
void UIApi::UI_SetNextItemStorageID(uint32_t storage_id) { ImGui::SetNextItemStorageID((ImGuiID)storage_id); }
float UIApi::UI_GetTreeNodeToLabelSpacing() { return ImGui::GetTreeNodeToLabelSpacing(); }
bool UIApi::UI_CollapsingHeader(const char* label, SPF_TreeNodeFlags flags) { return ImGui::CollapsingHeader(label, (ImGuiTreeNodeFlags)flags); }
void UIApi::UI_SetNextItemOpen(bool is_open, SPF_Cond cond) { ImGui::SetNextItemOpen(is_open, (ImGuiCond)cond); }
bool UIApi::UI_TreeNodeGetOpen(uint32_t storage_id) { return ImGui::TreeNodeGetOpen((ImGuiID)storage_id); }

bool UIApi::UI_Selectable(const char* label, bool selected, SPF_SelectableFlags flags, float size_x, float size_y) { return ImGui::Selectable(label, selected, (ImGuiSelectableFlags)flags, {size_x, size_y}); }
bool UIApi::UI_BeginListBox(const char* label, float size_x, float size_y) { return ImGui::BeginListBox(label, {size_x, size_y}); }
void UIApi::UI_EndListBox() { ImGui::EndListBox(); }
bool UIApi::UI_ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items) { return ImGui::ListBox(label, current_item, items, items_count, height_in_items); }

SPF_MultiSelectIO* UIApi::UI_BeginMultiSelect(SPF_MultiSelectFlags flags, int selection_size, int items_count) {
    return (SPF_MultiSelectIO*)ImGui::BeginMultiSelect((ImGuiMultiSelectFlags)flags, selection_size, items_count);
}

SPF_MultiSelectIO* UIApi::UI_EndMultiSelect() {
    return (SPF_MultiSelectIO*)ImGui::EndMultiSelect();
}

void UIApi::UI_SetNextItemSelectionUserData(int64_t selection_user_data) {
    ImGui::SetNextItemSelectionUserData((ImGuiSelectionUserData)selection_user_data);
}

bool UIApi::UI_IsItemToggledSelection() {
    return ImGui::IsItemToggledSelection();
}

void UIApi::UI_PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y, int stride) { ImGui::PlotLines(label, values, values_count, values_offset, overlay_text, scale_min, scale_max, {graph_size_x, graph_size_y}, stride); }
void UIApi::UI_PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y, int stride) { ImGui::PlotHistogram(label, values, values_count, values_offset, overlay_text, scale_min, scale_max, {graph_size_x, graph_size_y}, stride); }

void UIApi::UI_PlotLinesCallback(const char* label, SPF_PlotGetter values_getter, void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y) {
    ImGui::PlotLines(label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, {graph_size_x, graph_size_y});
}

void UIApi::UI_PlotHistogramCallback(const char* label, SPF_PlotGetter values_getter, void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y) {
    ImGui::PlotHistogram(label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, {graph_size_x, graph_size_y});
}

void UIApi::UI_Value_Bool(const char* prefix, bool b) { ImGui::Value(prefix, b); }
void UIApi::UI_Value_Int(const char* prefix, int v) { ImGui::Value(prefix, v); }
void UIApi::UI_Value_UInt(const char* prefix, unsigned int v) { ImGui::Value(prefix, v); }
void UIApi::UI_Value_Float(const char* prefix, float v, const char* float_format) { ImGui::Value(prefix, v, float_format); }

SPF_ListClipper UIApi::UI_ListClipper_Begin(int items_count, float items_height) {
    ImGuiListClipper* clipper = (ImGuiListClipper*)ImGui::MemAlloc(sizeof(ImGuiListClipper));
    IM_PLACEMENT_NEW(clipper) ImGuiListClipper();
    clipper->Begin(items_count, items_height);
    SPF_ListClipper res;
    res.DisplayStart = clipper->DisplayStart;
    res.DisplayEnd = clipper->DisplayEnd;
    res.ItemsCount = clipper->ItemsCount;
    res.ItemsHeight = clipper->ItemsHeight;
    res.StartPosY = clipper->StartPosY;
    res.InternalData = clipper;
    return res;
}
bool UIApi::UI_ListClipper_Step(SPF_ListClipper* clipper) {
    if (!clipper || !clipper->InternalData) return false;
    ImGuiListClipper* c = (ImGuiListClipper*)clipper->InternalData;
    bool res = c->Step();
    clipper->DisplayStart = c->DisplayStart;
    clipper->DisplayEnd = c->DisplayEnd;
    return res;
}
void UIApi::UI_ListClipper_End(SPF_ListClipper* clipper) {
    if (clipper && clipper->InternalData) {
        ImGuiListClipper* c = (ImGuiListClipper*)clipper->InternalData;
        c->End();
        c->~ImGuiListClipper();
        ImGui::MemFree(c);
        clipper->InternalData = nullptr;
    }
}

// --- VII. Menus ---

bool UIApi::UI_BeginMenuBar() { return ImGui::BeginMenuBar(); }
void UIApi::UI_EndMenuBar() { ImGui::EndMenuBar(); }
bool UIApi::UI_BeginMainMenuBar() { return ImGui::BeginMainMenuBar(); }
void UIApi::UI_EndMainMenuBar() { ImGui::EndMainMenuBar(); }
bool UIApi::UI_BeginMenu(const char* label, bool enabled) { return ImGui::BeginMenu(label, enabled); }
void UIApi::UI_EndMenu() { ImGui::EndMenu(); }
bool UIApi::UI_MenuItem(const char* label, const char* shortcut, bool selected, bool enabled) { return ImGui::MenuItem(label, shortcut, selected, enabled); }

// --- VIII. Tables ---

bool UIApi::UI_BeginTable(const char* str_id, int columns_count, SPF_TableFlags flags, float outer_size_x, float outer_size_y, float inner_width) { return ImGui::BeginTable(str_id, columns_count, (ImGuiTableFlags)flags, {outer_size_x, outer_size_y}, inner_width); }
void UIApi::UI_EndTable() { ImGui::EndTable(); }
void UIApi::UI_TableNextRow(SPF_TableRowFlags row_flags, float min_row_height) { ImGui::TableNextRow((ImGuiTableRowFlags)row_flags, min_row_height); }
bool UIApi::UI_TableNextColumn() { return ImGui::TableNextColumn(); }
bool UIApi::UI_TableSetColumnIndex(int column_n) { return ImGui::TableSetColumnIndex(column_n); }

void UIApi::UI_TableSetupColumn(const char* label, SPF_TableColumnFlags flags, float init_width_or_weight, uint32_t user_id) { ImGui::TableSetupColumn(label, (ImGuiTableColumnFlags)flags, init_width_or_weight, user_id); }
void UIApi::UI_TableSetupScrollFreeze(int cols_count, int rows_count) { ImGui::TableSetupScrollFreeze(cols_count, rows_count); }
void UIApi::UI_TableHeadersRow() { ImGui::TableHeadersRow(); }
void UIApi::UI_TableAngledHeadersRow() { ImGui::TableAngledHeadersRow(); }
void UIApi::UI_TableHeader(const char* label) { ImGui::TableHeader(label); }

const SPF_TableSortSpecs* UIApi::UI_TableGetSortSpecs() { return (const SPF_TableSortSpecs*)ImGui::TableGetSortSpecs(); }
void UIApi::UI_TableSetColumnEnabled(int column_n, bool enabled) { ImGui::TableSetColumnEnabled(column_n, enabled); }
bool UIApi::UI_TableGetHoveredColumn(int column_n) { return ImGui::TableGetColumnFlags(column_n) & ImGuiTableColumnFlags_IsHovered; }

int UIApi::UI_TableGetColumnCount() { return ImGui::TableGetColumnCount(); }
int UIApi::UI_TableGetColumnIndex() { return ImGui::TableGetColumnIndex(); }
int UIApi::UI_TableGetRowIndex() { return ImGui::TableGetRowIndex(); }
const char* UIApi::UI_TableGetColumnName(int column_n) { return ImGui::TableGetColumnName(column_n); }
SPF_TableColumnFlags UIApi::UI_TableGetColumnFlags(int column_n) { return (SPF_TableColumnFlags)ImGui::TableGetColumnFlags(column_n); }
void UIApi::UI_TableSetBgColor(int target, uint32_t color, int column_n) { ImGui::TableSetBgColor((ImGuiTableBgTarget)target, color, column_n); }

// --- IX. Popups & Tooltips ---

bool UIApi::UI_BeginPopup(const char* str_id, SPF_WindowFlags flags) { return ImGui::BeginPopup(str_id, (ImGuiWindowFlags)flags); }
bool UIApi::UI_BeginPopupModal(const char* name, bool* p_open, SPF_WindowFlags flags) { return ImGui::BeginPopupModal(name, p_open, (ImGuiWindowFlags)flags); }
void UIApi::UI_EndPopup() { ImGui::EndPopup(); }
void UIApi::UI_OpenPopup(const char* str_id, SPF_PopupFlags flags) { ImGui::OpenPopup(str_id, (ImGuiPopupFlags)flags); }
void UIApi::UI_OpenPopupOnItemClick(const char* str_id, SPF_PopupFlags flags) { ImGui::OpenPopupOnItemClick(str_id, (ImGuiPopupFlags)flags); }
void UIApi::UI_CloseCurrentPopup() { ImGui::CloseCurrentPopup(); }

bool UIApi::UI_BeginPopupContextItem(const char* str_id, SPF_PopupFlags flags) { return ImGui::BeginPopupContextItem(str_id, (ImGuiPopupFlags)flags); }
bool UIApi::UI_BeginPopupContextWindow(const char* str_id, SPF_PopupFlags flags) { return ImGui::BeginPopupContextWindow(str_id, (ImGuiPopupFlags)flags); }
bool UIApi::UI_BeginPopupContextVoid(const char* str_id, SPF_PopupFlags flags) { return ImGui::BeginPopupContextVoid(str_id, (ImGuiPopupFlags)flags); }
bool UIApi::UI_IsPopupOpen(const char* str_id, SPF_PopupFlags flags) { return ImGui::IsPopupOpen(str_id, (ImGuiPopupFlags)flags); }

void UIApi::UI_BeginTooltip() { ImGui::BeginTooltip(); }
void UIApi::UI_EndTooltip() { ImGui::EndTooltip(); }
void UIApi::UI_SetTooltip(const char* text) { if (text) ImGui::SetTooltip("%s", text); }
bool UIApi::UI_BeginItemTooltip() { return ImGui::BeginItemTooltip(); }
void UIApi::UI_SetItemTooltip(const char* text) { if (text) ImGui::SetItemTooltip("%s", text); }

// --- X. Drag & Drop ---

bool UIApi::UI_BeginDragDropSource(SPF_DragDropFlags flags) { return ImGui::BeginDragDropSource((ImGuiDragDropFlags)flags); }
bool UIApi::UI_SetDragDropPayload(const char* type, const void* data, size_t size, SPF_Cond cond) { return ImGui::SetDragDropPayload(type, data, size, (ImGuiCond)cond); }
void UIApi::UI_EndDragDropSource() { ImGui::EndDragDropSource(); }
bool UIApi::UI_BeginDragDropTarget() { return ImGui::BeginDragDropTarget(); }
const SPF_Payload_Handle* UIApi::UI_AcceptDragDropPayload(const char* type, SPF_DragDropFlags flags) { return (const SPF_Payload_Handle*)ImGui::AcceptDragDropPayload(type, (ImGuiDragDropFlags)flags); }
void UIApi::UI_EndDragDropTarget() { ImGui::EndDragDropTarget(); }
const SPF_Payload_Handle* UIApi::UI_GetDragDropPayload() { return (const SPF_Payload_Handle*)ImGui::GetDragDropPayload(); }

// --- XI. Style & Typography ---

SPF_Font_Handle UIApi::UI_GetFont(const char* font_key) {
    if (!font_key) return nullptr;
    return (SPF_Font_Handle)UI::UIManager::GetInstance().GetFont(font_key);
}
void UIApi::UI_PushFont(SPF_Font_Handle font_handle) { if (font_handle) ImGui::PushFont((ImFont*)font_handle); }
void UIApi::UI_PopFont() { ImGui::PopFont(); }

void UIApi::UI_PushStyleColor(SPF_StyleColor idx, float r, float g, float b, float a) { ImGui::PushStyleColor((ImGuiCol)idx, {r, g, b, a}); }
void UIApi::UI_PopStyleColor(int count) { ImGui::PopStyleColor(count); }
void UIApi::UI_PushStyleVarFloat(SPF_StyleVar idx, float val) { ImGui::PushStyleVar((ImGuiStyleVar)idx, val); }
void UIApi::UI_PushStyleVarVec2(SPF_StyleVar idx, float val_x, float val_y) { ImGui::PushStyleVar((ImGuiStyleVar)idx, {val_x, val_y}); }
void UIApi::UI_PopStyleVar(int count) { ImGui::PopStyleVar(count); }

SPF_Style_Handle* UIApi::UI_GetStyle() { return (SPF_Style_Handle*)&ImGui::GetStyle(); }
void UIApi::UI_Style_GetWindowPadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y) {
    if (style_handle && out_x && out_y) {
        *out_x = ((ImGuiStyle*)style_handle)->WindowPadding.x;
        *out_y = ((ImGuiStyle*)style_handle)->WindowPadding.y;
    }
}
void UIApi::UI_Style_GetItemSpacing(SPF_Style_Handle* style_handle, float* out_x, float* out_y) {
    if (style_handle && out_x && out_y) {
        *out_x = ((ImGuiStyle*)style_handle)->ItemSpacing.x;
        *out_y = ((ImGuiStyle*)style_handle)->ItemSpacing.y;
    }
}
void UIApi::UI_Style_GetFramePadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y) {
    if (style_handle && out_x && out_y) {
        *out_x = ((ImGuiStyle*)style_handle)->FramePadding.x;
        *out_y = ((ImGuiStyle*)style_handle)->FramePadding.y;
    }
}

void UIApi::UI_StyleColorsDark() { ImGui::StyleColorsDark(); }
void UIApi::UI_StyleColorsLight() { ImGui::StyleColorsLight(); }
void UIApi::UI_StyleColorsClassic() { ImGui::StyleColorsClassic(); }

void UIApi::UI_GetStyleColor(SPF_StyleColor idx, float* out_r, float* out_g, float* out_b, float* out_a) {
    ImVec4 c = ImGui::GetStyleColorVec4((ImGuiCol)idx);
    if (out_r) *out_r = c.x; if (out_g) *out_g = c.y; if (out_b) *out_b = c.z; if (out_a) *out_a = c.w;
}

void UIApi::UI_CalcTextSize(const char* text, float* out_w, float* out_h) {
    ImVec2 sz = ImGui::CalcTextSize(text);
    if (out_w) *out_w = sz.x; if (out_h) *out_h = sz.y;
}

void UIApi::UI_CalcTextSizeWithFont(SPF_Font font, float font_size, const char* text, float* out_w, float* out_h) {
    if (!text) return;
    const char* fontName = "regular";
    switch (font) {
        case SPF_FONT_BOLD: fontName = "bold"; break;
        case SPF_FONT_ITALIC: fontName = "italic"; break;
        case SPF_FONT_BOLD_ITALIC: fontName = "bold_italic"; break;
        case SPF_FONT_MEDIUM: fontName = "medium"; break;
        case SPF_FONT_MEDIUM_ITALIC: fontName = "medium_italic"; break;
        case SPF_FONT_MONOSPACE: fontName = "monospace"; break;
        case SPF_FONT_H1: fontName = "h1"; break;
        case SPF_FONT_H2: fontName = "h2"; break;
        case SPF_FONT_H3: fontName = "h3"; break;
        case SPF_FONT_H1_LARGE_BOLD: fontName = "h1_large_bold"; break;
    }
    ImFont* imFont = UI::UIManager::GetInstance().GetFont(fontName);
    if (imFont) {
        ImGui::PushFont(imFont, font_size);
        ImVec2 size = ImGui::CalcTextSize(text);
        ImGui::PopFont();
        if (out_w) *out_w = size.x; if (out_h) *out_h = size.y;
    } else {
        ImVec2 size = ImGui::CalcTextSize(text);
        if (out_w) *out_w = size.x; if (out_h) *out_h = size.y;
    }
}

uint32_t UIApi::UI_ColorConvertFloat4ToU32(float r, float g, float b, float a) { return ImGui::ColorConvertFloat4ToU32({r, g, b, a}); }
void UIApi::UI_ColorConvertU32ToFloat4(uint32_t in, float* out_r, float* out_g, float* out_b, float* out_a) {
    ImVec4 c = ImGui::ColorConvertU32ToFloat4(in);
    if (out_r) *out_r = c.x; if (out_g) *out_g = c.y; if (out_b) *out_b = c.z; if (out_a) *out_a = c.w;
}
void UIApi::UI_ColorConvertRGBtoHSV(float r, float g, float b, float* out_h, float* out_s, float* out_v) { ImGui::ColorConvertRGBtoHSV(r, g, b, *out_h, *out_s, *out_v); }
void UIApi::UI_ColorConvertHSVtoRGB(float h, float s, float v, float* out_r, float* out_g, float* out_b) { ImGui::ColorConvertHSVtoRGB(h, s, v, *out_r, *out_g, *out_b); }

SPF_TextStyle_Handle UIApi::UI_Style_Create() { return new SPF_TextStyle_Handle_t(); }
void UIApi::UI_Style_Destroy(SPF_TextStyle_Handle handle) { delete handle; }
void UIApi::UI_Style_SetFont(SPF_TextStyle_Handle handle, SPF_Font font) {
    if (!handle) return;
    const char* fontKey = "regular";
    switch (font) {
        case SPF_FONT_REGULAR: fontKey = "regular"; break;
        case SPF_FONT_BOLD: fontKey = "bold"; break;
        case SPF_FONT_ITALIC: fontKey = "italic"; break;
        case SPF_FONT_BOLD_ITALIC: fontKey = "bold_italic"; break;
        case SPF_FONT_MEDIUM: fontKey = "medium"; break;
        case SPF_FONT_MEDIUM_ITALIC: fontKey = "medium_italic"; break;
        case SPF_FONT_MONOSPACE: fontKey = "monospace"; break;
        case SPF_FONT_H1: fontKey = "h1"; break;
        case SPF_FONT_H2: fontKey = "h2"; break;
        case SPF_FONT_H3: fontKey = "h3"; break;
    }
    handle->style.Font(fontKey);
}
void UIApi::UI_Style_SetColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a) { if (handle) handle->style.Color({r, g, b, a}); }
void UIApi::UI_Style_SetHoverColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a) { if (handle) handle->style.HoverColor({r, g, b, a}); }
void UIApi::UI_Style_SetActiveColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a) { if (handle) handle->style.ActiveColor({r, g, b, a}); }
void UIApi::UI_Style_SetAlign(SPF_TextStyle_Handle handle, SPF_TextAlign align) { if (handle) handle->style.Align((SPF::UI::TextAlign)align); }
void UIApi::UI_Style_SetWrap(SPF_TextStyle_Handle handle, bool wrap) { if (handle) handle->style.Wrapped(wrap); }
void UIApi::UI_Style_SetPadding(SPF_TextStyle_Handle handle, float pad_x, float pad_y) { if (handle) handle->style.Padding({pad_x, pad_y}); }
void UIApi::UI_Style_SetSeparator(SPF_TextStyle_Handle handle, bool is_separator) { if (handle) handle->style.Separator(is_separator); }
void UIApi::UI_Style_SetUnderline(SPF_TextStyle_Handle handle, bool is_underline) { if (handle) handle->style.Underline(is_underline); }
void UIApi::UI_Style_SetStrikethrough(SPF_TextStyle_Handle handle, bool is_strikethrough) { if (handle) handle->style.Strikethrough(is_strikethrough); }

void UIApi::UI_TextStyled(SPF_TextStyle_Handle handle, const char* fmt, ...) {
    if (!fmt) return;
    va_list args; va_start(args, fmt);
    if (handle) SPF::UI::Typography::TextV(handle->style, fmt, args);
    else SPF::UI::Typography::TextV(SPF::UI::TextStyle::Regular(), fmt, args);
    va_end(args);
}
void UIApi::UI_RenderMarkdown(const char* markdown_text, SPF_TextStyle_Handle base_style_handle) {
    if (!markdown_text) return;
    if (base_style_handle) SPF::UI::Typography::RenderMarkdownText(markdown_text, base_style_handle->style);
    else SPF::UI::Typography::RenderMarkdownText(markdown_text, SPF::UI::TextStyle::Regular());
}

// --- XII. DrawList API ---

void UIApi::UI_DrawList_PushClipRect(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, bool intersect_with_current_clip_rect) { if (dl) ((ImDrawList*)dl)->PushClipRect({p_min_x, p_min_y}, {p_max_x, p_max_y}, intersect_with_current_clip_rect); }
void UIApi::UI_DrawList_PushClipRectFullScreen(SPF_DrawList_Handle dl) { if (dl) ((ImDrawList*)dl)->PushClipRectFullScreen(); }
void UIApi::UI_DrawList_PopClipRect(SPF_DrawList_Handle dl) { if (dl) ((ImDrawList*)dl)->PopClipRect(); }

void UIApi::UI_DrawList_AddLine(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness) { if (dl) ((ImDrawList*)dl)->AddLine({p1_x, p1_y}, {p2_x, p2_y}, col, thickness); }
void UIApi::UI_DrawList_AddRect(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, SPF_DrawFlags flags, float thickness) { if (dl) ((ImDrawList*)dl)->AddRect({p_min_x, p_min_y}, {p_max_x, p_max_y}, col, rounding, (ImDrawFlags)flags, thickness); }
void UIApi::UI_DrawList_AddRectFilled(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, SPF_DrawFlags flags) { if (dl) ((ImDrawList*)dl)->AddRectFilled({p_min_x, p_min_y}, {p_max_x, p_max_y}, col, rounding, (ImDrawFlags)flags); }
void UIApi::UI_DrawList_AddRectFilledMultiColor(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left) { if (dl) ((ImDrawList*)dl)->AddRectFilledMultiColor({p_min_x, p_min_y}, {p_max_x, p_max_y}, col_upr_left, col_upr_right, col_bot_right, col_bot_left); }

void UIApi::UI_DrawList_AddQuad(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col, float thickness) { if (dl) ((ImDrawList*)dl)->AddQuad({p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, {p4_x, p4_y}, col, thickness); }
void UIApi::UI_DrawList_AddQuadFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col) { if (dl) ((ImDrawList*)dl)->AddQuadFilled({p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, {p4_x, p4_y}, col); }
void UIApi::UI_DrawList_AddTriangle(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col, float thickness) { if (dl) ((ImDrawList*)dl)->AddTriangle({p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, col, thickness); }
void UIApi::UI_DrawList_AddTriangleFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col) { if (dl) ((ImDrawList*)dl)->AddTriangleFilled({p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, col); }

void UIApi::UI_DrawList_AddTriangleFilledMultiColor(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col1, uint32_t col2, uint32_t col3) {
    if (!dl) return;
    ImDrawList* drawList = (ImDrawList*)dl;
    if (((col1 | col2 | col3) & IM_COL32_A_MASK) == 0) return;
    if (drawList->_VtxCurrentIdx + 6 >= 65535) return;
    ImVec2 p[3] = { {p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y} };
    uint32_t c[3] = { col1, col2, col3 };
    uint32_t ct[3] = { col1 & ~IM_COL32_A_MASK, col2 & ~IM_COL32_A_MASK, col3 & ~IM_COL32_A_MASK };
    ImVec2 n[3];
    for (int i = 0; i < 3; i++) {
        ImVec2 d = { p[(i + 1) % 3].x - p[i].x, p[(i + 1) % 3].y - p[i].y };
        float m2 = d.x * d.x + d.y * d.y;
        if (m2 > 0.000001f) { float inv_m = 1.0f / sqrtf(m2); n[i] = { d.y * inv_m, -d.x * inv_m }; } else n[i] = { 0, 0 };
    }
    ImVec2 center = { (p[0].x + p[1].x + p[2].x) / 3.0f, (p[0].y + p[1].y + p[2].y) / 3.0f };
    if ((p[0].x - center.x) * n[0].x + (p[0].y - center.y) * n[0].y < 0) { for (int i = 0; i < 3; i++) { n[i].x = -n[i].x; n[i].y = -n[i].y; } }
    ImVec2 vn[3];
    for (int i = 0; i < 3; i++) {
        vn[i] = { n[i].x + n[(i + 2) % 3].x, n[i].y + n[(i + 2) % 3].y };
        float m2 = vn[i].x * vn[i].x + vn[i].y * vn[i].y;
        if (m2 > 0.000001f) { float inv_m = 1.0f / sqrtf(m2); vn[i].x *= inv_m; vn[i].y *= inv_m; }
    }
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    drawList->PrimReserve(21, 6);
    ImDrawIdx base_idx = (ImDrawIdx)drawList->_VtxCurrentIdx;
    drawList->PrimWriteIdx(base_idx); drawList->PrimWriteIdx((ImDrawIdx)(base_idx + 1)); drawList->PrimWriteIdx((ImDrawIdx)(base_idx + 2));
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        drawList->PrimWriteIdx(base_idx + i); drawList->PrimWriteIdx(base_idx + j); drawList->PrimWriteIdx(base_idx + 3 + j);
        drawList->PrimWriteIdx(base_idx + i); drawList->PrimWriteIdx(base_idx + 3 + j); drawList->PrimWriteIdx(base_idx + 3 + i);
    }
    for (int i = 0; i < 3; i++) drawList->PrimVtx(p[i], uv, c[i]);
    for (int i = 0; i < 3; i++) drawList->PrimVtx({ p[i].x + vn[i].x, p[i].y + vn[i].y }, uv, ct[i]);
}

void UIApi::UI_DrawList_AddCircle(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness) { if (dl) ((ImDrawList*)dl)->AddCircle({center_x, center_y}, radius, col, num_segments, thickness); }
void UIApi::UI_DrawList_AddCircleFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments) { if (dl) ((ImDrawList*)dl)->AddCircleFilled({center_x, center_y}, radius, col, num_segments); }

void UIApi::UI_DrawList_AddCircleFilledMultiColor(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col_inner, uint32_t col_outer, int num_segments) {
    if (!dl || radius <= 0.0f) return;
    ImDrawList* drawList = (ImDrawList*)dl;
    if (((col_inner | col_outer) & IM_COL32_A_MASK) == 0) return;
    if (num_segments <= 0) { num_segments = (int)(radius * 3.0f); if (num_segments < 16) num_segments = 16; if (num_segments > 64) num_segments = 64; }
    int vtx_count = num_segments * 2 + 1;
    int idx_count = num_segments * 9;
    if (drawList->_VtxCurrentIdx + vtx_count >= 65535) return;
    drawList->PrimReserve(idx_count, vtx_count);
    ImDrawIdx base_idx = (ImDrawIdx)drawList->_VtxCurrentIdx;
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();
    uint32_t col_outer_trans = col_outer & ~IM_COL32_A_MASK;
    for (int i = 0; i < num_segments; i++) {
        int j = (i + 1) % num_segments;
        drawList->PrimWriteIdx(base_idx); drawList->PrimWriteIdx(base_idx + 1 + i); drawList->PrimWriteIdx(base_idx + 1 + j);
        drawList->PrimWriteIdx(base_idx + 1 + i); drawList->PrimWriteIdx(base_idx + 1 + j); drawList->PrimWriteIdx(base_idx + 1 + num_segments + j);
        drawList->PrimWriteIdx(base_idx + 1 + i); drawList->PrimWriteIdx(base_idx + 1 + num_segments + j); drawList->PrimWriteIdx(base_idx + 1 + num_segments + i);
    }
    drawList->PrimVtx({ center_x, center_y }, uv, col_inner);
    float angle_step = (2.0f * 3.1415926535f) / (float)num_segments;
    for (int i = 0; i < num_segments; i++) {
        float a = (float)i * angle_step; float c = cosf(a), s = sinf(a);
        drawList->PrimVtx({ center_x + c * radius, center_y + s * radius }, uv, col_outer);
    }
    for (int i = 0; i < num_segments; i++) {
        float a = (float)i * angle_step; float c = cosf(a), s = sinf(a);
        drawList->PrimVtx({ center_x + c * (radius + 1.0f), center_y + s * (radius + 1.0f) }, uv, col_outer_trans);
    }
}

void UIApi::UI_AddRectFilled(float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (drawList) drawList->AddRectFilled({x1, y1}, {x2, y2}, ImGui::ColorConvertFloat4ToU32({r, g, b, a}));
}

void UIApi::UI_DrawList_AddNgon(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness) { if (dl) ((ImDrawList*)dl)->AddNgon({center_x, center_y}, radius, col, num_segments, thickness); }
void UIApi::UI_DrawList_AddNgonFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments) { if (dl) ((ImDrawList*)dl)->AddNgonFilled({center_x, center_y}, radius, col, num_segments); }
void UIApi::UI_DrawList_AddNgonContour(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness) {
    if (dl) ((ImDrawList*)dl)->AddNgon({center_x, center_y}, radius, col, num_segments, thickness);
}
void UIApi::UI_DrawList_AddEllipse(SPF_DrawList_Handle dl, float center_x, float center_y, float radius_x, float radius_y, uint32_t col, float rot, int num_segments, float thickness) {
    if (dl) ((ImDrawList*)dl)->AddEllipse({center_x, center_y}, {radius_x, radius_y}, col, rot, num_segments, thickness);
}
void UIApi::UI_DrawList_AddEllipseFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius_x, float radius_y, uint32_t col, float rot, int num_segments) {
    if (dl) ((ImDrawList*)dl)->AddEllipseFilled({center_x, center_y}, {radius_x, radius_y}, col, rot, num_segments);
}

void UIApi::UI_DrawList_AddBezierCubic(SPF_DrawList_Handle dl, float p1_x, float p1_y, float cp1_x, float cp1_y, float cp2_x, float cp2_y, float p2_x, float p2_y, uint32_t col, float thickness, int num_segments) { if (dl) ((ImDrawList*)dl)->AddBezierCubic({p1_x, p1_y}, {cp1_x, cp1_y}, {cp2_x, cp2_y}, {p2_x, p2_y}, col, thickness, num_segments); }
void UIApi::UI_DrawList_AddBezierQuadratic(SPF_DrawList_Handle dl, float p1_x, float p1_y, float cp_x, float cp_y, float p2_x, float p2_y, uint32_t col, float thickness, int num_segments) { if (dl) ((ImDrawList*)dl)->AddBezierQuadratic({p1_x, p1_y}, {cp_x, cp_y}, {p2_x, p2_y}, col, thickness, num_segments); }

void UIApi::UI_DrawList_AddPolyline(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col, SPF_DrawFlags flags, float thickness) {
    if (!dl || !points_x || !points_y || num_points < 2) return;
    std::vector<ImVec2> pts; pts.reserve(num_points);
    for(int i=0; i<num_points; ++i) pts.push_back({points_x[i], points_y[i]});
    ((ImDrawList*)dl)->AddPolyline(pts.data(), num_points, col, (ImDrawFlags)flags, thickness);
}
void UIApi::UI_DrawList_AddConvexPolyFilled(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col) {
    if (!dl || !points_x || !points_y || num_points < 3) return;
    std::vector<ImVec2> pts; pts.reserve(num_points);
    for(int i=0; i<num_points; ++i) pts.push_back({points_x[i], points_y[i]});
    ((ImDrawList*)dl)->AddConvexPolyFilled(pts.data(), num_points, col);
}
void UIApi::UI_DrawList_AddConcavePolyFilled(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col) {
    UI_DrawList_AddConvexPolyFilled(dl, points_x, points_y, num_points, col);
}

void UIApi::UI_DrawList_AddImage(SPF_DrawList_Handle dl, void* user_texture_id, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col) { if (dl) ((ImDrawList*)dl)->AddImage(user_texture_id, {p_min_x, p_min_y}, {p_max_x, p_max_y}, {uv_min_x, uv_min_y}, {uv_max_x, uv_max_y}, col); }
void UIApi::UI_DrawList_AddImageQuad(SPF_DrawList_Handle dl, void* user_texture_id, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, float uv1_x, float uv1_y, float uv2_x, float uv2_y, float uv3_x, float uv3_y, float uv4_x, float uv4_y, uint32_t col) { if (dl) ((ImDrawList*)dl)->AddImageQuad(user_texture_id, {p1_x, p1_y}, {p2_x, p2_y}, {p3_x, p3_y}, {p4_x, p4_y}, {uv1_x, uv1_y}, {uv2_x, uv2_y}, {uv3_x, uv3_y}, {uv4_x, uv4_y}, col); }
void UIApi::UI_DrawList_AddImageRounded(SPF_DrawList_Handle dl, void* user_texture_id, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col, float rounding, SPF_DrawFlags flags) { if (dl) ((ImDrawList*)dl)->AddImageRounded(user_texture_id, {p_min_x, p_min_y}, {p_max_x, p_max_y}, {uv_min_x, uv_min_y}, {uv_max_x, uv_max_y}, col, rounding, (ImDrawFlags)flags); }
void UIApi::UI_DrawList_AddCallback(SPF_DrawList_Handle dl, void (*callback)(const void* parent_list, const void* cmd), void* user_data) { if (dl) ((ImDrawList*)dl)->AddCallback((ImDrawCallback)callback, user_data); }

void UIApi::UI_DrawList_AddText(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text) { if (dl && text) ((ImDrawList*)dl)->AddText({pos_x, pos_y}, col, text); }
void UIApi::UI_DrawList_AddTextWithFont(SPF_DrawList_Handle dl, SPF_Font font, float font_size, float pos_x, float pos_y, uint32_t col, const char* text, float wrap_width) {
    if (!dl || !text) return;
    const char* fontName = "regular";
    switch (font) {
        case SPF_FONT_BOLD: fontName = "bold"; break;
        case SPF_FONT_ITALIC: fontName = "italic"; break;
        case SPF_FONT_BOLD_ITALIC: fontName = "bold_italic"; break;
        case SPF_FONT_MEDIUM: fontName = "medium"; break;
        case SPF_FONT_MEDIUM_ITALIC: fontName = "medium_italic"; break;
        case SPF_FONT_MONOSPACE: fontName = "monospace"; break;
        case SPF_FONT_H1: fontName = "h1"; break;
        case SPF_FONT_H2: fontName = "h2"; break;
        case SPF_FONT_H3: fontName = "h3"; break;
        case SPF_FONT_H1_LARGE_BOLD: fontName = "h1_large_bold"; break;
    }
    ImFont* imFont = UI::UIManager::GetInstance().GetFont(fontName);
    if (imFont) {
        ((ImDrawList*)dl)->AddText(imFont, font_size, {pos_x, pos_y}, col, text, nullptr, wrap_width);
    } else {
        ((ImDrawList*)dl)->AddText({pos_x, pos_y}, col, text);
    }
}

void UIApi::UI_DrawList_AddTextWithFontHandle(SPF_DrawList_Handle dl, SPF_Font_Handle font_handle, float font_size, float pos_x, float pos_y, uint32_t col, const char* text, float wrap_width) {
    if (dl && font_handle && text) {
        ((ImDrawList*)dl)->AddText((ImFont*)font_handle, font_size, {pos_x, pos_y}, col, text, nullptr, wrap_width);
    }
}

void UIApi::UI_DrawList_PathClear(SPF_DrawList_Handle dl) { if (dl) ((ImDrawList*)dl)->PathClear(); }
void UIApi::UI_DrawList_PathLineTo(SPF_DrawList_Handle dl, float pos_x, float pos_y) { if (dl) ((ImDrawList*)dl)->PathLineTo({pos_x, pos_y}); }
void UIApi::UI_DrawList_PathStroke(SPF_DrawList_Handle dl, uint32_t col, SPF_DrawFlags flags, float thickness) { if (dl) ((ImDrawList*)dl)->PathStroke(col, (ImDrawFlags)flags, thickness); }
void UIApi::UI_DrawList_PathFillConvex(SPF_DrawList_Handle dl, uint32_t col) { if (dl) ((ImDrawList*)dl)->PathFillConvex(col); }
void UIApi::UI_DrawList_PathArcTo(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, float a_min, float a_max, int num_segments) { if (dl) ((ImDrawList*)dl)->PathArcTo({center_x, center_y}, radius, a_min, a_max, num_segments); }
void UIApi::UI_DrawList_PathRect(SPF_DrawList_Handle dl, float rect_min_x, float rect_min_y, float rect_max_x, float rect_max_y, float rounding, SPF_DrawFlags flags) { if (dl) ((ImDrawList*)dl)->PathRect({rect_min_x, rect_min_y}, {rect_max_x, rect_max_y}, rounding, (ImDrawFlags)flags); }

void UIApi::UI_DrawList_ChannelsSplit(SPF_DrawList_Handle dl, int count) { if (dl) ((ImDrawList*)dl)->ChannelsSplit(count); }
void UIApi::UI_DrawList_ChannelsMerge(SPF_DrawList_Handle dl) { if (dl) ((ImDrawList*)dl)->ChannelsMerge(); }
void UIApi::UI_DrawList_ChannelsSetCurrent(SPF_DrawList_Handle dl, int n) { if (dl) ((ImDrawList*)dl)->ChannelsSetCurrent(n); }

void UIApi::UI_DrawList_PrimReserve(SPF_DrawList_Handle dl, int idx_count, int vtx_count) { if (dl) ((ImDrawList*)dl)->PrimReserve(idx_count, vtx_count); }
void UIApi::UI_DrawList_PrimRectUV(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col) { if (dl) ((ImDrawList*)dl)->PrimRectUV({p_min_x, p_min_y}, {p_max_x, p_max_y}, {uv_min_x, uv_min_y}, {uv_max_x, uv_max_y}, col); }
void UIApi::UI_DrawList_PrimVtx(SPF_DrawList_Handle dl, float x, float y, float u, float v, uint32_t col) { if (dl) ((ImDrawList*)dl)->PrimVtx({x, y}, {u, v}, col); }
void UIApi::UI_DrawList_PrimIdx(SPF_DrawList_Handle dl, uint16_t idx) { if (dl) ((ImDrawList*)dl)->PrimWriteIdx(idx); }

// --- XIII. Item Queries & State ---

bool UIApi::UI_IsItemHovered(SPF_HoveredFlags flags) { return ImGui::IsItemHovered((ImGuiHoveredFlags)flags); }
bool UIApi::UI_IsItemActive() { return ImGui::IsItemActive(); }
bool UIApi::UI_IsItemFocused() { return ImGui::IsItemFocused(); }
bool UIApi::UI_IsItemClicked(SPF_MouseButton mouse_button) { return ImGui::IsItemClicked((ImGuiMouseButton)mouse_button); }
bool UIApi::UI_IsItemVisible() { return ImGui::IsItemVisible(); }
bool UIApi::UI_IsItemEdited() { return ImGui::IsItemEdited(); }
bool UIApi::UI_IsItemActivated() { return ImGui::IsItemActivated(); }
bool UIApi::UI_IsItemDeactivated() { return ImGui::IsItemDeactivated(); }
bool UIApi::UI_IsItemDeactivatedAfterEdit() { return ImGui::IsItemDeactivatedAfterEdit(); }
bool UIApi::UI_IsItemToggledOpen() { return ImGui::IsItemToggledOpen(); }
bool UIApi::UI_IsAnyItemHovered() { return ImGui::IsAnyItemHovered(); }
bool UIApi::UI_IsAnyItemActive() { return ImGui::IsAnyItemActive(); }
bool UIApi::UI_IsAnyItemFocused() { return ImGui::IsAnyItemFocused(); }
uint32_t UIApi::UI_GetItemID() { return (uint32_t)ImGui::GetItemID(); }

void UIApi::UI_GetItemRectMin(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetItemRectMin();
    if (out_x) *out_x = pos.x; if (out_y) *out_y = pos.y;
}
void UIApi::UI_GetItemRectMax(float* out_x, float* out_y) {
    ImVec2 pos = ImGui::GetItemRectMax();
    if (out_x) *out_x = pos.x; if (out_y) *out_y = pos.y;
}
void UIApi::UI_GetItemRectSize(float* out_x, float* out_y) {
    ImVec2 sz = ImGui::GetItemRectSize();
    if (out_x) *out_x = sz.x; if (out_y) *out_y = sz.y;
}

// --- XIV. Tabs & Tab Bars ---

bool UIApi::UI_BeginTabBar(const char* str_id, SPF_TabBarFlags flags) { return ImGui::BeginTabBar(str_id, (ImGuiTabBarFlags)flags); }
void UIApi::UI_EndTabBar() { ImGui::EndTabBar(); }
bool UIApi::UI_BeginTabItem(const char* label, bool* p_open, SPF_TabItemFlags flags) { return ImGui::BeginTabItem(label, p_open, (ImGuiTabItemFlags)flags); }
void UIApi::UI_EndTabItem() { ImGui::EndTabItem(); }
bool UIApi::UI_TabItemButton(const char* label, SPF_TabItemFlags flags) { return ImGui::TabItemButton(label, (ImGuiTabItemFlags)flags); }
void UIApi::UI_SetTabItemClosed(const char* tab_or_docked_window_label) { ImGui::SetTabItemClosed(tab_or_docked_window_label); }

// --- XV. Docking & Viewports ---

uint32_t UIApi::UI_DockSpace(uint32_t id, float size_x, float size_y, SPF_DockNodeFlags flags) { return ImGui::DockSpace(id, {size_x, size_y}, (ImGuiDockNodeFlags)flags); }
uint32_t UIApi::UI_DockSpaceOverViewport(SPF_DockNodeFlags flags) { return ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), (ImGuiDockNodeFlags)flags); }
void UIApi::UI_SetNextWindowDockID(uint32_t dock_id, SPF_Cond cond) { ImGui::SetNextWindowDockID(dock_id, (ImGuiCond)cond); }
bool UIApi::UI_IsWindowDocked() { return ImGui::IsWindowDocked(); }

void UIApi::UI_DockBuilderDockWindow(const char* window_name, uint32_t node_id) { ImGui::DockBuilderDockWindow(window_name, node_id); }
void* UIApi::UI_DockBuilderGetNode(uint32_t node_id) { return (void*)ImGui::DockBuilderGetNode(node_id); }
uint32_t UIApi::UI_DockBuilderAddNode(uint32_t node_id, SPF_DockNodeFlags flags) { return ImGui::DockBuilderAddNode(node_id, (ImGuiDockNodeFlags)flags); }
void UIApi::UI_DockBuilderRemoveNode(uint32_t node_id) { ImGui::DockBuilderRemoveNode(node_id); }
void UIApi::UI_DockBuilderRemoveNodeDockedWindows(uint32_t node_id) { ImGui::DockBuilderRemoveNodeDockedWindows(node_id); }
void UIApi::UI_DockBuilderSetNodePos(uint32_t node_id, float pos_x, float pos_y) { ImGui::DockBuilderSetNodePos(node_id, {pos_x, pos_y}); }
void UIApi::UI_DockBuilderSetNodeSize(uint32_t node_id, float size_x, float size_y) { ImGui::DockBuilderSetNodeSize(node_id, {size_x, size_y}); }
uint32_t UIApi::UI_DockBuilderSplitNode(uint32_t node_id, SPF_Dir split_dir, float size_ratio, uint32_t* out_id_at_dir, uint32_t* out_id_at_opposite) { return ImGui::DockBuilderSplitNode(node_id, (ImGuiDir)split_dir, size_ratio, (ImGuiID*)out_id_at_dir, (ImGuiID*)out_id_at_opposite); }
void UIApi::UI_DockBuilderFinish(uint32_t node_id) { ImGui::DockBuilderFinish(node_id); }
uint32_t UIApi::UI_DockBuilderGetCentralNode(uint32_t node_id) { 
    ImGuiDockNode* node = ImGui::DockBuilderGetCentralNode(node_id);
    return node ? node->ID : 0;
}

void UIApi::UI_UpdatePlatformWindows() { ImGui::UpdatePlatformWindows(); }
void UIApi::UI_RenderPlatformWindowsDefault() { ImGui::RenderPlatformWindowsDefault(); }
void UIApi::UI_DestroyPlatformWindows() { ImGui::DestroyPlatformWindows(); }

// --- XVI. Internal & Custom Widget Utilities ---

void UIApi::UI_ItemSize(float size_x, float size_y, float text_baseline_y) { ImGui::ItemSize({size_x, size_y}, text_baseline_y); }
bool UIApi::UI_ItemAdd(float min_x, float min_y, float max_x, float max_y, uint32_t id, SPF_WindowFlags flags) { return ImGui::ItemAdd({min_x, min_y, max_x, max_y}, id, nullptr, (ImGuiWindowFlags)flags); }
void UIApi::UI_SetLastItemData(uint32_t item_id, SPF_InputTextFlags flags, SPF_HoveredFlags status_flags, float min_x, float min_y, float max_x, float max_y) { ImGui::SetLastItemData(item_id, (ImGuiItemFlags)flags, (ImGuiItemStatusFlags)status_flags, {min_x, min_y, max_x, max_y}); }
bool UIApi::UI_ButtonBehavior(float min_x, float min_y, float max_x, float max_y, uint32_t id, bool* out_hovered, bool* out_held, SPF_WindowFlags flags) { return ImGui::ButtonBehavior({min_x, min_y, max_x, max_y}, id, out_hovered, out_held, (ImGuiButtonFlags)flags); }
bool UIApi::UI_DragBehavior(uint32_t id, SPF_DataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags) { return ImGui::DragBehavior(id, (ImGuiDataType)data_type, p_v, v_speed, p_min, p_max, format, (ImGuiSliderFlags)flags); }
bool UIApi::UI_SliderBehavior(float min_x, float min_y, float max_x, float max_y, uint32_t id, SPF_DataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags) { 
    return ImGui::SliderBehavior(ImRect(min_x, min_y, max_x, max_y), id, (ImGuiDataType)data_type, p_v, p_min, p_max, format, (ImGuiSliderFlags)flags, NULL); 
}
bool UIApi::UI_SplitterBehavior(float bb_min_x, float bb_min_y, float bb_max_x, float bb_max_y, uint32_t id, int axis, float* size1, float* size2, float min_size1, float min_size2) { 
    return ImGui::SplitterBehavior(ImRect(bb_min_x, bb_min_y, bb_max_x, bb_max_y), id, (ImGuiAxis)axis, size1, size2, min_size1, min_size2); 
}

void UIApi::UI_RenderFrame(float min_x, float min_y, float max_x, float max_y, uint32_t col, bool border, float rounding) { ImGui::RenderFrame({min_x, min_y}, {max_x, max_y}, col, border, rounding); }
void UIApi::UI_RenderFrameBorder(float min_x, float min_y, float max_x, float max_y, float rounding) { ImGui::RenderFrameBorder({min_x, min_y}, {max_x, max_y}, rounding); }
void UIApi::UI_RenderText(float x, float y, const char* text, bool hide_text_after_hash) { ImGui::RenderText({x, y}, text, nullptr, hide_text_after_hash); }
void UIApi::UI_RenderTextClipped(float min_x, float min_y, float max_x, float max_y, const char* text, const char* text_end, float* out_text_size, float align_x, float align_y) { ImVec2 sz; ImGui::RenderTextClipped({min_x, min_y}, {max_x, max_y}, text, text_end, &sz, {align_x, align_y}); if (out_text_size) { out_text_size[0] = sz.x; out_text_size[1] = sz.y; } }
void UIApi::UI_RenderTextEllipsis(float x, float y, float max_width, const char* text) { 
    ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), ImVec2(x, y), ImVec2(x + max_width, y + 30.0f), x + max_width, text, nullptr, nullptr); 
}
void UIApi::UI_RenderArrow(float x, float y, uint32_t col, SPF_Dir dir, float scale) { ImGui::RenderArrow(ImGui::GetWindowDrawList(), {x, y}, col, (ImGuiDir)dir, scale); }
void UIApi::UI_RenderCheckMark(float x, float y, uint32_t col, float sz) { ImGui::RenderCheckMark(ImGui::GetWindowDrawList(), {x, y}, col, sz); }
void UIApi::UI_RenderBullet(SPF_DrawList_Handle dl, float x, float y, uint32_t col) { if (dl) ImGui::RenderBullet((ImDrawList*)dl, {x, y}, col); }

// --- XVII. Framework Utilities ---

SPF_Notification_Handle UIApi::UI_ShowNotification(const SPF_Notification_Params* params) {
    if (!params) return nullptr;
    return UIManager::GetInstance().ShowNotificationEx(params);
}

void UIApi::UI_HideNotification(SPF_Notification_Handle handle) {
    UIManager::GetInstance().HideNotification(handle);
}
void UIApi::UI_PlayTransition(SPF_TransitionType type, float duration, bool reverse, SPF_TransitionColor color) {
    UIManager::GetInstance().PlayTransition((int)type, duration, reverse, (int)color);
}
bool UIApi::UI_IsTransitionActive() { return UIManager::GetInstance().IsTransitionActive(); }

void UIApi::UI_SetMouseBlockState(bool blockAxes, bool blockButtons, bool blockWheel) {
    Input::InputManager::GetInstance().SetProgrammaticMouseBlock(blockAxes, blockButtons, blockWheel);
}
void UIApi::UI_SetMouseOverride(bool overridden) { UIManager::GetInstance().SetMouseOverride(overridden); }
bool UIApi::UI_IsMouseOverridden() { return UIManager::GetInstance().IsMouseOverridden(); }

// --- XIV. RESOURCE MANAGEMENT (TEXTURES & FONTS) ---

void* UIApi::UI_CreateTextureFromMemory(const void* data, size_t size, int* out_width, int* out_height) {
    return UIManager::GetInstance().CreatePluginTexture(data, size, out_width, out_height);
}

void* UIApi::UI_CreateTextureFromFile(const char* file_path, int* out_width, int* out_height) {
    return UIManager::GetInstance().CreatePluginTextureFromFile(file_path, out_width, out_height);
}

void UIApi::UI_DestroyTexture(void* texture_id) {
    UIManager::GetInstance().DestroyPluginTexture(texture_id);
}

SPF_Font_Handle UIApi::UI_LoadFontFromMemory(const char* name, const void* data, size_t data_size, const SPF_Font_Config* config) {
    if (!config) return nullptr;
    return reinterpret_cast<SPF_Font_Handle>(UIManager::GetInstance().LoadPluginFontFromMemory(name, data, data_size, config->size_pixels, config->merge_mode, config->ranges));
}

SPF_Font_Handle UIApi::UI_LoadFontFromFile(const char* name, const char* file_path, const SPF_Font_Config* config) {
    if (!config) return nullptr;
    return reinterpret_cast<SPF_Font_Handle>(UIManager::GetInstance().LoadPluginFontFromFile(name, file_path, config->size_pixels, config->merge_mode, config->ranges));
}

// --- Fill API ---

void UIApi::FillUIApi(SPF_UI_API* api) {
    if (!api) return;
    api->UI_RegisterDrawCallback = &UIApi::UI_RegisterDrawCallback;
    api->UI_RegisterDrawCallbackWithFlags = &UIApi::UI_RegisterDrawCallbackWithFlags;
    api->UI_GetWindowHandle = &UIApi::UI_GetWindowHandle;
    api->UI_SetVisibility = &UIApi::UI_SetVisibility;
    api->UI_IsVisible = &UIApi::UI_IsVisible;
    api->UI_SetFocus = &UIApi::UI_SetFocus;
    api->UI_GetCurrentContext = &UIApi::UI_GetCurrentContext;
    api->UI_SetCurrentContext = &UIApi::UI_SetCurrentContext;
    api->UI_SetAllocatorFunctions = &UIApi::UI_SetAllocatorFunctions;
    api->UI_MemAlloc = &UIApi::UI_MemAlloc;
    api->UI_MemFree = &UIApi::UI_MemFree;
    api->UI_GetDeltaTime = &UIApi::UI_GetDeltaTime;
    api->UI_GetTime = &UIApi::UI_GetTime;
    api->UI_GetFrameCount = &UIApi::UI_GetFrameCount;
    api->UI_GetFramerate = &UIApi::UI_GetFramerate;
    api->UI_GetIO_ConfigFlags = &UIApi::UI_GetIO_ConfigFlags;
    api->UI_GetIO_BackendFlags = &UIApi::UI_GetIO_BackendFlags;
    api->UI_GetMousePos = &UIApi::UI_GetMousePos;
    api->UI_IsMouseDown = &UIApi::UI_IsMouseDown;
    api->UI_IsMouseClicked = &UIApi::UI_IsMouseClicked;
    api->UI_IsMouseReleased = &UIApi::UI_IsMouseReleased;
    api->UI_IsMouseDoubleClicked = &UIApi::UI_IsMouseDoubleClicked;
    api->UI_IsMouseHoveringRect = &UIApi::UI_IsMouseHoveringRect;
    api->UI_IsMousePosValid = &UIApi::UI_IsMousePosValid;
    api->UI_GetMousePosOnOpeningCurrentPopup = &UIApi::UI_GetMousePosOnOpeningCurrentPopup;
    api->UI_GetMouseWheel = &UIApi::UI_GetMouseWheel;
    api->UI_GetMouseWheelH = &UIApi::UI_GetMouseWheelH;
    api->UI_Shortcut = &UIApi::UI_Shortcut;
    api->UI_SetNextItemShortcut = &UIApi::UI_SetNextItemShortcut;
    api->UI_SetItemKeyOwner = &UIApi::UI_SetItemKeyOwner;
    api->UI_IsMouseDragging = &UIApi::UI_IsMouseDragging;
    api->UI_GetMouseDragDelta = &UIApi::UI_GetMouseDragDelta;
    api->UI_ResetMouseDragDelta = &UIApi::UI_ResetMouseDragDelta;
    api->UI_SetMouseCursor = &UIApi::UI_SetMouseCursor;
    api->UI_GetMouseCursor = &UIApi::UI_GetMouseCursor;
    api->UI_IsKeyDown = &UIApi::UI_IsKeyDown;
    api->UI_IsKeyPressed = &UIApi::UI_IsKeyPressed;
    api->UI_IsKeyReleased = &UIApi::UI_IsKeyReleased;
    api->UI_GetKeyPressedAmount = &UIApi::UI_GetKeyPressedAmount;
    api->UI_IsMouseReleasedWithDelay = &UIApi::UI_IsMouseReleasedWithDelay;
    api->UI_GetKeyName = &UIApi::UI_GetKeyName;
    api->UI_GetClipboardText = &UIApi::UI_GetClipboardText;
    api->UI_SetClipboardText = &UIApi::UI_SetClipboardText;
    api->UI_IsWindowAppearing = &UIApi::UI_IsWindowAppearing;
    api->UI_IsWindowCollapsed = &UIApi::UI_IsWindowCollapsed;
    api->UI_IsWindowFocused = &UIApi::UI_IsWindowFocused;
    api->UI_IsWindowHovered = &UIApi::UI_IsWindowHovered;
    api->UI_GetStateStorage = &UIApi::UI_GetStateStorage;
    api->UI_SetStateStorage = &UIApi::UI_SetStateStorage;
    api->UI_GetWindowViewport = &UIApi::UI_GetWindowViewport;
    api->UI_GetWindowDrawList = &UIApi::UI_GetWindowDrawList;
    api->UI_GetBackgroundDrawList = &UIApi::UI_GetBackgroundDrawList;
    api->UI_GetForegroundDrawList = &UIApi::UI_GetForegroundDrawList;
    api->UI_GetWindowPos = &UIApi::UI_GetWindowPos;
    api->UI_GetWindowSize = &UIApi::UI_GetWindowSize;
    api->UI_GetWindowWidth = &UIApi::UI_GetWindowWidth;
    api->UI_GetWindowHeight = &UIApi::UI_GetWindowHeight;
    api->UI_GetWindowDpiScale = &UIApi::UI_GetWindowDpiScale;
    api->UI_SetWindowPos = &UIApi::UI_SetWindowPos;
    api->UI_SetWindowSize = &UIApi::UI_SetWindowSize;
    api->UI_SetNextWindowPos = &UIApi::UI_SetNextWindowPos;
    api->UI_SetNextWindowSize = &UIApi::UI_SetNextWindowSize;
    api->UI_SetNextWindowViewport = &UIApi::UI_SetNextWindowViewport;
    api->UI_SetNextWindowScroll = &UIApi::UI_SetNextWindowScroll;
    api->UI_GetScrollX = &UIApi::UI_GetScrollX;
    api->UI_GetScrollY = &UIApi::UI_GetScrollY;
    api->UI_GetScrollMaxX = &UIApi::UI_GetScrollMaxX;
    api->UI_GetScrollMaxY = &UIApi::UI_GetScrollMaxY;
    api->UI_SetScrollX = &UIApi::UI_SetScrollX;
    api->UI_SetScrollY = &UIApi::UI_SetScrollY;
    api->UI_SetScrollHereY = &UIApi::UI_SetScrollHereY;
    api->UI_SetNextWindowFocus = &UIApi::UI_SetNextWindowFocus;
    api->UI_SetNextWindowCollapsed = &UIApi::UI_SetNextWindowCollapsed;
    api->UI_SetWindowCollapsed = &UIApi::UI_SetWindowCollapsed;
    api->UI_SetWindowFocus = &UIApi::UI_SetWindowFocus;
    api->UI_FocusWindow = &UIApi::UI_FocusWindow;
    api->UI_BringWindowToFocusFront = &UIApi::UI_BringWindowToFocusFront;
    api->UI_SetWindowFontScale = &UIApi::UI_SetWindowFontScale;
    api->UI_SetNextWindowSizeConstraints = &UIApi::UI_SetNextWindowSizeConstraints;
    api->UI_SetNextWindowBgAlpha = &UIApi::UI_SetNextWindowBgAlpha;
    api->UI_SetNextWindowContentSize = &UIApi::UI_SetNextWindowContentSize;
    api->UI_BringWindowToDisplayFront = &UIApi::UI_BringWindowToDisplayFront;
    api->UI_BringWindowToDisplayBack = &UIApi::UI_BringWindowToDisplayBack;
    api->UI_GetMainViewportPos = &UIApi::UI_GetMainViewportPos;
    api->UI_GetMainViewportSize = &UIApi::UI_GetMainViewportSize;
    api->UI_BeginChild = &UIApi::UI_BeginChild;
    api->UI_EndChild = &UIApi::UI_EndChild;
    api->UI_GetContentRegionAvail = &UIApi::UI_GetContentRegionAvail;
    api->UI_GetContentRegionMax = &UIApi::UI_GetContentRegionMax;
    api->UI_GetWindowContentRegionMin = &UIApi::UI_GetWindowContentRegionMin;
    api->UI_GetWindowContentRegionMax = &UIApi::UI_GetWindowContentRegionMax;
    api->UI_GetCursorPos = &UIApi::UI_GetCursorPos;
    api->UI_GetCursorPosX = &UIApi::UI_GetCursorPosX;
    api->UI_GetCursorPosY = &UIApi::UI_GetCursorPosY;
    api->UI_SetCursorPos = &UIApi::UI_SetCursorPos;
    api->UI_SetCursorPosX = &UIApi::UI_SetCursorPosX;
    api->UI_SetCursorPosY = &UIApi::UI_SetCursorPosY;
    api->UI_GetCursorScreenPos = &UIApi::UI_GetCursorScreenPos;
    api->UI_SetCursorScreenPos = &UIApi::UI_SetCursorScreenPos;
    api->UI_GetCursorStartPos = &UIApi::UI_GetCursorStartPos;
    api->UI_Separator = &UIApi::UI_Separator;
    api->UI_SeparatorText = &UIApi::UI_SeparatorText;
    api->UI_AlignTextToFramePadding = &UIApi::UI_AlignTextToFramePadding;
    api->UI_SameLine = &UIApi::UI_SameLine;
    api->UI_NewLine = &UIApi::UI_NewLine;
    api->UI_Spacing = &UIApi::UI_Spacing;
    api->UI_Dummy = &UIApi::UI_Dummy;
    api->UI_Indent = &UIApi::UI_Indent;
    api->UI_Unindent = &UIApi::UI_Unindent;
    api->UI_BeginGroup = &UIApi::UI_BeginGroup;
    api->UI_EndGroup = &UIApi::UI_EndGroup;
    api->UI_PushID_Str = &UIApi::UI_PushID_Str;
    api->UI_PushID_Int = &UIApi::UI_PushID_Int;
    api->UI_PushID_Ptr = &UIApi::UI_PushID_Ptr;
    api->UI_PopID = &UIApi::UI_PopID;
    api->UI_PushItemFlag = &UIApi::UI_PushItemFlag;
    api->UI_PopItemFlag = &UIApi::UI_PopItemFlag;
    api->UI_BeginDisabled = &UIApi::UI_BeginDisabled;
    api->UI_EndDisabled = &UIApi::UI_EndDisabled;
    api->UI_GetID_Str = &UIApi::UI_GetID_Str;
    api->UI_PushItemWidth = &UIApi::UI_PushItemWidth;
    api->UI_PopItemWidth = &UIApi::UI_PopItemWidth;
    api->UI_SetNextItemWidth = &UIApi::UI_SetNextItemWidth;
    api->UI_CalcItemWidth = &UIApi::UI_CalcItemWidth;
    api->UI_PushTextWrapPos = &UIApi::UI_PushTextWrapPos;
    api->UI_PopTextWrapPos = &UIApi::UI_PopTextWrapPos;
    api->UI_GetTextLineHeight = &UIApi::UI_GetTextLineHeight;
    api->UI_GetTextLineHeightWithSpacing = &UIApi::UI_GetTextLineHeightWithSpacing;
    api->UI_GetFrameHeight = &UIApi::UI_GetFrameHeight;
    api->UI_GetFrameHeightWithSpacing = &UIApi::UI_GetFrameHeightWithSpacing;
    api->UI_Text = &UIApi::UI_Text;
    api->UI_TextUnformatted = &UIApi::UI_TextUnformatted;
    api->UI_TextLink = &UIApi::UI_TextLink;
    api->UI_TextLinkOpenURL = &UIApi::UI_TextLinkOpenURL;
    api->UI_TextColored = &UIApi::UI_TextColored;
    api->UI_TextDisabled = &UIApi::UI_TextDisabled;
    api->UI_TextWrapped = &UIApi::UI_TextWrapped;
    api->UI_LabelText = &UIApi::UI_LabelText;
    api->UI_BulletText = &UIApi::UI_BulletText;
    api->UI_Button = &UIApi::UI_Button;
    api->UI_ButtonEx = &UIApi::UI_ButtonEx;
    api->UI_SmallButton = &UIApi::UI_SmallButton;
    api->UI_InvisibleButton = &UIApi::UI_InvisibleButton;
    api->UI_ArrowButton = &UIApi::UI_ArrowButton;
    api->UI_Checkbox = &UIApi::UI_Checkbox;
    api->UI_CheckboxFlags = &UIApi::UI_CheckboxFlags;
    api->UI_RadioButton = &UIApi::UI_RadioButton;
    api->UI_RadioButtonFlags = &UIApi::UI_RadioButtonFlags;
    api->UI_ProgressBar = &UIApi::UI_ProgressBar;
    api->UI_Bullet = &UIApi::UI_Bullet;
    api->UI_Image = &UIApi::UI_Image;
    api->UI_ImageWithBg = &UIApi::UI_ImageWithBg;
    api->UI_ImageButton = &UIApi::UI_ImageButton;
    api->UI_BeginCombo = &UIApi::UI_BeginCombo;
    api->UI_EndCombo = &UIApi::UI_EndCombo;
    api->UI_Combo = &UIApi::UI_Combo;
    api->UI_DragFloat = &UIApi::UI_DragFloat;
    api->UI_DragFloat2 = &UIApi::UI_DragFloat2;
    api->UI_DragFloat3 = &UIApi::UI_DragFloat3;
    api->UI_DragFloat4 = &UIApi::UI_DragFloat4;
    api->UI_DragFloatRange2 = &UIApi::UI_DragFloatRange2;
    api->UI_DragInt = &UIApi::UI_DragInt;
    api->UI_DragInt2 = &UIApi::UI_DragInt2;
    api->UI_DragInt3 = &UIApi::UI_DragInt3;
    api->UI_DragInt4 = &UIApi::UI_DragInt4;
    api->UI_DragIntRange2 = &UIApi::UI_DragIntRange2;
    api->UI_DragScalar = &UIApi::UI_DragScalar;
    api->UI_DragScalarN = &UIApi::UI_DragScalarN;
    api->UI_SliderFloat = &UIApi::UI_SliderFloat;
    api->UI_SliderFloat2 = &UIApi::UI_SliderFloat2;
    api->UI_SliderFloat3 = &UIApi::UI_SliderFloat3;
    api->UI_SliderFloat4 = &UIApi::UI_SliderFloat4;
    api->UI_SliderAngle = &UIApi::UI_SliderAngle;
    api->UI_SliderInt = &UIApi::UI_SliderInt;
    api->UI_SliderInt2 = &UIApi::UI_SliderInt2;
    api->UI_SliderInt3 = &UIApi::UI_SliderInt3;
    api->UI_SliderInt4 = &UIApi::UI_SliderInt4;
    api->UI_SliderScalar = &UIApi::UI_SliderScalar;
    api->UI_SliderScalarN = &UIApi::UI_SliderScalarN;
    api->UI_VSliderFloat = &UIApi::UI_VSliderFloat;
    api->UI_VSliderInt = &UIApi::UI_VSliderInt;
    api->UI_VSliderScalar = &UIApi::UI_VSliderScalar;
    api->UI_InputText = &UIApi::UI_InputText;
    api->UI_InputTextMultiline = &UIApi::UI_InputTextMultiline;
    api->UI_InputTextWithHint = &UIApi::UI_InputTextWithHint;
    api->UI_InputFloat = &UIApi::UI_InputFloat;
    api->UI_InputFloat2 = &UIApi::UI_InputFloat2;
    api->UI_InputFloat3 = &UIApi::UI_InputFloat3;
    api->UI_InputFloat4 = &UIApi::UI_InputFloat4;
    api->UI_InputInt = &UIApi::UI_InputInt;
    api->UI_InputInt2 = &UIApi::UI_InputInt2;
    api->UI_InputInt3 = &UIApi::UI_InputInt3;
    api->UI_InputInt4 = &UIApi::UI_InputInt4;
    api->UI_InputDouble = &UIApi::UI_InputDouble;
    api->UI_InputScalar = &UIApi::UI_InputScalar;
    api->UI_InputScalarN = &UIApi::UI_InputScalarN;
    api->UI_ColorEdit3 = &UIApi::UI_ColorEdit3;
    api->UI_ColorEdit4 = &UIApi::UI_ColorEdit4;
    api->UI_ColorPicker3 = &UIApi::UI_ColorPicker3;
    api->UI_ColorPicker4 = &UIApi::UI_ColorPicker4;
    api->UI_ColorButton = &UIApi::UI_ColorButton;
    api->UI_SetColorEditOptions = &UIApi::UI_SetColorEditOptions;
    api->UI_TreeNode = &UIApi::UI_TreeNode;
    api->UI_TreeNodeEx = &UIApi::UI_TreeNodeEx;
    api->UI_TreePush = &UIApi::UI_TreePush;
    api->UI_TreePop = &UIApi::UI_TreePop;
    api->UI_SetNextItemStorageID = &UIApi::UI_SetNextItemStorageID;
    api->UI_GetTreeNodeToLabelSpacing = &UIApi::UI_GetTreeNodeToLabelSpacing;
    api->UI_CollapsingHeader = &UIApi::UI_CollapsingHeader;
    api->UI_SetNextItemOpen = &UIApi::UI_SetNextItemOpen;
    api->UI_TreeNodeGetOpen = &UIApi::UI_TreeNodeGetOpen;
    api->UI_Selectable = &UIApi::UI_Selectable;
    api->UI_BeginListBox = &UIApi::UI_BeginListBox;
    api->UI_EndListBox = &UIApi::UI_EndListBox;
    api->UI_ListBox = &UIApi::UI_ListBox;
    api->UI_BeginMultiSelect = &UIApi::UI_BeginMultiSelect;
    api->UI_EndMultiSelect = &UIApi::UI_EndMultiSelect;
    api->UI_SetNextItemSelectionUserData = &UIApi::UI_SetNextItemSelectionUserData;
    api->UI_IsItemToggledSelection = &UIApi::UI_IsItemToggledSelection;
    api->UI_PlotLines = &UIApi::UI_PlotLines;
    api->UI_PlotHistogram = &UIApi::UI_PlotHistogram;
    api->UI_PlotLinesCallback = &UIApi::UI_PlotLinesCallback;
    api->UI_PlotHistogramCallback = &UIApi::UI_PlotHistogramCallback;
    api->UI_Value_Bool = &UIApi::UI_Value_Bool;
    api->UI_Value_Int = &UIApi::UI_Value_Int;
    api->UI_Value_UInt = &UIApi::UI_Value_UInt;
    api->UI_Value_Float = &UIApi::UI_Value_Float;
    api->UI_ListClipper_Begin = &UIApi::UI_ListClipper_Begin;
    api->UI_ListClipper_Step = &UIApi::UI_ListClipper_Step;
    api->UI_ListClipper_End = &UIApi::UI_ListClipper_End;
    api->UI_BeginMenuBar = &UIApi::UI_BeginMenuBar;
    api->UI_EndMenuBar = &UIApi::UI_EndMenuBar;
    api->UI_BeginMainMenuBar = &UIApi::UI_BeginMainMenuBar;
    api->UI_EndMainMenuBar = &UIApi::UI_EndMainMenuBar;
    api->UI_BeginMenu = &UIApi::UI_BeginMenu;
    api->UI_EndMenu = &UIApi::UI_EndMenu;
    api->UI_MenuItem = &UIApi::UI_MenuItem;
    api->UI_BeginTable = &UIApi::UI_BeginTable;
    api->UI_EndTable = &UIApi::UI_EndTable;
    api->UI_TableNextRow = &UIApi::UI_TableNextRow;
    api->UI_TableNextColumn = &UIApi::UI_TableNextColumn;
    api->UI_TableSetColumnIndex = &UIApi::UI_TableSetColumnIndex;
    api->UI_TableSetupColumn = &UIApi::UI_TableSetupColumn;
    api->UI_TableSetupScrollFreeze = &UIApi::UI_TableSetupScrollFreeze;
    api->UI_TableHeadersRow = &UIApi::UI_TableHeadersRow;
    api->UI_TableAngledHeadersRow = &UIApi::UI_TableAngledHeadersRow;
    api->UI_TableHeader = &UIApi::UI_TableHeader;
    api->UI_TableGetSortSpecs = &UIApi::UI_TableGetSortSpecs;
    api->UI_TableSetColumnEnabled = &UIApi::UI_TableSetColumnEnabled;
    api->UI_TableGetHoveredColumn = &UIApi::UI_TableGetHoveredColumn;
    api->UI_TableGetColumnCount = &UIApi::UI_TableGetColumnCount;
    api->UI_TableGetColumnIndex = &UIApi::UI_TableGetColumnIndex;
    api->UI_TableGetRowIndex = &UIApi::UI_TableGetRowIndex;
    api->UI_TableGetColumnName = &UIApi::UI_TableGetColumnName;
    api->UI_TableGetColumnFlags = &UIApi::UI_TableGetColumnFlags;
    api->UI_TableSetBgColor = &UIApi::UI_TableSetBgColor;
    api->UI_BeginPopup = &UIApi::UI_BeginPopup;
    api->UI_BeginPopupModal = &UIApi::UI_BeginPopupModal;
    api->UI_EndPopup = &UIApi::UI_EndPopup;
    api->UI_OpenPopup = &UIApi::UI_OpenPopup;
    api->UI_OpenPopupOnItemClick = &UIApi::UI_OpenPopupOnItemClick;
    api->UI_CloseCurrentPopup = &UIApi::UI_CloseCurrentPopup;
    api->UI_BeginPopupContextItem = &UIApi::UI_BeginPopupContextItem;
    api->UI_BeginPopupContextWindow = &UIApi::UI_BeginPopupContextWindow;
    api->UI_BeginPopupContextVoid = &UIApi::UI_BeginPopupContextVoid;
    api->UI_IsPopupOpen = &UIApi::UI_IsPopupOpen;
    api->UI_BeginTooltip = &UIApi::UI_BeginTooltip;
    api->UI_EndTooltip = &UIApi::UI_EndTooltip;
    api->UI_SetTooltip = &UIApi::UI_SetTooltip;
    api->UI_BeginItemTooltip = &UIApi::UI_BeginItemTooltip;
    api->UI_SetItemTooltip = &UIApi::UI_SetItemTooltip;
    api->UI_BeginDragDropSource = &UIApi::UI_BeginDragDropSource;
    api->UI_SetDragDropPayload = &UIApi::UI_SetDragDropPayload;
    api->UI_EndDragDropSource = &UIApi::UI_EndDragDropSource;
    api->UI_BeginDragDropTarget = &UIApi::UI_BeginDragDropTarget;
    api->UI_AcceptDragDropPayload = &UIApi::UI_AcceptDragDropPayload;
    api->UI_EndDragDropTarget = &UIApi::UI_EndDragDropTarget;
    api->UI_GetDragDropPayload = &UIApi::UI_GetDragDropPayload;
    api->UI_GetFont = &UIApi::UI_GetFont;
    api->UI_PushFont = &UIApi::UI_PushFont;
    api->UI_PopFont = &UIApi::UI_PopFont;
    api->UI_PushStyleColor = &UIApi::UI_PushStyleColor;
    api->UI_PopStyleColor = &UIApi::UI_PopStyleColor;
    api->UI_PushStyleVarFloat = &UIApi::UI_PushStyleVarFloat;
    api->UI_PushStyleVarVec2 = &UIApi::UI_PushStyleVarVec2;
    api->UI_PopStyleVar = &UIApi::UI_PopStyleVar;
    api->UI_GetStyle = &UIApi::UI_GetStyle;
    api->UI_Style_GetWindowPadding = &UIApi::UI_Style_GetWindowPadding;
    api->UI_Style_GetItemSpacing = &UIApi::UI_Style_GetItemSpacing;
    api->UI_Style_GetFramePadding = &UIApi::UI_Style_GetFramePadding;
    api->UI_StyleColorsDark = &UIApi::UI_StyleColorsDark;
    api->UI_StyleColorsLight = &UIApi::UI_StyleColorsLight;
    api->UI_StyleColorsClassic = &UIApi::UI_StyleColorsClassic;
    api->UI_GetStyleColor = &UIApi::UI_GetStyleColor;
    api->UI_CalcTextSize = &UIApi::UI_CalcTextSize;
    api->UI_CalcTextSizeWithFont = &UIApi::UI_CalcTextSizeWithFont;
    api->UI_ColorConvertFloat4ToU32 = &UIApi::UI_ColorConvertFloat4ToU32;
    api->UI_ColorConvertU32ToFloat4 = &UIApi::UI_ColorConvertU32ToFloat4;
    api->UI_ColorConvertRGBtoHSV = &UIApi::UI_ColorConvertRGBtoHSV;
    api->UI_ColorConvertHSVtoRGB = &UIApi::UI_ColorConvertHSVtoRGB;
    api->UI_Style_Create = &UIApi::UI_Style_Create;
    api->UI_Style_Destroy = &UIApi::UI_Style_Destroy;
    api->UI_Style_SetFont = &UIApi::UI_Style_SetFont;
    api->UI_Style_SetColor = &UIApi::UI_Style_SetColor;
    api->UI_Style_SetHoverColor = &UIApi::UI_Style_SetHoverColor;
    api->UI_Style_SetActiveColor = &UIApi::UI_Style_SetActiveColor;
    api->UI_Style_SetAlign = &UIApi::UI_Style_SetAlign;
    api->UI_Style_SetWrap = &UIApi::UI_Style_SetWrap;
    api->UI_Style_SetPadding = &UIApi::UI_Style_SetPadding;
    api->UI_Style_SetSeparator = &UIApi::UI_Style_SetSeparator;
    api->UI_Style_SetUnderline = &UIApi::UI_Style_SetUnderline;
    api->UI_Style_SetStrikethrough = &UIApi::UI_Style_SetStrikethrough;
    api->UI_TextStyled = &UIApi::UI_TextStyled;
    api->UI_RenderMarkdown = &UIApi::UI_RenderMarkdown;
    api->UI_DrawList_PushClipRect = &UIApi::UI_DrawList_PushClipRect;
    api->UI_DrawList_PushClipRectFullScreen = &UIApi::UI_DrawList_PushClipRectFullScreen;
    api->UI_DrawList_PopClipRect = &UIApi::UI_DrawList_PopClipRect;
    api->UI_DrawList_AddLine = &UIApi::UI_DrawList_AddLine;
    api->UI_DrawList_AddRect = &UIApi::UI_DrawList_AddRect;
    api->UI_DrawList_AddRectFilled = &UIApi::UI_DrawList_AddRectFilled;
    api->UI_DrawList_AddRectFilledMultiColor = &UIApi::UI_DrawList_AddRectFilledMultiColor;
    api->UI_DrawList_AddQuad = &UIApi::UI_DrawList_AddQuad;
    api->UI_DrawList_AddQuadFilled = &UIApi::UI_DrawList_AddQuadFilled;
    api->UI_DrawList_AddTriangle = &UIApi::UI_DrawList_AddTriangle;
    api->UI_DrawList_AddTriangleFilled = &UIApi::UI_DrawList_AddTriangleFilled;
    api->UI_DrawList_AddTriangleFilledMultiColor = &UIApi::UI_DrawList_AddTriangleFilledMultiColor;
    api->UI_DrawList_AddCircle = &UIApi::UI_DrawList_AddCircle;
    api->UI_DrawList_AddCircleFilled = &UIApi::UI_DrawList_AddCircleFilled;
    api->UI_DrawList_AddCircleFilledMultiColor = &UIApi::UI_DrawList_AddCircleFilledMultiColor;
    api->UI_AddRectFilled = &UIApi::UI_AddRectFilled;
    api->UI_DrawList_AddNgon = &UIApi::UI_DrawList_AddNgon;
    api->UI_DrawList_AddNgonFilled = &UIApi::UI_DrawList_AddNgonFilled;
    api->UI_DrawList_AddNgonContour = &UIApi::UI_DrawList_AddNgonContour;
    api->UI_DrawList_AddEllipse = &UIApi::UI_DrawList_AddEllipse;
    api->UI_DrawList_AddEllipseFilled = &UIApi::UI_DrawList_AddEllipseFilled;
    api->UI_DrawList_AddBezierCubic = &UIApi::UI_DrawList_AddBezierCubic;
    api->UI_DrawList_AddBezierQuadratic = &UIApi::UI_DrawList_AddBezierQuadratic;
    api->UI_DrawList_AddPolyline = &UIApi::UI_DrawList_AddPolyline;
    api->UI_DrawList_AddConvexPolyFilled = &UIApi::UI_DrawList_AddConvexPolyFilled;
    api->UI_DrawList_AddConcavePolyFilled = &UIApi::UI_DrawList_AddConcavePolyFilled;
    api->UI_DrawList_AddImage = &UIApi::UI_DrawList_AddImage;
    api->UI_DrawList_AddImageQuad = &UIApi::UI_DrawList_AddImageQuad;
    api->UI_DrawList_AddImageRounded = &UIApi::UI_DrawList_AddImageRounded;
    api->UI_DrawList_AddCallback = &UIApi::UI_DrawList_AddCallback;
    api->UI_DrawList_AddText = &UIApi::UI_DrawList_AddText;
    api->UI_DrawList_AddTextWithFont = &UIApi::UI_DrawList_AddTextWithFont;
    api->UI_DrawList_PathClear = &UIApi::UI_DrawList_PathClear;
    api->UI_DrawList_PathLineTo = &UIApi::UI_DrawList_PathLineTo;
    api->UI_DrawList_PathStroke = &UIApi::UI_DrawList_PathStroke;
    api->UI_DrawList_PathFillConvex = &UIApi::UI_DrawList_PathFillConvex;
    api->UI_DrawList_PathArcTo = &UIApi::UI_DrawList_PathArcTo;
    api->UI_DrawList_PathRect = &UIApi::UI_DrawList_PathRect;
    api->UI_DrawList_ChannelsSplit = &UIApi::UI_DrawList_ChannelsSplit;
    api->UI_DrawList_ChannelsMerge = &UIApi::UI_DrawList_ChannelsMerge;
    api->UI_DrawList_ChannelsSetCurrent = &UIApi::UI_DrawList_ChannelsSetCurrent;
    api->UI_DrawList_PrimReserve = &UIApi::UI_DrawList_PrimReserve;
    api->UI_DrawList_PrimRectUV = &UIApi::UI_DrawList_PrimRectUV;
    api->UI_DrawList_PrimVtx = &UIApi::UI_DrawList_PrimVtx;
    api->UI_DrawList_PrimIdx = &UIApi::UI_DrawList_PrimIdx;
    api->UI_IsItemHovered = &UIApi::UI_IsItemHovered;
    api->UI_IsItemActive = &UIApi::UI_IsItemActive;
    api->UI_IsItemFocused = &UIApi::UI_IsItemFocused;
    api->UI_IsItemClicked = &UIApi::UI_IsItemClicked;
    api->UI_IsItemVisible = &UIApi::UI_IsItemVisible;
    api->UI_IsItemEdited = &UIApi::UI_IsItemEdited;
    api->UI_IsItemActivated = &UIApi::UI_IsItemActivated;
    api->UI_IsItemDeactivated = &UIApi::UI_IsItemDeactivated;
    api->UI_IsItemDeactivatedAfterEdit = &UIApi::UI_IsItemDeactivatedAfterEdit;
    api->UI_IsItemToggledOpen = &UIApi::UI_IsItemToggledOpen;
    api->UI_IsAnyItemHovered = &UIApi::UI_IsAnyItemHovered;
    api->UI_IsAnyItemActive = &UIApi::UI_IsAnyItemActive;
    api->UI_IsAnyItemFocused = &UIApi::UI_IsAnyItemFocused;
    api->UI_GetItemID = &UIApi::UI_GetItemID;
    api->UI_GetItemRectMin = &UIApi::UI_GetItemRectMin;
    api->UI_GetItemRectMax = &UIApi::UI_GetItemRectMax;
    api->UI_GetItemRectSize = &UIApi::UI_GetItemRectSize;
    api->UI_BeginTabBar = &UIApi::UI_BeginTabBar;
    api->UI_EndTabBar = &UIApi::UI_EndTabBar;
    api->UI_BeginTabItem = &UIApi::UI_BeginTabItem;
    api->UI_EndTabItem = &UIApi::UI_EndTabItem;
    api->UI_TabItemButton = &UIApi::UI_TabItemButton;
    api->UI_SetTabItemClosed = &UIApi::UI_SetTabItemClosed;
    api->UI_DockSpace = &UIApi::UI_DockSpace;
    api->UI_DockSpaceOverViewport = &UIApi::UI_DockSpaceOverViewport;
    api->UI_SetNextWindowDockID = &UIApi::UI_SetNextWindowDockID;
    api->UI_IsWindowDocked = &UIApi::UI_IsWindowDocked;
    api->UI_DockBuilderDockWindow = &UIApi::UI_DockBuilderDockWindow;
    api->UI_DockBuilderGetNode = &UIApi::UI_DockBuilderGetNode;
    api->UI_DockBuilderAddNode = &UIApi::UI_DockBuilderAddNode;
    api->UI_DockBuilderRemoveNode = &UIApi::UI_DockBuilderRemoveNode;
    api->UI_DockBuilderRemoveNodeDockedWindows = &UIApi::UI_DockBuilderRemoveNodeDockedWindows;
    api->UI_DockBuilderSetNodePos = &UIApi::UI_DockBuilderSetNodePos;
    api->UI_DockBuilderSetNodeSize = &UIApi::UI_DockBuilderSetNodeSize;
    api->UI_DockBuilderSplitNode = &UIApi::UI_DockBuilderSplitNode;
    api->UI_DockBuilderFinish = &UIApi::UI_DockBuilderFinish;
    api->UI_DockBuilderGetCentralNode = &UIApi::UI_DockBuilderGetCentralNode;
    api->UI_UpdatePlatformWindows = &UIApi::UI_UpdatePlatformWindows;
    api->UI_RenderPlatformWindowsDefault = &UIApi::UI_RenderPlatformWindowsDefault;
    api->UI_DestroyPlatformWindows = &UIApi::UI_DestroyPlatformWindows;
    api->UI_ItemSize = &UIApi::UI_ItemSize;
    api->UI_ItemAdd = &UIApi::UI_ItemAdd;
    api->UI_SetLastItemData = &UIApi::UI_SetLastItemData;
    api->UI_ButtonBehavior = &UIApi::UI_ButtonBehavior;
    api->UI_DragBehavior = &UIApi::UI_DragBehavior;
    api->UI_SliderBehavior = &UIApi::UI_SliderBehavior;
    api->UI_SplitterBehavior = &UIApi::UI_SplitterBehavior;
    api->UI_RenderFrame = &UIApi::UI_RenderFrame;
    api->UI_RenderFrameBorder = &UIApi::UI_RenderFrameBorder;
    api->UI_RenderText = &UIApi::UI_RenderText;
    api->UI_RenderTextClipped = &UIApi::UI_RenderTextClipped;
    api->UI_RenderTextEllipsis = &UIApi::UI_RenderTextEllipsis;
    api->UI_RenderArrow = &UIApi::UI_RenderArrow;
    api->UI_RenderCheckMark = &UIApi::UI_RenderCheckMark;
    api->UI_RenderBullet = &UIApi::UI_RenderBullet;
    api->UI_ShowNotification = &UIApi::UI_ShowNotification;
    api->UI_HideNotification = &UIApi::UI_HideNotification;
    api->UI_PlayTransition = &UIApi::UI_PlayTransition;
    api->UI_IsTransitionActive = &UIApi::UI_IsTransitionActive;
    api->UI_SetMouseBlockState = &UIApi::UI_SetMouseBlockState;
    api->UI_SetMouseOverride = &UIApi::UI_SetMouseOverride;
    api->UI_IsMouseOverridden = &UIApi::UI_IsMouseOverridden;
    api->UI_CreateTextureFromMemory = &UIApi::UI_CreateTextureFromMemory;
    api->UI_CreateTextureFromFile = &UIApi::UI_CreateTextureFromFile;
    api->UI_DestroyTexture = &UIApi::UI_DestroyTexture;
    api->UI_LoadFontFromMemory = &UIApi::UI_LoadFontFromMemory;
    api->UI_LoadFontFromFile = &UIApi::UI_LoadFontFromFile;
    api->UI_DrawList_AddTextWithFontHandle = &UIApi::UI_DrawList_AddTextWithFontHandle;
}

}  // namespace Modules::API
SPF_NS_END
