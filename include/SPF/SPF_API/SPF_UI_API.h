/**                                                                                               
* @file SPF_UI_API.h                                                                          
* @brief C-style API for creating and managing plugin UI windows.
*                                                                                                 
* @details This API provides a stable C interface to the framework's underlying 
*          ImGui-based rendering engine. It uses an immediate-mode paradigm where 
*          the UI is defined and processed every frame inside a draw callback.
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Immediate Mode**: UI elements are not persistent objects. They are "drawn" 
*    every frame within the draw callback. State must be managed by the plugin.
*                                                                                                 
* 2. **Draw Callbacks**: The framework calls a plugin-provided function to render 
*    each window. This function receives a pointer to the 'SPF_UI_API' table.
*                                                                                                 
* 3. **Window Lifecycle**: Windows are declared in the manifest, registered in 
*    'OnRegisterUI', and drawn during the frame rendering phase.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // 1. Define your rendering logic
* void MyPlugin_RenderWindow(SPF_UI_API* ui, void* user_data) {
*     ui->UI_Text("Welcome to my Plugin!");
*     
*     static bool my_bool = false;
*     if (ui->UI_Checkbox("Enable Feature", &my_bool)) {
*         // React to change...
*     }
* }
*
* // 2. Register the callback during initialization
* void OnRegisterUI(SPF_UI_API* api) {
*     api->UI_RegisterDrawCallback("MyPlugin", "MainWindow", MyPlugin_RenderWindow, nullptr);
* }
* @endcode                                                                                        
*/ 
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

// Forward-declare SPF_Font_Handle for dynamic font management
typedef struct ImFont* SPF_Font_Handle;

// Forward-declare SPF_Style_Handle for accessing global style variables
typedef struct ImGuiStyle* SPF_Style_Handle;

// Forward-declare SPF_Payload_Handle for Drag and Drop operations
typedef struct ImGuiPayload* SPF_Payload_Handle;

/**
 * @enum SPF_StyleVar
 * @brief C-style enum mirroring ImGui's ImGuiStyleVar_ enum for style variables.
 * @details This provides a stable way for plugins to refer to style variables without
 *          needing to include ImGui headers directly. This can be expanded as needed.
 */
typedef enum
{
    SPF_STYLE_VAR_ALPHA,                // float,     Alpha
    SPF_STYLE_VAR_WINDOW_PADDING,       // ImVec2,    WindowPadding
    SPF_STYLE_VAR_WINDOW_ROUNDING,      // float,     WindowRounding
    SPF_STYLE_VAR_WINDOW_BORDERSIZE,    // float,     WindowBorderSize
    SPF_STYLE_VAR_WINDOW_TITLE_ALIGN,   // ImVec2,    WindowTitleAlign
    SPF_STYLE_VAR_CHILD_ROUNDING,       // float,     ChildRounding
    SPF_STYLE_VAR_CHILD_BORDERSIZE,     // float,     ChildBorderSize
    SPF_STYLE_VAR_POPUP_ROUNDING,       // float,     PopupRounding
    SPF_STYLE_VAR_POPUP_BORDERSIZE,     // float,     PopupBorderSize
    SPF_STYLE_VAR_FRAME_PADDING,        // ImVec2,    FramePadding
    SPF_STYLE_VAR_FRAME_ROUNDING,       // float,     FrameRounding
    SPF_STYLE_VAR_FRAME_BORDERSIZE,     // float,     FrameBorderSize
    SPF_STYLE_VAR_ITEM_SPACING,         // ImVec2,    ItemSpacing
    SPF_STYLE_VAR_ITEM_INNER_SPACING,   // ImVec2,    ItemInnerSpacing
    SPF_STYLE_VAR_INDENT_SPACING,       // float,     IndentSpacing
    SPF_STYLE_VAR_SCROLLBAR_SIZE,       // float,     ScrollbarSize
    SPF_STYLE_VAR_SCROLLBAR_ROUNDING,   // float,     ScrollbarRounding
    SPF_STYLE_VAR_GRAB_MINSIZE,         // float,     GrabMinSize
    SPF_STYLE_VAR_GRAB_ROUNDING,        // float,     GrabRounding
    SPF_STYLE_VAR_TAB_ROUNDING,         // float,     TabRounding
    SPF_STYLE_VAR_TAB_BORDERSIZE,       // float,     TabBorderSize
    SPF_STYLE_VAR_BUTTON_TEXT_ALIGN,    // ImVec2,    ButtonTextAlign
    SPF_STYLE_VAR_SELECTABLE_TEXT_ALIGN,// ImVec2,    SelectableTextAlign
    SPF_STYLE_VAR_COUNT
} SPF_StyleVar;


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
      *     `UI_RegisterDrawCallback` for each window, linking the window ID from the
      *     manifest to your corresponding draw callback function.
      * 4.  **Draw Widgets**: Inside your draw callback, use the provided `SPF_UI_API*`
      *      pointer to call widget functions (`Text`, `Button`, etc.) to build your UI.
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
    void (*UI_RegisterDrawCallback)(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data);

    /**
     * @brief Registers a draw callback for a window with additional control flags.
     *
     * @details This function is an alternative way to register a window. It extends
     *          `UI_RegisterDrawCallback` by allowing you to provide `SPF_Window_Flags`
     *          to control the window's behavior (e.g., disable resizing, add a menu bar).
     *
     * @param pluginName The name of the plugin owning this window.
     * @param windowId The unique identifier for the window.
     * @param drawCallback A function pointer for rendering the window's content.
     * @param user_data An optional pointer to user data.
     * @param flags A bitmask of `SPF_Window_Flags` to control window properties.
     */
    void (*UI_RegisterDrawCallbackWithFlags)(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_Window_Flags flags);

    /**
     * @brief Gets a handle to a window for programmatic control.
     * @param pluginName The name of the plugin owning this window.
     * @param windowId The unique identifier for the window.
     * @return A handle to the window, or NULL if not found.
     */
    SPF_Window_Handle* (*UI_GetWindowHandle)(const char* pluginName, const char* windowId);

    /**
     * @brief Programmatically sets the visibility of a window.
     * @param handle The window handle obtained from GetWindowHandle.
     * @param isVisible The new visibility state.
     */
    void (*UI_SetVisibility)(SPF_Window_Handle* handle, bool isVisible);

    /**
     * @brief Gets the current visibility of a window.
     * @param handle The window handle.
     * @return True if the window is currently visible, false otherwise.
     */
    bool (*UI_IsVisible)(SPF_Window_Handle* handle);



    // --- Basic Widgets ---

    /**
     * @brief Displays text.
     * @details This function renders a simple text string. It automatically wraps
     *          if the text is too long for the current line and does not support
     *          printf-style formatting by default.
     * @param text The string to display.
     */
    void (*UI_Text)(const char* text);

    /**
     * @brief Displays colored text.
     * @details Renders a text string with a specified color. Supports printf-style formatting.
     * @param r, g, b, a Color components (0.0f to 1.0f).
     * @param text The format string for the text.
     * @param ... Optional arguments for printf-style formatting.
     */
    void (*UI_TextColored)(float r, float g, float b, float a, const char* text);

    /**
     * @brief Displays disabled (grayed-out) text.
     * @details Renders a text string with the disabled text color from the current style.
     *          Supports printf-style formatting.
     * @param text The format string for the text.
     * @param ... Optional arguments for printf-style formatting.
     */
    void (*UI_TextDisabled)(const char* text);

    /**
     * @brief Displays text that wraps automatically within the current content region.
     * @details Renders a text string that will automatically wrap to the next line if
     *          it exceeds the available width. Supports printf-style formatting.
     * @param text The format string for the text.
     * @param ... Optional arguments for printf-style formatting.
     */
    void (*UI_TextWrapped)(const char* text);

    /**
     * @brief Displays a label followed by text.
     * @details This is typically used for read-only fields or displaying values.
     *          The label is on the left, and the text is on the right. Supports printf-style formatting.
     * @param label The label string.
     * @param text The format string for the value text.
     * @param ... Optional arguments for printf-style formatting.
     */
    void (*UI_LabelText)(const char* label, const char* text);

    /**
     * @brief Displays text preceded by a bullet point.
     * @details Renders a text string with a bullet point marker to its left.
     *          Supports printf-style formatting.
     * @param text The format string for the text.
     * @param ... Optional arguments for printf-style formatting.
     */
    void (*UI_BulletText)(const char* text);

    /**
     * @brief Displays a clickable button.
     * @param label The text displayed on the button.
     * @param width The width of the button. Use 0 for auto-width.
     * @param height The height of the button. Use 0 for auto-height.
     * @return True if the button was clicked this frame, false otherwise.
     */
    bool (*UI_Button)(const char* label, float width, float height);

    /**
     * @brief Displays a small clickable button.
     * @details A compact version of `UI_Button`.
     * @param label The text displayed on the button.
     * @return True if the button was clicked this frame, false otherwise.
     */
    bool (*UI_SmallButton)(const char* label);

    /**
     * @brief Creates an invisible button for custom interaction areas.
     * @details This function creates a clickable area that draws nothing, but responds
     *          to mouse input. It's often used as a canvas for custom drawing
     *          or to implement custom interaction logic.
     * @param str_id A unique string identifier for the invisible button.
     * @param width The width of the invisible button.
     * @param height The height of the invisible button.
     * @return True if the invisible button was clicked this frame, false otherwise.
     */
    bool (*UI_InvisibleButton)(const char* str_id, float width, float height);

    /**
     * @brief Displays a checkbox for boolean values.
     * @param label The text label for the checkbox.
     * @param v A pointer to the boolean variable to be linked to the checkbox state.
     * @return True if the checkbox's state changed this frame, false otherwise.
     */
    bool (*UI_Checkbox)(const char* label, bool* v);

    /**
     * @brief Displays a radio button.
     * @details When several radio buttons are in a group, only one can be active.
     * @param label The text label for the radio button.
     * @param active The current active state of this radio button.
     * @return True if the radio button was clicked this frame, false otherwise.
     */
    bool (*UI_RadioButton)(const char* label, bool active);

    /**
     * @brief Displays a progress bar.
     * @param fraction The progress as a float between 0.0f and 1.0f.
     * @param width The width of the progress bar. Use 0 for auto-width.
     * @param height The height of the progress bar. Use 0 for auto-height.
     * @param overlay An optional text string to display over the progress bar.
     */
    void (*UI_ProgressBar)(float fraction, float width, float height, const char* overlay);

    /**
     * @brief Displays a simple bullet point.
     */
    void (*UI_Bullet)();

    // --- Layout & Spacing ---

    /**
     * @brief Adds a horizontal line separator.
     * @details Visually separates widgets on the same line or in the same column.
     */
    void (*UI_Separator)();

    /**
     * @brief Adds a vertical space.
     * @details Inserts vertical blank space equal to one line height.
     */
    void (*UI_Spacing)();

    /**
     * @brief Indents the following widgets.
     * @details Moves the cursor position to the right, effectively indenting all
     *          subsequent widgets until `UI_Unindent` is called.
     * @param indent_w The amount of horizontal space to indent by. Use 0.0f for default.
     */
    void (*UI_Indent)(float indent_w);

    /**
     * @brief Unindents the following widgets.
     * @details Moves the cursor position to the left, canceling the effect of `Indent`.
     * @param indent_w The amount of horizontal space to unindent by. Use 0.0f for default.
     */
    void (*UI_Unindent)(float indent_w);

    /**
     * @brief Places the next widget on the same line as the previous one.
     * @details Useful for arranging multiple small widgets horizontally.
     * @param offset_from_start_x Optional horizontal offset from the start of the line. Use 0.0f for default.
     * @param spacing Optional horizontal spacing between the current and previous widget. Use -1.0f for default.
     */
    void (*UI_SameLine)(float offset_from_start_x, float spacing);

    // --- Input Widgets ---

    /**
     * @brief Displays a single-line text input field.
     * @param label The text label for the input field.
     * @param buf A character buffer to store the input text.
     * @param buf_size The size of the character buffer.
     * @return True if the input text was modified this frame, false otherwise.
     */
    bool (*UI_InputText)(const char* label, char* buf, size_t buf_size);

    /**
     * @brief Displays an integer input field.
     * @param label The text label for the input field.
     * @param v A pointer to the integer variable to be linked to the input field.
     * @param step The step value for arrow buttons (increment/decrement).
     * @param step_fast The step value when holding Shift and using arrow buttons.
     * @param flags Additional flags to customize behavior (e.g., `ImGuiInputTextFlags_CharsHexadecimal`).
     * @return True if the input value was modified this frame, false otherwise.
     */
    bool (*UI_InputInt)(const char* label, int* v, int step, int step_fast, int flags);

    /**
     * @brief Displays a floating-point number input field.
     * @param label The text label for the input field.
     * @param v A pointer to the float variable to be linked to the input field.
     * @param step The step value for arrow buttons (increment/decrement).
     * @param step_fast The step value when holding Shift and using arrow buttons.
     * @param format The format string for displaying the float (e.g., "%.3f").
     * @param flags Additional flags to customize behavior.
     * @return True if the input value was modified this frame, false otherwise.
     */
    bool (*UI_InputFloat)(const char* label, float* v, float step, float step_fast, const char* format, int flags);

    /**
     * @brief Displays a double-precision floating-point number input field.
     * @param label The text label for the input field.
     * @param v A pointer to the double variable to be linked to the input field.
     * @param step The step value for arrow buttons.
     * @param step_fast The fast step value.
     * @param format The format string for displaying the double.
     * @return True if the input value was modified this frame, false otherwise.
     */
    bool (*UI_InputDouble)(const char* label, double* v, double step, double step_fast, const char* format);

    /**
     * @brief Begins a combo box (dropdown list).
     * @details This function must be followed by calls to `UI_Selectable` for each item
     *          and then `UI_EndCombo`.
     * @param label The text label for the combo box.
     * @param preview_value The text to display when the combo box is closed.
     * @return True if the combo box is open, false otherwise.
     */
    bool (*UI_BeginCombo)(const char* label, const char* preview_value);

    /**
     * @brief Ends a combo box.
     * @details Must be called after `UI_BeginCombo`.
     */
    void (*UI_EndCombo)();

    /**
     * @brief Displays a selectable item, typically used within combo boxes or menus.
     * @param label The text label for the selectable item.
     * @param selected The current selected state of the item.
     * @return True if the item was clicked this frame, false otherwise.
     */
    bool (*UI_Selectable)(const char* label, bool selected);

    // --- Tree Nodes ---

    /**
     * @brief Displays a collapsible tree node.
     * @details Used to create hierarchical UI elements. Widgets drawn after `UI_TreeNode`
     *          will appear as children until `UI_TreePop` is called.
     * @param label The text label for the tree node.
     * @return True if the tree node is open (expanded), false otherwise.
     */
    bool (*UI_TreeNode)(const char* label);

    /**
     * @brief Pushes a string ID onto the ID stack, intended for use with custom tree nodes.
     * @details This function should be used when manually managing the ID stack for tree nodes
     *          or custom collapsible headers.
     * @param str_id The string identifier to push.
     */
    void (*UI_TreePush)(const char* str_id);

    /**
     * @brief Pops an ID from the ID stack, matching a previous `UI_TreePush`.
     */
    void (*UI_TreePop)();

    // --- Tabs ---

    /**
     * @brief Begins a tab bar.
     * @details This function creates a horizontal bar that can contain multiple tab items.
     *          It must be matched with an `UI_EndTabBar`.
     * @param str_id A unique string identifier for the tab bar.
     * @return True if the tab bar is visible, false otherwise.
     */
    bool (*UI_BeginTabBar)(const char* str_id);

    /**
     * @brief Ends a tab bar.
     * @details Must be called after `UI_BeginTabBar`.
     */
    void (*UI_EndTabBar)();

    /**
     * @brief Begins a tab item within a tab bar.
     * @details This function creates a clickable tab within a tab bar.
     *          It must be matched with an `UI_EndTabItem`.
     * @param label The text label for the tab item.
     * @return True if the tab item is currently selected and its content is visible, false otherwise.
     */
    bool (*UI_BeginTabItem)(const char* label);

    /**
     * @brief Ends a tab item.
     * @details Must be called after `UI_BeginTabItem`.
     */
    void (*UI_EndTabItem)();

    // --- Tables ---

    /**
     * @brief Begins a table.
     * @details This function creates a table layout. It must be matched with an `UI_EndTable`.
     * @param str_id A unique string identifier for the table.
     * @param column The number of columns in the table.
     * @return True if the table is visible, false otherwise.
     */
    bool (*UI_BeginTable)(const char* str_id, int column);

    /**
     * @brief Ends a table.
     * @details Must be called after `UI_BeginTable`.
     */
    void (*UI_EndTable)();

    /**
     * @brief Advances to the next row in a table.
     * @details Call this function to start a new row after drawing all columns for the current row.
     */
    void (*UI_TableNextRow)();

    /**
     * @brief Advances to the next column in a table.
     * @details Call this function to move to the next column within the current row.
     * @return True if there is a next column to move to, false if it's the last column.
     */
    bool (*UI_TableNextColumn)();

    /**
     * @brief Sets up a column in a table.
     * @details This function defines properties for a table column, such as its label.
     *          It should be called after `UI_BeginTable` and before any calls to `UI_TableNextRow` or `UI_TableNextColumn`.
     * @param label The label for the column header.
     */
    void (*UI_TableSetupColumn)(const char* label);

    // --- Popups & Tooltips ---

    /**
     * @brief Opens a popup.
     * @details Popups are modal windows that appear on top of other content.
     *          They are usually triggered by an item being hovered or clicked.
     * @param str_id A unique string identifier for the popup.
     */
    void (*UI_OpenPopup)(const char* str_id);

    /**
     * @brief Begins a popup.
     * @details This function must be matched with an `UI_EndPopup`. The content of the popup
     *          will be drawn between these calls.
     * @param str_id The string identifier of the popup to begin.
     * @return True if the popup is open and its content is visible, false otherwise.
     */
    bool (*UI_BeginPopup)(const char* str_id);

    /**
     * @brief Ends a popup.
     * @details Must be called after `UI_BeginPopup`.
     */
    void (*UI_EndPopup)();

    /**
     * @brief Checks if the last item is hovered by the mouse.
     * @return True if the last item is hovered, false otherwise.
     */
    bool (*UI_IsItemHovered)();

    /**
     * @brief Checks if the last item is active (e.g., being clicked or dragged).
     * @return True if the last item is active, false otherwise.
     */
    bool (*UI_IsItemActive)();

    /**
     * @brief Sets a tooltip for the immediately preceding item.
     * @param text The text to display in the tooltip.
     * @param ... Optional arguments for printf-style formatting.
     */
    void (*UI_SetTooltip)(const char* text);

    // --- Advanced Inputs ---

    /**
     * @brief Displays a multi-line text input field.
     * @param label The text label for the input field.
     * @param buf A character buffer to store the input text.
     * @param buf_size The size of the character buffer.
     * @return True if the input text was modified this frame, false otherwise.
     */
    bool (*UI_InputTextMultiline)(const char* label, char* buf, size_t buf_size);

    /**
     * @brief Displays a 2-element float slider.
     * @param label The text label.
     * @param v A pointer to an array of 2 floats.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_SliderFloat2)(const char* label, float v[2], float v_min, float v_max);
    
    /**
     * @brief Displays a 3-element float slider.
     * @param label The text label.
     * @param v A pointer to an array of 3 floats.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_SliderFloat3)(const char* label, float v[3], float v_min, float v_max);
    
    /**
     * @brief Displays a 4-element float slider.
     * @param label The text label.
     * @param v A pointer to an array of 4 floats.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_SliderFloat4)(const char* label, float v[4], float v_min, float v_max);
    
    /**
     * @brief Displays a 2-element integer slider.
     * @param label The text label.
     * @param v A pointer to an array of 2 integers.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_SliderInt2)(const char* label, int v[2], int v_min, int v_max);
    
    /**
     * @brief Displays a 3-element integer slider.
     * @param label The text label.
     * @param v A pointer to an array of 3 integers.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_SliderInt3)(const char* label, int v[3], int v_min, int v_max);
    
    /**
     * @brief Displays a 4-element integer slider.
     * @param label The text label.
     * @param v A pointer to an array of 4 integers.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_SliderInt4)(const char* label, int v[4], int v_min, int v_max);
    
    /**
     * @brief Displays a 3-element color editor (RGB).
     * @param label The text label.
     * @param col A pointer to an array of 3 floats (RGB components).
     * @return True if the color was modified.
     */
    bool (*UI_ColorEdit3)(const char* label, float col[3]);
    
    /**
     * @brief Displays a 4-element color editor (RGBA).
     * @param label The text label.
     * @param col A pointer to an array of 4 floats (RGBA components).
     * @return True if the color was modified.
     */
    bool (*UI_ColorEdit4)(const char* label, float col[4]);
    
    /**
     * @brief Displays a float drag control.
     * @param label The text label.
     * @param v A pointer to a float variable.
     * @param v_speed The speed at which the value changes when dragged.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_DragFloat)(const char* label, float* v, float v_speed, float v_min, float v_max);
    
    /**
     * @brief Displays an integer drag control.
     * @param label The text label.
     * @param v A pointer to an integer variable.
     * @param v_speed The speed at which the value changes when dragged.
     * @param v_min The minimum value.
     * @param v_max The maximum value.
     * @return True if the value was modified.
     */
    bool (*UI_DragInt)(const char* label, int* v, float v_speed, int v_min, int v_max);

    /**
     * @brief Displays an integer slider.
     * @param label The text label for the slider.
     * @param v A pointer to the integer variable to be linked to the slider.
     * @param v_min The minimum value of the slider.
     * @param v_max The maximum value of the slider.
     * @param format The format string for displaying the integer (e.g., "%d units").
     * @return True if the slider's value was modified this frame, false otherwise.
     */
    bool (*UI_SliderInt)(const char* label, int* v, int v_min, int v_max, const char* format);

    /**
     * @brief Displays a floating-point number slider.
     * @param label The text label for the slider.
     * @param v A pointer to the float variable to be linked to the slider.
     * @param v_min The minimum value of the slider.
     * @param v_max The maximum value of the slider.
     * @param format The format string for displaying the float (e.g., "%.3f").
     * @return True if the slider's value was modified this frame, false otherwise.
     */
    bool (*UI_SliderFloat)(const char* label, float* v, float v_min, float v_max, const char* format);

    // --- Style ---

    /**
     * @brief Pushes a color onto the style stack.
     * @details Changes the color of subsequent widgets until `UI_PopStyleColor` is called.
     *          Each `UI_PushStyleColor` must be matched with a `UI_PopStyleColor`.
     * @param im_gui_color_idx The index of the color variable to change (e.g., `ImGuiCol_Text`).
     * @param r, g, b, a The new color components (0.0f to 1.0f).
     */
    void (*UI_PushStyleColor)(int im_gui_color_idx, float r, float g, float b, float a);

    /**
     * @brief Pops `count` colors from the style stack.
     * @details Restores the previous colors.
     * @param count The number of colors to pop.
     */
    void (*UI_PopStyleColor)(int count);

    /**
     * @brief Pushes a float style variable onto the style stack.
     * @details Changes a float-type style variable until `PopStyleVar` is called.
     *          Each `UI_PushStyleVarFloat` must be matched with a `PopStyleVar`.
     * @param im_gui_stylevar_idx The index of the style variable to change (e.g., `ImGuiStyleVar_Alpha`).
     * @param val The new float value.
     */
    void (*UI_PushStyleVarFloat)(int im_gui_stylevar_idx, float val);

    /**
     * @brief Pushes a 2-element vector style variable onto the style stack.
     * @details Changes an ImVec2-type style variable until `UI_PopStyleVar` is called.
     *          Each `UI_PushStyleVarVec2` must be matched with a `UI_PopStyleVar`.
     * @param im_gui_stylevar_idx The index of the style variable to change (e.g., `ImGuiStyleVar_WindowPadding`).
     * @param val_x The X component of the new vector value.
     * @param val_y The Y component of the new vector value.
     */
    void (*UI_PushStyleVarVec2)(int im_gui_stylevar_idx, float val_x, float val_y);

    /**
     * @brief Pops `count` style variables from the style stack.
     * @details Restores the previous style variables.
     * @param count The number of style variables to pop.
     */
    void (*UI_PopStyleVar)(int count);

    // --- Custom Drawing ---

    /**
     * @brief Gets the size of the main viewport (usually the game window).
     * @param[out] out_width Pointer to a float to store the viewport's width.
     * @param[out] out_height Pointer to a float to store the viewport's height.
     */
    void (*UI_GetViewportSize)(float* out_width, float* out_height);

    /**
     * @brief Adds a filled rectangle to the foreground draw list of the current window.
     * @details This is a convenient function for simple custom drawing. For more advanced
     *          drawing, use `UI_GetWindowDrawList` and `UI_DrawList_AddRectFilled`.
     * @param x1, y1 The top-left corner of the rectangle.
     * @param x2, y2 The bottom-right corner of the rectangle.
     * @param r, g, b, a The color components (0.0f to 1.0f).
     */
    void (*UI_AddRectFilled)(float x1, float y1, float x2, float y2, float r, float g, float b, float a);


    // --- Text Styling API (v1.0 - SPF-377) ---
    // The following set of functions allows for the creation and manipulation of text style objects.
    // These objects can then be passed to rendering functions like UI_`TextStyled` and `UI_RenderMarkdown`
    // to control typography, color, layout, and more.
    //
    // Workflow:
    // 1. Create a style object with `UI_Style_Create()`.
    // 2. Configure it using the `UI_Style_Set...()` functions.
    // 3. Pass the handle to a rendering function like `UI_TextStyled()`.
    // 4. Destroy the style object with `UI_Style_Destroy()` when it's no longer needed to release memory.

    /**
     * @brief Creates a new, empty text style handle.
     * @details This handle represents a collection of style properties. It must be destroyed
     *          with `Style_Destroy` to prevent memory leaks.
     * @return A new `SPF_TextStyle_Handle`.
     */
    SPF_TextStyle_Handle (*UI_Style_Create)();

    /**
     * @brief Destroys a text style handle and releases its memory.
     * @param handle The style handle to destroy.
     */
    void (*UI_Style_Destroy)(SPF_TextStyle_Handle handle);

    /**
     * @brief Sets the font for the text style.
     * @param handle The style handle to modify.
     * @param font The desired font from the `SPF_Font` enum.
     */
    void (*UI_Style_SetFont)(SPF_TextStyle_Handle handle, SPF_Font font);

    /**
     * @brief Sets the color of the text.
     * @param handle The style handle to modify.
     * @param r, g, b, a The color components (0.0f to 1.0f).
     */
    void (*UI_Style_SetColor)(SPF_TextStyle_Handle handle, float r, float g, float b, float a);

    /**
     * @brief Sets the horizontal alignment of the text.
     * @details Centering and right-alignment are relative to the available content region width.
     * @param handle The style handle to modify.
     * @param align The desired alignment from the `SPF_TextAlign` enum.
     */
    void (*UI_Style_SetAlign)(SPF_TextStyle_Handle handle, SPF_TextAlign align);

    /**
     * @brief Enables or disables automatic text wrapping.
     * @param handle The style handle to modify.
     * @param wrap Set to true to enable wrapping, false to disable.
     */
    void (*UI_Style_SetWrap)(SPF_TextStyle_Handle handle, bool wrap);

    /**
     * @brief Sets padding around the text block.
     * @param handle The style handle to modify.
     * @param pad_x Horizontal padding.
     * @param pad_y Vertical padding.
     */
    void (*UI_Style_SetPadding)(SPF_TextStyle_Handle handle, float pad_x, float pad_y);

    /**
     * @brief Turns the text into a separator with a label.
     * @details When true, the text will be rendered as a horizontal line with the text embedded in it.
     * @param handle The style handle to modify.
     * @param is_separator Set to true to render as a separator.
     */
    void (*UI_Style_SetSeparator)(SPF_TextStyle_Handle handle, bool is_separator);

    /**
     * @brief Enables or disables an underline decoration.
     * @param handle The style handle to modify.
     * @param is_underline Set to true to draw an underline.
     */
    void (*UI_Style_SetUnderline)(SPF_TextStyle_Handle handle, bool is_underline);

    /**
     * @brief Enables or disables a strikethrough decoration.
     * @param handle The style handle to modify.
     * @param is_strikethrough Set to true to draw a strikethrough line.
     */
    void (*UI_Style_SetStrikethrough)(SPF_TextStyle_Handle handle, bool is_strikethrough);

    // --- Styled Rendering (v1.0 - SPF-377) ---

    /**
     * @brief Renders text with a specific style, supporting printf-style formatting.
     * @details This function is the styled equivalent of the basic 'Text' function. It allows
     *          for applying a complex style object (created via Style_Create) to a piece of
     *          text. The format string 'fmt' and subsequent arguments work exactly like the
     *          standard C printf function.
     * @param handle A handle to a style object created with `UI_Style_Create`. If NULL, default
     *               styling will be used.
     * @param fmt A printf-style format string.
     * @param ... Optional subsequent arguments for the format string.
     */
    void (*UI_TextStyled)(SPF_TextStyle_Handle handle, const char* fmt, ...);

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
    void (*UI_RenderMarkdown)(const char* markdown_text, SPF_TextStyle_Handle base_style_handle);


    // --- Custom Widget API ---
    // The following functions provide low-level access to the drawing and interaction primitives
    // needed to create fully custom widgets beyond the standard set.
    //
    // Workflow for a custom widget:
    // 1. Create a canvas for your widget, typically with `UI_InvisibleButton()`.
    // 2. Query mouse state relative to the widget using functions like `UI_IsItemHovered()`, `UI_GetMousePos()`, etc.
    // 3. Get the window's draw list using `UI_GetWindowDrawList()`.
    // 4. Use the `UI_DrawList_...()` functions to draw your custom shapes, text, and visuals.

    /**
     * @brief Converts an RGBA color from four floats (0.0-1.0) to a packed 32-bit integer color.
     * @details This is the format required by all `UI_DrawList` functions.
     * @return A 32-bit unsigned integer representing the color (e.g., 0xAABBGGRR).
     */
    uint32_t (*UI_ColorConvertFloat4ToU32)(float r, float g, float b, float a);

    /**
     * @brief Gets a handle to the draw list for the current window.
     * @details The draw list is the primary tool for custom drawing. It contains all the commands
     *          to draw shapes, text, and images. This handle is valid for the current frame only.
     * @return A handle to the draw list.
     */
    SPF_DrawList_Handle (*UI_GetWindowDrawList)();

    // --- DrawList Drawing Functions ---

    /**
     * @brief Adds a line to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y The starting point of the line.
     * @param p2_x, p2_y The ending point of the line.
     * @param col The color of the line as a packed 32-bit integer.
     * @param thickness The thickness of the line in pixels.
     */
    void (*UI_DrawList_AddLine)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness);

    /**
     * @brief Adds a filled rectangle to the draw list.
     * @param dl The draw list handle.
     * @param p_min_x, p_min_y The top-left corner of the rectangle.
     * @param p_max_x, p_max_y The bottom-right corner of the rectangle.
     * @param col The fill color as a packed 32-bit integer.
     * @param rounding The radius of the corners. 0 for a sharp rectangle.
     */
    void (*UI_DrawList_AddRectFilled)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding);

    /**
     * @brief Adds a filled circle to the draw list.
     * @param dl The draw list handle.
     * @param center_x, center_y The center of the circle.
     * @param radius The radius of the circle.
     * @param col The fill color as a packed 32-bit integer.
     * @param num_segments The number of segments to use to approximate the circle. More segments = smoother circle.
     */
    void (*UI_DrawList_AddCircleFilled)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments);

    /**
     * @brief Adds text to the draw list at a specific screen position.
     * @details Unlike `UI_Text()`, this is a low-level draw command and does not interact with layout.
     * @param dl The draw list handle.
     * @param pos_x, pos_y The top-left screen coordinate to start drawing the text.
     * @param col The color of the text as a packed 32-bit integer.
     * @param text The text to draw.
     */
    void (*UI_DrawList_AddText)(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text);

    /**
     * @brief Adds a rectangle (outline) to the draw list.
     * @param dl The draw list handle.
     * @param p_min_x, p_min_y The top-left corner of the rectangle.
     * @param p_max_x, p_max_y The bottom-right corner of the rectangle.
     * @param col The color of the outline.
     * @param rounding The radius of the corners. 0 for a sharp rectangle.
     * @param thickness The thickness of the outline.
     */
    void (*UI_DrawList_AddRect)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, float thickness);
    
    /**
     * @brief Adds a filled quadrilateral to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y, p2_x, p2_y, p3_x, p3_y, p4_x, p4_y The four corner points of the quad.
     * @param col The fill color.
     */
    void (*UI_DrawList_AddQuadFilled)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col);

    /**
     * @brief Adds a filled triangle to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y, p2_x, p2_y, p3_x, p3_y The three corner points of the triangle.
     * @param col The fill color.
     */
    void (*UI_DrawList_AddTriangleFilled)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col);
    
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
    void (*UI_DrawList_AddBezierCubic)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col, float thickness, int num_segments);

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
    void (*UI_DrawList_AddPolyline)(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col, bool closed, float thickness);

    /**
     * @brief Clears the current path in the draw list. A path is a sequence of points that can be stroked or filled.
     */
    void (*UI_DrawList_PathClear)(SPF_DrawList_Handle dl);

    /**
     * @brief Adds a line from the current path position to a new position.
     * @param dl The draw list handle.
     * @param pos_x, pos_y The new position to draw a line to.
     */
    void (*UI_DrawList_PathLineTo)(SPF_DrawList_Handle dl, float pos_x, float pos_y);

    /**
     * @brief Draws an outline of the current path.
     * @param dl The draw list handle.
     * @param col The color of the outline.
     * @param closed If true, a line will be drawn from the last point to the first before stroking.
     * @param thickness The thickness of the outline.
     */
    void (*UI_DrawList_PathStroke)(SPF_DrawList_Handle dl, uint32_t col, bool closed, float thickness);

    /**
     * @brief Fills the interior of the current path (if it's a convex polygon).
     * @param dl The draw list handle.
     * @param col The fill color.
     */
    void (*UI_DrawList_PathFillConvex)(SPF_DrawList_Handle dl, uint32_t col);
    

    // --- Advanced Interaction API (v1.1 - SPF-412) ---

    /**
     * @brief Gets the current position of the mouse cursor in screen coordinates.
     * @param[out] out_x Pointer to a float to store the x-coordinate.
     * @param[out] out_y Pointer to a float to store the y-coordinate.
     */
    void (*UI_GetMousePos)(float* out_x, float* out_y);

    /**
     * @brief Checks if the user is currently dragging the mouse with a specific button held down.
     * @param mouse_button_index The index of the mouse button (0=Left, 1=Right, 2=Middle).
     * @return True if the user is dragging with the specified button, false otherwise.
     */
    bool (*UI_IsMouseDragging)(int mouse_button_index);

    /**
     * @brief Gets the total displacement of the mouse since a drag operation started.
     * @param mouse_button_index The index of the mouse button being dragged.
     * @param[out] out_dx Pointer to a float to store the horizontal displacement.
     * @param[out] out_dy Pointer to a float to store the vertical displacement.
     */
    void (*UI_GetMouseDragDelta)(int mouse_button_index, float* out_dx, float* out_dy);

    /**
     * @brief Checks if a mouse button is currently held down.
     * @param mouse_button_index The index of the mouse button (0=Left, 1=Right, 2=Middle).
     * @return True if the button is held down.
     */
    bool (*UI_IsMouseDown)(int mouse_button_index);

    /**
     * @brief Checks if a mouse button was clicked (pressed and released) in the current frame.
     * @param mouse_button_index The index of the mouse button.
     * @return True if the button was clicked this frame.
     */
    bool (*UI_IsMouseClicked)(int mouse_button_index);

    /**
     * @brief Checks if a mouse button was released in the current frame.
     * @param mouse_button_index The index of the mouse button.
     * @return True if the button was released this frame.
     */
    bool (*UI_IsMouseReleased)(int mouse_button_index);

    /**
     * @brief Checks if a mouse button was double-clicked.
     * @param mouse_button_index The index of the mouse button.
     * @return True if the button was double-clicked.
     */
    bool (*UI_IsMouseDoubleClicked)(int mouse_button_index);

    /**
     * @brief Gets the vertical scroll amount of the mouse wheel for the current frame.
     * @return A positive value for scrolling up, a negative value for scrolling down, and 0 if no scroll.
     */
    float (*UI_GetMouseWheel)();

    
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
    void (*UI_GetContentRegionAvail)(float* out_x, float* out_y);

    /**
     * @brief Gets the position of the current window.
     * @param[out] out_x Pointer to a float to store the window's x-coordinate.
     * @param[out] out_y Pointer to a float to store the window's y-coordinate.
     */
    void (*UI_GetWindowPos)(float* out_x, float* out_y);

    /**
     * @brief Gets the size of the current window's content region.
     * @param[out] out_x Pointer to a float to store the window's width.
     * @param[out] out_y Pointer to a float to store the window's height.
     */
    void (*UI_GetWindowSize)(float* out_x, float* out_y);

    /**
     * @brief Gets the screen-space position of the layout cursor.
     * @details This is where the next widget will be drawn.
     * @param[out] out_x Pointer to a float to store the cursor's x-coordinate.
     * @param[out] out_y Pointer to a float to store the cursor's y-coordinate.
     */
    void (*UI_GetCursorScreenPos)(float* out_x, float* out_y);

    /**
     * @brief Sets the screen-space position of the layout cursor.
     * @param x The new x-coordinate for the cursor.
     * @param y The new y-coordinate for the cursor.
     */
    void (*UI_SetCursorScreenPos)(float x, float y);

    /**
     * @brief Gets the bounding box of the last drawn item.
     * @param[out] min_x, min_y Pointer to store the top-left corner of the item.
     * @param[out] max_x, max_y Pointer to store the bottom-right corner of the item.
     */
    void (*UI_GetItemRectMin)(float* out_x, float* out_y);
    void (*UI_GetItemRectMax)(float* out_x, float* out_y);

    /**
     * @brief Gets the size of the last drawn item.
     * @param[out] out_x Pointer to a float to store the item's width.
     * @param[out] out_y Pointer to a float to store the item's height.
     */
    void (*UI_GetItemRectSize)(float* out_x, float* out_y);


    // --- Miscellaneous Utilities API ---

    /**
     * @brief Retrieves the content of the system clipboard.
     * @return A read-only pointer to a string containing the clipboard text.
     */
    const char* (*UI_GetClipboardText)();

    /**
     * @brief Sets the content of the system clipboard.
     * @param text The string to set as the clipboard content.
     */
    void (*UI_SetClipboardText)(const char* text);

    /**
     * @brief Gets a font handle by its key.
     * @details The available font keys are defined by the framework (e.g., "regular", "bold", "h1").
     *          This handle can be used with `UI_PushFont`.
     * @param font_key The string identifier for the font.
     * @return An opaque handle to the font, or NULL if not found.
     */
    SPF_Font_Handle* (*UI_GetFont)(const char* font_key);

    /**
     * @brief Pushes a font onto the font stack, making it the active font for subsequent widgets.
     * @details Must be matched with a call to `UI_PopFont`.
     * @param font_handle The font handle obtained from `UI_GetFont`.
     */
    void (*UI_PushFont)(SPF_Font_Handle* font_handle);

    /**
     * @brief Pops the current font from the font stack, restoring the previous font.
     */
    void (*UI_PopFont)();

    /**
     * @brief Gets a handle to the global ImGui style object.
     * @details This handle can be used to query various style properties. The returned
     *          handle points to the live style object, so its properties reflect the
     *          current ImGui style settings.
     * @return A handle to the global style object.
     */
    SPF_Style_Handle* (*UI_GetStyle)();

    /**
     * @brief Gets the WindowPadding from the global style.
     * @param style_handle The style handle obtained from `UI_GetStyle`.
     * @param[out] out_x Pointer to a float to store the X component of window padding.
     * @param[out] out_y Pointer to a float to store the Y component of window padding.
     */
    void (*UI_Style_GetWindowPadding)(SPF_Style_Handle* style_handle, float* out_x, float* out_y);

    /**
     * @brief Gets the ItemSpacing from the global style.
     * @param style_handle The style handle obtained from `UI_GetStyle`.
     * @param[out] out_x Pointer to a float to store the X component of item spacing.
     * @param[out] out_y Pointer to a float to store the Y component of item spacing.
     */
    void (*UI_Style_GetItemSpacing)(SPF_Style_Handle* style_handle, float* out_x, float* out_y);
    
    /**
     * @brief Gets the FramePadding from the global style.
     * @param style_handle The style handle obtained from `UI_GetStyle`.
     * @param[out] out_x Pointer to a float to store the X component of frame padding.
     * @param[out] out_y Pointer to a float to store the Y component of frame padding.
     */
    void (*UI_Style_GetFramePadding)(SPF_Style_Handle* style_handle, float* out_x, float* out_y);

    /**
     * @brief Pushes a string identifier onto the ID stack.
     * @details Use this to create unique IDs for widgets in loops or complex components.
     *          Each `UI_PushID` must be matched with a `UI_PopID`.
     * @param str_id A string to be used as an ID.
     */
    void (*UI_PushID_Str)(const char* str_id);

    /**
     * @brief Pushes an integer identifier onto the ID stack.
     * @param int_id An integer to be used as an ID.
     */
    void (*UI_PushID_Int)(int int_id);
    
    /**
     * @brief Pushes a pointer identifier onto the ID stack.
     * @param ptr_id A pointer to be used as an ID.
     */
    void (*UI_PushID_Ptr)(void* ptr_id);

    /**
     * @brief Pops the last identifier from the ID stack.
     */
    void (*UI_PopID)();

    /**
     * @brief Calculates a unique ID from a string, without pushing it to the stack.
     * @param str_id The string to hash into an ID.
     * @return The calculated 32-bit ID.
     */
    uint32_t (*UI_GetID_Str)(const char* str_id);


    // --- Drag and Drop API ---

    /**
     * @brief Begins a drag and drop source.
     * @details Call this after an item to make it a draggable source.
     *          If it returns true, you must call `UI_SetDragDropPayload` and then `UI_EndDragDropSource`.
     * @return True if the user is dragging this item, false otherwise.
     */
    bool (*UI_BeginDragDropSource)();

    /**
     * @brief Sets the data payload for the current drag and drop operation.
     * @details This function is called within a `UI_BeginDragDropSource`/`UI_EndDragDropSource` block.
     * @param type A string identifier for the type of payload (e.g., "LIST_ITEM").
     * @param data A pointer to the data to be transferred.
     * @param size The size of the data in bytes.
     * @return True if the payload was set successfully.
     */
    bool (*UI_SetDragDropPayload)(const char* type, const void* data, size_t size);

    /**
     * @brief Ends a drag and drop source.
     * @details Must be called after `UI_BeginDragDropSource`.
     */
    void (*UI_EndDragDropSource)();

    /**
     * @brief Begins a drag and drop target.
     * @details Call this after an item to make it a drop target. If it returns true,
     *          you can call `UI_AcceptDragDropPayload` and must then call `UI_EndDragDropTarget`.
     * @return True if a draggable item is hovering over this target.
     */
    bool (*UI_BeginDragDropTarget)();

    /**
     * @brief Accepts a drag and drop payload.
     * @details This function is called within a `UI_BeginDragDropTarget`/`UI_EndDragDropTarget` block.
     *          It checks if the payload type matches and, if so, returns the payload.
     * @param type The string identifier for the type of payload to accept.
     * @return A handle to the payload if the type matches and the drop occurred, otherwise NULL.
     *         The handle is only valid for the current frame.
     */
    const SPF_Payload_Handle* (*UI_AcceptDragDropPayload)(const char* type);

    /**
     * @brief Ends a drag and drop target.
     * @details Must be called after `UI_BeginDragDropTarget`.
     */
    void (*UI_EndDragDropTarget)();

} SPF_UI_API;
