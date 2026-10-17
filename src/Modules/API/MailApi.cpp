#include "SPF/Modules/API/MailApi.hpp"

#include "SPF/Data/GameData/MailService.hpp"
#include "SPF/SPF_API/SPF_Mail_API.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace SPF::Modules::API {

using namespace SPF::Data::GameData;

namespace {
int SafeCopyToBuffer(const std::string& src, char* out_buffer, int buffer_size) {
  if (!out_buffer || buffer_size <= 0) return static_cast<int>(src.size());
  int copyLen = static_cast<int>(src.size());
  if (copyLen >= buffer_size) copyLen = buffer_size - 1;
  std::memcpy(out_buffer, src.c_str(), copyLen);
  out_buffer[copyLen] = '\0';
  return static_cast<int>(src.size());
}

void CopyToField(char* dst, const std::string& src) {
  if (!dst) return;
  size_t copyLen = src.size();
  if (copyLen >= SPF_MAIL_TEXT_MAX) copyLen = SPF_MAIL_TEXT_MAX - 1;
  std::memcpy(dst, src.c_str(), copyLen);
  dst[copyLen] = '\0';
}
}  // namespace

void MailApi::FillMailApi(SPF_Mail_API* mail_api) {
  if (!mail_api) return;

  mail_api->MAIL_IsReady = &T_MAIL_IsReady;
  mail_api->MAIL_RefreshOffsets = &T_MAIL_RefreshOffsets;

  mail_api->MAIL_Send = &T_MAIL_Send;
  mail_api->MAIL_SendCustomMail = &T_MAIL_SendCustomMail;

  mail_api->MAIL_HasMail = &T_MAIL_HasMail;
  mail_api->MAIL_UpdateMail = &T_MAIL_UpdateMail;
  mail_api->MAIL_DeleteMail = &T_MAIL_DeleteMail;

  mail_api->MAIL_GetInboxCount = &T_MAIL_GetInboxCount;
  mail_api->MAIL_GetPendingCount = &T_MAIL_GetPendingCount;
  mail_api->MAIL_GetMailInfo = &T_MAIL_GetMailInfo;
  mail_api->MAIL_GetMailBody = &T_MAIL_GetMailBody;
  mail_api->MAIL_MarkRead = &T_MAIL_MarkRead;
  mail_api->MAIL_DeleteMailByIndex = &T_MAIL_DeleteMailByIndex;

  mail_api->MAIL_GetDeliveredCount = &T_MAIL_GetDeliveredCount;
  mail_api->MAIL_GetDelivered = &T_MAIL_GetDelivered;

  mail_api->MAIL_CreateMailText = &T_MAIL_CreateMailText;
  mail_api->MAIL_GetMailTextCount = &T_MAIL_GetMailTextCount;
  mail_api->MAIL_GetMailTextInfo = &T_MAIL_GetMailTextInfo;

  mail_api->MAIL_CreateMailDef = &T_MAIL_CreateMailDef;
  mail_api->MAIL_GetMailDefCount = &T_MAIL_GetMailDefCount;
  mail_api->MAIL_GetMailDefName = &T_MAIL_GetMailDefName;
}

// --- Lifecycle ---

bool MailApi::T_MAIL_IsReady() { return MailService::GetInstance().IsReady(); }

bool MailApi::T_MAIL_RefreshOffsets() { return MailService::GetInstance().TryFindAllOffsets(); }

// --- Delivery ---

int MailApi::T_MAIL_Send(const char* mail_def_name, const char* const* keys, const char* const* values, int count) {
  MailService::Params params;
  if (keys && values && count > 0) {
    params.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
      if (keys[i] && values[i]) {
        params.emplace_back(keys[i], values[i]);
      }
    }
  }
  return MailService::GetInstance().Send(mail_def_name, params);
}

int MailApi::T_MAIL_SendCustomMail(const char* from, const char* subject, const char* body, const char* owner, const char* tag) {
  return MailService::GetInstance().SendCustomMail(from, subject, body, owner, tag);
}

// --- Custom letter identity management ---

bool MailApi::T_MAIL_HasMail(const char* owner, const char* tag) { return MailService::GetInstance().HasMail(owner, tag); }

int MailApi::T_MAIL_UpdateMail(const char* owner, const char* tag, const char* from, const char* subject, const char* body) {
  return MailService::GetInstance().UpdateMail(owner, tag, from, subject, body);
}

bool MailApi::T_MAIL_DeleteMail(const char* owner, const char* tag) { return MailService::GetInstance().DeleteMail(owner, tag); }

// --- Inbox/pending read ---

int MailApi::T_MAIL_GetInboxCount() { return MailService::GetInstance().GetInboxCount(); }

int MailApi::T_MAIL_GetPendingCount() { return MailService::GetInstance().GetPendingCount(); }

bool MailApi::T_MAIL_GetMailInfo(int index, bool pending, SPF_MAIL_MailInfo* out) {
  if (!out || index < 0) return false;
  MailService::MailInfo info;
  if (!MailService::GetInstance().GetMailInfo(static_cast<size_t>(index), pending, info)) return false;
  out->id = info.id;
  out->read = info.read;
  out->pending = info.pending;
  out->index = static_cast<uint32_t>(info.index);
  CopyToField(out->name, info.name);
  CopyToField(out->subject, info.subject);
  CopyToField(out->owner, info.owner);
  CopyToField(out->tag, info.tag);
  return true;
}

bool MailApi::T_MAIL_GetMailBody(int mail_id, bool pending, char* out_from, char* out_subject, char* out_body, int buffer_size) {
  std::string from;
  std::string subject;
  std::string body;
  if (!MailService::GetInstance().GetMailBody(mail_id, pending, from, subject, body)) return false;
  SafeCopyToBuffer(from, out_from, buffer_size);
  SafeCopyToBuffer(subject, out_subject, buffer_size);
  SafeCopyToBuffer(body, out_body, buffer_size);
  return true;
}

bool MailApi::T_MAIL_MarkRead(int mail_id) { return MailService::GetInstance().MarkRead(mail_id); }

bool MailApi::T_MAIL_DeleteMailByIndex(int index, bool pending) {
  if (index < 0) return false;
  return MailService::GetInstance().DeleteMail(static_cast<size_t>(index), pending);
}

// --- Full delivered enumeration ---

int MailApi::T_MAIL_GetDeliveredCount() {
  std::vector<MailService::MailInstance> list;
  if (!MailService::GetInstance().EnumerateDelivered(list)) return 0;
  return static_cast<int>(list.size());
}

bool MailApi::T_MAIL_GetDelivered(int index, SPF_MAIL_MailInstance* out) {
  if (!out || index < 0) return false;
  std::vector<MailService::MailInstance> list;
  if (!MailService::GetInstance().EnumerateDelivered(list)) return false;
  if (static_cast<size_t>(index) >= list.size()) return false;
  const MailService::MailInstance& m = list[static_cast<size_t>(index)];
  out->id = m.id;
  out->read = m.read;
  out->pending = m.pending;
  out->index = static_cast<uint32_t>(m.index);
  out->category = m.category;
  CopyToField(out->name, m.name);
  CopyToField(out->from, m.from);
  CopyToField(out->subject, m.subject);
  CopyToField(out->body, m.body);
  CopyToField(out->owner, m.owner);
  CopyToField(out->tag, m.tag);
  return true;
}

// --- Runtime mail_text units ---

bool MailApi::T_MAIL_CreateMailText(const char* name, const char* from, const char* subject, const char* body) {
  return MailService::GetInstance().CreateMailText(name, from, subject, body);
}

int MailApi::T_MAIL_GetMailTextCount() {
  std::vector<MailService::MailTextInfo> texts;
  if (!MailService::GetInstance().EnumerateMailTexts(texts)) return 0;
  return static_cast<int>(texts.size());
}

bool MailApi::T_MAIL_GetMailTextInfo(int index, SPF_MAIL_TextInfo* out) {
  if (!out || index < 0) return false;
  std::vector<MailService::MailTextInfo> texts;
  if (!MailService::GetInstance().EnumerateMailTexts(texts)) return false;
  if (static_cast<size_t>(index) >= texts.size()) return false;
  const MailService::MailTextInfo& t = texts[static_cast<size_t>(index)];
  out->nameId = t.nameId;
  CopyToField(out->name, t.name);
  CopyToField(out->from, t.from);
  CopyToField(out->subject, t.subject);
  CopyToField(out->body, t.body);
  return true;
}

// --- Runtime mail defs ---

bool MailApi::T_MAIL_CreateMailDef(const char* mail_text_name, char* out_def_name, int buffer_size) {
  std::string defName;
  if (!MailService::GetInstance().CreateMailDef(mail_text_name, defName)) return false;
  SafeCopyToBuffer(defName, out_def_name, buffer_size);
  return true;
}

int MailApi::T_MAIL_GetMailDefCount() {
  std::vector<std::string> defs;
  if (!MailService::GetInstance().EnumerateMailDefs(defs)) return 0;
  return static_cast<int>(defs.size());
}

int MailApi::T_MAIL_GetMailDefName(int index, char* out_buffer, int buffer_size) {
  if (index < 0) return -1;
  std::vector<std::string> defs;
  if (!MailService::GetInstance().EnumerateMailDefs(defs)) return -1;
  if (static_cast<size_t>(index) >= defs.size()) return -1;
  return SafeCopyToBuffer(defs[static_cast<size_t>(index)], out_buffer, buffer_size);
}

}  // namespace SPF::Modules::API
