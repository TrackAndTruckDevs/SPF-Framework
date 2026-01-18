#pragma once

#include <stdbool.h>
#include <stdint.h>

// Note: This is a C-style header for ABI stability.

// Forward-declare handle type
typedef struct SPF_Window_Handle SPF_Window_Handle;

// Forward-declare SPF_TextStyle_Handle
typedef struct SPF_TextStyle_Handle_t* SPF_TextStyle_Handle;

// Forward-declare SPF_DrawList_Handle for custom drawing
typedef struct SPF_DrawList_Handle_t* SPF_DrawList_Handle;

/**
 * @enum SPF_Font
 * @brief Available font styles for SPF UI elements.
 */
typedef enum {
    SPF_FONT_REGULAR,
    SPF_FONT_BOLD,
    SPF_FONT_ITALIC,
    SPF_FONT_BOLD_ITALIC,
    SPF_FONT_MEDIUM,
    SPF_FONT_MEDIUM_ITALIC,
    SPF_FONT_MONOSPACE,
    SPF_FONT_H1,
    SPF_FONT_H2,
    SPF_FONT_H3
} SPF_Font;

/**
 * @enum SPF_TextAlign
 * @brief Text alignment options for SPF UI elements.
 */
typedef enum {
    SPF_TEXT_ALIGN_LEFT,
    SPF_TEXT_ALIGN_CENTER,
    SPF_TEXT_ALIGN_RIGHT
} SPF_TextAlign;

/**
 * @enum SPF_Window_Flags
 * @brief Flags to control the behavior of a window registered by a plugin.
 */
typedef enum {
    SPF_WINDOW_FLAG_NONE = 0,
    SPF_WINDOW_FLAG_NO_TITLE = 1 << 0,  // Disable the title bar.
    SPF_WINDOW_FLAG_NO_RESIZE = 1 << 1, // Disable user resizing.
    SPF_WINDOW_FLAG_NO_MOVE = 1 << 2,   // Disable user moving the window.
    SPF_WINDOW_FLAG_NO_SCROLLBAR = 1 << 3,   // Disable scrollbar (scrolling with mouse wheel is still possible).
    SPF_WINDOW_FLAG_NO_COLLAPSE = 1 << 4,    // Disable the collapse button.
    SPF_WINDOW_FLAG_ALWAYS_AUTO_RESIZE = 1 << 5, // Auto-resize window to fit contents every frame.
    SPF_WINDOW_FLAG_MENU_BAR = 1 << 6,           // The window has a menu bar.
    SPF_WINDOW_FLAG_HORIZONTAL_SCROLLBAR = 1 << 7, // Allow horizontal scrollbar.
} SPF_Window_Flags;

/**
 * @brief A callback function that a plugin provides to draw the content of its window.
 * @param builder A pointer to the UI builder API, used to construct widgets.
 * @param user_data A pointer to user-defined data, passed during registration.
 */
typedef void (*SPF_DrawCallback)(struct SPF_UI_API* builder, void* user_data);

/**
 * @struct SPF_UI_API
 * @brief C-style API for creating and managing plugin UI windows using an immediate-mode paradigm.
 *
 * @details This API provides a stable C interface to the framework's underlying
 *          ImGui-based rendering engine. It allows plugins to register windows,
 *          define their content using simple widget calls, and control their behavior.
 *          The function pointers within this struct map directly to ImGui functions.
 * 
 *          The window title is handled automatically by the framework using the localization
 *          key `windowId.title` (e.g., "MainWindow.title"). If no translation is specified,
 *          the windowId itself will be used as the title.
 *
 * @section Workflow
 * 1.  **Declare in Manifest**: First, declare all your windows in the `ui` section
 *     of your plugin's manifest (`GetManifestData`). This tells the framework
 *     what windows your plugin has, their default visibility, size, etc.
 * 2.  **Implement a Draw Callback**: For each window, create a C-style function in your
 *     plugin that will be responsible for drawing its content. This function must
 *     match the `SPF_DrawCallback` signature.
 * 3.  **Register the Callback**: Implement the `OnRegisterUI` lifecycle function in your
 *     plugin. The framework will call this function once. Inside it, call
 *     `RegisterDrawCallback` for each window, linking the window ID from the
 *     manifest to your corresponding draw callback function.
 * 4.  **Draw Widgets**: Inside your draw callback, use the provided `SPF_UI_API*`
 *     pointer to call widget functions (`Text`, `Button`, etc.) to build your UI.
 *     This is done every frame the window is visible.
 */
typedef struct SPF_UI_API {
    /**
     * @brief Registers a draw callback for a window that was declared in the plugin's manifest.
     *
     * @details This function links a window ID (which you defined in your manifest's `ui`
     *          section) to a C-function in your plugin that will be called to draw the
     *          window's contents every frame. This should be called from your plugin's
     *          `OnRegisterUI` lifecycle function.
     *
     * @param pluginName The name of the plugin owning this window (must match manifest).
     * @param windowId The unique identifier for the window (must match manifest).
     * @param drawCallback A function pointer that will be called to render the window's content.
     * @param user_data An optional pointer to user data that will be passed to the callback.
     */
    void (*RegisterDrawCallback)(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data);

    /**
     * @brief Registers a draw callback for a window with additional control flags.
     *
     * @details This function is an alternative way to register a window. It extends
     *          `RegisterDrawCallback` by allowing you to provide `SPF_Window_Flags`
     *          to control the window's behavior (e.g., disable resizing, add a menu bar).
     *
     * @param pluginName The name of the plugin owning this window.
     * @param windowId The unique identifier for the window.
     * @param drawCallback A function pointer for rendering the window's content.
     * @param user_data An optional pointer to user data.
     * @param flags A bitmask of `SPF_Window_Flags` to control window properties.
     */
    void (*RegisterDrawCallbackWithFlags)(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_Window_Flags flags);

    /**
     * @brief Gets a handle to a window for programmatic control.
     * @param pluginName The name of the plugin owning this window.
     * @param windowId The unique identifier for the window.
     * @return A handle to the window, or NULL if not found.
     */
    SPF_Window_Handle* (*GetWindowHandle)(const char* pluginName, const char* windowId);

    /**
     * @brief Programmatically sets the visibility of a window.
     * @param handle The window handle obtained from GetWindowHandle.
     * @param isVisible The new visibility state.
     */
    void (*SetVisibility)(SPF_Window_Handle* handle, bool isVisible);

    /**
     * @brief Gets the current visibility of a window.
     * @param handle The window handle.
     * @return True if the window is currently visible, false otherwise.
     */
    bool (*IsVisible)(SPF_Window_Handle* handle);



    // --- Basic Widgets ---

    void (*Text)(const char* text);
    void (*TextColored)(float r, float g, float b, float a, const char* text);
    void (*TextDisabled)(const char* text);
    void (*TextWrapped)(const char* text);
    void (*LabelText)(const char* label, const char* text);
    void (*BulletText)(const char* text);

    bool (*Button)(const char* label, float width, float height);
    bool (*SmallButton)(const char* label);
    bool (*InvisibleButton)(const char* str_id, float width, float height);

    bool (*Checkbox)(const char* label, bool* v);
    // It's generally unsafe to pass pointers to bitfields across ABI boundaries.
    // bool (*CheckboxFlags)(const char* label, int* flags, int flags_value);

    bool (*RadioButton)(const char* label, bool active);
    // bool (*RadioButtonFlags)(const char* label, int* v, int v_button);

    void (*ProgressBar)(float fraction, float width, float height, const char* overlay);
    void (*Bullet)();

    // --- Layout & Spacing ---

    void (*Separator)();
    void (*Spacing)();
    void (*Indent)(float indent_w);
    void (*Unindent)(float indent_w);
    void (*SameLine)(float offset_from_start_x, float spacing);

    // --- Input Widgets ---

    bool (*InputText)(const char* label, char* buf, size_t buf_size);
    bool (*InputInt)(const char* label, int* v, int step, int step_fast, int flags);
    bool (*InputFloat)(const char* label, float* v, float step, float step_fast, const char* format, int flags);
    bool (*InputDouble)(const char* label, double* v, double step, double step_fast, const char* format);

    bool (*BeginCombo)(const char* label, const char* preview_value);
    void (*EndCombo)();
    bool (*Selectable)(const char* label, bool selected);

    // --- Tree Nodes ---
    bool (*TreeNode)(const char* label);
    void (*TreePush)(const char* str_id);
    void (*TreePop)();

    // --- Tabs ---
    bool (*BeginTabBar)(const char* str_id);
    void (*EndTabBar)();
    bool (*BeginTabItem)(const char* label);
    void (*EndTabItem)();

    // --- Tables ---
    bool (*BeginTable)(const char* str_id, int column);
    void (*EndTable)();
    void (*TableNextRow)();
    bool (*TableNextColumn)();
    void (*TableSetupColumn)(const char* label);

    // --- Popups & Tooltips ---
    void (*OpenPopup)(const char* str_id);
    bool (*BeginPopup)(const char* str_id);
    void (*EndPopup)();
    bool (*IsItemHovered)();
    bool (*IsItemActive)();
    void (*SetTooltip)(const char* text);

    // --- Advanced Inputs ---
    bool (*InputTextMultiline)(const char* label, char* buf, size_t buf_size);
    bool (*SliderFloat2)(const char* label, float v[2], float v_min, float v_max);
    bool (*SliderFloat3)(const char* label, float v[3], float v_min, float v_max);
    bool (*SliderFloat4)(const char* label, float v[4], float v_min, float v_max);
    bool (*SliderInt2)(const char* label, int v[2], int v_min, int v_max);
    bool (*SliderInt3)(const char* label, int v[3], int v_min, int v_max);
    bool (*SliderInt4)(const char* label, int v[4], int v_min, int v_max);
    bool (*ColorEdit3)(const char* label, float col[3]);
    bool (*ColorEdit4)(const char* label, float col[4]);
    bool (*DragFloat)(const char* label, float* v, float v_speed, float v_min, float v_max);
    bool (*DragInt)(const char* label, int* v, float v_speed, int v_min, int v_max);

    bool (*SliderInt)(const char* label, int* v, int v_min, int v_max, const char* format);
    bool (*SliderFloat)(const char* label, float* v, float v_min, float v_max, const char* format);

    // --- Style ---

    void (*PushStyleColor)(int im_gui_color_idx, float r, float g, float b, float a);
    void (*PopStyleColor)(int count);

    void (*PushStyleVarFloat)(int im_gui_stylevar_idx, float val);
    void (*PushStyleVarVec2)(int im_gui_stylevar_idx, float val_x, float val_y);
    void (*PopStyleVar)(int count);

    // --- Custom Drawing ---

    void (*GetViewportSize)(float* out_width, float* out_height); //game window size
    void (*AddRectFilled)(float x1, float y1, float x2, float y2, float r, float g, float b, float a);


    // --- Text Styling API (v1.0 - SPF-377) ---
    // The following set of functions allows for the creation and manipulation of text style objects.
    // These objects can then be passed to rendering functions like `TextStyled` and `RenderMarkdown`
    // to control typography, color, layout, and more.
    //
    // Workflow:
    // 1. Create a style object with `Style_Create()`.
    // 2. Configure it using the `Style_Set...()` functions.
    // 3. Pass the handle to a rendering function like `TextStyled()`.
    // 4. Destroy the style object with `Style_Destroy()` when it's no longer needed to release memory.

    /**
     * @brief Creates a new, empty text style handle.
     * @details This handle represents a collection of style properties. It must be destroyed
     *          with `Style_Destroy` to prevent memory leaks.
     * @return A new `SPF_TextStyle_Handle`.
     */
    SPF_TextStyle_Handle (*Style_Create)();

    /**
     * @brief Destroys a text style handle and releases its memory.
     * @param handle The style handle to destroy.
     */
    void (*Style_Destroy)(SPF_TextStyle_Handle handle);

    /**
     * @brief Sets the font for the text style.
     * @param handle The style handle to modify.
     * @param font The desired font from the `SPF_Font` enum.
     */
    void (*Style_SetFont)(SPF_TextStyle_Handle handle, SPF_Font font);

    /**
     * @brief Sets the color of the text.
     * @param handle The style handle to modify.
     * @param r, g, b, a The color components (0.0f to 1.0f).
     */
    void (*Style_SetColor)(SPF_TextStyle_Handle handle, float r, float g, float b, float a);

    /**
     * @brief Sets the horizontal alignment of the text.
     * @details Centering and right-alignment are relative to the available content region width.
     * @param handle The style handle to modify.
     * @param align The desired alignment from the `SPF_TextAlign` enum.
     */
    void (*Style_SetAlign)(SPF_TextStyle_Handle handle, SPF_TextAlign align);

    /**
     * @brief Enables or disables automatic text wrapping.
     * @param handle The style handle to modify.
     * @param wrap Set to true to enable wrapping, false to disable.
     */
    void (*Style_SetWrap)(SPF_TextStyle_Handle handle, bool wrap);

    /**
     * @brief Sets padding around the text block.
     * @param handle The style handle to modify.
     * @param pad_x Horizontal padding.
     * @param pad_y Vertical padding.
     */
    void (*Style_SetPadding)(SPF_TextStyle_Handle handle, float pad_x, float pad_y);

    /**
     * @brief Turns the text into a separator with a label.
     * @details When true, the text will be rendered as a horizontal line with the text embedded in it.
     * @param handle The style handle to modify.
     * @param is_separator Set to true to render as a separator.
     */
    void (*Style_SetSeparator)(SPF_TextStyle_Handle handle, bool is_separator);

    /**
     * @brief Enables or disables an underline decoration.
     * @param handle The style handle to modify.
     * @param is_underline Set to true to draw an underline.
     */
    void (*Style_SetUnderline)(SPF_TextStyle_Handle handle, bool is_underline);

    /**
     * @brief Enables or disables a strikethrough decoration.
     * @param handle The style handle to modify.
     * @param is_strikethrough Set to true to draw a strikethrough line.
     */
    void (*Style_SetStrikethrough)(SPF_TextStyle_Handle handle, bool is_strikethrough);

    // --- Styled Rendering (v1.0 - SPF-377) ---

    /**
     * @brief Renders text with a specific style, supporting printf-style formatting.
     * @details This function is the styled equivalent of the basic 'Text' function. It allows
     *          for applying a complex style object (created via Style_Create) to a piece of
     *          text. The format string 'fmt' and subsequent arguments work exactly like the
     *          standard C printf function.
     * @param handle A handle to a style object created with `Style_Create`. If NULL, default
     *               styling will be used.
     * @param fmt A printf-style format string.
     * @param ... Optional subsequent arguments for the format string.
     */
    void (*TextStyled)(SPF_TextStyle_Handle handle, const char* fmt, ...);

    /**
     * @brief Renders a block of text formatted with Markdown.
     * @details Supports basic Markdown syntax like headers (#, ##), bold (**),
     *          italic (*), code blocks (```), and links.
     * @param markdown_text The string containing the Markdown to render.
     * @param base_style_handle An optional style handle to apply base properties like
     *                          padding or a default color to the entire block. The renderer
     *                          will still override fonts and colors for specific Markdown
     *                          elements (e.g., H1 will use the 'h1' font). If NULL,
     *                          a default style is used.
     */
    void (*RenderMarkdown)(const char* markdown_text, SPF_TextStyle_Handle base_style_handle);


    // --- Custom Widget API ---
    // The following functions provide low-level access to the drawing and interaction primitives
    // needed to create fully custom widgets beyond the standard set.
    //
    // Workflow for a custom widget:
    // 1. Create a canvas for your widget, typically with `InvisibleButton()`.
    // 2. Query mouse state relative to the widget using functions like `IsItemHovered()`, `GetMousePos()`, etc.
    // 3. Get the window's draw list using `GetWindowDrawList()`.
    // 4. Use the `DrawList_...()` functions to draw your custom shapes, text, and visuals.

    /**
     * @brief Converts an RGBA color from four floats (0.0-1.0) to a packed 32-bit integer color.
     * @details This is the format required by all `DrawList` functions.
     * @return A 32-bit unsigned integer representing the color (e.g., 0xAABBGGRR).
     */
    uint32_t (*ColorConvertFloat4ToU32)(float r, float g, float b, float a);

    /**
     * @brief Gets a handle to the draw list for the current window.
     * @details The draw list is the primary tool for custom drawing. It contains all the commands
     *          to draw shapes, text, and images. This handle is valid for the current frame only.
     * @return A handle to the draw list.
     */
    SPF_DrawList_Handle (*GetWindowDrawList)();

    // --- DrawList Drawing Functions ---

    /**
     * @brief Adds a line to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y The starting point of the line.
     * @param p2_x, p2_y The ending point of the line.
     * @param col The color of the line as a packed 32-bit integer.
     * @param thickness The thickness of the line in pixels.
     */
    void (*DrawList_AddLine)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness);

    /**
     * @brief Adds a filled rectangle to the draw list.
     * @param dl The draw list handle.
     * @param p_min_x, p_min_y The top-left corner of the rectangle.
     * @param p_max_x, p_max_y The bottom-right corner of the rectangle.
     * @param col The fill color as a packed 32-bit integer.
     * @param rounding The radius of the corners. 0 for a sharp rectangle.
     */
    void (*DrawList_AddRectFilled)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding);

    /**
     * @brief Adds a filled circle to the draw list.
     * @param dl The draw list handle.
     * @param center_x, center_y The center of the circle.
     * @param radius The radius of the circle.
     * @param col The fill color as a packed 32-bit integer.
     * @param num_segments The number of segments to use to approximate the circle. More segments = smoother circle.
     */
    void (*DrawList_AddCircleFilled)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments);

    /**
     * @brief Adds text to the draw list at a specific screen position.
     * @details Unlike `Text()`, this is a low-level draw command and does not interact with layout.
     * @param dl The draw list handle.
     * @param pos_x, pos_y The top-left screen coordinate to start drawing the text.
     * @param col The color of the text as a packed 32-bit integer.
     * @param text The text to draw.
     */
    void (*DrawList_AddText)(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text);

    /**
     * @brief Adds a rectangle (outline) to the draw list.
     * @param dl The draw list handle.
     * @param p_min_x, p_min_y The top-left corner of the rectangle.
     * @param p_max_x, p_max_y The bottom-right corner of the rectangle.
     * @param col The color of the outline.
     * @param rounding The radius of the corners. 0 for a sharp rectangle.
     * @param thickness The thickness of the outline.
     */
    void (*DrawList_AddRect)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, float thickness);
    
    /**
     * @brief Adds a filled quadrilateral to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y, p2_x, p2_y, p3_x, p3_y, p4_x, p4_y The four corner points of the quad.
     * @param col The fill color.
     */
    void (*DrawList_AddQuadFilled)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col);

    /**
     * @brief Adds a filled triangle to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y, p2_x, p2_y, p3_x, p3_y The three corner points of the triangle.
     * @param col The fill color.
     */
    void (*DrawList_AddTriangleFilled)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col);
    
    /**
     * @brief Adds a cubic Bezier curve to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y The starting point of the curve.
     * @param p2_x, p2_y The first control point.
     * @param p3_x, p3_y The second control point.
     * @param p4_x, p4_y The ending point of the curve.
     * @param col The color of the curve.
     * @param thickness The thickness of the curve.
     * @param num_segments The number of line segments to use to approximate the curve.
     */
    void (*DrawList_AddBezierCubic)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col, float thickness, int num_segments);

    // --- DrawList Path/Polyline Functions ---

    /**
     * @brief Draws a polyline (a sequence of connected lines) from a set of points.
     * @details This is useful for drawing graphs or simple non-closing shapes.
     * @param dl The draw list handle.
     * @param points_x An array of floats for the x-coordinates of the points.
     * @param points_y An array of floats for the y-coordinates of the points.
     * @param num_points The number of points in the arrays.
     * @param col The color of the line.
     * @param closed If true, a line will be drawn from the last point to the first.
     * @param thickness The thickness of the lines.
     */
    void (*DrawList_AddPolyline)(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col, bool closed, float thickness);

    /**
     * @brief Clears the current path in the draw list. A path is a sequence of points that can be stroked or filled.
     */
    void (*DrawList_PathClear)(SPF_DrawList_Handle dl);

    /**
     * @brief Adds a line from the current path position to a new position.
     * @param dl The draw list handle.
     * @param pos_x, pos_y The new position to draw a line to.
     */
    void (*DrawList_PathLineTo)(SPF_DrawList_Handle dl, float pos_x, float pos_y);

    /**
     * @brief Draws an outline of the current path.
     * @param dl The draw list handle.
     * @param col The color of the outline.
     * @param closed If true, a line will be drawn from the last point to the first before stroking.
     * @param thickness The thickness of the outline.
     */
    void (*DrawList_PathStroke)(SPF_DrawList_Handle dl, uint32_t col, bool closed, float thickness);

    /**
     * @brief Fills the interior of the current path (if it's a convex polygon).
     * @param dl The draw list handle.
     * @param col The fill color.
     */
    void (*DrawList_PathFillConvex)(SPF_DrawList_Handle dl, uint32_t col);
    

    // --- Advanced Interaction API (v1.1 - SPF-412) ---

    /**
     * @brief Gets the current position of the mouse cursor in screen coordinates.
     * @param[out] out_x Pointer to a float to store the x-coordinate.
     * @param[out] out_y Pointer to a float to store the y-coordinate.
     */
    void (*GetMousePos)(float* out_x, float* out_y);

    /**
     * @brief Checks if the user is currently dragging the mouse with a specific button held down.
     * @param mouse_button_index The index of the mouse button (0=Left, 1=Right, 2=Middle).
     * @return True if the user is dragging with the specified button, false otherwise.
     */
    bool (*IsMouseDragging)(int mouse_button_index);

    /**
     * @brief Gets the total displacement of the mouse since a drag operation started.
     * @param mouse_button_index The index of the mouse button being dragged.
     * @param[out] out_dx Pointer to a float to store the horizontal displacement.
     * @param[out] out_dy Pointer to a float to store the vertical displacement.
     */
    void (*GetMouseDragDelta)(int mouse_button_index, float* out_dx, float* out_dy);

    /**
     * @brief Checks if a mouse button is currently held down.
     * @param mouse_button_index The index of the mouse button (0=Left, 1=Right, 2=Middle).
     * @return True if the button is held down.
     */
    bool (*IsMouseDown)(int mouse_button_index);

    /**
     * @brief Checks if a mouse button was clicked (pressed and released) in the current frame.
     * @param mouse_button_index The index of the mouse button.
     * @return True if the button was clicked this frame.
     */
    bool (*IsMouseClicked)(int mouse_button_index);

    /**
     * @brief Checks if a mouse button was released in the current frame.
     * @param mouse_button_index The index of the mouse button.
     * @return True if the button was released this frame.
     */
    bool (*IsMouseReleased)(int mouse_button_index);

    /**
     * @brief Checks if a mouse button was double-clicked.
     * @param mouse_button_index The index of the mouse button.
     * @return True if the button was double-clicked.
     */
    bool (*IsMouseDoubleClicked)(int mouse_button_index);

    /**
     * @brief Gets the vertical scroll amount of the mouse wheel for the current frame.
     * @return A positive value for scrolling up, a negative value for scrolling down, and 0 if no scroll.
     */
    float (*GetMouseWheel)();

    
    // --- Layout & Positioning API ---
    // The following functions provide information about the current window, layout state,
    // and positioning of items.

    /**
     * @brief Gets the available content region within the current window.
     * @details This is useful for sizing custom widgets to fill available space.
     *          It's equivalent to ImGui::GetContentRegionAvail().
     * @param[out] out_x Pointer to a float to store the available width.
     * @param[out] out_y Pointer to a float to store the available height.
     */
    void (*GetContentRegionAvail)(float* out_x, float* out_y);

    /**
     * @brief Gets the position of the current window.
     * @param[out] out_x Pointer to a float to store the window's x-coordinate.
     * @param[out] out_y Pointer to a float to store the window's y-coordinate.
     */
    void (*GetWindowPos)(float* out_x, float* out_y);

    /**
     * @brief Gets the size of the current window's content region.
     * @param[out] out_x Pointer to a float to store the window's width.
     * @param[out] out_y Pointer to a float to store the window's height.
     */
    void (*GetWindowSize)(float* out_x, float* out_y);

    /**
     * @brief Gets the screen-space position of the layout cursor.
     * @details This is where the next widget will be drawn.
     * @param[out] out_x Pointer to a float to store the cursor's x-coordinate.
     * @param[out] out_y Pointer to a float to store the cursor's y-coordinate.
     */
    void (*GetCursorScreenPos)(float* out_x, float* out_y);

    /**
     * @brief Sets the screen-space position of the layout cursor.
     * @param x The new x-coordinate for the cursor.
     * @param y The new y-coordinate for the cursor.
     */
    void (*SetCursorScreenPos)(float x, float y);

    /**
     * @brief Gets the bounding box of the last drawn item.
     * @param[out] min_x, min_y Pointer to store the top-left corner of the item.
     * @param[out] max_x, max_y Pointer to store the bottom-right corner of the item.
     */
    void (*GetItemRectMin)(float* out_x, float* out_y);
    void (*GetItemRectMax)(float* out_x, float* out_y);

    /**
     * @brief Gets the size of the last drawn item.
     * @param[out] out_x Pointer to a float to store the item's width.
     * @param[out] out_y Pointer to a float to store the item's height.
     */
    void (*GetItemRectSize)(float* out_x, float* out_y);

} SPF_UI_API;
