/**
 * @file ExampleJsonAPI.cpp
 * @brief Implementation of JsonWriter / JsonIO / custom Config context examples.
 *
 * @details JSON BUILDER RULES:
 *   - Parent owns children after Json_SetNode — do not destroy child handles yourself.
 *   - Always Json_DestroyHandle on the root you created (even after Save).
 *   - Json_SaveToFile(path, pretty=true) writes human-readable output.
 *
 * CUSTOM CONFIG CONTEXT:
 *   Cfg_CreateCustomContext(fullPath) → independent SPF_Config_Handle with Cfg_Get/Set/Save.
 *   AutoSave=false means explicit Cfg_Save only (see checkbox in the tab).
 *   Destroy the context on unload so no late Cfg_* runs against freed state.
 */
#include "ExampleJsonAPI.hpp"

#include "SPF/SPF_API/SPF_Config_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

namespace ExamplePlugin {

void JsonAPI_OnUnload() {
  // Framework may also tear contexts down; nulling prevents double-use after unload starts.
  g_ctx.customConfigHandle = nullptr;
}

void RenderCustomJsonTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.jsonWriterAPI || !g_ctx.jsonIOAPI || !g_ctx.coreAPI || !g_ctx.coreAPI->config) {
    ui->UI_Text("JSON Writer/IO or Config API is not available.");
    return;
  }

  auto writer = g_ctx.jsonWriterAPI;
  auto io = g_ctx.jsonIOAPI;
  auto config = g_ctx.coreAPI->config;

  ui->UI_TextWrapped("This tab demonstrates creating, saving, and managing custom JSON files through the new API.");
  ui->UI_Separator();

  // --- Part 1: JSON Builder & IO ---
  if (ui->UI_TreeNode("1. JSON Builder & Serialization")) {
    static char jsonOutput[1024] = "";
    char dataDir[512];
    g_ctx.environmentAPI->Env_GetPluginDataDir(g_ctx.environmentHandle, dataDir, sizeof(dataDir));

    static char fileName[128] = "custom_test.json";
    char fullPath[1024];
    g_ctx.coreAPI->formatting->Fmt_Format(fullPath, sizeof(fullPath), "%s\\%s", dataDir, fileName);

    ui->UI_InputText("File Name", fileName, sizeof(fileName), SPF_INPUT_TEXT_FLAG_NONE);
    ui->UI_InputText("Full Path (Read Only)", fullPath, sizeof(fullPath), SPF_INPUT_TEXT_FLAG_READ_ONLY);

    if (ui->UI_Button("Create & Save Test JSON", 0, 0)) {
      // Build: { "plugin": "ExamplePlugin", "data": { "value": 123, "active": true }, "tags": ["test", "api"] }
      SPF_JsonValue_Handle* root = writer->Json_CreateObject();
      writer->Json_SetString(root, "plugin", PLUGIN_NAME);

      SPF_JsonValue_Handle* dataNode = writer->Json_CreateObject();
      writer->Json_SetInt(dataNode, "value", 123);
      writer->Json_SetBool(dataNode, "active", true);
      writer->Json_SetNode(root, "data", dataNode);

      SPF_JsonValue_Handle* tagsArray = writer->Json_CreateArray();
      writer->Json_ArrayAppendString(tagsArray, "test");
      writer->Json_ArrayAppendString(tagsArray, "api");
      writer->Json_SetNode(root, "tags", tagsArray);

      if (io->Json_SaveToFile(root, fullPath, true)) {
        io->Json_ToString(root, true, jsonOutput, sizeof(jsonOutput));
        SPF_Notification_Params p = {SPF_NOTIFICATION_SUCCESS, "JSON created and saved successfully!", SPF_NOTIF_MODE_TOP, 3.0f};
        ui->UI_ShowNotification(&p);
      }

      // Destroy root only — children are owned by the tree.
      writer->Json_DestroyHandle(root);
    }

    if (jsonOutput[0] != '\0') {
      ui->UI_Text("Generated JSON:");
      ui->UI_InputTextMultiline("##JsonPreview", jsonOutput, sizeof(jsonOutput), 0, 150, SPF_INPUT_TEXT_FLAG_READ_ONLY);
    }

    ui->UI_TreePop();
  }

  // --- Part 2: Custom Config Context ---
  if (ui->UI_TreeNode("2. Custom Config Context")) {
    char dataDir[512];
    g_ctx.environmentAPI->Env_GetPluginDataDir(g_ctx.environmentHandle, dataDir, sizeof(dataDir));

    static char configFileName[128] = "custom_config.json";
    char fullConfigPath[1024];
    g_ctx.coreAPI->formatting->Fmt_Format(fullConfigPath, sizeof(fullConfigPath), "%s\\%s", dataDir, configFileName);

    ui->UI_InputText("Config File Name", configFileName, sizeof(configFileName), SPF_INPUT_TEXT_FLAG_NONE);
    ui->UI_InputText("Full Config Path (Read Only)", fullConfigPath, sizeof(fullConfigPath), SPF_INPUT_TEXT_FLAG_READ_ONLY);

    if (ui->UI_Button("Open as Config Context", 0, 0)) {
      g_ctx.customConfigHandle = config->Cfg_CreateCustomContext(fullConfigPath);
      if (g_ctx.customConfigHandle) {
        SPF_Notification_Params p = {SPF_NOTIFICATION_SUCCESS, "Custom config context created!", SPF_NOTIF_MODE_TOP, 3.0f};
        ui->UI_ShowNotification(&p);
      }
    }

    // Guard: only draw controls while the context is open (may have been closed on unload path).
    if (g_ctx.customConfigHandle) {
      ui->UI_Separator();
      ui->UI_TextColored(0.4f, 1.0f, 0.4f, 1.0f, "Context Active!");

      // Key/value access identical to the plugin's main settings.json handle.
      static int myInt = 0;
      myInt = config->Cfg_GetInt32(g_ctx.customConfigHandle, "ui.test_value", 0);
      if (ui->UI_SliderInt("Value in Custom File", &myInt, 0, 100, "%d", SPF_SLIDER_FLAG_NONE)) {
        config->Cfg_SetInt32(g_ctx.customConfigHandle, "ui.test_value", myInt);
      }

      if (ui->UI_Checkbox("Auto Save", &g_ctx.isCustomConfigAutoSave)) {
        config->Cfg_SetAutoSave(g_ctx.customConfigHandle, g_ctx.isCustomConfigAutoSave);
      }

      // AutoSave=false → require explicit save so students see both modes.
      if (!g_ctx.isCustomConfigAutoSave) {
        if (ui->UI_Button("Manual Save", 0, 0)) {
          config->Cfg_Save(g_ctx.customConfigHandle);
        }
      }
    }

    ui->UI_TreePop();
  }
}

}  // namespace ExamplePlugin
