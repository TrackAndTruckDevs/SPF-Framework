# SPF UI API

The SPF UI API allows your plugin to create and manage its own in-game windows, rendered by the framework's powerful UI engine. The API is a C-style wrapper around the popular **Dear ImGui** library, and it uses an immediate-mode paradigm.

## Core Concepts

### Immediate Mode UI
Unlike traditional "retained-mode" UI toolkits, an immediate-mode UI is rebuilt from code every single frame. You don't create a button object and then listen for its events. Instead, every frame that your window is visible, you call `ui->UI_Button("My Button")`, and the function tells you if it was clicked *on that frame*. This results in a very simple and dynamic way of creating user interfaces.

### The Draw Callback
The heart of the UI system is the **draw callback**. This is a function you create in your plugin that the framework will call every frame for each of your visible windows. Inside this function, you call the various widget functions (`UI_Text`, `UI_Button`, `UI_Checkbox`, etc.) to draw your UI for that frame.

## Workflow

Creating a UI window involves three main steps:

1.  **Declare in Manifest:** In your `BuildManifest` function, you must declare all your windows in the `ui` section. This tells the framework about your window's existence, its unique ID, default visibility, size, etc.
2.  **Implement a Draw Callback:** For each window, create a C-function in your plugin that matches the `SPF_DrawCallback` signature. This function will contain the logic for drawing your window's content.
3.  **Register the Callback:** In your plugin's `OnRegisterUI` lifecycle function, you must call `UI_RegisterDrawCallback` to link the window ID from your manifest to the corresponding draw callback function in your code.

## Getting the API

The UI API is provided in two places:

*   The `UI_RegisterDrawCallback` function is called from the `OnRegisterUI` lifecycle event, which passes the `SPF_UI_API` pointer as an argument.
*   The widget functions (`UI_Text`, `UI_Button`, etc.) are used inside your draw callback, which also receives the `SPF_UI_API` pointer as its first argument.

# Text Styling & Markdown API

Beyond the basic `Text` and `TextColored` widgets, the UI API provides a powerful system for advanced text styling and Markdown rendering. This allows for fine-grained control over fonts, colors, alignment, and layout, enabling rich and dynamic text presentations.

This system is exposed through a set of functions that operate on an `SPF_TextStyle_Handle`. This handle is an opaque pointer to a style object that you can create and configure.

## Styling Workflow

Using the styling API follows a clear lifecycle:

1.  **Create:** Create a new style object with `UI_Style_Create()`. This gives you a handle.
2.  **Configure:** Use the various `UI_Style_Set...()` functions (e.g., `UI_Style_SetFont`, `UI_Style_SetColor`) to modify the style object. These functions can be chained.
3.  **Use:** Pass the configured handle to a rendering function like `UI_TextStyled()` or `UI_RenderMarkdown()`.
4.  **Destroy:** When you are finished with the style object for the frame, you **must** call `UI_Style_Destroy()` to release its memory and prevent leaks.

## API Reference

---
**`SPF_TextStyle_Handle UI_Style_Create()`**
Creates a new, empty text style handle. This handle must be destroyed with `UI_Style_Destroy`.

---
**`void UI_Style_Destroy(SPF_TextStyle_Handle handle)`**
Destroys a text style handle and releases its memory.

---
**`void UI_Style_SetFont(SPF_TextStyle_Handle handle, SPF_Font font)`**
Sets the font for the text style. The available fonts are defined in the `SPF_Font` enum (e.g., `SPF_FONT_H1`, `SPF_FONT_BOLD`).

---
**`void UI_Style_SetColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a)`**
Sets the color of the text. Color components are floats from `0.0f` to `1.0f`.

---
**`void UI_Style_SetAlign(SPF_TextStyle_Handle handle, SPF_TextAlign align)`**
Sets the horizontal alignment of the text (`SPF_TEXT_ALIGN_LEFT`, `SPF_TEXT_ALIGN_CENTER`, `SPF_TEXT_ALIGN_RIGHT`). Centering and right-alignment are relative to the available content region width.

---
**`void UI_Style_SetWrap(SPF_TextStyle_Handle handle, bool wrap)`**
Enables or disables automatic text wrapping within the available content region.

---
**`void UI_Style_SetPadding(SPF_TextStyle_Handle handle, float pad_x, float pad_y)`**
Sets horizontal (x) and vertical (y) padding around the text block.

---
**`void UI_Style_SetSeparator(SPF_TextStyle_Handle handle, bool is_separator)`**
If `true`, renders the text as a label inside a horizontal line separator.

---
**`void UI_Style_SetUnderline(SPF_TextStyle_Handle handle, bool is_underline)`**
If `true`, draws a line under the text.

---
**`void UI_Style_SetStrikethrough(SPF_TextStyle_Handle handle, bool is_strikethrough)`**
If `true`, draws a line through the middle of the text.

---
**`void UI_TextStyled(SPF_TextStyle_Handle handle, const char* fmt, ...)`**
Renders text using a specific style object. This function is variadic, meaning it supports `printf`-style formatting. If the handle is `NULL`, default styling is used.

---
**`void UI_RenderMarkdown(const char* markdown_text, SPF_TextStyle_Handle base_style_handle)`**
Renders a block of text formatted with Markdown. It supports basic syntax like headers (`#`), bold (`**text**`), italic (`*text*`), code blocks (```), and links (`[text](url)`). The `base_style_handle` is optional and can be used to apply a base style (like padding) to the entire block.

## Styling Example

This example demonstrates how to create and use multiple styles to render a rich UI block.

```c
void DrawStylingExample(SPF_UI_API* ui, void* user_data) {
    // 1. Create style handles
    SPF_TextStyle_Handle h1_style = ui->UI_Style_Create();
    SPF_TextStyle_Handle centered_text_style = ui->UI_Style_Create();
    SPF_TextStyle_Handle separator_style = ui->UI_Style_Create();
    SPF_TextStyle_Handle markdown_base_style = ui->UI_Style_Create();

    // 2. Configure the styles
    ui->UI_Style_SetFont(h1_style, SPF_FONT_H1);
    ui->UI_Style_SetColor(h1_style, 1.0f, 0.84f, 0.0f, 1.0f); // Gold color
    ui->UI_Style_SetAlign(h1_style, SPF_TEXT_ALIGN_CENTER);

    ui->UI_Style_SetAlign(centered_text_style, SPF_TEXT_ALIGN_CENTER);
    ui->UI_Style_SetWrap(centered_text_style, true);
    ui->UI_Style_SetPadding(centered_text_style, 0.f, 10.f);

    ui->UI_Style_SetSeparator(separator_style, true);

    // 3. Use the styles to render UI
    ui->UI_TextStyled(h1_style, "Styling API Demo");

    ui->UI_TextStyled(centered_text_style, "This text is centered, wrapped, and has vertical padding.");

    ui->UI_TextStyled(separator_style, "Markdown Section");

    const char* markdown =
        "## Markdown Features\n"
        "You can mix **bold** and *italic* text.\n"
        "```\n// Even code blocks are supported!\nint x = 42;\n```\n"
        "> Blockquotes provide visual emphasis.";
    
    ui->UI_Style_SetPadding(markdown_base_style, 10.0f, 5.0f);
    ui->UI_RenderMarkdown(markdown, markdown_base_style);
    
    // 4. Clean up the style handles for this frame
    ui->UI_Style_Destroy(h1_style);
    ui->UI_Style_Destroy(centered_text_style);
    ui->UI_Style_Destroy(separator_style);
    ui->UI_Style_Destroy(markdown_base_style);
}
```

## Main Functions

---
**`void UI_RegisterDrawCallback(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data)`**
This is the most important function. It links a `windowId` from your manifest to a draw function in your plugin. You **must** call this from your `OnRegisterUI` function for every window you want to render.

*   `pluginName`: Your plugin's name.
*   `windowId`: The unique ID of the window, which must match an entry in your manifest.
*   `drawCallback`: A pointer to your function that will draw the window's contents.
*   `user_data`: An optional pointer to your own data that will be passed to your draw callback.

---
**`SPF_Window_Handle* UI_GetWindowHandle(const char* pluginName, const char* windowId)`**
Retrieves a handle to one of your windows, which can be used to control it programmatically.

---
**`void UI_SetVisibility(SPF_Window_Handle* handle, bool isVisible)`** and **`bool UI_IsVisible(SPF_Window_Handle* handle)`**
Gets or sets the visibility of a window using its handle.

# Custom Drawing & Interaction API
To create fully custom interactive widgets, you need access to low-level drawing primitives and detailed input state. This section of the API provides that functionality.

The typical workflow is:
1. Create a "canvas" for your widget using `UI_InvisibleButton()`. This gives you a bounding box and basic interaction state (`UI_IsItemHovered`, `UI_IsItemActive`).
2. Get the window's `DrawList` using `UI_GetWindowDrawList()`.
3. Query detailed mouse state using functions like `UI_GetMousePos` and `UI_IsMouseDragging`.
4. Use the `UI_DrawList_...` functions to draw your custom shapes and text onto the canvas.

## API Reference

---
**`SPF_DrawList_Handle UI_GetWindowDrawList()`**
Gets a handle to the draw list for the current window. The draw list is the primary tool for custom drawing and is valid only for the current frame.

---
**`uint32_t UI_ColorConvertFloat4ToU32(float r, float g, float b, float a)`**
A helper function to convert an RGBA color from four floats (0.0-1.0) to a packed 32-bit integer color (`0xAABBGGRR`) required by all `UI_DrawList` functions.

### Drawing Primitives
*   `UI_DrawList_AddLine(...)`: Adds a line between two points.
*   `UI_DrawList_AddRect(...)`: Adds a rectangle outline.
*   `UI_DrawList_AddRectFilled(...)`: Adds a filled rectangle.
*   `UI_DrawList_AddQuadFilled(...)`: Adds a filled quadrilateral.
*   `UI_DrawList_AddTriangleFilled(...)`: Adds a filled triangle.
*   `UI_DrawList_AddCircleFilled(...)`: Adds a filled circle.
*   `UI_DrawList_AddBezierCubic(...)`: Adds a smooth cubic Bezier curve.
*   `UI_DrawList_AddText(...)`: Draws text at a specific screen position, ignoring layout.

### Path & Polyline Functions
These functions allow you to build complex shapes.
*   `UI_DrawList_AddPolyline(...)`: Draws a sequence of connected lines from an array of points.
*   `UI_DrawList_PathClear()`: Clears the internal path buffer.
*   `UI_DrawList_PathLineTo(...)`: Adds a new point to the path.
*   `UI_DrawList_PathStroke(...)`: Draws an outline of the constructed path.
*   `UI_DrawList_PathFillConvex(...)`: Fills the constructed path (must be a convex shape).

### Advanced Interaction
*   `UI_GetMousePos(...)`: Gets the absolute screen coordinates of the mouse cursor.
*   `UI_GetMouseDragDelta(...)`: Gets how far the mouse has been dragged since the button was clicked.
*   `UI_IsMouseDown(...)`: Checks if a mouse button is currently held down.
*   `UI_IsMouseClicked(...)`: Checks if a mouse button was pressed and released this frame.
*   `UI_IsMouseReleased(...)`: Checks if a mouse button was released this frame.
*   `UI_IsMouseDoubleClicked(...)`: Checks for a double-click.
*   `UI_GetMouseWheel()`: Gets the mouse wheel's vertical scroll value for this frame.

# Layout & Positioning API
These functions provide information about the current window and layout state, allowing for precise placement of custom elements.

*   `UI_GetContentRegionAvail(...)`: Returns the remaining available space in the current window.
*   `UI_GetWindowPos(...)` / `UI_GetWindowSize(...)`: Return the position and size of the current window.
*   `UI_GetCursorScreenPos()`: Returns the absolute screen position where the next widget will be drawn.
*   `UI_SetCursorScreenPos(...)`: Manually sets the absolute screen position for the next widget.
*   `UI_GetItemRectMin(...)` / `UI_GetItemRectMax(...)` / `UI_GetItemRectSize(...)`: Return the bounding box (top-left corner, bottom-right corner) and size of the previously drawn widget.

# Miscellaneous Utilities

### ID Management
Essential for creating complex widgets or widgets in loops to avoid ID conflicts.
*   `UI_PushID_Str(const char* id)` / `UI_PushID_Int(int id)` / `UI_PushID_Ptr(void* id)`: Pushes a unique identifier onto the ID stack. Must be paired with a `UI_PopID()`.
*   `UI_PopID()`: Pops the last ID from the stack.
*   `UI_GetID_Str(const char* id)`: Calculates a unique ID from a string in the current ID context without pushing to the stack.

### Clipboard Management
*   `UI_GetClipboardText()`: Returns the contents of the system clipboard as a string.
*   `UI_SetClipboardText(const char* text)`: Sets the system clipboard to the given string.

### Font Management
*   `UI_GetFont(const char* font_key)`: Retrieves an opaque handle to a font loaded by the framework (e.g., "bold", "h1").
*   `UI_PushFont(SPF_Font_Handle* font_handle)`: Pushes a font onto the stack, making it active for all subsequent drawing. Must be paired with a `UI_PopFont()`.
*   `UI_PopFont()`: Restores the previous font from the stack.

### Global Style Access
Allows plugins to make their custom widgets consistent with the look and feel of the rest of the UI.
*   `UI_GetStyle()`: Returns a handle to the global style object.
*   `UI_Style_GetWindowPadding(...)`: Gets the `WindowPadding` (a 2D vector) from a style handle.
*   `UI_Style_GetItemSpacing(...)`: Gets the `ItemSpacing` from a style handle.
*   `UI_Style_GetFramePadding(...)`: Gets the `FramePadding` from a style handle.

## Widget Reference

The `SPF_UI_API` struct contains a large number of function pointers for creating widgets. These functions are direct C-style mappings of their counterparts in the Dear ImGui library. Below are some of the most common categories.

### Basic Widgets
*   `UI_Text(const char* text)`
*   `UI_TextColored(float r, float g, float b, float a, const char* text)`
*   `UI_Button(const char* label, float width, float height)`
*   `UI_Checkbox(const char* label, bool* v)`

### Input Widgets
*   `UI_InputText(const char* label, char* buf, size_t buf_size)`
*   `UI_InputInt(const char* label, int* v, ...)`
*   `UI_InputFloat(const char* label, float* v, ...)`
*   `UI_SliderInt(const char* label, int* v, int v_min, int v_max, ...)`
*   `UI_SliderFloat(const char* label, float* v, float v_min, float v_max, ...)`
*   `UI_ColorEdit3` / `UI_ColorEdit4`

### Layout & Spacing
*   `UI_Separator()`
*   `UI_Spacing()`
*   `UI_SameLine(...)`
*   `UI_Indent()` / `UI_Unindent()`

For a complete list of all available widgets and their specific parameters, please refer to the `SPF_UI_API.h` header file and the official **Dear ImGui** documentation.

## Complete Example

This example shows how to declare, register, and draw a simple window with a button and a checkbox.

**1. Manifest Definition (`BuildManifest`)**
```c
void MyPlugin_BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // ... basic metadata ...
    
    // Declare a window with default visibility and size
    api->Defaults_AddWindow(h, "MyMainWindow", true, true, 100, 100, 400, 300, false, false);
}
```

**2. UI Registration and Draw Callback Implementation**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_UI_API.h"

// Global state for our checkbox
static bool s_myCheckboxValue = false;

// 3. Implement the draw callback
void DrawMyMainWindow(SPF_UI_API* ui, void* user_data) {
    ui->UI_Text("This is my custom plugin window!");
    ui->UI_Separator();
    
    if (ui->UI_Button("Click Me!", 0, 0)) {
        // This code runs when the button is clicked
    }

    ui->UI_Checkbox("My Checkbox", &s_myCheckboxValue);
}

// 2. Register the callback in the OnRegisterUI lifecycle function
void MyPlugin_OnRegisterUI(SPF_UI_API* ui_api) {
    if (ui_api) {
        ui_api->UI_RegisterDrawCallback("MyPlugin", "MyMainWindow", &DrawMyMainWindow, NULL);
    }
}
```
