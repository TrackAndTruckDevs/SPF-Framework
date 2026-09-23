/**
 * @file ExampleJsonAPI.hpp
 * @brief Complete example of SPF JsonWriter / JsonIO / custom Config contexts — plugin data files.
 *
 * @details Two complementary demos under the Custom JSON tab:
 *   1. Build a JSON tree in memory (Json_Create / Json_Set) and save it with Json_SaveToFile.
 *   2. Open any JSON file as an SPF_Config_Handle (Cfg_CreateCustomContext) for key/value access.
 *
 * DEVELOPER NOTE: Use JsonWriter for structured data you own;
 * use custom config context when you want the familiar Cfg_Get / Cfg_Set API
 * over a non-default file.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Destroys g_ctx.customConfigHandle if still open. Called from OnUnload.
 * After this, the UI must not call Cfg_* on the old handle.
 */
void JsonAPI_OnUnload();

/**
 * @brief Renders the Custom JSON tab: JSON builder + custom config context demo.
 * @details Needs jsonWriterAPI, jsonIOAPI, environment (data dir), and config API.
 */
void RenderCustomJsonTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
