/**
 * @file ExampleMailAPI.hpp
 * @brief Complete example of the SPF Mail API — send, read, delete letters.
 *
 * @details Mail covers two delivery paths: def-based letters (mail_data defs with
 * placeholder substitution) and fully custom letters (from/subject/body) that carry
 * a plugin-chosen (owner, tag) identity pair surviving save/reload.
 *
 * DEVELOPER NOTE: For "deliver a letter" or "read the mailbox", start here.
 * MAIL_IsReady() is false until the world is loaded — gate all MAIL_* calls on it.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Renders the Mail tab: send custom/def letters, read inbox/pending, delete.
 * @details Requires g_ctx.mailAPI (cached in OnActivated) and MAIL_IsReady().
 */
void RenderMailTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
