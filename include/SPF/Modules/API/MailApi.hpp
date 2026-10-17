#pragma once

#include "SPF/SPF_API/SPF_Mail_API.h"

namespace SPF::Modules::API {
class MailApi {
 public:
  static void FillMailApi(SPF_Mail_API* mail_api);

 private:
  static bool T_MAIL_IsReady();
  static bool T_MAIL_RefreshOffsets();

  static int T_MAIL_Send(const char* mail_def_name, const char* const* keys, const char* const* values, int count);
  static int T_MAIL_SendCustomMail(const char* from, const char* subject, const char* body, const char* owner, const char* tag);

  static bool T_MAIL_HasMail(const char* owner, const char* tag);
  static int T_MAIL_UpdateMail(const char* owner, const char* tag, const char* from, const char* subject, const char* body);
  static bool T_MAIL_DeleteMail(const char* owner, const char* tag);

  static int T_MAIL_GetInboxCount();
  static int T_MAIL_GetPendingCount();
  static bool T_MAIL_GetMailInfo(int index, bool pending, SPF_MAIL_MailInfo* out);
  static bool T_MAIL_GetMailBody(int mail_id, bool pending, char* out_from, char* out_subject, char* out_body, int buffer_size);
  static bool T_MAIL_MarkRead(int mail_id);
  static bool T_MAIL_DeleteMailByIndex(int index, bool pending);

  static int T_MAIL_GetDeliveredCount();
  static bool T_MAIL_GetDelivered(int index, SPF_MAIL_MailInstance* out);

  static bool T_MAIL_CreateMailText(const char* name, const char* from, const char* subject, const char* body);
  static int T_MAIL_GetMailTextCount();
  static bool T_MAIL_GetMailTextInfo(int index, SPF_MAIL_TextInfo* out);

  static bool T_MAIL_CreateMailDef(const char* mail_text_name, char* out_def_name, int buffer_size);
  static int T_MAIL_GetMailDefCount();
  static int T_MAIL_GetMailDefName(int index, char* out_buffer, int buffer_size);
};

}  // namespace SPF::Modules::API
