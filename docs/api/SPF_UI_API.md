# SPF UI API

The SPF UI API allows your plugin to create and manage its own in-game windows, rendered by the framework's powerful UI engine. The API is a C-style wrapper around the popular **Dear ImGui** library, and it uses an immediate-mode paradigm.

## Core Concepts

### Immediate Mode UI
Unlike traditional "retained-mode" UI toolkits, an immediate-mode UI is rebuilt from code every single frame. You don't create a button object and then listen for its events. Instead, every frame that your window is visible, you call `ui->Button("My Button")`, and the function tells you if it was clicked *on that frame*. This results in a very simple and dynamic way of creating user interfaces.

### The Draw Callback
The heart of the UI system is the **draw callback**. This is a function you create in your plugin that the framework will call every frame for each of your visible windows. Inside this function, you call the various widget functions (`Text`, `Button`, `Checkbox`, etc.) to draw your UI for that frame.

## Workflow

Creating a UI window involves three main steps:

1.  **Declare in Manifest:** In your `GetManifestData` function, you must declare all your windows in the `ui` section. This tells the framework about your window's existence, its unique ID, default visibility, size, etc.
2.  **Implement a Draw Callback:** For each window, create a C-function in your plugin that matches the `SPF_DrawCallback` signature. This function will contain the logic for drawing your window's content.
3.  **Register the Callback:** In your plugin's `OnRegisterUI` lifecycle function, you must call `RegisterDrawCallback` to link the window ID from your manifest to the corresponding draw callback function in your code.

## Getting the API

The UI API is provided in two places:

*   The `RegisterDrawCallback` function is called from the `OnRegisterUI` lifecycle event, which passes the `SPF_UI_API` pointer as an argument.
*   The widget functions (`Text`, `Button`, etc.) are used inside your draw callback, which also receives the `SPF_UI_API` pointer as its first argument.

# Text Styling & Markdown API

Beyond the basic `Text` and `TextColored` widgets, the UI API provides a powerful system for advanced text styling and Markdown rendering. This allows for fine-grained control over fonts, colors, alignment, and layout, enabling rich and dynamic text presentations.

This system is exposed through a set of functions that operate on an `SPF_TextStyle_Handle`. This handle is an opaque pointer to a style object that you can create and configure.

## Styling Workflow

Using the styling API follows a clear lifecycle:

1.  **Create:** Create a new style object with `Style_Create()`. This gives you a handle.
2.  **Configure:** Use the various `Style_Set...()` functions (e.g., `Style_SetFont`, `Style_SetColor`) to modify the style object. These functions can be chained.
3.  **Use:** Pass the configured handle to a rendering function like `TextStyled()` or `RenderMarkdown()`.
4.  **Destroy:** When you are finished with the style object for the frame, you **must** call `Style_Destroy()` to release its memory and prevent leaks.

## API Reference

---
**`SPF_TextStyle_Handle Style_Create()`**
Creates a new, empty text style handle. This handle must be destroyed with `Style_Destroy`.

---
**`void Style_Destroy(SPF_TextStyle_Handle handle)`**
Destroys a text style handle and releases its memory.

---
**`void Style_SetFont(SPF_TextStyle_Handle handle, SPF_Font font)`**
Sets the font for the text style. The available fonts are defined in the `SPF_Font` enum (e.g., `SPF_FONT_H1`, `SPF_FONT_BOLD`).

---
**`void Style_SetColor(SPF_TextStyle_Handle handle, float r, float g, float b, float a)`**
Sets the color of the text. Color components are floats from `0.0f` to `1.0f`.

---
**`void Style_SetAlign(SPF_TextStyle_Handle handle, SPF_TextAlign align)`**
Sets the horizontal alignment of the text (`SPF_TEXT_ALIGN_LEFT`, `SPF_TEXT_ALIGN_CENTER`, `SPF_TEXT_ALIGN_RIGHT`). Centering and right-alignment are relative to the available content region width.

---
**`void Style_SetWrap(SPF_TextStyle_Handle handle, bool wrap)`**
Enables or disables automatic text wrapping within the available content region.

---
**`void Style_SetPadding(SPF_TextStyle_Handle handle, float pad_x, float pad_y)`**
Sets horizontal (x) and vertical (y) padding around the text block.

---
**`void Style_SetSeparator(SPF_TextStyle_Handle handle, bool is_separator)`**
If `true`, renders the text as a label inside a horizontal line separator.

---
**`void Style_SetUnderline(SPF_TextStyle_Handle handle, bool is_underline)`**
If `true`, draws a line under the text.

---
**`void Style_SetStrikethrough(SPF_TextStyle_Handle handle, bool is_strikethrough)`**
If `true`, draws a line through the middle of the text.

---
**`void TextStyled(SPF_TextStyle_Handle handle, const char* fmt, ...)`**
Renders text using a specific style object. This function is variadic, meaning it supports `printf`-style formatting. If the handle is `NULL`, default styling is used.

---
**`void RenderMarkdown(const char* markdown_text, SPF_TextStyle_Handle base_style_handle)`**
Renders a block of text formatted with Markdown. It supports basic syntax like headers (`#`), bold (`**text**`), italic (`*text*`), code blocks (```), and links (`[text](url)`). The `base_style_handle` is optional and can be used to apply a base style (like padding) to the entire block.

## Styling Example

This example demonstrates how to create and use multiple styles to render a rich UI block.

```c
void DrawStylingExample(SPF_UI_API* ui, void* user_data) {
    // 1. Create style handles
    SPF_TextStyle_Handle h1_style = ui->Style_Create();
    SPF_TextStyle_Handle centered_text_style = ui->Style_Create();
    SPF_TextStyle_Handle separator_style = ui->Style_Create();
    SPF_TextStyle_Handle markdown_base_style = ui->Style_Create();

    // 2. Configure the styles
    ui->Style_SetFont(h1_style, SPF_FONT_H1);
    ui->Style_SetColor(h1_style, 1.0f, 0.84f, 0.0f, 1.0f); // Gold color
    ui->Style_SetAlign(h1_style, SPF_TEXT_ALIGN_CENTER);

    ui->Style_SetAlign(centered_text_style, SPF_TEXT_ALIGN_CENTER);
    ui->Style_SetWrap(centered_text_style, true);
    ui->Style_SetPadding(centered_text_style, 0.f, 10.f);

    ui->Style_SetSeparator(separator_style, true);

    // 3. Use the styles to render UI
    ui->TextStyled(h1_style, "Styling API Demo");

    ui->TextStyled(centered_text_style, "This text is centered, wrapped, and has vertical padding.");

    ui->TextStyled(separator_style, "Markdown Section");

    const char* markdown =
        "## Markdown Features\n"
        "You can mix **bold** and *italic* text.\n"
        "```\n// Even code blocks are supported!\nint x = 42;\n```\n"
        "> Blockquotes provide visual emphasis.";
    
    ui->Style_SetPadding(markdown_base_style, 10.0f, 5.0f);
    ui->RenderMarkdown(markdown, markdown_base_style);
    
    // 4. Clean up the style handles for this frame
    ui->Style_Destroy(h1_style);
    ui->Style_Destroy(centered_text_style);
    ui->Style_Destroy(separator_style);
    ui->Style_Destroy(markdown_base_style);
}
```

## Main Functions

---
**`void RegisterDrawCallback(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data)`**
This is the most important function. It links a `windowId` from your manifest to a draw function in your plugin. You **must** call this from your `OnRegisterUI` function for every window you want to render.

*   `pluginName`: Your plugin's name.
*   `windowId`: The unique ID of the window, which must match an entry in your manifest.
*   `drawCallback`: A pointer to your function that will draw the window's contents.
*   `user_data`: An optional pointer to your own data that will be passed to your draw callback.

---
**`SPF_Window_Handle* GetWindowHandle(const char* pluginName, const char* windowId)`**
Retrieves a handle to one of your windows, which can be used to control it programmatically.

---
**`void SetVisibility(SPF_Window_Handle* handle, bool isVisible)`** and **`bool IsVisible(SPF_Window_Handle* handle)`**
Gets or sets the visibility of a window using its handle.

## Widget Reference

The `SPF_UI_API` struct contains a large number of function pointers for creating widgets. These functions are direct C-style mappings of their counterparts in the Dear ImGui library. Below are some of the most common categories.

### Basic Widgets
*   `Text(const char* text)`
*   `TextColored(float r, float g, float b, float a, const char* text)`
*   `Button(const char* label, float width, float height)`
*   `Checkbox(const char* label, bool* v)`

### Input Widgets
*   `InputText(const char* label, char* buf, size_t buf_size)`
*   `InputInt(const char* label, int* v, ...)`
*   `InputFloat(const char* label, float* v, ...)`
*   `SliderInt(const char* label, int* v, int v_min, int v_max, ...)`
*   `SliderFloat(const char* label, float* v, float v_min, float v_max, ...)`
*   `ColorEdit3` / `ColorEdit4`

### Layout & Spacing
*   `Separator()`
*   `Spacing()`
*   `SameLine(...)`
*   `Indent()` / `Unindent()`

For a complete list of all available widgets and their specific parameters, please refer to the `SPF_UI_API.h` header file and the official **Dear ImGui** documentation.

## Complete Example

This example shows how to declare, register, and draw a simple window with a button and a checkbox.

**1. Manifest Definition (`GetManifestData`)**
```c
// In GetManifestData()
out_manifest.ui.windowsCount = 1;
auto& my_window = out_manifest.ui.windows[0];
strncpy_s(my_window.name, "MyMainWindow", sizeof(my_window.name));
my_window.isVisible = true; // Make it visible by default
```

**2. UI Registration and Draw Callback Implementation**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_UI_API.h"

// Global state for our checkbox
static bool s_myCheckboxValue = false;

// 3. Implement the draw callback
void DrawMyMainWindow(SPF_UI_API* ui, void* user_data) {
    ui->Text("This is my custom plugin window!");
    ui->Separator();
    
    if (ui->Button("Click Me!", 0, 0)) {
        // This code runs when the button is clicked
    }

    ui->Checkbox("My Checkbox", &s_myCheckboxValue);
}

// 2. Register the callback in the OnRegisterUI lifecycle function
SPF_PLUGIN_ENTRY void MyPlugin_OnRegisterUI(SPF_UI_API* ui_api) {
    if (ui_api) {
        ui_api->RegisterDrawCallback("MyPlugin", "MyMainWindow", &DrawMyMainWindow, NULL);
    }
}
```
