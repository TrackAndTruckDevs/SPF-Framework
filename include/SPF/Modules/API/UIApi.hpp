#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"  // For SPF_UI_API and SPF_DrawCallback
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class UIApi {
 public:
  // This method will fill the provided SPF_UI_API structure with function pointers
  // to our implementation.
  static void FillUIApi(SPF_UI_API* api);

  // --- I. Plugin Registration & Window Lifecycle ---
  static void UI_RegisterDrawCallback(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data);
  static void UI_RegisterDrawCallbackWithFlags(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_WindowFlags flags);
  static SPF_Window_Handle* UI_GetWindowHandle(const char* pluginName, const char* windowId);
  static void UI_SetVisibility(SPF_Window_Handle* handle, bool isVisible);
  static bool UI_IsVisible(SPF_Window_Handle* handle);
  static void UI_SetFocus(SPF_Window_Handle* handle);

  // --- II. Main Loop, Context & IO ---
  static void* UI_GetCurrentContext();
  static void UI_SetCurrentContext(void* ctx);
  static void UI_SetAllocatorFunctions(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data);
  static void* UI_MemAlloc(size_t sz);
  static void UI_MemFree(void* ptr);
  static float UI_GetDeltaTime();
  static double UI_GetTime();
  static int UI_GetFrameCount();
  static float UI_GetFramerate();
  static int UI_GetIO_ConfigFlags();
  static int UI_GetIO_BackendFlags();

  // Mouse Input
  static void UI_GetMousePos(float* out_x, float* out_y);
  static bool UI_IsMouseDown(SPF_MouseButton button);
  static bool UI_IsMouseClicked(SPF_MouseButton button);
  static bool UI_IsMouseReleased(SPF_MouseButton button);
  static bool UI_IsMouseDoubleClicked(SPF_MouseButton button);
  static bool UI_IsMouseHoveringRect(float min_x, float min_y, float max_x, float max_y, bool clip);
  static bool UI_IsMousePosValid();
  static void UI_GetMousePosOnOpeningCurrentPopup(float* out_x, float* out_y);
  static float UI_GetMouseWheel();
  static float UI_GetMouseWheelH();

  // Keyboard & Shortcut
  static bool UI_Shortcut(int key_chord, SPF_InputFlags flags);
  static void UI_SetNextItemShortcut(int key_chord, SPF_InputFlags flags);
  static void UI_SetItemKeyOwner(SPF_Key key);
  static bool UI_IsMouseDragging(SPF_MouseButton button);
  static void UI_GetMouseDragDelta(SPF_MouseButton button, float* out_dx, float* out_dy);
  static void UI_ResetMouseDragDelta(SPF_MouseButton button);
  static void UI_SetMouseCursor(SPF_MouseCursor cursor);
  static SPF_MouseCursor UI_GetMouseCursor();

  // Keyboard Input
  static bool UI_IsKeyDown(int key_index);
  static bool UI_IsKeyPressed(int key_index);
  static bool UI_IsKeyReleased(int key_index);
  static int UI_GetKeyPressedAmount(int key_index, float repeat_delay, float rate);
  static bool UI_IsMouseReleasedWithDelay(SPF_MouseButton button, float delay);
  static const char* UI_GetKeyName(int key_index);

  // Clipboard
  static const char* UI_GetClipboardText();
  static void UI_SetClipboardText(const char* text);

  // --- III. Windows, Layout & Positioning ---
  static bool UI_IsWindowAppearing();
  static bool UI_IsWindowCollapsed();
  static bool UI_IsWindowFocused(SPF_FocusedFlags flags);
  static bool UI_IsWindowHovered(SPF_HoveredFlags flags);
  static SPF_Storage_Handle UI_GetStateStorage();
  static void UI_SetStateStorage(SPF_Storage_Handle storage);
  static void* UI_GetWindowViewport();
  static SPF_DrawList_Handle UI_GetWindowDrawList();
  static SPF_DrawList_Handle UI_GetBackgroundDrawList();
  static SPF_DrawList_Handle UI_GetForegroundDrawList();

  // Window Manipulation
  static void UI_GetWindowPos(float* out_x, float* out_y);
  static void UI_GetWindowSize(float* out_x, float* out_y);
  static float UI_GetWindowWidth();
  static float UI_GetWindowHeight();
  static float UI_GetWindowDpiScale();
  static void UI_SetWindowPos(float x, float y, SPF_Cond cond);
  static void UI_SetWindowSize(float x, float y, SPF_Cond cond);
  static void UI_SetNextWindowPos(float x, float y, SPF_Cond cond, float pivot_x, float pivot_y);
  static void UI_SetNextWindowSize(float x, float y, SPF_Cond cond);
  static void UI_SetNextWindowViewport(uint32_t viewport_id);
  static void UI_SetNextWindowScroll(float scroll_x, float scroll_y);
  static float UI_GetScrollX();
  static float UI_GetScrollY();
  static float UI_GetScrollMaxX();
  static float UI_GetScrollMaxY();
  static void UI_SetScrollX(float scroll_x);
  static void UI_SetScrollY(float scroll_y);
  static void UI_SetScrollHereY(float center_y_ratio);
  static void UI_SetNextWindowFocus();
  static void UI_SetNextWindowCollapsed(bool collapsed, SPF_Cond cond);
  static void UI_SetWindowCollapsed(bool collapsed, SPF_Cond cond);
  static void UI_SetWindowFocus();
  static void UI_FocusWindow(void* window);
  static void UI_BringWindowToFocusFront(void* window);
  static void UI_SetWindowFontScale(float scale);
  static void UI_SetNextWindowSizeConstraints(float min_x, float min_y, float max_x, float max_y);
  static void UI_SetNextWindowBgAlpha(float alpha);
  static void UI_SetNextWindowContentSize(float size_x, float size_y);
  static void UI_BringWindowToDisplayFront();
  static void UI_BringWindowToDisplayBack();
  static void UI_GetMainViewportPos(float* out_x, float* out_y);
  static void UI_GetMainViewportSize(float* out_x, float* out_y);

  // Child Windows
  static bool UI_BeginChild(const char* str_id, float size_x, float size_y, bool border, SPF_WindowFlags flags);
  static void UI_EndChild();

  // Content Regions & Cursor
  static void UI_GetContentRegionAvail(float* out_x, float* out_y);
  static void UI_GetContentRegionMax(float* out_x, float* out_y);
  static void UI_GetWindowContentRegionMin(float* out_x, float* out_y);
  static void UI_GetWindowContentRegionMax(float* out_x, float* out_y);
  static void UI_GetCursorPos(float* out_x, float* out_y);
  static float UI_GetCursorPosX();
  static float UI_GetCursorPosY();
  static void UI_SetCursorPos(float x, float y);
  static void UI_SetCursorPosX(float x);
  static void UI_SetCursorPosY(float y);
  static void UI_GetCursorScreenPos(float* out_x, float* out_y);
  static void UI_SetCursorScreenPos(float x, float y);
  static void UI_GetCursorStartPos(float* out_x, float* out_y);

  // Layout Helpers
  static void UI_Separator();
  static void UI_SeparatorText(const char* label);
  static void UI_AlignTextToFramePadding();
  static void UI_SameLine(float offset_from_start_x, float spacing);
  static void UI_NewLine();
  static void UI_Spacing();
  static void UI_Dummy(float size_x, float size_y);
  static void UI_Indent(float indent_w);
  static void UI_Unindent(float indent_w);
  static void UI_BeginGroup();
  static void UI_EndGroup();

  // ID Stack
  static void UI_PushID_Str(const char* str_id);
  static void UI_PushID_Int(int int_id);
  static void UI_PushID_Ptr(const void* ptr_id);
  static void UI_PopID();
  static void UI_PushItemFlag(int flags, bool enabled);
  static void UI_PopItemFlag();
  static void UI_BeginDisabled(bool disabled);
  static void UI_EndDisabled();
  static uint32_t UI_GetID_Str(const char* str_id);

  // Layout Metrics & Item Sizing
  static void UI_PushItemWidth(float item_width);
  static void UI_PopItemWidth();
  static void UI_SetNextItemWidth(float item_width);
  static float UI_CalcItemWidth();
  static void UI_PushTextWrapPos(float wrap_local_pos_x);
  static void UI_PopTextWrapPos();
  static float UI_GetTextLineHeight();
  static float UI_GetTextLineHeightWithSpacing();
  static float UI_GetFrameHeight();
  static float UI_GetFrameHeightWithSpacing();

  // --- IV. Basic Widgets ---
  // Text
  static void UI_Text(const char* text);
  static void UI_TextUnformatted(const char* text);
  static bool UI_TextLink(const char* label);
  static void UI_TextLinkOpenURL(const char* label, const char* url);
  static void UI_TextColored(float r, float g, float b, float a, const char* text);
  static void UI_TextDisabled(const char* text);
  static void UI_TextWrapped(const char* text);
  static void UI_LabelText(const char* label, const char* text);
  static void UI_BulletText(const char* text);

  // Buttons & Interaction
  static bool UI_Button(const char* label, float width, float height);
  static bool UI_ButtonEx(const char* label, float width, float height, const char* tooltip, SPF_TextStyle_Handle style);
  static bool UI_SmallButton(const char* label);
  static bool UI_InvisibleButton(const char* str_id, float width, float height);
  static bool UI_ArrowButton(const char* str_id, SPF_Dir dir);
  static bool UI_Checkbox(const char* label, bool* v);
  static bool UI_CheckboxFlags(const char* label, int* flags, int flags_value);
  static bool UI_RadioButton(const char* label, bool active);
  static bool UI_RadioButtonFlags(const char* label, int* v, int v_button);
  static void UI_ProgressBar(float fraction, float width, float height, const char* overlay);
  static void UI_Bullet();

  // Images
  static void UI_Image(void* user_texture_id, float width, float height);
  static void UI_ImageWithBg(void* user_texture_id, float width, float height, float bg_col[4], float tint_col[4]);
  static bool UI_ImageButton(const char* str_id, void* user_texture_id, float width, float height);

  // --- V. Advanced Inputs ---
  // Combo Box
  static bool UI_BeginCombo(const char* label, const char* preview_value, SPF_ComboFlags flags);
  static void UI_EndCombo();
  static bool UI_Combo(const char* label, int* current_item, const char* const items[], int items_count);

  // Drags
  static bool UI_DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, SPF_SliderFlags flags);
  static bool UI_DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max, SPF_SliderFlags flags);
  static bool UI_DragScalar(const char* label, SPF_DataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);
  static bool UI_DragScalarN(const char* label, SPF_DataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);

  // Sliders
  static bool UI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderScalar(const char* label, SPF_DataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderScalarN(const char* label, SPF_DataType data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);
  static bool UI_VSliderFloat(const char* label, float size_x, float size_y, float* v, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_VSliderInt(const char* label, float size_x, float size_y, int* v, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
  static bool UI_VSliderScalar(const char* label, float size_x, float size_y, SPF_DataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);

  // Direct Inputs
  static bool UI_InputText(const char* label, char* buf, size_t buf_size, SPF_InputTextFlags flags);
  static bool UI_InputTextMultiline(const char* label, char* buf, size_t buf_size, float size_x, float size_y, SPF_InputTextFlags flags);
  static bool UI_InputTextWithHint(const char* label, const char* hint, char* buf, size_t buf_size, SPF_InputTextFlags flags);
  static bool UI_InputFloat(const char* label, float* v, float step, float step_fast, const char* format, SPF_InputTextFlags flags);
  static bool UI_InputFloat2(const char* label, float v[2], const char* format, SPF_InputTextFlags flags);
  static bool UI_InputFloat3(const char* label, float v[3], const char* format, SPF_InputTextFlags flags);
  static bool UI_InputFloat4(const char* label, float v[4], const char* format, SPF_InputTextFlags flags);
  static bool UI_InputInt(const char* label, int* v, int step, int step_fast, SPF_InputTextFlags flags);
  static bool UI_InputInt2(const char* label, int v[2], SPF_InputTextFlags flags);
  static bool UI_InputInt3(const char* label, int v[3], SPF_InputTextFlags flags);
  static bool UI_InputInt4(const char* label, int v[4], SPF_InputTextFlags flags);
  static bool UI_InputDouble(const char* label, double* v, double step, double step_fast, const char* format, SPF_InputTextFlags flags);
  static bool UI_InputScalar(const char* label, SPF_DataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, SPF_InputTextFlags flags);
  static bool UI_InputScalarN(const char* label, SPF_DataType data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, SPF_InputTextFlags flags);

  // Color Editor & Picker
  static bool UI_ColorEdit3(const char* label, float col[3], SPF_ColorEditFlags flags);
  static bool UI_ColorEdit4(const char* label, float col[4], SPF_ColorEditFlags flags);
  static bool UI_ColorPicker3(const char* label, float col[3], SPF_ColorEditFlags flags);
  static bool UI_ColorPicker4(const char* label, float col[4], SPF_ColorEditFlags flags);
  static bool UI_ColorButton(const char* desc_id, float r, float g, float b, float a, SPF_ColorEditFlags flags, float size_x, float size_y);
  static void UI_SetColorEditOptions(SPF_ColorEditFlags flags);

  // --- VI. Advanced Widgets ---
  // Trees & Hierarchies
  static bool UI_TreeNode(const char* label);
  static bool UI_TreeNodeEx(const char* str_id, const char* label, SPF_TreeNodeFlags flags);
  static void UI_TreePush(const char* str_id);
  static void UI_TreePop();
  static void UI_SetNextItemStorageID(uint32_t storage_id);
  static float UI_GetTreeNodeToLabelSpacing();
  static bool UI_CollapsingHeader(const char* label, SPF_TreeNodeFlags flags);
  static void UI_SetNextItemOpen(bool is_open, SPF_Cond cond);
  static bool UI_TreeNodeGetOpen(uint32_t storage_id);

  // Selectables & Lists
  static bool UI_Selectable(const char* label, bool selected, SPF_SelectableFlags flags, float size_x, float size_y);
  static bool UI_BeginListBox(const char* label, float size_x, float size_y);
  static void UI_EndListBox();
  static bool UI_ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items);

  // Multi-Select
  static SPF_MultiSelectIO* UI_BeginMultiSelect(SPF_MultiSelectFlags flags, int selection_size, int items_count);
  static SPF_MultiSelectIO* UI_EndMultiSelect();
  static void UI_SetNextItemSelectionUserData(int64_t selection_user_data);
  static bool UI_IsItemToggledSelection();

  // Data Visualization
  static void UI_PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y, int stride);
  static void UI_PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y, int stride);
  static void UI_PlotLinesCallback(const char* label, SPF_PlotGetter values_getter, void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y);
  static void UI_PlotHistogramCallback(const char* label, SPF_PlotGetter values_getter, void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y);

  // Simple Value Display
  static void UI_Value_Bool(const char* prefix, bool b);
  static void UI_Value_Int(const char* prefix, int v);
  static void UI_Value_UInt(const char* prefix, unsigned int v);
  static void UI_Value_Float(const char* prefix, float v, const char* float_format);

  // List Clipper
  static SPF_ListClipper UI_ListClipper_Begin(int items_count, float items_height);
  static bool UI_ListClipper_Step(SPF_ListClipper* clipper);
  static void UI_ListClipper_End(SPF_ListClipper* clipper);

  // --- VII. Menus ---
  static bool UI_BeginMenuBar();
  static void UI_EndMenuBar();
  static bool UI_BeginMainMenuBar();
  static void UI_EndMainMenuBar();
  static bool UI_BeginMenu(const char* label, bool enabled);
  static void UI_EndMenu();
  static bool UI_MenuItem(const char* label, const char* shortcut, bool selected, bool enabled);

  // --- VIII. Tables ---
  static bool UI_BeginTable(const char* str_id, int columns_count, SPF_TableFlags flags, float outer_size_x, float outer_size_y, float inner_width);
  static void UI_EndTable();
  static void UI_TableNextRow(SPF_TableRowFlags row_flags, float min_row_height);
  static bool UI_TableNextColumn();
  static bool UI_TableSetColumnIndex(int column_n);

  // Table Settings
  static void UI_TableSetupColumn(const char* label, SPF_TableColumnFlags flags, float init_width_or_weight, uint32_t user_id);
  static void UI_TableSetupScrollFreeze(int cols_count, int rows_count);
  static void UI_TableHeadersRow();
  static void UI_TableAngledHeadersRow();
  static void UI_TableHeader(const char* label);

  // Table State & Sorting
  static const SPF_TableSortSpecs* UI_TableGetSortSpecs();
  static void UI_TableSetColumnEnabled(int column_n, bool enabled);
  static bool UI_TableGetHoveredColumn(int column_n);

  // Table Metadata
  static int UI_TableGetColumnCount();
  static int UI_TableGetColumnIndex();
  static int UI_TableGetRowIndex();
  static const char* UI_TableGetColumnName(int column_n);
  static SPF_TableColumnFlags UI_TableGetColumnFlags(int column_n);
  static void UI_TableSetBgColor(int target, uint32_t color, int column_n);

  // --- IX. Popups & Tooltips ---

  static bool UI_BeginPopup(const char* str_id, SPF_WindowFlags flags);
  static bool UI_BeginPopupModal(const char* name, bool* p_open, SPF_WindowFlags flags);
  static void UI_EndPopup();
  static void UI_OpenPopup(const char* str_id, SPF_PopupFlags flags);
  static void UI_OpenPopupOnItemClick(const char* str_id, SPF_PopupFlags flags);
  static void UI_CloseCurrentPopup();
  static bool UI_BeginPopupContextItem(const char* str_id, SPF_PopupFlags flags);
  static bool UI_BeginPopupContextWindow(const char* str_id, SPF_PopupFlags flags);
  static bool UI_BeginPopupContextVoid(const char* str_id, SPF_PopupFlags flags);
  static bool UI_IsPopupOpen(const char* str_id, SPF_PopupFlags flags);

  // Tooltips
  static void UI_BeginTooltip();
  static void UI_EndTooltip();
  static void UI_SetTooltip(const char* text);
  static bool UI_BeginItemTooltip();
  static void UI_SetItemTooltip(const char* text);

  // --- X. Drag & Drop ---
  static bool UI_BeginDragDropSource(SPF_DragDropFlags flags);
  static bool UI_SetDragDropPayload(const char* type, const void* data, size_t size, SPF_Cond cond);
  static void UI_EndDragDropSource();
  static bool UI_BeginDragDropTarget();
  static const SPF_Payload_Handle* UI_AcceptDragDropPayload(const char* type, SPF_DragDropFlags flags);
  static void UI_EndDragDropTarget();
  static const SPF_Payload_Handle* UI_GetDragDropPayload();

  // --- XI. Style & Typography ---
  static SPF_Font_Handle UI_GetFont(const char* font_key);
  static void UI_PushFont(SPF_Font_Handle font_handle);
  static void UI_PopFont();
  static void UI_PushStyleColor(SPF_StyleColor idx, float r, float g, float b, float a);
  static void UI_PopStyleColor(int count);
  static void UI_PushStyleVarFloat(SPF_StyleVar idx, float val);
  static void UI_PushStyleVarVec2(SPF_StyleVar idx, float val_x, float val_y);
  static void UI_PopStyleVar(int count);
  static SPF_Style_Handle* UI_GetStyle();
  static void UI_Style_GetWindowPadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y);
  static void UI_Style_GetItemSpacing(SPF_Style_Handle* style_handle, float* out_x, float* out_y);
  static void UI_Style_GetFramePadding(SPF_Style_Handle* style_handle, float* out_x, float* out_y);
  static void UI_StyleColorsDark();
  static void UI_StyleColorsLight();
  static void UI_StyleColorsClassic();
  static void UI_GetStyleColor(SPF_StyleColor idx, float* out_r, float* out_g, float* out_b, float* out_a);

  // Text Utilities
  static void UI_CalcTextSize(const char* text, float* out_w, float* out_h);
  static void UI_CalcTextSizeWithFont(SPF_Font font, float font_size, const char* text, float* out_w, float* out_h);
  static uint32_t UI_ColorConvertFloat4ToU32(float r, float g, float b, float a);
  static void UI_ColorConvertU32ToFloat4(uint32_t in, float* out_r, float* out_g, float* out_b, float* out_a);
  static void UI_ColorConvertRGBtoHSV(float r, float g, float b, float* out_h, float* out_s, float* out_v);
  static void UI_ColorConvertHSVtoRGB(float h, float s, float v, float* out_r, float* out_g, float* out_b);

  // SPF Text Styling API
  static SPF_TextStyle_Handle UI_Style_Create();
  static void UI_Style_Destroy(SPF_TextStyle_Handle handle);
  static void UI_Style_SetFont(SPF_TextStyle_Handle handle, SPF_Font font);
  static void UI_Style_SetColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a);
  static void UI_Style_SetHoverColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a);
  static void UI_Style_SetActiveColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a);
  static void UI_Style_SetAlign(SPF_TextStyle_Handle handle, SPF_TextAlign align);
  static void UI_Style_SetWrap(SPF_TextStyle_Handle handle, bool wrap);
  static void UI_Style_SetPadding(SPF_TextStyle_Handle handle, float pad_x, float pad_y);
  static void UI_Style_SetSeparator(SPF_TextStyle_Handle handle, bool is_separator);
  static void UI_Style_SetUnderline(SPF_TextStyle_Handle handle, bool is_underline);
  static void UI_Style_SetStrikethrough(SPF_TextStyle_Handle handle, bool is_strikethrough);
  static void UI_TextStyled(SPF_TextStyle_Handle handle, const char* fmt, ...);
  static void UI_RenderMarkdown(const char* markdown_text, SPF_TextStyle_Handle base_style_handle);

  // --- XII. DrawList API ---
  static void UI_DrawList_PushClipRect(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, bool intersect_with_current_clip_rect);
  static void UI_DrawList_PushClipRectFullScreen(SPF_DrawList_Handle dl);
  static void UI_DrawList_PopClipRect(SPF_DrawList_Handle dl);
  static void UI_DrawList_AddLine(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness);
  static void UI_DrawList_AddRect(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, SPF_DrawFlags flags, float thickness);
  static void UI_DrawList_AddRectFilled(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, SPF_DrawFlags flags);
  static void UI_DrawList_AddRectFilledMultiColor(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left);
  static void UI_DrawList_AddQuad(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col, float thickness);
  static void UI_DrawList_AddQuadFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col);
  static void UI_DrawList_AddTriangle(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col, float thickness);
  static void UI_DrawList_AddTriangleFilled(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col);
  static void UI_DrawList_AddTriangleFilledMultiColor(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col1, uint32_t col2, uint32_t col3);
  static void UI_DrawList_AddCircle(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness);
  static void UI_DrawList_AddCircleFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments);
  static void UI_DrawList_AddCircleFilledMultiColor(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col_inner, uint32_t col_outer, int num_segments);
  static void UI_AddRectFilled(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
  static void UI_DrawList_AddNgon(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness);
  static void UI_DrawList_AddNgonFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments);
  static void UI_DrawList_AddNgonContour(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness);
  static void UI_DrawList_AddEllipse(SPF_DrawList_Handle dl, float center_x, float center_y, float radius_x, float radius_y, uint32_t col, float rot, int num_segments, float thickness);
  static void UI_DrawList_AddEllipseFilled(SPF_DrawList_Handle dl, float center_x, float center_y, float radius_x, float radius_y, uint32_t col, float rot, int num_segments);
  static void UI_DrawList_AddBezierCubic(SPF_DrawList_Handle dl, float p1_x, float p1_y, float cp1_x, float cp1_y, float cp2_x, float cp2_y, float p2_x, float p2_y, uint32_t col, float thickness, int num_segments);
  static void UI_DrawList_AddBezierQuadratic(SPF_DrawList_Handle dl, float p1_x, float p1_y, float cp_x, float cp_y, float p2_x, float p2_y, uint32_t col, float thickness, int num_segments);
  static void UI_DrawList_AddPolyline(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col, SPF_DrawFlags flags, float thickness);
  static void UI_DrawList_AddConvexPolyFilled(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col);
  static void UI_DrawList_AddConcavePolyFilled(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col);
  static void UI_DrawList_AddImage(SPF_DrawList_Handle dl, void* user_texture_id, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col);
  static void UI_DrawList_AddImageQuad(SPF_DrawList_Handle dl, void* user_texture_id, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, float uv1_x, float uv1_y, float uv2_x, float uv2_y, float uv3_x, float uv3_y, float uv4_x, float uv4_y, uint32_t col);
  static void UI_DrawList_AddImageRounded(SPF_DrawList_Handle dl, void* user_texture_id, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col, float rounding, SPF_DrawFlags flags);
  static void UI_DrawList_AddCallback(SPF_DrawList_Handle dl, void (*callback)(const void* parent_list, const void* cmd), void* user_data);
  static void UI_DrawList_AddText(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text);
  static void UI_DrawList_AddTextWithFont(SPF_DrawList_Handle dl, SPF_Font font, float font_size, float pos_x, float pos_y, uint32_t col, const char* text, float wrap_width);
  static void UI_DrawList_PathClear(SPF_DrawList_Handle dl);
  static void UI_DrawList_PathLineTo(SPF_DrawList_Handle dl, float pos_x, float pos_y);
  static void UI_DrawList_PathStroke(SPF_DrawList_Handle dl, uint32_t col, SPF_DrawFlags flags, float thickness);
  static void UI_DrawList_PathFillConvex(SPF_DrawList_Handle dl, uint32_t col);
  static void UI_DrawList_PathArcTo(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, float a_min, float a_max, int num_segments);
  static void UI_DrawList_PathRect(SPF_DrawList_Handle dl, float rect_min_x, float rect_min_y, float rect_max_x, float rect_max_y, float rounding, SPF_DrawFlags flags);
  static void UI_DrawList_ChannelsSplit(SPF_DrawList_Handle dl, int count);
  static void UI_DrawList_ChannelsMerge(SPF_DrawList_Handle dl);
  static void UI_DrawList_ChannelsSetCurrent(SPF_DrawList_Handle dl, int n);
  static void UI_DrawList_PrimReserve(SPF_DrawList_Handle dl, int idx_count, int vtx_count);
  static void UI_DrawList_PrimRectUV(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col);
  static void UI_DrawList_PrimVtx(SPF_DrawList_Handle dl, float x, float y, float u, float v, uint32_t col);
  static void UI_DrawList_PrimIdx(SPF_DrawList_Handle dl, uint16_t idx);

  // --- XIII. Item Queries & State ---
  static bool UI_IsItemHovered(SPF_HoveredFlags flags);
  static bool UI_IsItemActive();
  static bool UI_IsItemFocused();
  static bool UI_IsItemClicked(SPF_MouseButton mouse_button);
  static bool UI_IsItemVisible();
  static bool UI_IsItemEdited();
  static bool UI_IsItemActivated();
  static bool UI_IsItemDeactivated();
  static bool UI_IsItemDeactivatedAfterEdit();
  static bool UI_IsItemToggledOpen();
  static bool UI_IsAnyItemHovered();
  static bool UI_IsAnyItemActive();
  static bool UI_IsAnyItemFocused();
  static uint32_t UI_GetItemID();
  static void UI_GetItemRectMin(float* out_x, float* out_y);
  static void UI_GetItemRectMax(float* out_x, float* out_y);
  static void UI_GetItemRectSize(float* out_x, float* out_y);

  // --- XIV. Tabs & Tab Bars ---
  static bool UI_BeginTabBar(const char* str_id, SPF_TabBarFlags flags);
  static void UI_EndTabBar();
  static bool UI_BeginTabItem(const char* label, bool* p_open, SPF_TabItemFlags flags);
  static void UI_EndTabItem();
  static bool UI_TabItemButton(const char* label, SPF_TabItemFlags flags);
  static void UI_SetTabItemClosed(const char* tab_or_docked_window_label);

  // --- XV. Docking & Viewports ---
  static uint32_t UI_DockSpace(uint32_t id, float size_x, float size_y, SPF_DockNodeFlags flags);
  static uint32_t UI_DockSpaceOverViewport(SPF_DockNodeFlags flags);
  static void UI_SetNextWindowDockID(uint32_t dock_id, SPF_Cond cond);
  static bool UI_IsWindowDocked();
  static void UI_DockBuilderDockWindow(const char* window_name, uint32_t node_id);
  static void* UI_DockBuilderGetNode(uint32_t node_id);
  static uint32_t UI_DockBuilderAddNode(uint32_t node_id, SPF_DockNodeFlags flags);
  static void UI_DockBuilderRemoveNode(uint32_t node_id);
  static void UI_DockBuilderRemoveNodeDockedWindows(uint32_t node_id);
  static void UI_DockBuilderSetNodePos(uint32_t node_id, float pos_x, float pos_y);
  static void UI_DockBuilderSetNodeSize(uint32_t node_id, float size_x, float size_y);
  static uint32_t UI_DockBuilderSplitNode(uint32_t node_id, SPF_Dir split_dir, float size_ratio, uint32_t* out_id_at_dir, uint32_t* out_id_at_opposite);
  static void UI_DockBuilderFinish(uint32_t node_id);
  static uint32_t UI_DockBuilderGetCentralNode(uint32_t node_id);
  static void UI_UpdatePlatformWindows();
  static void UI_RenderPlatformWindowsDefault();
  static void UI_DestroyPlatformWindows();

  // --- XVI. Internal & Custom Widget Utilities ---
  static void UI_ItemSize(float size_x, float size_y, float text_baseline_y);
  static bool UI_ItemAdd(float min_x, float min_y, float max_x, float max_y, uint32_t id, SPF_WindowFlags flags);
  static void UI_SetLastItemData(uint32_t item_id, SPF_InputTextFlags flags, SPF_HoveredFlags status_flags, float min_x, float min_y, float max_x, float max_y);
  static bool UI_ButtonBehavior(float min_x, float min_y, float max_x, float max_y, uint32_t id, bool* out_hovered, bool* out_held, SPF_WindowFlags flags);
  static bool UI_DragBehavior(uint32_t id, SPF_DataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SliderBehavior(float min_x, float min_y, float max_x, float max_y, uint32_t id, SPF_DataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);
  static bool UI_SplitterBehavior(float bb_min_x, float bb_min_y, float bb_max_x, float bb_max_y, uint32_t id, int axis, float* size1, float* size2, float min_size1, float min_size2);
  static void UI_RenderFrame(float min_x, float min_y, float max_x, float max_y, uint32_t col, bool border, float rounding);
  static void UI_RenderFrameBorder(float min_x, float min_y, float max_x, float max_y, float rounding);
  static void UI_RenderText(float x, float y, const char* text, bool hide_text_after_hash);
  static void UI_RenderTextClipped(float min_x, float min_y, float max_x, float max_y, const char* text, const char* text_end, float* out_text_size, float align_x, float align_y);
  static void UI_RenderTextEllipsis(float x, float y, float max_width, const char* text);
  static void UI_RenderArrow(float x, float y, uint32_t col, SPF_Dir dir, float scale);
  static void UI_RenderCheckMark(float x, float y, uint32_t col, float sz);
  static void UI_RenderBullet(SPF_DrawList_Handle dl, float x, float y, uint32_t col);

  // --- XVII. Framework Utilities ---
  static SPF_Notification_Handle UI_ShowNotification(const SPF_Notification_Params* params);
  static void UI_HideNotification(SPF_Notification_Handle handle);
  static void UI_PlayTransition(SPF_TransitionType type, float duration, bool reverse, SPF_TransitionColor color);
  static bool UI_IsTransitionActive();
  static void UI_SetMouseBlockState(bool blockAxes, bool blockButtons, bool blockWheel);
  static void UI_SetMouseOverride(bool overridden);
  static bool UI_IsMouseOverridden();
};
}  // namespace Modules::API
SPF_NS_END
