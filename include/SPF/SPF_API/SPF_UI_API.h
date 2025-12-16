#pragma once

#include <stdbool.h>
#include <stdint.h>

// Note: This is a C-style header for ABI stability.

// Forward-declare handle type
typedef struct SPF_Window_Handle SPF_Window_Handle;

/**
 * @enum SPF_Window_Flags
 * @brief Flags to control the behavior of a window registered by a plugin.
 */
typedef enum {
    SPF_WINDOW_FLAG_NONE      = 0,
    SPF_WINDOW_FLAG_NO_RESIZE = 1 << 0, // Disable user resizing.
    SPF_WINDOW_FLAG_NO_MOVE   = 1 << 1, // Disable user moving the window.
    SPF_WINDOW_FLAG_NO_TITLE  = 1 << 2, // Disable the title bar.
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

    void (*GetViewportSize)(float* out_width, float* out_height);
    void (*AddRectFilled)(float x1, float y1, float x2, float y2, float r, float g, float b, float a);

} SPF_UI_API;
