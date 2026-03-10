# SPF UI API Reference

The `SPF_UI_API` is the central interface for creating user interfaces within the SPF Framework. It wraps the powerful ImGui library, providing a stable, C-compatible ABI (Application Binary Interface) that plugins can use to render windows, widgets, and overlays.

This API operates on an **Immediate Mode** paradigm. This means:
1.  **No retained objects**: You do not create a "Button" object and store it.
2.  **Draw every frame**: You call `UI_Button(...)` inside your draw callback every single frame.
3.  **State is external**: The UI system manages the visual state (hovered, active), but the data (e.g., the boolean for a checkbox) is stored in your plugin's variables.

## Getting Started

To use the API, you must obtain the `SPF_UI_API*` pointer. This is typically done in the `OnRegisterUI` lifecycle event.

### Example Initialization (C++)
```cpp
#include "SPF/SPF_API/SPF_UI_API.h"

// 1. Define your draw callback
void MyPlugin_DrawMainWindow(SPF_UI_API* ui, void* user_data) {
    ui->UI_Text("Hello, World!");
    
    static bool show_demo = false;
    if (ui->UI_Button("Toggle Demo", 0, 0)) {
        show_demo = !show_demo;
    }
}

// 2. Register it during the framework's UI initialization phase
extern "C" SPF_PLUGIN_EXPORT void OnRegisterUI(SPF_UI_API* ui) {
    // "MainWindow" must match an ID in your manifest.json
    ui->UI_RegisterDrawCallback("MyPlugin", "MainWindow", MyPlugin_DrawMainWindow, nullptr);
}
```

---

## 0. Data Types & Enums

This section details the enumerations and structures used throughout the API. Understanding these flags is crucial for customizing the behavior of windows and widgets.

### SPF_WindowFlags
Flags for `UI_RegisterDrawCallbackWithFlags`, `UI_BeginChild`, `UI_BeginPopup`.

| Flag | Value | Description |
| :--- | :--- | :--- |
| `SPF_WINDOW_FLAG_NONE` | `0` | Default window. Has title bar, resizable, scrollable, collapsible. |
| `SPF_WINDOW_FLAG_NO_TITLE_BAR` | `1 << 0` | Hides the title bar. You won't be able to move the window unless you implement custom dragging. |
| `SPF_WINDOW_FLAG_NO_RESIZE` | `1 << 1` | Disables the ability to resize the window from the edges/corners. |
| `SPF_WINDOW_FLAG_NO_MOVE` | `1 << 2` | Disables the ability to move the window. |
| `SPF_WINDOW_FLAG_NO_SCROLLBAR` | `1 << 3` | Hides vertical and horizontal scrollbars, even if content overflows. |
| `SPF_WINDOW_FLAG_NO_SCROLL_WITH_MOUSE` | `1 << 4` | Disables scrolling via the mouse wheel. |
| `SPF_WINDOW_FLAG_NO_COLLAPSE` | `1 << 5` | Hides the collapse button (small triangle) in the title bar. |
| `SPF_WINDOW_FLAG_ALWAYS_AUTO_RESIZE` | `1 << 6` | The window will automatically resize every frame to fit its content exactly. |
| `SPF_WINDOW_FLAG_NO_BACKGROUND` | `1 << 7` | The window background is transparent. Only widgets are visible. |
| `SPF_WINDOW_FLAG_NO_SAVED_SETTINGS` | `1 << 8` | The window's position, size, and collapse state will NOT be saved to `imgui.ini`. |
| `SPF_WINDOW_FLAG_NO_MOUSE_INPUTS` | `1 << 9` | The window ignores all mouse interaction. Clicks pass through to the window/game behind it. |
| `SPF_WINDOW_FLAG_MENU_BAR` | `1 << 10` | Reserves space at the top of the window for a menu bar (use `UI_BeginMenuBar`). |
| `SPF_WINDOW_FLAG_HORIZONTAL_SCROLLBAR` | `1 << 11` | Enables the horizontal scrollbar. |
| `SPF_WINDOW_FLAG_NO_FOCUS_ON_APPEARING` | `1 << 12` | The window won't steal focus when it first appears. |
| `SPF_WINDOW_FLAG_NO_BRING_TO_FRONT_ON_FOCUS`| `1 << 13` | Clicking the window won't bring it to the front of the display stack. |
| `SPF_WINDOW_FLAG_ALWAYS_VERTICAL_SCROLLBAR` | `1 << 14` | The vertical scrollbar is always visible, even if content fits. |
| `SPF_WINDOW_FLAG_ALWAYS_HORIZONTAL_SCROLLBAR`| `1 << 15` | The horizontal scrollbar is always visible. |
| `SPF_WINDOW_FLAG_NO_NAV_INPUTS` | `1 << 16` | No gamepad/keyboard navigation within the window. |
| `SPF_WINDOW_FLAG_NO_NAV_FOCUS` | `1 << 17` | No focus transfer via navigation (e.g. Ctrl+Tab). |
| `SPF_WINDOW_FLAG_UNSAVED_DOCUMENT` | `1 << 18` | Appends a `(*)` to the title, indicating unsaved changes. |
| `SPF_WINDOW_FLAG_NO_DOCKING` | `1 << 19` | Prevents the window from being docked into other windows/nodes. |

### SPF_MultiSelectFlags
Flags for `UI_BeginMultiSelect`. Controls how multiple items can be selected, ranged, and cleared.

| Flag | Description |
| :--- | :--- |
| `SPF_MULTI_SELECT_FLAG_NONE` | Default behavior. |
| `SPF_MULTI_SELECT_FLAG_SINGLE_SELECT` | Only one item can be selected at a time. |
| `SPF_MULTI_SELECT_FLAG_NO_SELECT_ALL` | Disable 'Select All' shortcut (Ctrl+A). |
| `SPF_MULTI_SELECT_FLAG_NO_RANGE_SELECT` | Disable range selection (Shift+Click). |
| `SPF_MULTI_SELECT_FLAG_NO_AUTO_SELECT` | Disable automatic selection on navigation or click. |
| `SPF_MULTI_SELECT_FLAG_NO_AUTO_CLEAR` | Disable automatic clearing of selection. |
| `SPF_MULTI_SELECT_FLAG_NO_AUTO_CLEAR_ON_CLICK_OUTSIDE` | Do not clear selection when clicking on empty space. |
| `SPF_MULTI_SELECT_FLAG_NAV_WRAPPING` | Enable keyboard navigation wrapping within the selection scope. |
| `SPF_MULTI_SELECT_FLAG_LOOP` | Selection loops around when reaching boundaries. |
| `SPF_MULTI_SELECT_FLAG_BOX_SELECT_1D` | Enable 1D box-selection (marquee). |
| `SPF_MULTI_SELECT_FLAG_BOX_SELECT_2D` | Enable 2D box-selection (marquee). |
| `SPF_MULTI_SELECT_FLAG_BOX_SELECT_NO_SCROLL` | Disable scrolling during box-selection. |
| `SPF_MULTI_SELECT_FLAG_CLEAR_ON_ESCAPE` | Clear selection when the Escape key is pressed. |
| `SPF_MULTI_SELECT_FLAG_CLEAR_ON_CLICK_VOID` | Clear selection when clicking on the window background. |
| `SPF_MULTI_SELECT_FLAG_SCOPE_WINDOW` | The selection scope is limited to the current window. |
| `SPF_MULTI_SELECT_FLAG_SCOPE_RECT` | The selection scope is defined by a specific rectangle. |
| `SPF_MULTI_SELECT_FLAG_SELECT_ON_CLICK` | Select items immediately on mouse down. |
| `SPF_MULTI_SELECT_FLAG_SELECT_ON_DEFAULT_PURPOSE` | Use default interaction rules for selection. |

### SPF_InputFlags
Flags for `UI_Shortcut` and `UI_SetNextItemShortcut`. Defines how input is routed.

| Flag | Description |
| :--- | :--- |
| `SPF_INPUT_FLAG_NONE` | Default behavior. |
| `SPF_INPUT_FLAG_REPEAT` | Enable key repeat for the shortcut. |
| `SPF_INPUT_FLAG_ROUTE_ACTIVE` | Route to active item only. |
| `SPF_INPUT_FLAG_ROUTE_FOCUSED` | Route to the focused window stack (Default). |
| `SPF_INPUT_FLAG_ROUTE_GLOBAL` | Global shortcut, processed regardless of focus. |
| `SPF_INPUT_FLAG_ROUTE_ALWAYS` | Always process, do not participate in routing logic. |
| `SPF_INPUT_FLAG_ROUTE_OVER_FOCUSED` | Overrides focused route even if not in the focus stack. |
| `SPF_INPUT_FLAG_ROUTE_OVER_ACTIVE` | Overrides active item route. |
| `SPF_INPUT_FLAG_ROUTE_UNLESS_BG_FOCUSED` | Process only if no ImGui window is focused. |
| `SPF_INPUT_FLAG_ROUTE_FROM_ROOT_WINDOW` | Evaluate route from the root window's perspective. |
| `SPF_INPUT_FLAG_TOOLTIP` | Automatically display a tooltip for the shortcut. |

### SPF_SelectionRequestType
Defines the type of selection modification being requested by the system.

| Enum | Description |
| :--- | :--- |
| `SPF_SELECTION_REQUEST_NONE` | No request. |
| `SPF_SELECTION_REQUEST_SET_ALL` | Request to set the selection state for all items in the scope. |
| `SPF_SELECTION_REQUEST_SET_RANGE` | Request to set the selection state for a specific range of items. |

### SPF_Storage_Handle
`SPF_Storage_Handle` is an opaque pointer to the internal ImGuiStorage system.
It allows for direct manipulation of persisted UI states.

### SPF_PlotGetter
`SPF_PlotGetter` is a callback function pointer for dynamic data retrieval in plot widgets.

```c
typedef float (*SPF_PlotGetter)(void* data, int idx);
```

### SPF_SelectionRequest
Represents a single request to update the selection state from the Multi-Select API.

| Field | Type | Description |
| :--- | :--- | :--- |
| `type` | `SPF_SelectionRequestType` | The type of modification (SetAll or SetRange). |
| `selected` | `bool` | The target selection state (true = selected, false = unselected). |
| `range_direction` | `int64_t` | Direction of the range selection (-1 or +1). |
| `first_item_user_data` | `int64_t` | User data ID of the first item in the range. |
| `last_item_user_data` | `int64_t` | User data ID of the last item in the range. |

### SPF_MultiSelectIO
Data exchange structure for the Multi-Select API.

| Field | Type | Description |
| :--- | :--- | :--- |
| `requests` | `SPF_SelectionRequest*` | Array of selection requests to be executed. |
| `requests_count` | `int` | Number of valid requests in the array. |
| `range_src_item_user_data` | `int64_t` | User data ID of the item where range selection started. |
| `range_dst_item_user_data` | `int64_t` | User data ID of the item where range selection ended. |

### SPF_InputTextFlags
Flags for `UI_InputText`, `UI_InputTextMultiline`.

| Flag | Value | Description |
| :--- | :--- | :--- |
| `SPF_INPUT_TEXT_FLAG_NONE` | `0` | Default behavior. |
| `SPF_INPUT_TEXT_FLAG_CHARS_DECIMAL` | `1 << 0` | Allow `0123456789.+-` |
| `SPF_INPUT_TEXT_FLAG_CHARS_HEXADECIMAL` | `1 << 1` | Allow `0123456789ABCDEFabcdef` |
| `SPF_INPUT_TEXT_FLAG_CHARS_UPPERCASE` | `1 << 2` | Turn a..z into A..Z. |
| `SPF_INPUT_TEXT_FLAG_CHARS_NO_BLANK` | `1 << 3` | Filter out spaces and tabs. |
| `SPF_INPUT_TEXT_FLAG_AUTO_SELECT_ALL` | `1 << 4` | Select entire text when focused. |
| `SPF_INPUT_TEXT_FLAG_ENTER_RETURNS_TRUE` | `1 << 5` | Return 'true' when Enter is pressed (as opposed to every change). |
| `SPF_INPUT_TEXT_FLAG_CALLBACK_COMPLETION` | `1 << 6` | Callback on Tab key. |
| `SPF_INPUT_TEXT_FLAG_CALLBACK_HISTORY` | `1 << 7` | Callback on Up/Down arrows. |
| `SPF_INPUT_TEXT_FLAG_CALLBACK_ALWAYS` | `1 << 8` | Callback on every frame (user data updated). |
| `SPF_INPUT_TEXT_FLAG_CALLBACK_CHAR_FILTER` | `1 << 9` | Callback on character input (can replace/discard chars). |
| `SPF_INPUT_TEXT_FLAG_ALLOW_TAB_INPUT` | `1 << 10` | Pressing Tab enters a `\t` character instead of moving focus. |
| `SPF_INPUT_TEXT_FLAG_CTRL_ENTER_FOR_NEW_LINE`| `1 << 11` | In multiline mode, Enter adds a newline only if Ctrl is held. |
| `SPF_INPUT_TEXT_FLAG_NO_HORIZONTAL_SCROLL` | `1 << 12` | Disable horizontal scroll (wrapping mode). |
| `SPF_INPUT_TEXT_FLAG_ALWAYS_OVERWRITE` | `1 << 13` | Overwrite mode. |
| `SPF_INPUT_TEXT_FLAG_READ_ONLY` | `1 << 14` | Read-only mode. |
| `SPF_INPUT_TEXT_FLAG_PASSWORD` | `1 << 15` | Display `*` instead of characters. |
| `SPF_INPUT_TEXT_FLAG_NO_UNDO_REDO` | `1 << 16` | Disable undo/redo system. |
| `SPF_INPUT_TEXT_FLAG_CHARS_SCIENTIFIC` | `1 << 17` | Allow scientific notation (`e`, `E`). |

### SPF_ColorEditFlags
Flags for `UI_ColorEdit3`, `UI_ColorPicker4`, etc.

| Flag | Value | Description |
| :--- | :--- | :--- |
| `SPF_COLOR_EDIT_FLAG_NONE` | `0` | Default. |
| `SPF_COLOR_EDIT_FLAG_NO_ALPHA` | `1 << 1` | Ignore Alpha component (read 3 values instead of 4). |
| `SPF_COLOR_EDIT_FLAG_NO_PICKER` | `1 << 2` | Disable picker popup when clicking the colored square. |
| `SPF_COLOR_EDIT_FLAG_NO_OPTIONS` | `1 << 3` | Disable right-click menu options. |
| `SPF_COLOR_EDIT_FLAG_NO_SMALL_PREVIEW` | `1 << 4` | Hide the small preview square. |
| `SPF_COLOR_EDIT_FLAG_NO_INPUTS` | `1 << 5` | Hide the numeric inputs (sliders/text). |
| `SPF_COLOR_EDIT_FLAG_NO_TOOLTIP` | `1 << 6` | Disable tooltip on hover. |
| `SPF_COLOR_EDIT_FLAG_NO_LABEL` | `1 << 7` | Hide the text label. |
| `SPF_COLOR_EDIT_FLAG_ALPHA_BAR` | `1 << 16` | Show vertical alpha bar (in picker). |
| `SPF_COLOR_EDIT_FLAG_ALPHA_PREVIEW` | `1 << 17` | Show alpha checkerboard in preview. |
| `SPF_COLOR_EDIT_FLAG_DISPLAY_RGB` | `1 << 20` | Display inputs as RGB (0-255). |
| `SPF_COLOR_EDIT_FLAG_DISPLAY_HSV` | `1 << 21` | Display inputs as HSV. |
| `SPF_COLOR_EDIT_FLAG_DISPLAY_HEX` | `1 << 22` | Display inputs as Hex. |
| `SPF_COLOR_EDIT_FLAG_FLOAT` | `1 << 24` | Inputs are 0.0-1.0 floats instead of 0-255 ints. |
| `SPF_COLOR_EDIT_FLAG_INPUT_RGB` | `1 << 27` | Force RGB input mode. |
| `SPF_COLOR_EDIT_FLAG_INPUT_HSV` | `1 << 28` | Force HSV input mode. |

### SPF_StyleVar
Variable IDs for `UI_PushStyleVar`.

| Enum | Type | Description |
| :--- | :--- | :--- |
| `SPF_STYLE_VAR_ALPHA` | Float | Global transparency. |
| `SPF_STYLE_VAR_WINDOW_PADDING` | Vec2 | Padding within window borders. |
| `SPF_STYLE_VAR_WINDOW_ROUNDING` | Float | Radius of window corners. |
| `SPF_STYLE_VAR_WINDOW_BORDERSIZE` | Float | Thickness of window border. |
| `SPF_STYLE_VAR_WINDOW_MIN_SIZE` | Vec2 | Minimum window size. |
| `SPF_STYLE_VAR_WINDOW_TITLE_ALIGN` | Vec2 | Alignment of title text (0.0=left, 0.5=center). |
| `SPF_STYLE_VAR_CHILD_ROUNDING` | Float | Radius of child window corners. |
| `SPF_STYLE_VAR_CHILD_BORDERSIZE` | Float | Thickness of child window border. |
| `SPF_STYLE_VAR_POPUP_ROUNDING` | Float | Radius of popup corners. |
| `SPF_STYLE_VAR_POPUP_BORDERSIZE` | Float | Thickness of popup border. |
| `SPF_STYLE_VAR_FRAME_PADDING` | Vec2 | Padding within widget frames. |
| `SPF_STYLE_VAR_FRAME_ROUNDING` | Float | Radius of widget corners. |
| `SPF_STYLE_VAR_FRAME_BORDERSIZE` | Float | Thickness of widget border. |
| `SPF_STYLE_VAR_ITEM_SPACING` | Vec2 | Spacing between widgets. |
| `SPF_STYLE_VAR_ITEM_INNER_SPACING` | Vec2 | Spacing within a complex widget. |
| `SPF_STYLE_VAR_INDENT_SPACING` | Float | Indentation width. |
| `SPF_STYLE_VAR_CELL_PADDING` | Vec2 | Padding within table cells. |
| `SPF_STYLE_VAR_SCROLLBAR_SIZE` | Float | Width/Height of scrollbar. |
| `SPF_STYLE_VAR_SCROLLBAR_ROUNDING` | Float | Radius of scrollbar. |
| `SPF_STYLE_VAR_GRAB_MINSIZE` | Float | Minimum size of slider/scrollbar grab. |
| `SPF_STYLE_VAR_GRAB_ROUNDING` | Float | Radius of grab handle. |
| `SPF_STYLE_VAR_TAB_ROUNDING` | Float | Radius of tabs. |
| `SPF_STYLE_VAR_BUTTON_TEXT_ALIGN` | Vec2 | Alignment of button text. |
| `SPF_STYLE_VAR_SELECTABLE_TEXT_ALIGN` | Vec2 | Alignment of selectable text. |

### SPF_StyleColor
Color IDs for `UI_PushStyleColor`.

| Enum | Description |
| :--- | :--- |
| `SPF_COLOR_TEXT` | Standard text color. |
| `SPF_COLOR_TEXT_DISABLED` | Grayed out text. |
| `SPF_COLOR_WINDOW_BG` | Background of normal windows. |
| `SPF_COLOR_CHILD_BG` | Background of child windows. |
| `SPF_COLOR_POPUP_BG` | Background of popups/tooltips. |
| `SPF_COLOR_BORDER` | Window/widget border color. |
| `SPF_COLOR_BORDER_SHADOW` | Shadow behind the border. |
| `SPF_COLOR_FRAME_BG` | Background of checkbox, radio, inputs. |
| `SPF_COLOR_FRAME_BG_HOVERED` | Frame background when hovered. |
| `SPF_COLOR_FRAME_BG_ACTIVE` | Frame background when clicked. |
| `SPF_COLOR_TITLE_BG` | Title bar background. |
| `SPF_COLOR_TITLE_BG_ACTIVE` | Active title bar background. |
| `SPF_COLOR_TITLE_BG_COLLAPSED` | Collapsed title bar background. |
| `SPF_COLOR_MENU_BAR_BG` | Menu bar background. |
| `SPF_COLOR_SCROLLBAR_BG` | Scrollbar track background. |
| `SPF_COLOR_SCROLLBAR_GRAB` | Scrollbar handle. |
| `SPF_COLOR_SCROLLBAR_GRAB_HOVERED` | Scrollbar handle hovered. |
| `SPF_COLOR_SCROLLBAR_GRAB_ACTIVE` | Scrollbar handle clicked. |
| `SPF_COLOR_CHECK_MARK` | Checkmark color. |
| `SPF_COLOR_SLIDER_GRAB` | Slider handle. |
| `SPF_COLOR_SLIDER_GRAB_ACTIVE` | Slider handle active. |
| `SPF_COLOR_BUTTON` | Button background. |
| `SPF_COLOR_BUTTON_HOVERED` | Button hovered. |
| `SPF_COLOR_BUTTON_ACTIVE` | Button clicked. |
| `SPF_COLOR_HEADER` | Header (tree, selectable) background. |
| `SPF_COLOR_HEADER_HOVERED` | Header hovered. |
| `SPF_COLOR_HEADER_ACTIVE` | Header clicked. |
| `SPF_COLOR_SEPARATOR` | Separator line color. |
| `SPF_COLOR_RESIZE_GRIP` | Resize grip color. |
| `SPF_COLOR_TAB` | Tab background. |
| `SPF_COLOR_TAB_HOVERED` | Tab hovered. |
| `SPF_COLOR_TAB_ACTIVE` | Active tab background. |
| `SPF_COLOR_TAB_UNFOCUSED` | Tab in unfocused window. |
| `SPF_COLOR_TAB_UNFOCUSED_ACTIVE` | Active tab in unfocused window. |
| `SPF_COLOR_PLOT_LINES` | Line plot color. |
| `SPF_COLOR_PLOT_HISTOGRAM` | Histogram bar color. |
| `SPF_COLOR_TABLE_HEADER_BG` | Table header background. |
| `SPF_COLOR_TABLE_BORDER_STRONG` | Table outer border. |
| `SPF_COLOR_TABLE_BORDER_LIGHT` | Table inner border. |
| `SPF_COLOR_TABLE_ROW_BG` | Table row background (even). |
| `SPF_COLOR_TABLE_ROW_BG_ALT` | Table row background (odd). |
| `SPF_COLOR_TEXT_SELECTED_BG` | Selected text highlight. |
| `SPF_COLOR_DRAG_DROP_TARGET` | Drag & drop overlay. |
| `SPF_COLOR_NAV_HIGHLIGHT` | Gamepad/Keyboard highlight. |
| `SPF_COLOR_MODAL_WINDOW_DIM_BG` | Darkening behind modal windows. |

### SPF_ButtonFlags (enum)
| Flag | Description |
| :--- | :--- |
| `SPF_BUTTON_FLAG_NONE` | Default behavior. |
| `SPF_BUTTON_FLAG_MOUSE_BUTTON_LEFT` | React to left mouse button (Default). |
| `SPF_BUTTON_FLAG_MOUSE_BUTTON_RIGHT`| React to right mouse button. |
| `SPF_BUTTON_FLAG_MOUSE_BUTTON_MIDDLE`| React to middle mouse button. |
| `SPF_BUTTON_FLAG_DONT_CLOSE_POPUPS` | Clicking won't close active popups. |
| `SPF_BUTTON_FLAG_DISABLED` | Button is visually and logically disabled. |
| `SPF_BUTTON_FLAG_PRESSED_ON_CLICK` | Trigger on click (Default). |
| `SPF_BUTTON_FLAG_PRESSED_ON_RELEASE` | Trigger only when mouse is released. |

### SPF_DataType (enum)
Used by raw behaviors and internal utilities to identify numeric types.
*   `SPF_DATA_TYPE_S8`, `U8`, `S16`, `U16`, `S32`, `U32`, `S64`, `U64`, `FLOAT`, `DOUBLE`.

### SPF_TableSortSpecs (struct)
Provides sorting state for tables.

```c
typedef struct SPF_TableSortSpecs {
    SPF_TableColumnSortSpecs* Specs;      // Array of sorted columns
    int                       SpecsCount; // Number of sorted columns
    bool                      SpecsDirty; // Set when sorting changes
} SPF_TableSortSpecs;
```

### SPF_ListClipper (struct)
Helper for efficient large-list rendering.

```c
typedef struct SPF_ListClipper {
    int   DisplayStart; // First visible item
    int   DisplayEnd;   // Last visible item
    int   ItemsCount;   // Total item count
    float ItemsHeight;  // Height of one item
    void* TempData;     // Internal
} SPF_ListClipper;
```

---

## 1. Plugin Registration & Window Lifecycle

This section covers the functions used to integrate your plugin with the framework's UI manager. Unlike a standalone ImGui application where you manually call `Begin()` and `End()` for every window in the main loop, the SPF framework manages the window container for you. 

Your responsibility is to provide a function that draws the *interior* of the window. The framework handles the title bar, docking, resizing, and persistent visibility settings based on the plugin's manifest.

---
### UI_RegisterDrawCallback

**`void UI_RegisterDrawCallback(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data)`**

Registers a standard rendering function for a specific window declared in the plugin manifest.

*   **Parameters:**
    *   `pluginName`: The unique programmatic name of your plugin (e.g., `"NavigationSystem"`). This must match the name defined in your manifest.
    *   `windowId`: The ID of the window as defined in your manifest's `ui` section (e.g., `"MainWindow"`).
    *   `drawCallback`: A function pointer of type `SPF_DrawCallback`. This function is called every frame the window is visible.
    *   `user_data`: A custom pointer passed back to your callback every frame. Use this to pass state objects or context to your drawing logic.

*   **Logic:** Once registered, the framework will invoke the `drawCallback` whenever it needs to render that window ID. You should typically call this during the `OnRegisterUI` lifecycle event.

*   **Example:**
    ```c
    void MyWindow_Draw(SPF_UI_API* ui, void* user_data) {
        MyState* state = (MyState*)user_data;
        ui->UI_Text("Current Value: %d", state->counter);
    }

    // Inside OnRegisterUI:
    ui->UI_RegisterDrawCallback("MyPlugin", "MainWindow", MyWindow_Draw, &g_State);
    ```

---
### UI_RegisterDrawCallbackWithFlags

**`void UI_RegisterDrawCallbackWithFlags(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_WindowFlags flags)`**

An advanced version of the registration function that allows you to force specific `SPF_WindowFlags` for the window's container.

*   **Parameters:**
    *   `flags`: A bitmask of `SPF_WindowFlags`. These flags override the default framework behavior for this specific window.

*   **Usage:** Use this when you need a specialized window, such as an overlay without a title bar or a window that should not be resizable by the user.

*   **Example:**
    ```c
    // Register a persistent HUD overlay that cannot be moved or resized
    ui->UI_RegisterDrawCallbackWithFlags("MyPlugin", "HUD", DrawHUD, NULL, 
        SPF_WINDOW_FLAG_NO_TITLE_BAR | SPF_WINDOW_FLAG_NO_MOVE | SPF_WINDOW_FLAG_NO_RESIZE);
    ```

---
### UI_GetWindowHandle

**`SPF_Window_Handle* UI_GetWindowHandle(const char* pluginName, const char* windowId)`**

Retrieves an opaque handle to a registered window. This handle is required for programmatic control functions like toggling visibility or forcing focus.

*   **Returns:** A valid `SPF_Window_Handle*` if the window is found, or `NULL` if the window is not registered or the IDs are incorrect.

*   **Example:**
    ```c
    SPF_Window_Handle* hMain = ui->UI_GetWindowHandle("MyPlugin", "MainWindow");
    if (hMain) {
        // Handle is valid for use in UI_SetVisibility, etc.
    }
    ```

---
### UI_SetVisibility

**`void UI_SetVisibility(SPF_Window_Handle* handle, bool isVisible)`**

Programmatically shows or hides a framework-managed window.

*   **Parameters:**
    *   `handle`: The window handle obtained via `UI_GetWindowHandle`.
    *   `isVisible`: `true` to show the window, `false` to hide it.

*   **Logic:** This function updates the framework's internal visibility state. If a window is hidden, its draw callback will immediately stop being invoked. This is equivalent to the user clicking the "X" button or toggling the window in the framework's window manager.

---
### UI_IsVisible

**`bool UI_IsVisible(SPF_Window_Handle* handle)`**

Queries the current visibility state of a window.

*   **Returns:** `true` if the window is currently visible to the user.

---
### UI_SetFocus

**`void UI_SetFocus(SPF_Window_Handle* handle)`**

Forces the UI focus onto a specific framework-managed window.

*   **Logic:** This will bring the window to the front of the display stack (if not docked) and make it the active window for keyboard/gamepad navigation. If the window is currently hidden, this call has no effect.

---

---

## 2. Main Loop, Context & IO

This section provides access to the global UI state, timing data, and raw input from the mouse and keyboard. These functions are essential for creating dynamic, responsive interfaces that react to user input or elapsed time.

### Context & Memory Management

The UI system maintains a global state (context) that tracks window positions, focus, and widget states.

---
**`void* UI_GetCurrentContext()`**
Retrieves the raw pointer to the internal ImGui context.
*   **Usage:** Only required if you are integrating low-level C++ code or external libraries that need direct access to the ImGui state.
*   **Returns:** A pointer to the active `ImGuiContext`.

---
**`void UI_SetCurrentContext(void* ctx)`**
Sets the active ImGui context. The framework handles this automatically before calling your draw functions.

---
**`void UI_SetAllocatorFunctions(void* (*alloc_func)(size_t, void*), void (*free_func)(void*, void*), void* user_data)`**
Overrides the default memory allocation functions used by the UI system. This allows you to redirect UI memory usage to your plugin's custom heap or memory tracking system.

---
**`void* UI_MemAlloc(size_t sz)` / `void UI_MemFree(void* ptr)`**
Allocates or frees memory using the UI system's active allocator.
*   **Safety:** Always use these functions if you are passing buffers back to the UI API that it is expected to manage (e.g., resizing text input buffers).

### Timing & Performance

---
**`float UI_GetDeltaTime()`**
Returns the time elapsed since the last frame, in seconds.
*   **Example:** Smoothly moving an element.
    ```c
    static float x = 0.0f;
    x += 100.0f * ui->UI_GetDeltaTime(); // Move 100 pixels per second
    ```

---
**`double UI_GetTime()`**
Returns the total number of seconds elapsed since the framework was initialized. Useful for periodic effects.

---
**`int UI_GetFrameCount()`**
Returns the total number of frames rendered since startup.

---
**`float UI_GetFramerate()`**
Returns the current average frame rate (FPS).

### Mouse State Queries

These functions allow you to check the mouse state regardless of whether any specific widget is being interacted with.

---
**`void UI_GetMousePos(float* out_x, float* out_y)`**
Gets the absolute screen coordinates of the mouse cursor.
*   `(0, 0)` is the top-left corner of the game window.

---
**`bool UI_IsMouseDown(SPF_MouseButton button)`**
Returns `true` if the specified mouse button is currently held down.
*   `button`: `SPF_MOUSE_BUTTON_LEFT` (0), `RIGHT` (1), `MIDDLE` (2).

---
**`bool UI_IsMouseClicked(SPF_MouseButton button)`**
Returns `true` only in the frame the button was pressed.

---
**`bool UI_IsMouseReleased(SPF_MouseButton button)`**
Returns `true` only in the frame the button was released.

---
**`bool UI_IsMouseDoubleClicked(SPF_MouseButton button)`**
Returns `true` if a double-click was detected on the specified button.

---
**`bool UI_IsMouseHoveringRect(float min_x, float min_y, float max_x, float max_y, bool clip)`**
Checks if the mouse is within a specific rectangular area.
*   `clip`: If `true`, the check is clipped by the current window's boundaries.

---
**`float UI_GetMouseWheel()`**
Returns the vertical scroll amount (positive = up, negative = down).

---
**`void UI_SetMouseCursor(SPF_MouseCursor cursor)`**
Overrides the OS mouse cursor shape.
*   **Options:** `SPF_MOUSE_CURSOR_ARROW`, `TEXT_INPUT`, `HAND`, `RESIZE_ALL`, `RESIZE_NS`, `RESIZE_EW`.

### Keyboard & Shortcut System

The shortcut system is the recommended way to handle hotkeys as it respects UI focus and prevents input "leakage".

---
**`bool UI_Shortcut(int key_chord, int flags)`**
Checks if a key combination was triggered.
*   **Example:**
    ```c
    if (ui->UI_Shortcut(SPF_MOD_CTRL | SPF_KEY_S, 0)) {
        // Handle Save action
    }
    ```

---
**`void UI_SetNextItemShortcut(int key_chord, int flags)`**
Associates a shortcut with the widget rendered immediately after this call. The UI will automatically display the shortcut text (e.g., "Ctrl+O") next to the widget.

---
**`bool UI_IsKeyDown(int key_index)`**
Returns `true` if the specific key is currently held. Use `SPF_Key` values for `key_index`.

---
**`bool UI_IsKeyPressed(int key_index)`**
Returns `true` only in the frame the key was first pressed.

---
**`const char* UI_GetKeyName(int key_index)`**
Returns a human-readable name for a key (e.g., "Left Alt").

### System Clipboard

---
**`const char* UI_GetClipboardText()`**
Returns the UTF-8 text currently stored in the system's clipboard.

---
**`void UI_SetClipboardText(const char* text)`**
Copies the provided text into the system clipboard.

---

---

## 3. Windows, Layout & Positioning

This section details functions for querying the current window state, manipulating its dimensions and position, and controlling the internal layout cursor that determines where widgets are placed.

### Window State Queries

These functions allow your drawing logic to adapt based on the window's current status (e.g., skipping complex logic if the window is collapsed).

---
**`bool UI_IsWindowAppearing()`**
Returns `true` if the current window is appearing for the first time in the current session. Use this to perform one-time initialization of window-specific state.

---
**`bool UI_IsWindowCollapsed()`**
Returns `true` if the window is currently minimized (collapsed to its title bar).

---
**`bool UI_IsWindowFocused(SPF_FocusedFlags flags)`**
Checks if the current window has focus.
*   **Flags:**
    *   `SPF_FOCUSED_FLAG_NONE`: Standard focus check.
    *   `SPF_FOCUSED_FLAG_CHILD_WINDOWS`: Returns true if any child of this window is focused.
    *   `SPF_FOCUSED_FLAG_ROOT_WINDOW`: Check if the root parent window is focused.
    *   `SPF_FOCUSED_FLAG_ANY_WINDOW`: Returns true if ANY window in the system is focused.

---
**`bool UI_IsWindowHovered(SPF_HoveredFlags flags)`**
Checks if the mouse cursor is over the current window.
*   **Flags:**
    *   `SPF_HOVERED_FLAG_NONE`: Simple hover check.
    *   `SPF_HOVERED_FLAG_CHILD_WINDOWS`: Include child windows in the check.
    *   `SPF_HOVERED_FLAG_ALLOW_WHEN_BLOCKED_BY_POPUP`: Still returns true even if a modal popup is active.

---
**`SPF_DrawList_Handle UI_GetWindowDrawList()`**
Retrieves the draw list for the current window. This is used for low-level vector drawing (see Section XII).

### Size and Position Management

While the framework manages window placement, you can programmatically query or override these values.

---
**`void UI_GetWindowPos(float* out_x, float* out_y)`**
Gets the absolute screen coordinates of the current window's top-left corner.

---
**`void UI_GetWindowSize(float* out_x, float* out_y)`**
Gets the current width and height of the window, including the title bar and borders.

---
**`float UI_GetWindowWidth()` / `UI_GetWindowHeight()`**
Convenience functions for getting a single dimension.

---
**`void UI_SetNextWindowPos(float x, float y, SPF_Cond cond, float pivot_x, float pivot_y)`**
Sets the position of the window rendered immediately after this call.
*   `cond`: Condition (e.g., `SPF_COND_FIRST_USE_EVER`, `SPF_COND_ALWAYS`).
*   `pivot`: Alignment point within the window (e.g., `0.5, 0.5` to center the window at `x, y`).

---
**`void UI_SetNextWindowSize(float x, float y, SPF_Cond cond)`**
Sets the width and height for the next window. Use `0.0f` for any axis to keep the default or auto-resize behavior.

---
**`void UI_SetNextWindowCollapsed(bool collapsed, SPF_Cond cond)`**
Forces the next window to start in a collapsed or expanded state.

### Content Region & Scrolling

---
**`void UI_GetContentRegionAvail(float* out_x, float* out_y)`**
Returns the remaining space (width and height) from the current cursor position to the bottom-right corner of the window. Essential for dynamic widget sizing.
*   **Example:** Making a button fill the remaining width.
    ```c
    float avail_x, avail_y;
    ui->UI_GetContentRegionAvail(&avail_x, &avail_y);
    ui->UI_Button("Wide Button", avail_x, 0);
    ```

---
**`float UI_GetScrollY()` / `UI_SetScrollY(float scroll_y)`**
Gets or sets the current vertical scroll position.

---
**`float UI_GetScrollMaxY()`**
Returns the maximum possible vertical scroll value (total content height - window height).

---
**`void UI_SetScrollHereY(float center_y_ratio)`**
Adjusts the scroll position so the last rendered item is visible.
*   `center_y_ratio`: `0.0` (top), `0.5` (center), `1.0` (bottom).

### Cursor Management

The "Cursor" is the internal virtual point where the next widget will be drawn.

---
**`void UI_GetCursorPos(float* out_x, float* out_y)` / `UI_SetCursorPos(float x, float y)`**
Gets or sets the cursor position **relative** to the top-left of the window's content area.

---
**`void UI_GetCursorScreenPos(float* out_x, float* out_y)` / `UI_SetCursorScreenPos(float x, float y)`**
Gets or sets the cursor position in **absolute screen coordinates**. Use this when integrating with `DrawList` commands.

---
**`void UI_GetCursorStartPos(float* out_x, float* out_y)`**
Returns the position where the cursor was at the beginning of the window rendering (after padding).

### Layout Helpers

---
**`void UI_Separator()`**
Adds a horizontal line. If used inside a menu or a list, it adds a vertical divider or horizontal line as appropriate.

---
**`void UI_SameLine(float offset_from_start_x, float spacing)`**
Prevents the next widget from jumping to a new line. It places it to the right of the previous widget.
*   `offset_from_start_x`: Specific X position (relative to window start).
*   `spacing`: Extra space to add between items.

---
**`void UI_NewLine()`**
Forces the cursor to the next line.

---
**`void UI_Spacing()` / `UI_Dummy(float w, float h)`**
`Spacing` adds a standard gap. `Dummy` adds an invisible element of a specific size, reserving space in the layout.

---
**`void UI_Indent(float indent_w)` / `UI_Unindent(float indent_w)`**
Increases or decreases the left margin for all subsequent widgets.

### Grouping

---
**`void UI_BeginGroup()` / `UI_EndGroup()`**
Captures multiple widgets and treats them as a single item for layout.
*   **Utility:** Allows you to use `UI_IsItemHovered()` on a collection of widgets or wrap them in a single `SameLine` flow.

---
**`void UI_GetItemRectMin(float* x, float* y)` / `UI_GetItemRectMax(float* x, float* y)`**
Gets the screen boundaries of the widget rendered immediately before this call.

---

---

## 4. Basic Widgets

Basic widgets are the primary interactive elements used to display information and receive simple user input. They are designed to be intuitive and easy to use within the immediate-mode flow.

### Text Display

These functions render strings using the current window's font and style.

---
**`void UI_Text(const char* text)`**
Renders a basic string. Supports standard `printf` formatting internally in the framework.
*   **Example:** `ui->UI_Text("Current Status: %s", status_str);`

---
**`void UI_TextColored(float r, float g, float b, float a, const char* text)`**
Renders text with a specific RGBA color.
*   **Example:** `ui->UI_TextColored(1.0f, 0.0f, 0.0f, 1.0f, "Error: Connection Lost");`

---
**`void UI_TextDisabled(const char* text)`**
Renders text using the "Disabled" color defined in the style (usually a faded gray). Use this for labels of inactive features.

---
**`void UI_TextWrapped(const char* text)`**
Renders text that automatically wraps to the next line when it reaches the right edge of the window. This is the preferred way to display long descriptions.

---
**`void UI_LabelText(const char* label, const char* text)`**
Displays a "Key: Value" pair with a consistent layout. The label is placed on the left, and the text on the right.

---
**`void UI_BulletText(const char* text)`**
Renders a line of text preceded by a small bullet point. Useful for creating simple unformatted lists.

### Buttons

Buttons are the primary way to trigger actions. They return `true` only in the frame they are clicked.

---
**`bool UI_Button(const char* label, float width, float height)`**
Displays a standard clickable button.
*   **Parameters:**
    *   `width / height`: `0.0f` to automatically size based on the label text.
*   **Example:**
    ```c
    if (ui->UI_Button("Apply Changes", 120, 30)) {
        ApplySettings();
    }
    ```

---
**`bool UI_ButtonEx(const char* label, float width, float height, const char* tooltip, SPF_TextStyle_Handle style)`**
An advanced button that follows framework branding and supports tooltips.
*   **Behavior:**
    *   **Idle:** Uses text color from `style` (defaults to White).
    *   **Hover:** Automatically turns **Gold** and displays `tooltip` if provided.
    *   **Active:** Automatically turns **Dark** (background color) for tactile feedback.
*   **Parameters:**
    *   `width / height`: `0.0f` for auto-sizing.
    *   `tooltip`: Optional text to display when hovered (NULL to disable).
    *   `style`: Optional text style handle (NULL for framework defaults).
*   **Example:**
    ```c
    if (ui->UI_ButtonEx(ICON_FA_SAVE " Save", 150, 40, "Commit your changes", NULL)) {
        SaveData();
    }
    ```

---
**`bool UI_SmallButton(const char* label)`**
A button with zero vertical padding. It is designed to fit perfectly within a line of standard text.

---
**`bool UI_InvisibleButton(const char* str_id, float width, float height, SPF_ButtonFlags flags)`**
An invisible hit-box. Use this to create custom interactive areas or to handle clicks on custom-drawn graphics.

---
**`bool UI_ArrowButton(const char* str_id, SPF_Dir dir)`**
A small square button displaying an arrow icon.
*   **Directions:** `SPF_DIR_LEFT`, `RIGHT`, `UP`, `DOWN`.

### Checkboxes and Radio Buttons

These widgets manage boolean states or selections from a set of options.

---
**`bool UI_Checkbox(const char* label, bool* v)`**
Displays a labeled square with a checkmark.
*   **Parameters:**
    *   `v`: A pointer to a boolean. It is updated automatically when the user clicks the widget.
*   **Returns:** `true` if the value was modified this frame.

---
**`bool UI_RadioButton(const char* label, bool active)`**
Displays a circular selection dot. Note that this version does NOT update the value; it only displays the state.
*   **Example:**
    ```c
    static int choice = 0;
    if (ui->UI_RadioButton("Option A", choice == 0)) choice = 0;
    if (ui->UI_RadioButton("Option B", choice == 1)) choice = 1;
    ```

---
**`bool UI_RadioButtonFlags(const char* label, int* v, int v_button)`**
A shortcut for radio buttons that act on bitmasks. It sets `*v` to `v_button` if clicked.

### Feedback and Visualization

---
**`void UI_ProgressBar(float fraction, float width, float height, const char* overlay)`**
Displays a horizontal bar filled proportionally to `fraction`.
*   **Parameters:**
    *   `fraction`: Value from `0.0f` to `1.0f`.
    *   `overlay`: Text to display in the center of the bar (e.g., "75%"). Pass `NULL` for no text.

---
**`void UI_Bullet()`**
Draws a small standalone bullet point.

### Images

The UI system can display textures loaded by the game engine or the framework.

---
**`void UI_Image(void* user_texture_id, float width, float height)`**
Displays a texture at the specified size.
*   `user_texture_id`: An opaque pointer to the texture resource.

---
**`bool UI_ImageButton(const char* str_id, void* user_texture_id, float width, float height)`**
Uses an image as the label for a button. Returns `true` if clicked.

### Value Display Helpers

Quickly display common data types as text without manual formatting.

---
**`void UI_Value_Bool(const char* prefix, bool b)`**
Displays `Prefix: true` or `Prefix: false`.

---
**`void UI_Value_Int(const char* prefix, int v)`**
Displays `Prefix: 123`.

---
**`void UI_Value_Float(const char* prefix, float v, const char* float_format)`**
Displays a float with custom formatting (e.g., `"%.3f"`).

---

---

## 5. Advanced Inputs (Drags, Sliders, Inputs)

This section covers widgets designed for precise data entry. Unlike basic buttons or checkboxes, these widgets allow users to manipulate complex data types like floating-point numbers, integers, and colors.

### Drag Widgets

Drag widgets allow users to change a value by clicking and dragging horizontally. They are space-efficient and support manual text entry on double-click or Ctrl+Click.

---
**`bool UI_DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags)`**
The workhorse of numeric input. 
*   **Parameters:**
    *   `v`: Pointer to the float value.
    *   `v_speed`: Sensitivity of the drag (0.1f to 1.0f is typical).
    *   `v_min / v_max`: Clamping range. Use `0.0f` for both to disable clamping.
    *   `format`: Display format (e.g., `"%.3f"`).
*   **Example:**
    ```c
    static float power = 50.0f;
    ui->UI_DragFloat("Engine Power", &power, 0.5f, 0.0f, 100.0f, "%.1f %%", 0);
    ```

---
**`bool UI_DragFloat2(const char* label, float* v, ...)` / `UI_DragFloat3(...)` / `UI_DragFloat4(...)`**
Manipulate arrays of 2, 3, or 4 floats simultaneously. Perfect for vectors (XYZ) or screen positions.

---
**`bool UI_DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags)`**
Same as `DragFloat`, but for integer types.

---
**`bool UI_DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, ...)`**
A specialized widget for defining a range (Min/Max) where the min value cannot exceed the max.

### Slider Widgets

Sliders provide a clear visual representation of a value within a fixed range.

---
**`bool UI_SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, SPF_SliderFlags flags)`**
Displays a horizontal bar where the "grab" handle represents the current value.
*   **Example:**
    ```c
    static float volume = 0.8f;
    ui->UI_SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.2f", 0);
    ```

---
**`bool UI_SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, SPF_SliderFlags flags)`**
Integer version of the slider.

---
**`bool UI_SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, SPF_SliderFlags flags)`**
A specialized slider that converts degrees (for the UI) to radians (for your code).
*   **Example:**
    ```c
    static float rotation_rad = 0.0f;
    ui->UI_SliderAngle("Yaw", &rotation_rad, -180.0f, 180.0f, "%.0f deg", 0);
    ```

---
**`bool UI_VSliderFloat(const char* label, float width, float height, float* v, ...)`**
A vertical slider. Useful for audio mixers or height adjustments.

### Text & Numeric Inputs

These widgets are designed for direct keyboard entry.

---
**`bool UI_InputText(const char* label, char* buf, size_t buf_size, SPF_InputTextFlags flags)`**
Standard single-line text input field.
*   **Parameters:**
    *   `buf`: A char array to store the string.
    *   `buf_size`: Size of the buffer (including null terminator).
*   **Returns:** `true` if the text was changed.

---
**`bool UI_InputTextMultiline(const char* label, char* buf, size_t buf_size, float size_x, float size_y, SPF_InputTextFlags flags)`**
A large text area for multi-line notes or logs. Supports internal scrolling.

---
**`bool UI_InputFloat(const char* label, float* v, float step, float step_fast, const char* format, SPF_InputTextFlags flags)`**
Numeric input with `[-]` and `[+]` buttons.
*   `step`: Amount added/subtracted per single click.
*   `step_fast`: Amount added/subtracted when holding Ctrl+Click.

---
**`bool UI_InputInt(const char* label, int* v, int step, int step_fast, SPF_InputTextFlags flags)`**
Integer version with increment/decrement buttons.

### Color Editors & Pickers

The UI system provides professional-grade color selection tools.

---
**`bool UI_ColorEdit3(const char* label, float* col, SPF_ColorEditFlags flags)`**
Displays a small colored square that opens a picker on click, alongside RGB/Hex inputs.
*   **Parameters:**
    *   `col`: Array of 3 floats (Red, Green, Blue).

---
**`bool UI_ColorEdit4(const char* label, float* col, SPF_ColorEditFlags flags)`**
Includes an Alpha (transparency) channel. `col` must be an array of 4 floats.

---
**`bool UI_ColorPicker3(const char* label, float* col, SPF_ColorEditFlags flags)`**
Displays the full color wheel and saturation/brightness square directly in the layout, without a popup.

---
**`bool UI_ColorButton(const char* desc_id, float r, float g, float b, float a, SPF_ColorEditFlags flags, float width, float height)`**
A simple clickable colored square. Often used to build custom palettes.

---

---

## 6. Advanced Widgets (Trees, Selectables, Lists, Plots)

This section details widgets used for organizing data into hierarchies, creating selectable lists, and visualizing numerical arrays through graphs and histograms. These widgets are essential for managing complex configurations or displaying real-time engine telemetry.

### Hierarchical Trees

Trees allow you to create collapsible sections of content. They are perfect for property inspectors or file browsers.

---
**`bool UI_TreeNode(const char* label)`**
Creates a simple collapsible branch.
*   **Returns:** `true` if the node is open. If it returns `true`, you **must** call `UI_TreePop()` at the end of the branch.
*   **Example:**
    ```c
    if (ui->UI_TreeNode("Engine Parameters")) {
        ui->UI_Text("RPM: 1500");
        ui->UI_TreePop();
    }
    ```

---
**`bool UI_TreeNodeEx(const char* label, SPF_TreeNodeFlags flags)`**
An extended version of the tree node supporting flags for selection, bullet points, and default states.
*   **Flags:** `SPF_TREE_NODE_FLAG_DEFAULT_OPEN`, `SELECTED`, `LEAF`, `BULLET`.

---
**`void UI_TreePush(const char* str_id)` / `void UI_TreePop()`**
Manual control over the tree hierarchy. `Push` starts an indentation level without a label, while `Pop` ends it. Every `Push` or successful `TreeNode` call must be balanced with a `Pop`.

---
**`void UI_SetNextItemOpen(bool is_open, SPF_Cond cond)`**
Programmatically forces the next tree node to be open or closed based on a condition.

### Collapsing Headers

Headers are similar to tree nodes but span the full width of the window and have a distinct background color. They do not require a `Pop` call.

---
**`bool UI_CollapsingHeader(const char* label, SPF_TreeNodeFlags flags)`**
Displays a full-width title bar that toggles visibility of the content below it.
*   **Example:**
    ```c
    if (ui->UI_CollapsingHeader("Advanced Settings", 0)) {
        ui->UI_Checkbox("Enable Debug Logging", &g_Debug);
        ui->UI_SliderFloat("Update Rate", &g_Rate, 0.1f, 10.0f, "%.1f", 0);
    }
    ```

---
**`bool UI_CollapsingHeaderClosable(const char* label, bool* p_visible, SPF_TreeNodeFlags flags)`**
A header that includes a small "X" button to close or delete the entire section.

### Selectables

Selectables are the foundation for building custom lists and menus. They look like simple text but react to clicks and hovers.

---
**`bool UI_Selectable(const char* label, bool selected, SPF_SelectableFlags flags, float size_x, float size_y)`**
A simple selectable item. Returns `true` if clicked.
*   **Parameters:**
    *   `selected`: If `true`, the item is highlighted.
*   **Example:**
    ```c
    static int selection = 0;
    for (int i = 0; i < 5; i++) {
        char buf[32]; sprintf(buf, "Item %d", i);
        if (ui->UI_Selectable(buf, selection == i, 0, 0, 0)) {
            selection = i;
        }
    }
    ```

### List Boxes

A scrolling container for a set of selectable items.

---
**`bool UI_ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)`**
Creates a standard scrolling list.
*   **Parameters:**
    *   `current_item`: Pointer to the index of the selected item.
    *   `items`: Array of strings.
    *   `height_in_items`: The visible height of the box (e.g., 5 rows).

### Multi-Select API

Allows for complex item selection (Shift+Click, Ctrl+A, Marquee).

---
**`SPF_MultiSelectIO* UI_BeginMultiSelect(SPF_MultiSelectFlags flags, int selection_size, int items_count)`**
Starts a multi-selection scope. Returns a pointer to an IO object containing an array of `SPF_SelectionRequest` that your plugin must process to update its selection state.

---
**`SPF_MultiSelectIO* UI_EndMultiSelect()`**
Ends the scope and returns the final interaction state.

---
**`void UI_SetNextItemSelectionUserData(int64_t user_data)`**
Associates a unique ID (index or pointer) with the next item for selection tracking.

---
**`bool UI_IsItemToggledSelection()`**
Returns `true` if the last item's selection state was toggled by the Multi-Select system this frame.

### Plots and Histograms

Visualize arrays of data as interactive graphs.

---
**`void UI_PlotLines(const char* label, const float* values, int count, int offset, const char* overlay, float scale_min, float scale_max, float size_x, float size_y, int stride)`**
Renders a line graph.
*   **Parameters:**
    *   `values`: Pointer to an array of floats.
    *   `scale_min / max`: The Y-axis boundaries. Use `FLT_MAX` for auto-scaling.
*   **Example:**
    ```c
    static float history[100]; // Assume filled with CPU usage
    ui->UI_PlotLines("CPU Load", history, 100, 0, NULL, 0.0f, 100.0f, 0, 80, sizeof(float));
    ```

---
**`void UI_PlotHistogram(const char* label, const float* values, int count, ...)`**
Same as `PlotLines`, but renders vertical bars. Ideal for frequency analysis or distribution data.

---

---

## 7. Menus

Menus are hierarchical selection interfaces used for navigation, toolbars, and context-sensitive actions. The API supports two main types: Global Menu Bars (at the very top of the screen) and Window Menu Bars (attached to a specific window).

### Global Main Menu Bar

This menu bar stays fixed at the top of the main application window. It is typically used for high-level plugin settings or global tools.

---
**`bool UI_BeginMainMenuBar()` / `void UI_EndMainMenuBar()`**
Starts the global menu bar.
*   **Returns:** `true` if the bar is visible and you should continue adding menus.
*   **Example:**
    ```c
    if (ui->UI_BeginMainMenuBar()) {
        if (ui->UI_BeginMenu("Plugins", true)) {
            if (ui->UI_MenuItem("Reload Settings", "Ctrl+R", false, true)) {
                ReloadPlugin();
            }
            ui->UI_EndMenu();
        }
        ui->UI_EndMainMenuBar();
    }
    ```

### Window Menu Bar

To use a menu bar inside a window, the window must have been registered with the `SPF_WINDOW_FLAG_MENU_BAR` flag.

---
**`bool UI_BeginMenuBar()` / `void UI_EndMenuBar()`**
Starts a menu bar attached to the current window.
*   **Example:**
    ```c
    if (ui->UI_BeginMenuBar()) {
        if (ui->UI_BeginMenu("Edit", true)) {
            if (ui->UI_MenuItem("Undo", "Ctrl+Z", false, true)) { /* ... */ }
            ui->UI_EndMenu();
        }
        ui->UI_EndMenuBar();
    }
    ```

### Menus & Menu Items

---
**`bool UI_BeginMenu(const char* label, bool enabled)` / `void UI_EndMenu()`**
Creates a dropdown menu. Can be nested inside other menus to create sub-menus.
*   **Parameters:**
    *   `enabled`: If `false`, the menu label is grayed out and cannot be opened.

---
**`bool UI_MenuItem(const char* label, const char* shortcut, bool selected, bool enabled)`**
An actionable entry in a menu.
*   **Parameters:**
    *   `shortcut`: Optional text displayed on the right (e.g., "Alt+F4").
    *   `selected`: If `true`, a checkmark is displayed next to the label.
*   **Returns:** `true` if the item was clicked.

---
**`bool UI_MenuItemEx(const char* label, const char* icon, const char* shortcut, bool selected, bool enabled)`**
An extended version allowing for icons (if supported by the font).

---

## 8. Tables

Tables are the most powerful tool in the `SPF_UI_API` for displaying structured, multi-column data. They support column resizing, reordering, automatic headers, and complex sorting logic.

### Table Lifecycle

---
**`bool UI_BeginTable(const char* str_id, int columns, SPF_TableFlags flags, float outer_size_x, float outer_size_y, float inner_width)`**
Initializes a table container.
*   **Parameters:**
    *   `str_id`: Unique identifier for the table.
    *   `columns`: Number of columns.
    *   `flags`: Bitmask of `SPF_TableFlags` (e.g., `SPF_TABLE_FLAG_BORDERS`, `ROW_BG`, `RESIZABLE`).
*   **Example:**
    ```c
    if (ui->UI_BeginTable("AssetTable", 3, SPF_TABLE_FLAG_BORDERS | SPF_TABLE_FLAG_ROW_BG, 0, 0, 0)) {
        // ... Table content ...
        ui->UI_EndTable();
    }
    ```

---
**`void UI_EndTable()`**
Ends the table. Must be called if `BeginTable` returned `true`.

### Column Setup

---
**`void UI_TableSetupColumn(const char* label, SPF_TableColumnFlags flags, float init_width_or_weight, uint32_t user_id)`**
Defines the properties of a column.
*   **Parameters:**
    *   `flags`: Controls sizing (Fixed vs Stretch), default sorting, etc.
    *   `init_width_or_weight`: Pixel width for fixed columns, or weight for stretch columns.

---
**`void UI_TableHeadersRow()`**
Automatically renders a row containing the labels defined in `TableSetupColumn`.

---
**`void UI_TableAngledHeadersRow()`**
Renders headers with rotated text (usually 45 degrees). Useful for narrow columns with long labels.

### Rows & Columns

---
**`void UI_TableNextRow(SPF_TableRowFlags row_flags, float min_row_height)`**
Starts a new row in the table.

---
**`bool UI_TableNextColumn()`**
Advances the cursor to the next cell.
*   **Returns:** `false` if the column is currently clipped (not visible), allowing you to skip rendering content for that cell.

---
**`bool UI_TableSetColumnIndex(int column_n)`**
Directly jumps to a specific column index.

### Advanced Table Features

---
**`SPF_TableSortSpecs* UI_TableGetSortSpecs()`**
Retrieves the current sorting requirements requested by the user (by clicking headers).
*   **Logic:** You use this structure to determine which column should be sorted and in which direction (Ascending/Descending).

---
**`void UI_TableSetBgColor(SPF_TableBgTarget target, uint32_t color, int column_n)`**
Overrides the background color of a specific row, column, or cell.
*   **Target:** `SPF_TABLE_BG_TARGET_ROW_BG0`, `TARGET_CELL_BG`, etc.

---
**`int UI_TableGetColumnCount()`**
Returns the total number of columns in the current table.

---
**`const char* UI_TableGetColumnName(int column_n)`**
Returns the label of the specified column.

---

---

## 9. Popups & Tooltips

Popups are temporary windows that appear over other content, usually triggered by a specific user action like a right-click or a button press. Tooltips provide context-sensitive information when hovering over an item.

### Popups

---
**`bool UI_BeginPopup(const char* str_id, SPF_WindowFlags flags)` / `void UI_EndPopup()`**
Creates a context-sensitive window that disappears if the user clicks outside or presses Escape.
*   **Example:**
    ```c
    if (ui->UI_Button("Options", 0, 0)) {
        ui->UI_OpenPopup("MyPopup");
    }
    if (ui->UI_BeginPopup("MyPopup", 0)) {
        ui->UI_Text("Popup Content");
        ui->UI_EndPopup();
    }
    ```

---
**`void UI_OpenPopup(const char* str_id)`**
Programmatically marks a popup as "open". Must match the ID passed to `BeginPopup`.

---
**`bool UI_BeginPopupModal(const char* name, bool* p_open, SPF_WindowFlags flags)`**
A special popup that blocks interaction with the rest of the UI until it is closed. Perfect for "Are you sure?" dialogs.
*   **Parameters:**
    *   `p_open`: Optional pointer to a boolean. If provided, a close button (X) is shown.

---
**`void UI_CloseCurrentPopup()`**
Closes the popup currently being rendered. Often called from a "Close" button inside the popup.

---
**`bool UI_IsPopupOpen(const char* str_id)`**
Checks if a specific popup is currently visible.

### Tooltips

Tooltips are small floating boxes that follow the mouse.

---
**`void UI_SetTooltip(const char* text)`**
Sets a basic text tooltip for the **last rendered item**.
*   **Example:**
    ```c
    ui->UI_Button("Delete", 0, 0);
    if (ui->UI_IsItemHovered()) {
        ui->UI_SetTooltip("Warning: This action is permanent.");
    }
    ```

---
**`void UI_BeginTooltip()` / `void UI_EndTooltip()`**
Allows for complex, multi-widget tooltips (e.g., containing images or multiple lines of text).
*   **Example:**
    ```c
    if (ui->UI_IsItemHovered()) {
        ui->UI_BeginTooltip();
        ui->UI_TextColored(1, 0, 0, 1, "CRITICAL ERROR");
        ui->UI_Image(error_icon, 32, 32);
        ui->UI_EndTooltip();
    }
    ```

---

## 10. Drag & Drop

The Drag & Drop system allows users to transfer data (like file paths, numeric values, or object references) between different UI elements.

### Drag Source (Origin)

---
**`bool UI_BeginDragDropSource(SPF_DragDropFlags flags)` / `void UI_EndDragDropSource()`**
Turns the last rendered item into a draggable source.
*   **Example:**
    ```c
    ui->UI_Text("Drag me!");
    if (ui->UI_BeginDragDropSource(0)) {
        int my_id = 42;
        ui->UI_SetDragDropPayload("MY_TYPE", &my_id, sizeof(int), 0);
        ui->UI_Text("Moving ID: %d", my_id); // Tooltip during drag
        ui->UI_EndDragDropSource();
    }
    ```

---
**`bool UI_SetDragDropPayload(const char* type, const void* data, size_t size, SPF_Cond cond)`**
Attaches a data buffer to the drag operation.
*   `type`: A unique string identifying the data format.

### Drop Target (Destination)

---
**`bool UI_BeginDragDropTarget()` / `void UI_EndDragDropTarget()`**
Makes the last rendered item receptive to drops.

---
**`const SPF_Payload* UI_AcceptDragDropPayload(const char* type, SPF_DragDropFlags flags)`**
Checks if the current drag operation matches the requested type and accepts it.
*   **Returns:** A pointer to the payload containing the data, or `NULL` if not accepted.
*   **Example:**
    ```c
    ui->UI_Button("Drop Here", 100, 100);
    if (ui->UI_BeginDragDropTarget()) {
        const SPF_Payload* payload = ui->UI_AcceptDragDropPayload("MY_TYPE", 0);
        if (payload) {
            int received_id = *(const int*)payload->Data;
            ProcessDrop(received_id);
        }
        ui->UI_EndDragDropTarget();
    }
    ```

---

---

## 11. Style & Typography

The Style API allows you to customize the visual appearance of every element in the UI. You can modify colors, spacing, rounding, and fonts. Because this is an immediate-mode API, style changes are handled using a **Push/Pop stack**—you "push" a style change, render your widgets, and then "pop" the change to restore the previous state.

### Style Colors

---
**`void UI_PushStyleColor(SPF_StyleColor idx, float r, float g, float b, float a)`**
Changes one of the standard UI colors (see the `SPF_StyleColor` enum in Section 0).
*   **Example:**
    ```c
    ui->UI_PushStyleColor(SPF_COLOR_BUTTON, 0.8f, 0.2f, 0.2f, 1.0f); // Make buttons red
    ui->UI_Button("Destructive Action", 0, 0);
    ui->UI_PopStyleColor(1); // Restore previous button color
    ```

---
**`void UI_Style_SetColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a)`**
Sets the base text color for a custom style handle.

---
**`void UI_Style_SetHoverColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a)`**
Sets the text color when the element is hovered. Primarily used by `UI_ButtonEx`.

---
**`void UI_Style_SetActiveColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a)`**
Sets the text color when the element is pressed/active. Primarily used by `UI_ButtonEx`.

---
**`void UI_PopStyleColor(int count)`**
Removes the specified number of color changes from the stack.

### Style Variables

Style variables control layout and geometry properties like padding, rounding, and borders.

---
**`void UI_PushStyleVarFloat(SPF_StyleVar idx, float val)`**
Changes a single float variable (e.g., `SPF_STYLE_VAR_ALPHA` or `WINDOW_ROUNDING`).

---
**`void UI_PushStyleVarVec2(SPF_StyleVar idx, float x, float y)`**
Changes a variable that requires two values (e.g., `SPF_STYLE_VAR_WINDOW_PADDING` or `ITEM_SPACING`).

---
**`void UI_PopStyleVar(int count)`**
Restores the previous style variables from the stack.

### Typography & Fonts

The framework provides access to high-quality fonts for different use cases.

---
**`SPF_Font_Handle UI_GetFont(const char* font_key)`**
Retrieves a handle to a pre-loaded font by its key.
*   **Default Keys:** `"default"`, `"bold"`, `"h1"`, `"h2"`, `"monospace"`.

---
**`void UI_PushFont(SPF_Font_Handle handle)`**
Sets the active font for all subsequent text rendering.
*   **Example:**
    ```c
    ui->UI_PushFont(ui->UI_GetFont("h1"));
    ui->UI_Text("Section Header");
    ui->UI_PopFont();
    ```

---
**`void UI_PopFont()`**
Restores the previous font.

---
**`void UI_SetWindowFontScale(float scale)`**
Scales the text within the current window. Useful for accessibility or high-DPI adjustments.

### Markdown Rendering

The SPF Framework includes a built-in Markdown parser for rendering rich text documentation.

---
**`void UI_RenderMarkdown(const char* markdown_text)`**
Renders a block of text with support for headers (`#`), bold (`**`), italics (`*`), lists, and code blocks.
*   **Color Highlighting:** You can use `<#RRGGBB>text</>` tags to colorize parts of the text.
    *   *Example:* `ui->UI_RenderMarkdown("This is <#ff0000>red text</>.");`
*   **Logic:** It automatically handles word wrapping and style application based on the Markdown tags.

---

## 12. DrawList API (Low-level Vector Drawing)

The DrawList API is an advanced tool for rendering custom graphics directly into the GPU command buffer. You can use it to draw lines, shapes, and custom textures on top of or behind standard widgets.

### Getting DrawLists

---
**`SPF_DrawList_Handle UI_GetWindowDrawList()`**
Returns the draw list for the current window. Coordinates are absolute screen space.

---
**`SPF_DrawList_Handle UI_GetBackgroundDrawList()`**
Used to draw *behind* all windows (e.g., for full-screen overlays or custom wallpaper).

---
**`SPF_DrawList_Handle UI_GetForegroundDrawList()`**
Used to draw *on top* of everything, including modal windows.

### Drawing Primitives

All color parameters in this section use a packed `uint32_t` in `0xAABBGGRR` format.

---
**`void UI_DrawList_AddLine(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness)`**
Draws a straight line between two points.

---
**`void UI_DrawList_AddRect(SPF_DrawList_Handle dl, float min_x, float min_y, float max_x, float max_y, uint32_t col, float rounding, SPF_DrawFlags flags, float thickness)`**
Draws an outlined rectangle. Use `AddRectFilled` for a solid shape.

---
**`void UI_DrawList_AddCircle(SPF_DrawList_Handle dl, float cx, float cy, float radius, uint32_t col, int segments, float thickness)`**
Draws an outlined circle. `segments = 0` uses automatic smoothing.

---
**`void UI_DrawList_AddText(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text)`**
Renders a string at a specific screen position without any layout logic.

---
**`void UI_DrawList_AddTextWithFont(SPF_DrawList_Handle dl, SPF_Font font, float font_size, float pos_x, float pos_y, uint32_t col, const char* text, float wrap_width)`**
Renders text using a specific font style and size from the `SPF_Font` enumeration.
*   **Parameters:**
    *   `font`: One of the `SPF_FONT_` enum values (e.g., `SPF_FONT_BOLD`).
    *   `font_size`: The size in pixels.
    *   `wrap_width`: Optional width for automatic line wrapping (0.0 for no wrap).

---
Renders a texture into the specified rectangular area. Supports custom UV mapping and tinting.

### Stateful Path Building

For complex polygons or curves, use the path API.

---
**`void UI_DrawList_PathClear(SPF_DrawList_Handle dl)`**
Resets the current path.

---
**`void UI_DrawList_PathLineTo(SPF_DrawList_Handle dl, float x, float y)`**
Adds a point to the current path.

---
**`void UI_DrawList_PathFillConvex(SPF_DrawList_Handle dl, uint32_t col)`**
Fills the area enclosed by the points in the path.

---

---

## 13. Item Queries & State

Item queries allow you to inspect the state of the widget that was rendered immediately before the query call. This is fundamental for creating reactive interfaces where one element's behavior depends on the interaction with another.

### Interaction Queries

---
**`bool UI_IsItemHovered(SPF_HoveredFlags flags)`**
Returns `true` if the mouse is currently over the last rendered widget.
*   **Example:**
    ```c
    ui->UI_Button("Action", 0, 0);
    if (ui->UI_IsItemHovered(0)) {
        ui->UI_SetTooltip("Click to execute action");
    }
    ```

---
**`bool UI_IsItemActive()`**
Returns `true` if the last widget is currently being interacted with (e.g., a button is being held down, or a slider is being dragged).

---
**`bool UI_IsItemFocused()`**
Returns `true` if the last widget has keyboard/gamepad focus.

---
**`bool UI_IsItemClicked(SPF_MouseButton button)`**
Returns `true` if the last widget was clicked with the specified mouse button.

---
**`bool UI_IsItemVisible()`**
Returns `true` if the last widget is visible on screen (i.e., not clipped or outside the scroll region). Use this to skip expensive processing for off-screen elements.

### Value & State Change Queries

---
**`bool UI_IsItemEdited()`**
Returns `true` if the last widget's value was modified during this frame (e.g., the user typed a character in an `InputText`).

---
**`bool UI_IsItemDeactivated()`**
Returns `true` if the widget was active in the previous frame but is no longer active in this frame (e.g., the user released a slider).

---
**`bool UI_IsItemDeactivatedAfterEdit()`**
Similar to `IsItemDeactivated`, but only returns `true` if the widget's value was actually changed during the interaction. This is perfect for triggering "Save" or "Apply" logic only when a user finishes editing.

---
**`bool UI_IsItemToggledOpen()`**
Returns `true` if the last item (like a Tree node or Collapsing Header) was opened or closed this frame.

### Geometry Queries

---
**`void UI_GetItemRectMin(float* x, float* y)` / `void UI_GetItemRectMax(float* x, float* y)`**
Retrieves the absolute screen coordinates of the bounding box of the last rendered widget.

---
**`void UI_GetItemRectSize(float* x, float* y)`**
Retrieves the width and height of the last rendered widget.

---
**`void UI_SetItemAllowOverlap()`**
Allows the next widget to overlap with the current one. Useful for drawing custom icons on top of buttons or inputs.

---

## 14. Tabs & Tab Bars

Tab bars are an effective way to organize large amounts of content into distinct, selectable panes within a single window or region.

### Tab Bar Lifecycle

---
**`bool UI_BeginTabBar(const char* str_id, SPF_TabBarFlags flags)` / `void UI_EndTabBar()`**
Initializes a container for tab items.
*   **Returns:** `true` if the tab bar is visible.
*   **Example:**
    ```c
    if (ui->UI_BeginTabBar("ConfigTabs", 0)) {
        // ... Tab items ...
        ui->UI_EndTabBar();
    }
    ```

### Tab Items

---
**`bool UI_BeginTabItem(const char* label, bool* p_open, SPF_TabItemFlags flags)` / `void UI_EndTabItem()`**
Defines a single selectable tab.
*   **Returns:** `true` if the tab is currently selected and active. You should only render the tab's content if this returns `true`.
*   **Parameters:**
    *   `p_open`: Optional pointer to a boolean. If provided, a close "X" button appears on the tab.
*   **Example:**
    ```c
    if (ui->UI_BeginTabItem("General", NULL, 0)) {
        ui->UI_Text("General Settings Content");
        ui->UI_EndTabItem();
    }
    ```

---
**`void UI_SetTabItemClosed(const char* label)`**
Programmatically closes a tab by its label name.

### Specialized Tab Widgets

---
**`bool UI_TabItemButton(const char* label, SPF_TabItemFlags flags)`**
Creates a button that looks like a tab but does not behave like a selectable pane. Useful for "Add New Tab" functionality.

---

---

## 15. Docking & Viewports

The Docking system allows users to snap windows to the sides of the application or group them into tabbed stacks. Viewports allow UI windows to be moved outside the main game window as independent OS windows.

### DockSpace Creation

---
**`SPF_ID UI_DockSpace(SPF_ID id, float size_x, float size_y, SPF_DockNodeFlags flags)`**
Creates a docking region within the current window.
*   **Usage:** Typically used when a window is configured with `SPF_WINDOW_FLAG_NO_BACKGROUND` to turn the entire window into a docking host.

---
**`SPF_ID UI_DockSpaceOverViewport(SPF_Viewport_Handle* viewport, SPF_DockNodeFlags flags)`**
Creates a docking host that covers the entire specified viewport (usually the main game window).

### Programmatic Docking

---
**`void UI_SetNextWindowDockID(SPF_ID dock_id, SPF_Cond cond)`**
Forces the next window to be docked into a specific node.

---
**`bool UI_IsWindowDocked()`**
Returns `true` if the current window is currently attached to a dock node.

---
**`SPF_ID UI_GetWindowDockID()`**
Returns the ID of the dock node the current window is attached to.

### DockBuilder API (Direct Manipulation)

The DockBuilder API allows you to programmatically define the initial layout of your plugin's windows.

---
**`void UI_DockBuilderDockWindow(const char* window_name, SPF_ID node_id)`**
Assigns a window to a specific dock node.

---
**`SPF_ID UI_DockBuilderAddNode(SPF_ID node_id, SPF_DockNodeFlags flags)`**
Creates a new dock node.

---
**`SPF_ID UI_DockBuilderSplitNode(SPF_ID node_id, SPF_Dir split_dir, float size_ratio_for_node_at_dir, SPF_ID* out_id_at_dir, SPF_ID* out_id_opposite_dir)`**
Splits an existing node into two sections (e.g., Left/Right or Top/Bottom).

---

## 16. Internal & Custom Widget Utilities

These functions provide low-level access to the UI's layout engine, allowing you to create completely new types of interactive widgets from scratch.

### Layout Reservation

---
**`void UI_ItemSize(float width, float height, float baseline_offset)`**
Reserves a rectangular area in the current layout. It advances the cursor but does not render anything or handle input.

---
**`bool UI_ItemAdd(float min_x, float min_y, float max_x, float max_y, SPF_ID id)`**
Registers a reserved area as an "item". This enables hit-testing, allowing the widget to be hovered, clicked, or focused.
*   **Returns:** `false` if the item is clipped (not visible), in which case you can skip rendering.

### Interaction Logic

---
**`bool UI_ButtonBehavior(float min_x, float min_y, float max_x, float max_y, SPF_ID id, bool* out_hovered, bool* out_held, SPF_ButtonFlags flags)`**
Implements the standard logic for clicking, hovering, and holding. Use this to make custom-drawn elements behave exactly like standard buttons.

---
**`bool UI_DragBehavior(SPF_ID id, SPF_DataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags)`**
Implements the logic for horizontal dragging to change a numeric value.

---
**`bool UI_SliderBehavior(float min_x, float min_y, float max_x, float max_y, SPF_ID id, SPF_DataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags)`**
Implements the visual slider logic (mapping a click position to a value range).

### ID Management

Every interactive widget needs a unique ID.

---
**`void UI_PushID(const char* str_id)` / `void UI_PushIDInt(int int_id)`**
Pushes an identifier onto the ID stack. All widgets rendered within the stack will have their IDs hashed with this value. This prevents ID collisions when rendering similar widgets in a loop.

---
**`void UI_PopID()`**
Removes the last ID from the stack.

---
**`SPF_ID UI_GetID(const char* str_id)`**
Returns the calculated hash for a given string based on the current ID stack.

---

---

## 17. Framework Utilities

This final section covers specialized utilities provided by the SPF Framework that enhance the user experience and provide deeper integration with the host application or game engine.

### Notifications System

The framework includes a built-in notification system for displaying temporary, non-intrusive messages to the user.

---
**`void UI_ShowNotification(SPF_NotificationType type, const char* message, SPF_Notification_DisplayMode mode)`**
Triggers an animated on-screen notification.
*   **Parameters:**
    *   `type`: The severity/category of the message.
        *   `SPF_NOTIFICATION_SUCCESS`: Green, for successful operations.
        *   `SPF_NOTIFICATION_INFO`: Blue, for general information.
        *   `SPF_NOTIFICATION_WARNING`: Yellow/Orange, for non-critical issues.
        *   `SPF_NOTIFICATION_ERROR`: Red, for failures.
    *   `message`: The text to display.
    *   `mode`:
        *   `SPF_NOTIF_MODE_STACK`: Notifications appear in a stack (usually bottom-right).
        *   `SPF_NOTIF_MODE_TOP`: Appears centered at the top of the screen.

*   **Example:**
    ```c
    if (SaveConfig()) {
        ui->UI_ShowNotification(SPF_NOTIFICATION_SUCCESS, "Configuration saved successfully!", SPF_NOTIF_MODE_STACK);
    } else {
        ui->UI_ShowNotification(SPF_NOTIFICATION_ERROR, "Failed to save config!", SPF_NOTIF_MODE_STACK);
    }
    ```

### Cinematic Transitions

Transitions allow plugins to perform full-screen visual effects, often used when loading new scenes or switching between major UI states.

---
**`void UI_PlayTransition(SPF_TransitionType type, float duration, bool reverse, SPF_TransitionColor color)`**
Plays a screen-space transition effect.
*   **Parameters:**
    *   `type`:
        *   `SPF_TRANS_FADE`: Simple alpha fade.
        *   `SPF_TRANS_WIPE_LEFT / RIGHT / UP / DOWN`: Linear wipe effect.
        *   `SPF_TRANS_RADIAL`: Circular expansion/contraction.
    *   `duration`: Time in seconds for the transition to complete.
    *   `reverse`: If `true`, the effect plays in the opposite direction (e.g., Fade In vs Fade Out).
    *   `color`: The color of the transition mask (`SPF_TRANS_COLOR_BLACK`, `WHITE`, `THEME`).

### Mouse & Input Blocking

Because SPF plugins run on top of a game, it is often necessary to prevent mouse clicks or movement from being processed by the game engine when the user is interacting with the UI.

---
**`void UI_SetMouseBlockState(bool axes, bool buttons, bool wheel)`**
Selectively blocks mouse input from reaching the underlying game.
*   **Parameters:**
    *   `axes`: Block mouse movement (prevents the camera from turning).
    *   `buttons`: Block clicks (prevents shooting or interacting in-game).
    *   `wheel`: Block scrolling (prevents weapon switching, etc.).

---
**`void UI_SetMouseOverride(bool overridden)`**
Forcefully takes control of the mouse cursor, ensuring it remains visible and active even if the game normally hides it.

---
**`bool UI_IsMouseOverridden()`**
Returns `true` if the mouse control logic is currently in override mode.

### Specialized Rendering

---
**`void UI_RenderCheckMark(float x, float y, uint32_t col, float sz)`**
Directly renders a standard UI checkmark icon at the specified screen coordinates. This is useful when building custom lists where you want to maintain visual consistency with standard checkboxes.

---

**This concludes the SPF UI API documentation.**

*For any functions not explicitly detailed here, please refer to the comments in `include/SPF/SPF_API/SPF_UI_API.h`.*