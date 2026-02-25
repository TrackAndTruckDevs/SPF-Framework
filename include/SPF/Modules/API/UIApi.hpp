#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"  // For SPF_UI_API and SPF_DrawCallback
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class UIApi {
 public:
  // This method will fill the SPF_UI_API struct with pointers to our trampolines
  static void FillUIApi(SPF_UI_API* ui_api);

 private:
  // --- UI Builder Trampolines (from PluginManager) ---
  static void UI_Text(const char* text);
  static void UI_TextColored(float r, float g, float b, float a, const char* text);
  static void UI_TextDisabled(const char* text);
  static void UI_TextWrapped(const char* text);
  static void UI_LabelText(const char* label, const char* text);
  static void UI_BulletText(const char* text);
  static bool UI_Button(const char* label, float width, float height);
  static bool UI_SmallButton(const char* label);
  static bool UI_InvisibleButton(const char* str_id, float width, float height);
  static bool UI_Checkbox(const char* label, bool* v);
  static bool UI_RadioButton(const char* label, bool active);
  static void UI_ProgressBar(float fraction, float width, float height, const char* overlay);
  static void UI_Bullet();
  static void UI_Separator();
  static void UI_Spacing();
  static void UI_Indent(float indent_w);
  static void UI_Unindent(float indent_w);
  static void UI_SameLine(float offset_from_start_x, float spacing);
  static bool UI_BeginChild(const char* str_id, float size_x, float size_y, bool border, SPF_Window_Flags flags);
  static void UI_EndChild();
  static void UI_SetCursorPos(float x, float y);
  static void UI_GetCursorPos(float* out_x, float* out_y);
  static bool UI_IsWindowHovered();
  static float UI_GetIO_DeltaTime();
  static bool UI_InputText(const char* label, char* buf, size_t buf_size);
  static bool UI_InputInt(const char* label, int* v, int step, int step_fast, int flags);
  static bool UI_InputFloat(const char* label, float* v, float step, float step_fast, const char* format, int flags);
  static bool UI_InputDouble(const char* label, double* v, double step, double step_fast, const char* format);
  static bool UI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format);
  static bool UI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format);
  static void UI_PushStyleColor(int im_gui_color_idx, float r, float g, float b, float a);
  static void UI_PopStyleColor(int count);
  static void UI_PushStyleVarFloat(int im_gui_stylevar_idx, float val);
  static void UI_PushStyleVarVec2(int im_gui_stylevar_idx, float val_x, float val_y);
  static void UI_PopStyleVar(int count);
  static void UI_GetViewportSize(float* out_width, float* out_height);
  static void UI_AddRectFilled(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
  static bool UI_BeginCombo(const char* label, const char* preview_value);
  static void UI_EndCombo();
  static bool UI_Selectable(const char* label, bool selected);
  static bool UI_TreeNode(const char* label);
  static void UI_TreePush(const char* str_id);
  static void UI_TreePop();
  static bool UI_BeginTabBar(const char* str_id);
  static void UI_EndTabBar();
  static bool UI_BeginTabItem(const char* label);
  static void UI_EndTabItem();
  static bool UI_BeginTable(const char* str_id, int column);
  static void UI_EndTable();
  static void UI_TableNextRow();
  static bool UI_TableNextColumn();
  static void UI_TableSetupColumn(const char* label);
  static void UI_OpenPopup(const char* str_id);
  static bool UI_BeginPopup(const char* str_id);
  static void UI_EndPopup();
  static bool UI_IsItemHovered();
  static bool UI_IsItemActive();
  static void UI_SetTooltip(const char* text);
  static bool UI_InputTextMultiline(const char* label, char* buf, size_t buf_size);
  static bool UI_SliderFloat2(const char* label, float v[2], float v_min, float v_max);
  static bool UI_SliderFloat3(const char* label, float v[3], float v_min, float v_max);
  static bool UI_SliderFloat4(const char* label, float v[4], float v_min, float v_max);
  static bool UI_SliderInt2(const char* label, int v[2], int v_min, int v_max);
  static bool UI_SliderInt3(const char* label, int v[3], int v_min, int v_max);
  static bool UI_SliderInt4(const char* label, int v[4], int v_min, int v_max);
  static bool UI_ColorEdit3(const char* label, float col[3]);
  static bool UI_ColorEdit4(const char* label, float col[4]);
  static bool UI_DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max);
  static bool UI_DragInt(const char* label, int* v, float v_speed, int v_min, int v_max);

  // Text Styling Trampolines ---
  static SPF_TextStyle_Handle UI_Style_Create();
  static void UI_Style_Destroy(SPF_TextStyle_Handle handle);
  static void UI_Style_SetFont(SPF_TextStyle_Handle handle, SPF_Font font);
  static void UI_Style_SetColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a);
  static void UI_Style_SetAlign(SPF_TextStyle_Handle handle, SPF_TextAlign align);
  static void UI_Style_SetWrap(SPF_TextStyle_Handle handle, bool wrap);
  static void UI_Style_SetPadding(SPF_TextStyle_Handle handle, float pad_x, float pad_y);
  static void UI_Style_SetSeparator(SPF_TextStyle_Handle handle, bool is_separator);
  static void UI_Style_SetUnderline(SPF_TextStyle_Handle handle, bool is_underline);
  static void UI_Style_SetStrikethrough(SPF_TextStyle_Handle handle, bool is_strikethrough);
  static void UI_TextStyled(SPF_TextStyle_Handle handle, const char* fmt, ...);
  static void UI_RenderMarkdown(const char* markdown_text, SPF_TextStyle_Handle base_style_handle);

  // --- Custom Widget API Trampolines ---
  static uint32_t UI_ColorConvertFloat4ToU32(float r, float g, float b, float a);
  static SPF_DrawList_Handle UI_GetWindowDrawList();
  static void UI_DrawList_AddLine(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness);
  static void UI_DrawList_AddRectFilled(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding);
  static void UI_DrawList_AddCircleFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments);
  static void UI_DrawList_AddCircle(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness);
  static void UI_DrawList_AddRectFilledMultiColor(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left);
  static void UI_DrawList_PushClipRect(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, bool intersect_with_current_clip_rect);
  static void UI_DrawList_PopClipRect(SPF_DrawList_Handle dl);
  static void UI_DrawList_AddText(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text);
  static void UI_DrawList_AddRect(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, float thickness);
  static void UI_DrawList_AddQuadFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col);
  static void UI_DrawList_AddTriangleFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col);
  static void UI_DrawList_AddBezierCubic(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col, float thickness, int num_segments);
  static void UI_DrawList_AddPolyline(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col, bool closed, float thickness);
  static void UI_DrawList_PathClear(SPF_DrawList_Handle dl);
  static void UI_DrawList_PathLineTo(SPF_DrawList_Handle dl, float pos_x, float pos_y);
  static void UI_DrawList_PathStroke(SPF_DrawList_Handle dl, uint32_t col, bool closed, float thickness);
  static void UI_DrawList_PathFillConvex(SPF_DrawList_Handle dl, uint32_t col);
  static void UI_GetMousePos(float* out_x, float* out_y);
  static bool UI_IsMouseDragging(int mouse_button_index);
  static void UI_GetMouseDragDelta(int mouse_button_index, float* out_dx, float* out_dy);
  static bool UI_IsMouseDown(int mouse_button_index);
  static bool UI_IsMouseClicked(int mouse_button_index);
  static bool UI_IsMouseReleased(int mouse_button_index);
  static bool UI_IsMouseDoubleClicked(int mouse_button_index);
  static float UI_GetMouseWheel();
  static void UI_SetMouseBlockState(bool blockAxes, bool blockButtons, bool blockWheel);

  // --- Layout & Positioning API Trampolines ---
  static void UI_GetContentRegionAvail(float* out_x, float* out_y);
  static void UI_GetWindowPos(float* out_x, float* out_y);
  static void UI_GetWindowSize(float* out_x, float* out_y);
  static void UI_GetCursorScreenPos(float* out_x, float* out_y);
  static void UI_SetCursorScreenPos(float x, float y);
  static void UI_GetItemRectMin(float* out_x, float* out_y);
  static void UI_GetItemRectMax(float* out_x, float* out_y);
  static void UI_GetItemRectSize(float* out_x, float* out_y);

  // --- NEW: Miscellaneous Utilities API Trampolines (v1.3 - SPF-418) ---
  static const char* UI_GetClipboardText();
  static void UI_SetClipboardText(const char* text);
  static SPF_Font_Handle* UI_GetFont(const char* font_key);
  static void UI_PushFont(SPF_Font_Handle* font_handle);
  static void UI_PopFont();
  static SPF_Style_Handle* UI_GetStyle();
  static void UI_Style_GetWindowPadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y);
  static void UI_Style_GetItemSpacing(SPF_Style_Handle* style_handle, float* out_x, float* out_y);
  static void UI_Style_GetFramePadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y);
  static void UI_PushID_Str(const char* str_id);
  static void UI_PushID_Int(int int_id);
  static void UI_PushID_Ptr(void* ptr_id);
  static void UI_PopID();
  static uint32_t UI_GetID_Str(const char* str_id);

  // --- NEW: Drag and Drop API Trampolines ---
  static bool UI_BeginDragDropSource();
  static bool UI_SetDragDropPayload(const char* type, const void* data, size_t size);
  static void UI_EndDragDropSource();
  static bool UI_BeginDragDropTarget();
  static const SPF_Payload_Handle* UI_AcceptDragDropPayload(const char* type);
  static void UI_EndDragDropTarget();

  // --- Notifications ---
  static void UI_ShowNotification(SPF_NotificationType type, const char* message);

  // --- Transitions ---
  static void UI_PlayTransition(SPF_TransitionType type, float duration, bool reverse, SPF_TransitionColor color);
  static bool UI_IsTransitionActive();

  // --- UI Window Management Trampolines (from PluginManager) ---
  static void UI_SetVisibility(SPF_Window_Handle* handle, bool isVisible);
  static bool UI_IsVisible(SPF_Window_Handle* handle);
  static void UI_RegisterDrawCallback(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data);
  static void UI_RegisterDrawCallbackWithFlags(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_Window_Flags flags);
  static SPF_Window_Handle* UI_GetWindowHandle(const char* pluginName, const char* windowId);
};
}  // namespace Modules::API
SPF_NS_END
