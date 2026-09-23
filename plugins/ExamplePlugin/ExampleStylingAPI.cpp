/**
 * @file ExampleStylingAPI.cpp
 * @brief Implementation of the SPF Styling / UI assets API example.
 *
 * @details ASSET LIFETIME:
 *   OnActivated → CreateTextureFromMemory / LoadFontFromFile / LoadFontFromMemory
 *   Render*     → PushFont / UI_Image using cached g_ctx handles (lazy UI_GetFont fallback)
 *   OnUnload    → DestroyTexture / UnloadFont while the UI backend still exists
 *
 * STYLE HANDLES: UI_Style_Create → configure → use in one frame → UI_Style_Destroy.
 * Do not cache SPF_TextStyle_Handle across frames unless you also destroy them later —
 * this tab creates/destroys per call as the simplest correct pattern.
 */
#include "ExampleStylingAPI.hpp"

#include "SPF/SPF_API/SPF_Icons.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"
#include "font/DRKrapkaSquare.h"

#include <cstddef>
#include <cstdint>

namespace ExamplePlugin {

void StylingAPI_OnActivated() {
  if (!g_ctx.coreAPI || !g_ctx.uiAPI || !g_ctx.uiAPI->UI_CreateTextureFromMemory) {
    return;
  }
  auto ui = g_ctx.uiAPI;
  auto logger = g_ctx.coreAPI->logger;
  auto fmt = g_ctx.coreAPI->formatting;
  char log_buffer[256];

  // --- Memory texture (embedded byte array) ---
  // Real PNG bytes (1x1 white pixel). UI_CreateTextureFromMemory decodes the image format
  // itself and returns texture + dimensions; it does NOT take UV/filter/format parameters.
  static const unsigned char white_pixel_png[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                                                  0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xDE, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41, 0x54, 0x08, 0xD7, 0x63, 0xF8, 0xFF,
                                                  0xFF, 0x3F, 0x00, 0x05, 0xFE, 0x02, 0xFE, 0xDC, 0x44, 0x74, 0x06, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

  g_ctx.pluginTexture = ui->UI_CreateTextureFromMemory(white_pixel_png, sizeof(white_pixel_png), &g_ctx.textureWidth, &g_ctx.textureHeight);
  if (g_ctx.pluginTexture && logger && fmt) {
    fmt->Fmt_Format(log_buffer, sizeof(log_buffer), "Created texture from memory: %dx%d", g_ctx.textureWidth, g_ctx.textureHeight);
    logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
  } else if (logger) {
    logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, "Failed to load demo texture from memory (renderer might not be ready).");
  }

  // --- File texture ---
  // test.png lives in the plugin data dir (plugins/ExamplePlugin/data/test.png).
  if (g_ctx.environmentAPI && g_ctx.environmentHandle && ui->UI_CreateTextureFromFile && fmt) {
    char dataPath[512];
    g_ctx.environmentAPI->Env_GetPluginDataDir(g_ctx.environmentHandle, dataPath, sizeof(dataPath));
    char filePath[1024];
    fmt->Fmt_Format(filePath, sizeof(filePath), "%s\\test.png", dataPath);
    g_ctx.pluginFileTexture = ui->UI_CreateTextureFromFile(filePath, &g_ctx.fileTextureWidth, &g_ctx.fileTextureHeight);
    if (g_ctx.pluginFileTexture && logger) {
      logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Successfully loaded texture from data\\test.png.");
    } else if (logger) {
      logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, "Failed to load texture from file (is data\\test.png present?).");
    }
  }

  // --- Fonts: registered under names, resolved later via UI_GetFont ---
  // Font loads are ASYNCHRONOUS: the atlas rebuild is deferred to the next frame,
  // so these return null handles now; call UI_GetFont(name) on subsequent frames.
  SPF_Font_Config config = {24.0f, false, nullptr};
  if (g_ctx.environmentAPI && g_ctx.environmentHandle && ui->UI_LoadFontFromFile && fmt) {
    char dataPath[512];
    g_ctx.environmentAPI->Env_GetPluginDataDir(g_ctx.environmentHandle, dataPath, sizeof(dataPath));
    char fontPath[1024];
    fmt->Fmt_Format(fontPath, sizeof(fontPath), "%s\\Rushon Ground.ttf", dataPath);
    ui->UI_LoadFontFromFile("ExamplePlugin_CustomFont", fontPath, &config);
    if (logger) {
      logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Requested font from file. Available next frame.");
    }
  }
  // Memory font uses the embedded DRKrapkaSquare blob — same name-based lookup pattern.
  if (ui->UI_LoadFontFromMemory) {
    ui->UI_LoadFontFromMemory("ExamplePlugin_MemoryFont", Font_DRKrapkaSquare_compressed_data, Font_DRKrapkaSquare_compressed_size, &config);
    if (logger) {
      logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Requested font from memory. Available next frame.");
    }
  }
}

void StylingAPI_OnUnload() {
  // Guard: UI pointer must still be valid; textures belong to the UI backend.
  if (g_ctx.uiAPI) {
    if (g_ctx.pluginTexture) {
      g_ctx.uiAPI->UI_DestroyTexture(g_ctx.pluginTexture);
      g_ctx.pluginTexture = nullptr;
    }
    if (g_ctx.pluginFileTexture) {
      g_ctx.uiAPI->UI_DestroyTexture(g_ctx.pluginFileTexture);
      g_ctx.pluginFileTexture = nullptr;
    }
  }
}

void RenderStylingTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!ui->UI_Style_Create) {
    ui->UI_Text("Styling API not available in this version of the framework.");
    return;
  }

  ui->UI_TextWrapped("This tab demonstrates the features of the new Text Styling and Markdown API.");
  ui->UI_Separator();

  // 1. Create style handles
  SPF_TextStyle_Handle h1_style = ui->UI_Style_Create();
  SPF_TextStyle_Handle centered_text_style = ui->UI_Style_Create();
  SPF_TextStyle_Handle separator_style = ui->UI_Style_Create();
  SPF_TextStyle_Handle markdown_base_style = ui->UI_Style_Create();

  // 2. Configure the styles
  ui->UI_Style_SetFont(h1_style, SPF_FONT_H1);
  ui->UI_Style_SetColor(h1_style, 1.0f, 0.84f, 0.0f, 1.0f);  // Gold color
  ui->UI_Style_SetAlign(h1_style, SPF_TEXT_ALIGN_CENTER);

  ui->UI_Style_SetAlign(centered_text_style, SPF_TEXT_ALIGN_CENTER);
  ui->UI_Style_SetWrap(centered_text_style, true);
  ui->UI_Style_SetPadding(centered_text_style, 0.f, 10.f);

  ui->UI_Style_SetSeparator(separator_style, true);
  ui->UI_Style_SetColor(separator_style, 0.6f, 0.6f, 0.6f, 1.0f);  // Gray

  // 3. Use the styles to render UI
  ui->UI_TextStyled(h1_style, ICON_FA_FONT_AWESOME " Welcome to the Styling API! " ICON_FA_FONT_AWESOME);

  ui->UI_TextStyled(centered_text_style, "This text is centered and will wrap if it becomes too long to fit in the available space. This demonstrates alignment, wrapping, and vertical padding.");

  ui->UI_Spacing();

  // Icons with styling
  ui->UI_TextStyled(separator_style, ICON_FA_ICONS " Icon Integration Demo");
  ui->UI_Text("You can now use FontAwesome 7 icons directly in your UI!");

  ui->UI_Text(ICON_FA_PLAY " Play  " ICON_FA_PAUSE " Pause  " ICON_FA_STOP " Stop  " ICON_FA_FORWARD " Forward");

  ui->UI_TextColored(0.35f, 0.39f, 0.98f, 1.0f, ICON_FA_DISCORD " Discord");
  ui->UI_SameLine(0, 10);
  ui->UI_TextColored(1.0f, 0.0f, 0.0f, 1.0f, ICON_FA_YOUTUBE " YouTube");
  ui->UI_SameLine(0, 10);
  ui->UI_TextColored(0.1f, 1.0f, 0.1f, 1.0f, ICON_FA_GITHUB " GitHub");

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, "Unified Button API");
  ui->UI_TextWrapped("UI_ButtonEx provides framework-standard behavior (White -> Gold -> Dark) by default, or can be fully customized.");

  if (ui->UI_ButtonEx(ICON_FA_CIRCLE_CHECK " Default Framework Button", 0, 40, "Uses default White/Gold/Dark logic", NULL)) {
    SPF_Notification_Params p = {SPF_NOTIFICATION_SUCCESS, "Framework default button clicked!", SPF_NOTIF_MODE_TOP, 3.0f};
    ui->UI_ShowNotification(&p);
  }

  ui->UI_Spacing();
  ui->UI_Text("Custom Color Override Example:");

  SPF_TextStyle_Handle custom_btn_style = ui->UI_Style_Create();
  ui->UI_Style_SetColor(custom_btn_style, 0.5f, 1.0f, 0.5f, 1.0f);        // Idle: Light Green
  ui->UI_Style_SetHoverColor(custom_btn_style, 1.0f, 1.0f, 1.0f, 1.0f);   // Hover: White
  ui->UI_Style_SetActiveColor(custom_btn_style, 0.0f, 0.5f, 0.0f, 1.0f);  // Active: Dark Green

  if (ui->UI_ButtonEx(ICON_FA_BUG " Custom Styled Button", 300, 0, "Idle: Green | Hover: White | Active: Dark Green", custom_btn_style)) {
    SPF_Notification_Params p = {SPF_NOTIFICATION_HINT, "Custom styled button clicked!", SPF_NOTIF_MODE_TOP, 3.0f};
    ui->UI_ShowNotification(&p);
  }
  ui->UI_Style_Destroy(custom_btn_style);

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, "Markdown Demo");

  const char* markdown =
    "# Enhanced Markdown Test\n"
    "This is a demonstration of the **SPF v1.1.5** markdown engine.\n\n"
    "--- \n"
    "### 1. Custom Colors & Formatting\n"
    "You can now use <#ff4444>custom RGB colors</> directly in your text. \n"
    "This is <#44ff44>green text</>, and this is <#ffcc00>gold text</>.\n"
    "You can even combine them: **<#ff4444>Bold Red</>** and *<#44ff44>Italic Green</>*.\n\n"
    "--- \n\n"
    "Find plugin demonstrations, tutorials, and project updates on our YouTube Channel at [Track'n'Truck](https://www.youtube.com/@TrackAndTruck).\n\n"
    "--- \n"
    "### 2. Lists & Task Lists\n"
    "* Regular bullet point\n"
    "    * Nested point\n\n"
    "* [x] This is a completed task\n"
    "* [ ] This is a pending task\n\n\n"
    "1. First numbered item\n"
    "2. Second numbered item\n\n"
    "--- \n"
    "### 3. Blockquotes & Code\n"
    "> \"The best way to predict the future is to invent it.\"\n"
    "> — Alan Kay\n\n"
    "```cpp\n"
    "// New colors work everywhere!\n"
    "ui->UI_ShowNotification(..., \"<#00ff00>Success!</>\");\n"
    "```\n"
    "And `inline code` is still here.\n\n"
    "| Header 1 | Header 2 |\n"
    "|----------|----------|\n"
    "| Cell 1   | Cell 2   |\n\n"
    "--- \n"
    "### 4. GitHub-style Alerts\n"
    "> [!NOTE]\n"
    "> This is a blue information block.\n\n"
    "> [!TIP]\n"
    "> This is a green tip block.\n\n"
    "> [!IMPORTANT]\n"
    "> This is a purple important block.\n\n"
    "> [!WARNING]\n"
    "> This is a gold warning block.\n\n"
    "> [!CAUTION]\n"
    "> This is a red caution block.\n";

  ui->UI_Style_SetPadding(markdown_base_style, 10.0f, 5.0f);
  ui->UI_RenderMarkdown(markdown, markdown_base_style);

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, "Notification System Test");
  ui->UI_TextWrapped("Testing structure-based API with custom colors and programmatic control.");

  auto QuickNotif = [&](SPF_NotificationType type, const char* msg, SPF_Notification_DisplayMode mode) {
    SPF_Notification_Params p = {};
    p.type = type;
    p.message = msg;
    p.mode = mode;
    p.duration = -1.0f;  // Auto
    ui->UI_ShowNotification(&p);
  };

  ui->UI_TextDisabled("Standard Types (Top Mode)");
  if (ui->UI_Button(ICON_FA_CIRCLE_INFO " Info", 0, 0)) QuickNotif(SPF_NOTIFICATION_INFO, "General information message.", SPF_NOTIF_MODE_TOP);
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button(ICON_FA_CIRCLE_CHECK " Success", 0, 0)) QuickNotif(SPF_NOTIFICATION_SUCCESS, "Operation completed successfully!", SPF_NOTIF_MODE_TOP);
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button(ICON_FA_TRIANGLE_EXCLAMATION " Warning", 0, 0)) QuickNotif(SPF_NOTIFICATION_WARNING, "Warning: Something might be wrong.", SPF_NOTIF_MODE_TOP);

  if (ui->UI_Button(ICON_FA_CIRCLE_XMARK " Error", 0, 0)) QuickNotif(SPF_NOTIFICATION_ERROR, "A standard error occurred.", SPF_NOTIF_MODE_TOP);
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button(ICON_FA_RADIATION " Critical", 0, 0)) QuickNotif(SPF_NOTIFICATION_CRITICAL, "Critical system failure detected!", SPF_NOTIF_MODE_TOP);
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button(ICON_FA_LIGHTBULB " Hint", 0, 0)) QuickNotif(SPF_NOTIFICATION_HINT, "Did you know? You can customize these notifications.", SPF_NOTIF_MODE_TOP);

  ui->UI_Spacing();
  ui->UI_TextDisabled("Custom Styling (Randomized)");
  if (ui->UI_Button(ICON_FA_PAINT_ROLLER " Random Custom Notif", 0, 0)) {
    static float r[] = {1.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    static float g[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f};
    static float b[] = {1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    static const char* icons[] = {ICON_FA_GEAR, ICON_FA_ROCKET, ICON_FA_GHOST, ICON_FA_DRAGON, ICON_FA_TRUCK};
    static int rnd_idx = 0;
    rnd_idx = (rnd_idx + 1) % 5;

    SPF_Notification_Params p = {};
    p.type = SPF_NOTIFICATION_CUSTOM;
    p.message = "This is a **completely custom** notification\nwith random color and icon!";
    p.mode = SPF_NOTIF_MODE_STACK;
    p.duration = 5.0f;
    p.r = r[rnd_idx];
    p.g = g[rnd_idx];
    p.b = b[rnd_idx];
    p.a = 1.0f;
    p.custom_icon = icons[rnd_idx];
    ui->UI_ShowNotification(&p);
  }

  ui->UI_Spacing();
  ui->UI_TextDisabled("Programmatic Control (Manual Close)");
  static SPF_Notification_Handle hActive = nullptr;

  if (!hActive) {
    if (ui->UI_Button(ICON_FA_PLAY " Open Programmatic (Infinite)", 0, 0)) {
      SPF_Notification_Params p = {};
      p.type = SPF_NOTIFICATION_INFO;
      p.message = "<#ffcc00>Manual Management</>\nThis notification will stay until you click 'Close'.\nNote the static progress bar.";
      p.mode = SPF_NOTIF_MODE_TOP;
      p.duration = 0.0f;  // 0 = Programmatic (infinite)
      hActive = ui->UI_ShowNotification(&p);
    }
  } else {
    if (ui->UI_Button(ICON_FA_STOP " Close Programmatic Notification", 0, 0)) {
      ui->UI_HideNotification(hActive);
      hActive = nullptr;
    }
  }

  ui->UI_Spacing();
  ui->UI_TextDisabled("Sticky Mode (At cursor)");
  if (ui->UI_Button(ICON_FA_THUMBTACK " Toggle Sticky Help", 0, 0)) {
    SPF_Notification_Params p = {};
    p.type = SPF_NOTIFICATION_HINT;
    p.message =
      "**Sticky Help Tip**\n\n"
      "This window has no timeout. It will stay here until:\n"
      "1. You click the button again (Toggle).\n"
      "2. You click anywhere outside this notification.\n\n"
      "Useful for explaining complex UI elements!";
    p.mode = SPF_NOTIF_MODE_STICKY;
    p.duration = -1.0f;
    ui->UI_ShowNotification(&p);
  }

  ui->UI_Spacing();
  if (ui->UI_Button("Test Long Text (Top)", 0, 0)) {
    QuickNotif(SPF_NOTIFICATION_INFO,
               "This is a very long notification message designed to test the \n*automatic text wrapping* and **dynamic height** \nadjustment of the ***notification window***. "
               "It should handle multiple lines of text gracefully without cutting off the content or expanding beyond reasonable bounds.",
               SPF_NOTIF_MODE_TOP);
  }

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, "Window Management Test");
  ui->UI_TextWrapped("Use the button below to center the window and resize it to fit all tabs.");
  if (ui->UI_Button("Center and Fit Window", 0, 0)) {
    const char* tabs[] = {"General", "Traffic Inspector", "Camera", "Telemetry", "Events", "Virtual Input", "Styling API", "Environment", "Input Test"};

    float total_width = 0;
    SPF_Style_Handle* style = ui->UI_GetStyle();
    float frame_padding_x, frame_padding_y;
    ui->UI_Style_GetFramePadding(style, &frame_padding_x, &frame_padding_y);

    for (const char* tab : tabs) {
      float w, h;
      ui->UI_CalcTextSizeWithFont(SPF_FONT_REGULAR, 18.0f, tab, &w, &h);
      total_width += w + (frame_padding_x * 2.0f) + 4.0f;
    }

    float win_padding_x, win_padding_y;
    ui->UI_Style_GetWindowPadding(style, &win_padding_x, &win_padding_y);
    total_width += (win_padding_x * 2.0f) + 20.0f;

    float v_w, v_h;
    ui->UI_GetMainViewportSize(&v_w, &v_h);

    float win_h = 450.0f;
    float pos_x = (v_w - total_width) * 0.5f;
    float pos_y = (v_h - win_h) * 0.5f;

    ui->UI_SetWindowPos(pos_x, pos_y, SPF_COND_ALWAYS);
    ui->UI_SetWindowSize(total_width, win_h, SPF_COND_ALWAYS);

    QuickNotif(SPF_NOTIFICATION_SUCCESS, "Window centered and resized to fit all tabs!", SPF_NOTIF_MODE_TOP);
  }

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, "Custom Gradient API Test");
  ui->UI_TextWrapped("Demonstrating multi-color primitives for advanced custom widgets.");

  float canvas_x, canvas_y;
  ui->UI_GetCursorScreenPos(&canvas_x, &canvas_y);
  SPF_DrawList_Handle dl = ui->UI_GetWindowDrawList();

  uint32_t col_r = ui->UI_ColorConvertFloat4ToU32(1.0f, 0.0f, 0.0f, 1.0f);
  uint32_t col_g = ui->UI_ColorConvertFloat4ToU32(0.0f, 1.0f, 0.0f, 1.0f);
  uint32_t col_b = ui->UI_ColorConvertFloat4ToU32(0.0f, 0.0f, 1.0f, 1.0f);
  ui->UI_DrawList_AddTriangleFilledMultiColor(dl, canvas_x + 50, canvas_y + 10, canvas_x + 10, canvas_y + 90, canvas_x + 90, canvas_y + 90, col_r, col_g, col_b);

  uint32_t col_white = ui->UI_ColorConvertFloat4ToU32(1.0f, 1.0f, 1.0f, 1.0f);
  uint32_t col_gold = ui->UI_ColorConvertFloat4ToU32(1.0f, 0.84f, 0.0f, 1.0f);
  ui->UI_DrawList_AddCircleFilledMultiColor(dl, canvas_x + 150, canvas_y + 50, 40.0f, col_white, col_gold, 32);

  uint32_t col_tl = ui->UI_ColorConvertFloat4ToU32(1.0f, 0.0f, 1.0f, 1.0f);
  uint32_t col_tr = ui->UI_ColorConvertFloat4ToU32(0.0f, 1.0f, 1.0f, 1.0f);
  uint32_t col_br = ui->UI_ColorConvertFloat4ToU32(1.0f, 1.0f, 0.0f, 1.0f);
  uint32_t col_bl = ui->UI_ColorConvertFloat4ToU32(0.0f, 0.0f, 0.0f, 1.0f);
  ui->UI_DrawList_AddRectFilledMultiColor(dl, canvas_x + 220, canvas_y + 10, canvas_x + 350, canvas_y + 90, col_tl, col_tr, col_br, col_bl);

  ui->UI_Dummy(360, 100);

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, ICON_FA_FONT " Dynamic Font Rendering");
  ui->UI_TextWrapped("Demonstrating UI_LoadFontFromFile and UI_LoadFontFromMemory.");

  // Lazy resolve: font may register a frame after OnActivated request.
  if (!g_ctx.pluginFont) {
    g_ctx.pluginFont = ui->UI_GetFont("ExamplePlugin_CustomFont");
  }

  if (g_ctx.pluginFont) {
    ui->UI_PushFont(g_ctx.pluginFont);
    ui->UI_TextColored(0.4f, 0.7f, 1.0f, 1.0f, "This text uses a custom font loaded from FILE (Rushon Ground)!");
    ui->UI_Text("Sample: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789");
    ui->UI_PopFont();
  } else {
    ui->UI_TextColored(1.0f, 0.5f, 0.0f, 1.0f, "File font not loaded yet (queued for next frame or missing data\\Rushon Ground.ttf).");
  }

  ui->UI_Spacing();

  if (!g_ctx.memoryFont) {
    g_ctx.memoryFont = ui->UI_GetFont("ExamplePlugin_MemoryFont");
  }

  if (g_ctx.memoryFont) {
    ui->UI_PushFont(g_ctx.memoryFont);
    ui->UI_TextColored(0.7f, 1.0f, 0.4f, 1.0f, "This text uses a custom font loaded from MEMORY (DRKrapkaSquare)!");
    ui->UI_Text("Sample: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789");
    ui->UI_PopFont();
  } else {
    ui->UI_TextColored(1.0f, 0.5f, 0.0f, 1.0f, "Memory font not loaded yet (queued for next frame).");
  }

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, ICON_FA_IMAGE " Manual Texture Rendering");
  ui->UI_TextWrapped("Demonstrating UI_CreateTextureFromMemory. This image was loaded from a raw byte array in the plugin.");

  if (g_ctx.pluginTexture) {
    ui->UI_Image(g_ctx.pluginTexture, 128.0f, 128.0f);

    char dim_buf[64];
    g_ctx.coreAPI->formatting->Fmt_Format(dim_buf, sizeof(dim_buf), "Memory Texture Size: %d x %d", g_ctx.textureWidth, g_ctx.textureHeight);
    ui->UI_TextDisabled(dim_buf);
  } else {
    ui->UI_TextColored(1.0f, 0.0f, 0.0f, 1.0f, "Failed to load memory texture.");
  }

  ui->UI_Spacing();
  ui->UI_Text("File-based Texture Rendering:");
  if (g_ctx.pluginFileTexture) {
    ui->UI_Image(g_ctx.pluginFileTexture, 348.0f, 236.0f);
    char dim_buf[64];
    g_ctx.coreAPI->formatting->Fmt_Format(dim_buf, sizeof(dim_buf), "File Texture Size: %d x %d", g_ctx.fileTextureWidth, g_ctx.fileTextureHeight);
    ui->UI_TextDisabled(dim_buf);
  } else {
    ui->UI_TextColored(1.0f, 0.5f, 0.0f, 1.0f, "File texture not loaded (data\\test.png missing?)");
  }

  ui->UI_Spacing();
  ui->UI_TextStyled(separator_style, "Screen Transition API Test");
  ui->UI_TextWrapped("Test the cinematic screen transitions implemented in the framework.");

  if (ui->UI_Button("Fade To Black (2s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_FADE, 2.0f, false, SPF_TRANS_COLOR_BLACK);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Fade From Black (2s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_FADE, 2.0f, true, SPF_TRANS_COLOR_BLACK);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Cross Black (3s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_CROSS, 3.0f, false, SPF_TRANS_COLOR_BLACK);
  }

  if (ui->UI_Button("Fade To White (1s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_FADE, 1.0f, false, SPF_TRANS_COLOR_WHITE);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Flash White (0.5s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_FLASH, 0.5f, false, SPF_TRANS_COLOR_WHITE);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Cross White (2s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_CROSS, 2.0f, false, SPF_TRANS_COLOR_WHITE);
  }

  if (ui->UI_Button("Letterbox IN (1.5s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_LETTERBOX, 1.5f, false, SPF_TRANS_COLOR_BLACK);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Letterbox OUT (1.5s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_LETTERBOX, 1.5f, true, SPF_TRANS_COLOR_BLACK);
  }

  if (ui->UI_Button("Wipe Right (Gray, 1s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_WIPE_RIGHT, 1.0f, false, SPF_TRANS_COLOR_GRAY);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Wipe Down (Sepia, 1s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_WIPE_BOTTOM, 1.0f, false, SPF_TRANS_COLOR_SEPIA);
  }

  if (ui->UI_Button("Shutter H (2s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_SHUTTER_H, 2.0f, false, SPF_TRANS_COLOR_BLACK);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Shutter V (2s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_SHUTTER_V, 2.0f, false, SPF_TRANS_COLOR_BLACK);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Radial (3s)", 0, 0)) {
    ui->UI_PlayTransition(SPF_TRANS_RADIAL, 3.0f, false, SPF_TRANS_COLOR_BLACK);
  }

  // 4. Clean up the style handles (same-frame create/use/destroy pattern)
  ui->UI_Style_Destroy(h1_style);
  ui->UI_Style_Destroy(centered_text_style);
  ui->UI_Style_Destroy(separator_style);
  ui->UI_Style_Destroy(markdown_base_style);
}

}  // namespace ExamplePlugin
