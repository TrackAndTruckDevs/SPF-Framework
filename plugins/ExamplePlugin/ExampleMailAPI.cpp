/**
 * @file ExampleMailAPI.cpp
 * @brief Implementation of the SPF Mail API example.
 *
 * @details THREE FLOWS:
 *   - Send: custom letter (owner/tag identity), mail_data def letter, runtime text+def letter.
 *   - Read: inbox/pending counts, per-entry metadata, composed from/subject/body viewer.
 *   - Delete: by array index (both queues) and by (owner, tag) identity.
 *
 * WHY identity over id: game mail ids shift on delete and the pair (owner, tag)
 * persists in the save — reconcile by identity after reload, not by id.
 */
#include "ExampleMailAPI.hpp"

#include "SPF/SPF_API/SPF_Formatting_API.h"
#include "SPF/SPF_API/SPF_Mail_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstdarg>
#include <cstdio>

namespace ExamplePlugin {

namespace {

// --- Send custom letter inputs ---
char s_from[128] = "Example Fleet Office";
char s_subject[192] = "Hello from SPF";
char s_body[1024] = "This is a custom letter delivered by ExamplePlugin.";
char s_owner[128] = "ExamplePlugin";
char s_tag[128] = "demo_letter";

// --- Status / last action ---
char s_status[256] = "";

// --- Def-based send ---
int s_selectedDef = -1;

// --- Runtime mail_text + mail_def inputs ---
char s_rtName[192] = "mailtext.example_note";
char s_rtFrom[128] = "Logistics";
char s_rtSubject[192] = "Runtime note";
char s_rtBody[512] = "Created at runtime via MAIL_CreateMailText + MAIL_CreateMailDef.";
char s_rtDefName[512] = "";

// --- Composed body viewer ---
bool s_showBody = false;
int s_viewId = -1;
bool s_viewPending = false;
char s_viewFrom[2048] = "";
char s_viewSubject[2048] = "";
char s_viewBody[2048] = "";

void SetStatus(SPF_Formatting_API* format, const char* fmt, ...) {
  if (!format) return;
  va_list args;
  va_start(args, fmt);
  // Fmt_Format has no va_list variant — format into a temp via vsnprintf.
  vsnprintf(s_status, sizeof(s_status), fmt, args);
  va_end(args);
}

void RenderMailList(SPF_UI_API* ui, SPF_Mail_API* mail, bool pending) {
  auto format = g_ctx.coreAPI->formatting;
  const char* queueName = pending ? "Pending Queue" : "Inbox";
  int count = pending ? mail->MAIL_GetPendingCount() : mail->MAIL_GetInboxCount();

  ui->UI_Text(queueName);
  if (count <= 0) {
    ui->UI_Text("  (empty)");
    return;
  }

  for (int i = 0; i < count; ++i) {
    SPF_MAIL_MailInfo info;
    if (!mail->MAIL_GetMailInfo(i, pending, &info)) continue;

    char line[512];
    format->Fmt_Format(line, sizeof(line), "[%d] %s (%s)%s", info.id, info.subject, info.read ? "Read" : "Unread", info.owner[0] ? " [custom]" : "");
    ui->UI_Text(line);

    ui->UI_PushID_Int(i);

    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Body", 0, 0)) {
      if (mail->MAIL_GetMailBody(info.id, pending, s_viewFrom, s_viewSubject, s_viewBody, sizeof(s_viewFrom))) {
        s_viewId = info.id;
        s_viewPending = pending;
        s_showBody = true;
      } else {
        SetStatus(format, "GetMailBody failed for id=%d", info.id);
      }
    }

    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Mark Read", 0, 0)) {
      SetStatus(format, "MarkRead(%d): %s", info.id, mail->MAIL_MarkRead(info.id) ? "OK" : "FAIL");
    }

    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Del Idx", 0, 0)) {
      SetStatus(format, "DeleteMailByIndex(%d): %s", i, mail->MAIL_DeleteMailByIndex(i, pending) ? "OK" : "FAIL");
    }

    if (info.owner[0]) {
      ui->UI_SameLine(0, 5);
      if (ui->UI_Button("Del Own", 0, 0)) {
        SetStatus(format, "DeleteMail('%s','%s'): %s", info.owner, info.tag, mail->MAIL_DeleteMail(info.owner, info.tag) ? "OK" : "FAIL");
      }
    }

    ui->UI_PopID();
  }
}

}  // namespace

void RenderMailTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.mailAPI || !g_ctx.coreAPI) {
    ui->UI_Text("Mail API is not available.");
    return;
  }

  auto mail = g_ctx.mailAPI;
  auto format = g_ctx.coreAPI->formatting;

  if (!mail->MAIL_IsReady()) {
    ui->UI_Text("Mail Service is not ready.");
    if (ui->UI_Button("Refresh Offsets", 0, 0)) {
      SetStatus(format, "RefreshOffsets: %s", mail->MAIL_RefreshOffsets() ? "OK" : "FAIL");
    }
    if (s_status[0]) ui->UI_Text(s_status);
    return;
  }

  ui->UI_Text("This tab demonstrates delivering, reading and deleting letters.");
  if (s_status[0]) ui->UI_Text(s_status);
  ui->UI_Separator();

  // --- Section 1: Send custom letter (owner/tag identity) ---
  ui->UI_Text("Send Custom Letter:");

  ui->UI_InputText("From", s_from, sizeof(s_from), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_InputText("Subject", s_subject, sizeof(s_subject), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_InputTextMultiline("Body", s_body, sizeof(s_body), 0.0f, 100.0f, SPF_INPUT_TEXT_FLAG_CTRL_ENTER_FOR_NEW_LINE);
  ui->UI_InputText("Owner", s_owner, sizeof(s_owner), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_InputText("Tag", s_tag, sizeof(s_tag), SPF_INPUT_TEXT_FLAG_NONE);

  if (ui->UI_Button("Send Custom Letter", 0, 0)) {
    int id = mail->MAIL_SendCustomMail(s_from, s_subject, s_body, s_owner, s_tag);
    SetStatus(format, "SendCustomMail -> id=%d", id);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Send + Mark Read", 0, 0)) {
    int id = mail->MAIL_SendCustomMail(s_from, s_subject, s_body, s_owner, s_tag);
    if (id >= 0) mail->MAIL_MarkRead(id);
    SetStatus(format, "SendCustomMail+MarkRead -> id=%d", id);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("HasMail?", 0, 0)) {
    SetStatus(format, "HasMail('%s','%s'): %s", s_owner, s_tag, mail->MAIL_HasMail(s_owner, s_tag) ? "YES" : "NO");
  }

  ui->UI_Separator();

  // --- Section 2: Send a letter from an existing mail_data def ---
  ui->UI_Text("Send mail_data Def:");

  int defCount = mail->MAIL_GetMailDefCount();
  char defPreview[512] = "Select def...";
  if (s_selectedDef >= 0 && s_selectedDef < defCount) {
    mail->MAIL_GetMailDefName(s_selectedDef, defPreview, sizeof(defPreview));
  }

  if (ui->UI_BeginCombo("Mail Def", defPreview, SPF_COMBO_FLAG_NONE)) {
    for (int i = 0; i < defCount; ++i) {
      char itemName[512];
      if (mail->MAIL_GetMailDefName(i, itemName, sizeof(itemName)) < 0) continue;
      if (ui->UI_Selectable(itemName, s_selectedDef == i, SPF_SELECTABLE_FLAG_NONE, 0.0f, 0.0f)) {
        s_selectedDef = i;
      }
    }
    ui->UI_EndCombo();
  }

  if (ui->UI_Button("Send Def Letter", 0, 0)) {
    char defName[512];
    if (s_selectedDef >= 0 && s_selectedDef < defCount && mail->MAIL_GetMailDefName(s_selectedDef, defName, sizeof(defName)) >= 0) {
      int id = mail->MAIL_Send(defName, nullptr, nullptr, 0);
      SetStatus(format, "Send('%s') -> id=%d", defName, id);
    } else {
      SetStatus(format, "No def selected");
    }
  }

  ui->UI_Separator();

  // --- Section 3: Runtime mail_text + mail_def + send ---
  ui->UI_Text("Runtime mail_text + mail_def:");

  ui->UI_InputText("Text Name", s_rtName, sizeof(s_rtName), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_InputText("From", s_rtFrom, sizeof(s_rtFrom), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_InputText("Subject", s_rtSubject, sizeof(s_rtSubject), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_InputText("Body", s_rtBody, sizeof(s_rtBody), SPF_INPUT_TEXT_FLAG_NONE);

  if (ui->UI_Button("Create Text+Def and Send", 0, 0)) {
    if (!mail->MAIL_CreateMailText(s_rtName, s_rtFrom, s_rtSubject, s_rtBody)) {
      SetStatus(format, "CreateMailText('%s') failed", s_rtName);
    } else if (!mail->MAIL_CreateMailDef(s_rtName, s_rtDefName, sizeof(s_rtDefName))) {
      SetStatus(format, "CreateMailDef('%s') failed", s_rtName);
    } else {
      int id = mail->MAIL_Send(s_rtDefName, nullptr, nullptr, 0);
      SetStatus(format, "Runtime letter '%s' -> id=%d", s_rtDefName, id);
    }
  }

  ui->UI_Separator();

  // --- Section 4: Counts ---
  ui->UI_Text("Mailbox:");

  char counts[256];
  format->Fmt_Format(counts, sizeof(counts), "  Inbox: %d | Pending: %d | Delivered: %d", mail->MAIL_GetInboxCount(), mail->MAIL_GetPendingCount(), mail->MAIL_GetDeliveredCount());
  ui->UI_Text(counts);

  ui->UI_Separator();

  // --- Section 5: Read / delete (both queues) ---
  RenderMailList(ui, mail, false);
  ui->UI_Separator();
  RenderMailList(ui, mail, true);

  ui->UI_Separator();

  // --- Section 6: Composed body viewer ---
  ui->UI_Text("Letter Viewer:");
  if (!s_showBody) {
    ui->UI_Text("  Press \"Body\" on an entry above.");
    return;
  }

  char viewerHeader[128];
  format->Fmt_Format(viewerHeader, sizeof(viewerHeader), "  id=%d (%s)", s_viewId, s_viewPending ? "pending" : "inbox");
  ui->UI_Text(viewerHeader);

  ui->UI_InputText("Viewer From", s_viewFrom, sizeof(s_viewFrom), SPF_INPUT_TEXT_FLAG_READ_ONLY);
  ui->UI_InputText("Viewer Subject", s_viewSubject, sizeof(s_viewSubject), SPF_INPUT_TEXT_FLAG_READ_ONLY);
  ui->UI_InputTextMultiline("Viewer Body", s_viewBody, sizeof(s_viewBody), 0.0f, 140.0f,
                            (SPF_InputTextFlags)(SPF_INPUT_TEXT_FLAG_READ_ONLY | SPF_INPUT_TEXT_FLAG_NO_HORIZONTAL_SCROLL));

  if (ui->UI_Button("Close Viewer", 0, 0)) {
    s_showBody = false;
  }
}

}  // namespace ExamplePlugin
