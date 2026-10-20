#include "SPF/Data/GameData/MailService.hpp"

#include "SPF/Data/GameData/EconomyService.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Hooks/GameTools/PrismArrayResolver.hpp"
#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"
#include "SPF/Hooks/GameTools/ScsNameResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/Windows.hpp"  // IWYU pragma: keep

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <intrin.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <winnt.h>

namespace SPF::Data::GameData {

namespace {
// mail_ctrl layout discovered via MailDataFinder reflection attribute scan:
//   inbox base, last_id, unread_count, pending_mails, pmail_timers.
// mail_def_u layout discovered via MailDataFinder reflection attribute scan:
//   id, name(mail_text_ref), read, category, param_keys, param_values.
// Offsets stored as m_* members, assigned in Initialize() from finder.
constexpr size_t kMailEntryStride = 8;
constexpr size_t kMaxMailCount = 2000;  // race protection cap (game thread mutates)

using EmptyCtrlFn = uintptr_t (*)();
using PushFn = char (*)(uintptr_t array, uint32_t count, uintptr_t src);
using ArrayDtorFn = char (*)(uintptr_t array);
using CreateAndDeliverMailFn = int (*)(uintptr_t mailCtrl, uintptr_t request);
// Compose(mailCtrl+0x10, from*, subject*, body*(nullable), mailDef*)
// Outputs must be initialized prism_strings (InitEmpty) before the call.
using ComposeFn = void (*)(uintptr_t mailCtrlInbox, uintptr_t from, uintptr_t subject, uintptr_t body, uintptr_t mailDef);
// RemoveAtIndex(arrayBase, index): erases entry at index (refcount-managed).
using RemoveAtIndexFn = uintptr_t (*)(uintptr_t arrayBase, size_t index);
// LoadLocale(lang): loads + registers mail text units from /locale/%s/mails.sii.
// lang = nullptr -> uses the game's current language global (PTR_DAT_142d89060).
using LoadLocaleFn = void (*)(const char* lang);

// Unit system index sentinel - etalon (ConfirmBankDialogAction 141255fc4) fails when == 4.
// Filter-collector rebuild guard: count/capacity are qwords, cap runaway reads.
constexpr uint64_t kMaxMailTextCount = 4096;

// factory(engine) -> new empty mail_text unit (name_id = 0).
using MailTextFactoryFn = uintptr_t (*)(uintptr_t engine);
// registerName(unit, &name) -> nonzero on success; writes name_id at unit+0x0C.
using RegisterMailTextNameFn = uintptr_t (*)(uintptr_t unit, const char** name);
// mail_def factory(engine) -> new empty mail_def_u (same engine param as mail_text).
using MailDefFactoryFn = uintptr_t (*)(uintptr_t engine);

void InitArray(uint8_t* array, uintptr_t emptyCtrlAddr, uintptr_t vtableAddr, int32_t sharedCtrlInnerOff, int32_t ctrlRefcountOff, uint64_t ctrlRefcountInc, int32_t arraySizeOff, int32_t arrayDataOff, int32_t arrayCapOff, int32_t arrayCtrlOff) {
  *reinterpret_cast<uintptr_t*>(array + arrayDataOff) = 0;
  *reinterpret_cast<uint64_t*>(array + arraySizeOff) = 0;
  *reinterpret_cast<uint64_t*>(array + arrayCapOff) = 0;

  auto emptyCtrl = reinterpret_cast<EmptyCtrlFn>(emptyCtrlAddr);
  uintptr_t shared = emptyCtrl();
  uintptr_t ctrl = *reinterpret_cast<uintptr_t*>(shared + sharedCtrlInnerOff);
  *reinterpret_cast<uintptr_t*>(array + arrayCtrlOff) = ctrl;
  _InterlockedExchangeAdd64(reinterpret_cast<volatile __int64*>(ctrl + ctrlRefcountOff), static_cast<__int64>(ctrlRefcountInc));
  *reinterpret_cast<uintptr_t*>(array) = vtableAddr;
}

// Custom mail scaffold: one shared mail_text template + mail_def for all
// plugin-authored mails; per-instance keys/values carry the actual content.
constexpr const char* kCustomMailTextName = "mailtext.spf";
constexpr const char* kCustomMailDefName = "mail.spf";
// Instance keys double as Compose placeholders (keys are literal search
// strings); user-supplied text is stripped of them before delivery.
constexpr const char* kSpfKeyFrom = "$SPF_M_FROM$";
constexpr const char* kSpfKeySubject = "$SPF_M_SUBJ$";
constexpr const char* kSpfKeyBody = "$SPF_M_BODY$";
constexpr const char* kSpfKeyOwner = "$SPF_M_OWNER$";
constexpr const char* kSpfKeyTag = "$SPF_M_TAG$";

std::string StripMailPlaceholders(const char* text) {
  std::string out = text ? text : "";
  const char* keys[] = {kSpfKeyFrom, kSpfKeySubject, kSpfKeyBody, kSpfKeyOwner, kSpfKeyTag};
  for (const char* key : keys) {
    const size_t keyLen = std::char_traits<char>::length(key);
    size_t pos = 0;
    while ((pos = out.find(key, pos)) != std::string::npos) {
      out.erase(pos, keyLen);
    }
  }
  return out;
}

}  // namespace

namespace {

// Reads one key->value pair from a delivered letter instance's key/value arrays.
// Instance keys are literal placeholders ("$SPF_M_FROM$" etc.) copied from the
// MailRequest at delivery; Compose substitutes the paired values into the shared
// mail_text template at render time. Returns "" for unreadable instances or
// absent keys (game letters carry only their own params, no SPF_* keys).
std::string GetInstanceValue(Hooks::GameTools::PrismStringResolver& strRes, uintptr_t inst, const char* key, int32_t instKeysBase, int32_t instValuesBase, int32_t arraySizeOff, int32_t arrayDataOff, int32_t tempPrismSize) {
  if (!inst || !key) {
    return "";
  }
  const uintptr_t keysBase = inst + instKeysBase;
  const uintptr_t valuesBase = inst + instValuesBase;
  if (IsBadReadPtr(reinterpret_cast<void*>(keysBase + arrayDataOff), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(keysBase + arraySizeOff), sizeof(uint64_t)) ||
      IsBadReadPtr(reinterpret_cast<void*>(valuesBase + arrayDataOff), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(valuesBase + arraySizeOff), sizeof(uint64_t))) {
    return "";
  }

  const uint64_t keyCount = *reinterpret_cast<uint64_t*>(keysBase + arraySizeOff);
  const uint64_t valueCount = *reinterpret_cast<uint64_t*>(valuesBase + arraySizeOff);
  if (keyCount > kMaxMailCount || valueCount > kMaxMailCount) {
    return "";
  }
  const uintptr_t keysData = *reinterpret_cast<uintptr_t*>(keysBase + arrayDataOff);
  const uintptr_t valuesData = *reinterpret_cast<uintptr_t*>(valuesBase + arrayDataOff);
  if (!keysData || !valuesData) {
    return "";
  }

  const uint64_t count = keyCount < valueCount ? keyCount : valueCount;
  for (uint64_t i = 0; i < count; ++i) {
    const uintptr_t keyElem = keysData + i * tempPrismSize;
    if (IsBadReadPtr(reinterpret_cast<void*>(keyElem), tempPrismSize)) {
      continue;
    }
    const char* keyBuf = strRes.GetBuffer(reinterpret_cast<uint8_t*>(keyElem));
    if (!keyBuf || std::strcmp(keyBuf, key) != 0) {
      continue;
    }
    const uintptr_t valueElem = valuesData + i * tempPrismSize;
    if (IsBadReadPtr(reinterpret_cast<void*>(valueElem), tempPrismSize)) {
      return "";
    }
    const char* valueBuf = strRes.GetBuffer(reinterpret_cast<uint8_t*>(valueElem));
    return (valueBuf && !IsBadReadPtr(valueBuf, 1)) ? valueBuf : "";
  }
  return "";
}

// Copies every key->value pair of the letter instance into @p out. Custom
// letters expose owner/tag identity this way; game letters expose their own
// delivery params ($CITY$, $BRAND$, ...).
void CollectInstancePairs(Hooks::GameTools::PrismStringResolver& strRes, uintptr_t inst, std::vector<std::pair<std::string, std::string>>& out, int32_t instKeysBase, int32_t instValuesBase, int32_t arraySizeOff, int32_t arrayDataOff, int32_t tempPrismSize) {
  out.clear();
  if (!inst) {
    return;
  }
  const uintptr_t keysBase = inst + instKeysBase;
  const uintptr_t valuesBase = inst + instValuesBase;
  if (IsBadReadPtr(reinterpret_cast<void*>(keysBase + arrayDataOff), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(keysBase + arraySizeOff), sizeof(uint64_t)) ||
      IsBadReadPtr(reinterpret_cast<void*>(valuesBase + arrayDataOff), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(valuesBase + arraySizeOff), sizeof(uint64_t))) {
    return;
  }

  const uint64_t keyCount = *reinterpret_cast<uint64_t*>(keysBase + arraySizeOff);
  const uint64_t valueCount = *reinterpret_cast<uint64_t*>(valuesBase + arraySizeOff);
  if (keyCount > kMaxMailCount || valueCount > kMaxMailCount) {
    return;
  }
  const uintptr_t keysData = *reinterpret_cast<uintptr_t*>(keysBase + arrayDataOff);
  const uintptr_t valuesData = *reinterpret_cast<uintptr_t*>(valuesBase + arrayDataOff);
  if (!keysData || !valuesData) {
    return;
  }

  const uint64_t count = keyCount < valueCount ? keyCount : valueCount;
  out.reserve(static_cast<size_t>(count));
  for (uint64_t i = 0; i < count; ++i) {
    const uintptr_t keyElem = keysData + i * tempPrismSize;
    const uintptr_t valueElem = valuesData + i * tempPrismSize;
    if (IsBadReadPtr(reinterpret_cast<void*>(keyElem), tempPrismSize) || IsBadReadPtr(reinterpret_cast<void*>(valueElem), tempPrismSize)) {
      continue;
    }
    const char* keyBuf = strRes.GetBuffer(reinterpret_cast<uint8_t*>(keyElem));
    if (!keyBuf) {
      continue;
    }
    const char* valueBuf = strRes.GetBuffer(reinterpret_cast<uint8_t*>(valueElem));
    out.emplace_back(keyBuf, (valueBuf && !IsBadReadPtr(valueBuf, 1)) ? valueBuf : "");
  }
}

// Scans inbox then pending for the first letter whose "$SPF_M_OWNER$" and
// "$SPF_M_TAG$" instance keys equal (owner, tag). Returns the instance pointer
// (0 = not found) and fills its position (pending/index/id) for later
// delete/update. Duplicate owner+tag pairs are a plugin data error; the first
// match (inbox order) wins.
uintptr_t FindCustomInstance(uintptr_t mailCtrl, int32_t inboxBase, int32_t pendingBase, Hooks::GameTools::PrismStringResolver& strRes, const char* owner, const char* tag,
                             bool& outPending, size_t& outIndex, int32_t& outId, int32_t mailDefSize, int32_t mailDefIdOff, int32_t instKeysBase, int32_t instValuesBase, int32_t arraySizeOff,
                             int32_t arrayDataOff, int32_t tempPrismSize) {
  outPending = false;
  outIndex = 0;
  outId = -1;
  if (!mailCtrl || !owner || !tag) {
    return 0;
  }

  for (int pass = 0; pass < 2; ++pass) {
    const bool pending = pass == 1;
    const size_t arrayBase = mailCtrl + (pending ? pendingBase : inboxBase);
    const uintptr_t dataPtr = arrayBase + arrayDataOff;
    const uintptr_t sizePtr = arrayBase + arraySizeOff;
    if (IsBadReadPtr(reinterpret_cast<void*>(dataPtr), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) {
      continue;
    }
    const uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
    if (count > kMaxMailCount) {
      continue;
    }
    const uintptr_t data = *reinterpret_cast<uintptr_t*>(dataPtr);
    if (!data) {
      continue;
    }

    for (uint64_t i = 0; i < count; ++i) {
      const uintptr_t entryAddr = data + i * kMailEntryStride;
      if (IsBadReadPtr(reinterpret_cast<void*>(entryAddr), kMailEntryStride)) {
        continue;
      }
      const uintptr_t inst = *reinterpret_cast<uintptr_t*>(entryAddr);
      if (!inst || IsBadReadPtr(reinterpret_cast<void*>(inst), mailDefSize)) {
        continue;
      }
      if (GetInstanceValue(strRes, inst, kSpfKeyOwner, instKeysBase, instValuesBase, arraySizeOff, arrayDataOff, tempPrismSize) != owner) {
        continue;
      }
      if (GetInstanceValue(strRes, inst, kSpfKeyTag, instKeysBase, instValuesBase, arraySizeOff, arrayDataOff, tempPrismSize) != tag) {
        continue;
      }
      outPending = pending;
      outIndex = static_cast<size_t>(i);
      outId = *reinterpret_cast<int32_t*>(inst + mailDefIdOff);
      return inst;
    }
  }
  return 0;
}

}  // namespace

MailService::MailService() {
  WorldServiceRegistry::Get().Register(this, 110);  // finalized after EconomyService (100)
}

MailService& MailService::GetInstance() {
  static MailService instance;
  return instance;
}

void MailService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");
  logger->Info("Attempting to initialize MailService...");
  m_isInitialized = false;
  logger->Info("MailService initialization finished. Waiting for world data.");
}

void MailService::Reset() {
  m_isInitialized = false;
  m_createAndDeliverMailAddr = 0;
  m_composeAddr = 0;
  m_removeAtIndexAddr = 0;
  m_loadLocaleAddr = 0;
  m_mailTextFactoryAddr = 0;
  m_mailDefFactoryAddr = 0;
  m_registerNameAddr = 0;
  m_customMailTexts.clear();
  m_customMailDefs.clear();
  m_unitArrayObjectAddr = 0;
  m_unitArrayDataAddr = 0;
  m_unitArrayCountAddr = 0;
  m_unitArrayCapacityAddr = 0;

  m_finder.Reset();
}

bool MailService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  if (!m_finder.TryFind(*this)) return false;
  // mail_text layout: name_id u32 sits directly before `from`; unit ends after body prism_string.
  m_mailTextNameIdOff = m_mailTextFromOff - static_cast<int32_t>(sizeof(uint32_t));
  m_mailTextSize = m_mailTextBodyOff + m_tempPrismSize;
  m_isInitialized = m_createAndDeliverMailAddr != 0;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");
  if (m_isInitialized) {
    logger->Info("MailService: CreateAndDeliverMail found. Service is READY.");
  } else {
    logger->Error("MailService: CreateAndDeliverMail not found.");
  }
  return m_isInitialized;
}

uintptr_t MailService::GetMailCtrlAddr() const {
  uintptr_t managerEntry = ManagerCoreService::GetInstance().GetEconomyManagerAddr();
  if (!managerEntry) return 0;
  uintptr_t economy = *reinterpret_cast<uintptr_t*>(managerEntry);
  if (!economy) return 0;
  int32_t mailCtxOffset = EconomyService::GetInstance().GetEconomyToMailCtxOffset();
  if (mailCtxOffset == 0) return 0;
  return *reinterpret_cast<uintptr_t*>(economy + mailCtxOffset);
}

int MailService::Send(const char* mailDefName, const Params& params) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");

  if (!mailDefName || mailDefName[0] == '\0') {
    logger->Error("Send: mail def name is empty.");
    return -1;
  }
  if (!m_isInitialized || m_createAndDeliverMailAddr == 0) {
    logger->Error("Send: service is not ready.");
    return -1;
  }

  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  auto& arrRes = Hooks::GameTools::PrismArrayResolver::GetInstance();
  if (!strRes.IsInstalled() || !arrRes.IsInstalled()) {
    logger->Error("Send: prism resolvers are not installed.");
    return -1;
  }

  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) {
    logger->Error("Send: mail controller is null.");
    return -1;
  }

  const size_t requestSize = static_cast<size_t>(m_requestValuesOff) +
                             (static_cast<size_t>(m_requestValuesOff) - static_cast<size_t>(m_requestKeysOff));
  std::vector<uint8_t> request(requestSize, 0);

  uint8_t* name = request.data();
  if (!strRes.InitEmpty(name) || !strRes.Set(name, mailDefName)) {
    logger->Error("Send: failed to build request name for '{}'.", mailDefName);
    return -1;
  }

  uint8_t* keys = request.data() + m_requestKeysOff;
  uint8_t* values = request.data() + m_requestValuesOff;
  InitArray(keys, arrRes.GetEmptyCtrlAddr(), arrRes.GetArrayVtableAddr(), m_sharedCtrlInnerOff, m_ctrlRefcountOff, m_ctrlRefcountInc, m_arraySizeOff, m_arrayDataOff, m_arrayCapFieldOff - m_instKeysBase, m_arrayCtrlFieldOff - m_instKeysBase);
  InitArray(values, arrRes.GetEmptyCtrlAddr(), arrRes.GetArrayVtableAddr(), m_sharedCtrlInnerOff, m_ctrlRefcountOff, m_ctrlRefcountInc, m_arraySizeOff, m_arrayDataOff, m_arrayCapFieldOff - m_instKeysBase, m_arrayCtrlFieldOff - m_instKeysBase);

  auto pushFn = reinterpret_cast<PushFn>(arrRes.GetPushAddr());
  bool ok = true;

  for (const auto& kv : params) {
    uint8_t* slots[2] = {keys, values};
    const std::string* texts[2] = {&kv.first, &kv.second};

    for (int i = 0; i < 2 && ok; ++i) {
      std::vector<uint8_t> tmpPrism(m_tempPrismSize, 0);
      strRes.InitEmpty(tmpPrism.data());
      ok = strRes.Set(tmpPrism.data(), texts[i]->c_str());
      if (ok) {
        uintptr_t src = reinterpret_cast<uintptr_t>(tmpPrism.data()) + strRes.GetBufferOffset();
        ok = pushFn(reinterpret_cast<uintptr_t>(slots[i]), 1, src) != 0;
      }
      strRes.Destroy(tmpPrism.data());
    }
    if (!ok) break;
  }

  int result = -1;
  if (ok) {
    auto createMail = reinterpret_cast<CreateAndDeliverMailFn>(m_createAndDeliverMailAddr);
    result = createMail(mailCtrl, reinterpret_cast<uintptr_t>(request.data()));
  } else {
    logger->Error("Send: failed to push params for '{}'.", mailDefName);
  }

  auto arrayDtor = reinterpret_cast<ArrayDtorFn>(arrRes.GetDtorAddr());
  arrayDtor(reinterpret_cast<uintptr_t>(values));
  arrayDtor(reinterpret_cast<uintptr_t>(keys));
  strRes.Destroy(name);

  return ok ? result : -1;
}

int MailService::SendCustomMail(const char* from, const char* subject, const char* body, const char* owner, const char* tag) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");

  if (!owner || owner[0] == '\0' || !tag || tag[0] == '\0') {
    logger->Error("SendCustomMail: owner and tag must be non-empty.");
    return -1;
  }
  if (!m_isInitialized) {
    logger->Error("SendCustomMail: service is not ready.");
    return -1;
  }
  if (!EnsureCustomMailScaffold()) {
    logger->Error("SendCustomMail: scaffold creation failed.");
    return -1;
  }

  Params params;
  params.emplace_back(kSpfKeyFrom, StripMailPlaceholders(from));
  params.emplace_back(kSpfKeySubject, StripMailPlaceholders(subject));
  params.emplace_back(kSpfKeyBody, StripMailPlaceholders(body));
  params.emplace_back(kSpfKeyOwner, StripMailPlaceholders(owner));
  params.emplace_back(kSpfKeyTag, StripMailPlaceholders(tag));

  const int id = Send(kCustomMailDefName, params);
  if (id < 0) {
    logger->Error("SendCustomMail: delivery failed for owner='{}' tag='{}'.", owner, tag);
  } else {
    logger->Info("SendCustomMail: delivered id={} owner='{}' tag='{}'.", id, owner, tag);
  }
  return id;
}

int MailService::GetInboxCount() const {
  if (!m_isInitialized) return 0;
  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) return 0;

  uintptr_t sizePtr = mailCtrl + m_mailCtrlInboxBase + m_arraySizeOff;
  if (IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) return 0;
  uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
  return count > kMaxMailCount ? 0 : static_cast<int>(count);
}

int MailService::GetPendingCount() const {
  if (!m_isInitialized) return 0;
  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) return 0;

  uintptr_t sizePtr = mailCtrl + m_mailCtrlPendingBase + m_arraySizeOff;
  if (IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) return 0;
  uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
  return count > kMaxMailCount ? 0 : static_cast<int>(count);
}

bool MailService::GetMailInfo(size_t index, bool pending, MailInfo& out) {
  if (!m_isInitialized) return false;

  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) return false;

  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) return false;

  size_t arrayBase = mailCtrl + (pending ? m_mailCtrlPendingBase : m_mailCtrlInboxBase);
  uintptr_t dataPtr = arrayBase + m_arrayDataOff;
  uintptr_t sizePtr = arrayBase + m_arraySizeOff;
  if (IsBadReadPtr(reinterpret_cast<void*>(dataPtr), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) {
    return false;
  }

  uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
  if (count > kMaxMailCount || index >= count) return false;

  uintptr_t data = *reinterpret_cast<uintptr_t*>(dataPtr);
  if (!data) return false;
  uintptr_t entryAddr = data + index * kMailEntryStride;
  if (IsBadReadPtr(reinterpret_cast<void*>(entryAddr), kMailEntryStride)) return false;

  uintptr_t mailDef = *reinterpret_cast<uintptr_t*>(entryAddr);
  if (!mailDef || IsBadReadPtr(reinterpret_cast<void*>(mailDef), m_mailDefReadOff + 1)) return false;

  out.id = *reinterpret_cast<int32_t*>(mailDef + m_mailDefIdOff);
  out.read = *reinterpret_cast<uint8_t*>(mailDef + m_mailDefReadOff) != 0;
  out.pending = pending;
  out.index = index;

  const char* name = strRes.GetBuffer(reinterpret_cast<uint8_t*>(mailDef + m_mailDefNameOff));
  out.name = (name && !IsBadReadPtr(name, 1)) ? name : "";
  // Custom identity keys ride in the letter instance ("" for game letters).
  out.owner = GetInstanceValue(strRes, mailDef, kSpfKeyOwner, m_instKeysBase, m_instValuesBase, m_arraySizeOff, m_arrayDataOff, m_tempPrismSize);
  out.tag = GetInstanceValue(strRes, mailDef, kSpfKeyTag, m_instKeysBase, m_instValuesBase, m_arraySizeOff, m_arrayDataOff, m_tempPrismSize);
  ComposeText(mailDef, nullptr, &out.subject, nullptr);
  return true;
}

bool MailService::ComposeText(uintptr_t mailDef, std::string* from, std::string* subject, std::string* body) {
  if (!m_composeAddr || !mailDef) return false;

  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) return false;

  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) return false;

  // The unit registry is rebuilt on world reload; recreate the custom scaffold
  // so instance+0x18 name resolution does not miss.
  EnsureCustomMailScaffold();

  std::vector<uint8_t> fromPrism(m_tempPrismSize, 0);
  std::vector<uint8_t> subjectPrism(m_tempPrismSize, 0);
  std::vector<uint8_t> bodyPrism(m_tempPrismSize, 0);
  strRes.InitEmpty(fromPrism.data());
  strRes.InitEmpty(subjectPrism.data());
  strRes.InitEmpty(bodyPrism.data());

  auto compose = reinterpret_cast<ComposeFn>(m_composeAddr);

  // Load + register mail text units first, exactly like the game does before
  // rendering the mailbox (FUN_141259d10 <- FUN_141257440 -> FUN_14083e740).
  // Otherwise Compose finds no registered unit and writes "LOCALIZATION MISSING".
  if (m_loadLocaleAddr) {
    auto loadLocale = reinterpret_cast<LoadLocaleFn>(m_loadLocaleAddr);
    const std::string gameLang = GameObjectSessionService::GetInstance().GetGameLanguage();
    loadLocale(gameLang.empty() ? nullptr : gameLang.c_str());
  }

  compose(mailCtrl + m_mailCtrlInboxBase, reinterpret_cast<uintptr_t>(fromPrism.data()), reinterpret_cast<uintptr_t>(subjectPrism.data()), reinterpret_cast<uintptr_t>(bodyPrism.data()), mailDef);

  if (from) {
    const char* buf = strRes.GetBuffer(fromPrism.data());
    *from = (buf && !IsBadReadPtr(buf, 1)) ? buf : "";
  }
  if (subject) {
    const char* buf = strRes.GetBuffer(subjectPrism.data());
    *subject = (buf && !IsBadReadPtr(buf, 1)) ? buf : "";
  }
  if (body) {
    const char* buf = strRes.GetBuffer(bodyPrism.data());
    *body = (buf && !IsBadReadPtr(buf, 1)) ? buf : "";
  }

  strRes.Destroy(fromPrism.data());
  strRes.Destroy(subjectPrism.data());
  strRes.Destroy(bodyPrism.data());
  return true;
}

bool MailService::GetMailBody(int mailId, bool pending, std::string& from, std::string& subject, std::string& body) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");

  if (!m_isInitialized) return false;
  if (!m_composeAddr) {
    logger->Error("GetMailBody: compose function not found.");
    return false;
  }

  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) return false;

  size_t arrayBase = mailCtrl + (pending ? m_mailCtrlPendingBase : m_mailCtrlInboxBase);
  uintptr_t dataPtr = arrayBase + m_arrayDataOff;
  uintptr_t sizePtr = arrayBase + m_arraySizeOff;
  if (IsBadReadPtr(reinterpret_cast<void*>(dataPtr), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) {
    return false;
  }

  uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
  if (count > kMaxMailCount) return false;
  uintptr_t data = *reinterpret_cast<uintptr_t*>(dataPtr);
  if (!data) return false;

  uintptr_t mailDef = 0;
  for (uint64_t i = 0; i < count; ++i) {
    uintptr_t entryAddr = data + i * kMailEntryStride;
    if (IsBadReadPtr(reinterpret_cast<void*>(entryAddr), kMailEntryStride)) continue;
    uintptr_t def = *reinterpret_cast<uintptr_t*>(entryAddr);
    if (!def || IsBadReadPtr(reinterpret_cast<void*>(def), m_mailDefIdOff + sizeof(int32_t))) continue;
    if (*reinterpret_cast<int32_t*>(def + m_mailDefIdOff) == mailId) {
      mailDef = def;
      break;
    }
  }
  if (!mailDef) {
    logger->Error("GetMailBody: mail id {} not found in {}.", mailId, pending ? "pending" : "inbox");
    return false;
  }

  return ComposeText(mailDef, &from, &subject, &body);
}

bool MailService::MarkRead(int mailId) {
  if (!m_isInitialized) return false;

  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) return false;

  uintptr_t dataPtr = mailCtrl + m_mailCtrlInboxBase + m_arrayDataOff;
  uintptr_t sizePtr = mailCtrl + m_mailCtrlInboxBase + m_arraySizeOff;
  if (IsBadReadPtr(reinterpret_cast<void*>(dataPtr), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) {
    return false;
  }

  uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
  if (count > kMaxMailCount) return false;
  uintptr_t data = *reinterpret_cast<uintptr_t*>(dataPtr);
  if (!data) return false;

  for (uint64_t i = 0; i < count; ++i) {
    uintptr_t entryAddr = data + i * kMailEntryStride;
    if (IsBadReadPtr(reinterpret_cast<void*>(entryAddr), kMailEntryStride)) continue;
    uintptr_t mailDef = *reinterpret_cast<uintptr_t*>(entryAddr);
    if (!mailDef || IsBadReadPtr(reinterpret_cast<void*>(mailDef), m_mailDefReadOff + 1)) continue;
    if (*reinterpret_cast<int32_t*>(mailDef + m_mailDefIdOff) != mailId) continue;

    if (*reinterpret_cast<uint8_t*>(mailDef + m_mailDefReadOff) == 0) {
      *reinterpret_cast<uint8_t*>(mailDef + m_mailDefReadOff) = 1;
      uintptr_t unreadPtr = mailCtrl + m_mailCtrlUnreadOff;
      if (!IsBadReadPtr(reinterpret_cast<void*>(unreadPtr), sizeof(int32_t))) {
        int32_t* unread = reinterpret_cast<int32_t*>(unreadPtr);
        if (*unread > 0) --*unread;
      }
    }
    return true;
  }
  return false;
}

bool MailService::DeleteMail(size_t index, bool pending) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");

  if (!m_isInitialized) return false;
  if (!m_removeAtIndexAddr) {
    logger->Error("DeleteMail: remove-at-index function not found.");
    return false;
  }

  uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) return false;

  size_t arrayBase = mailCtrl + (pending ? m_mailCtrlPendingBase : m_mailCtrlInboxBase);
  uintptr_t sizePtr = arrayBase + m_arraySizeOff;
  if (IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) return false;

  uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
  if (count > kMaxMailCount || index >= count) {
    logger->Error("DeleteMail: index {} out of range (count {}).", index, count);
    return false;
  }

  auto removeAt = reinterpret_cast<RemoveAtIndexFn>(m_removeAtIndexAddr);
  removeAt(arrayBase, index);
  return true;
}

}  // namespace SPF::Data::GameData

namespace SPF::Data::GameData {

bool MailService::CreateMailText(const char* name, const char* from, const char* subject, const char* body) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");

  if (!name || name[0] == '\0') {
    logger->Error("CreateMailText: name is empty.");
    return false;
  }
  if (!m_isInitialized) {
    logger->Error("CreateMailText: service is not ready.");
    return false;
  }
  if (m_mailTextFactoryAddr == 0 || m_registerNameAddr == 0) {
    logger->Error("CreateMailText: factory or register-name address is missing.");
    return false;
  }

  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) {
    logger->Error("CreateMailText: prism string resolver is not installed.");
    return false;
  }

  // Same name again: update the unit created by the first call - registering a
  // second unit under a taken name would not rebind the registry to it.
  uintptr_t unit = 0;
  auto cached = m_customMailTexts.find(name);
  if (cached != m_customMailTexts.end()) {
    unit = cached->second;
    if (unit == 0 || IsBadReadPtr(reinterpret_cast<void*>(unit), m_mailTextSize)) {
      logger->Warn("CreateMailText: cached unit for '{}' is stale, recreating.", name);
      m_customMailTexts.erase(cached);
      unit = 0;
    }
  }

  if (unit == 0) {
    // Engine object for the factory: unit system table[idx]
    // (etalon: ConfirmBankDialogAction 141255fb2..141255fdb).
    const uintptr_t indexAddr = EconomyService::GetInstance().GetUnitSystemIndexAddr();
    const uintptr_t tableAddr = EconomyService::GetInstance().GetUnitSystemTableAddr();
    if (indexAddr == 0 || tableAddr == 0 || IsBadReadPtr(reinterpret_cast<void*>(indexAddr), sizeof(int32_t))) {
      logger->Error("CreateMailText: unit system index/table address missing.");
      return false;
    }
    const int32_t unitIndex = *reinterpret_cast<int32_t*>(indexAddr);
    if (unitIndex == m_unitSystemInvalidIndex) {
      logger->Error("CreateMailText: unit system index is invalid.");
      return false;
    }
    const uintptr_t engineSlot = tableAddr + static_cast<uintptr_t>(unitIndex) * sizeof(uintptr_t);
    if (IsBadReadPtr(reinterpret_cast<void*>(engineSlot), sizeof(uintptr_t))) {
      logger->Error("CreateMailText: unit system table entry out of range (index={}).", unitIndex);
      return false;
    }
    const uintptr_t engine = *reinterpret_cast<uintptr_t*>(engineSlot);
    if (engine == 0) {
      logger->Error("CreateMailText: engine object is null (index={}).", unitIndex);
      return false;
    }

    unit = reinterpret_cast<MailTextFactoryFn>(m_mailTextFactoryAddr)(engine);
    if (unit == 0 || IsBadReadPtr(reinterpret_cast<void*>(unit), m_mailTextSize)) {
      logger->Error("CreateMailText: factory returned an invalid unit.");
      return false;
    }

    const char* namePtr = name;
    const uintptr_t registered = reinterpret_cast<RegisterMailTextNameFn>(m_registerNameAddr)(unit, &namePtr);
    if (registered == 0) {
      logger->Error("CreateMailText: name registration rejected for '{}'.", name);
      return false;
    }
    m_customMailTexts.emplace(name, unit);
  }

  if (!strRes.Set(reinterpret_cast<uint8_t*>(unit) + m_mailTextFromOff, from ? from : "") || !strRes.Set(reinterpret_cast<uint8_t*>(unit) + m_mailTextSubjectOff, subject ? subject : "") ||
      !strRes.Set(reinterpret_cast<uint8_t*>(unit) + m_mailTextBodyOff, body ? body : "")) {
    logger->Error("CreateMailText: failed to write fields for '{}'.", name);
    return false;
  }

  logger->Info("CreateMailText: '{}' registered at unit 0x{:X} name_id={} (from='{}' subject='{}').", name, unit, *reinterpret_cast<int32_t*>(unit + m_mailTextNameIdOff), from ? from : "", subject ? subject : "");
  return true;
}

bool MailService::CreateMailDef(const char* mailTextName, std::string& outDefName) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");
  outDefName.clear();

  if (!mailTextName || mailTextName[0] == '\0') {
    logger->Error("CreateMailDef: mail text name is empty.");
    return false;
  }
  if (!m_isInitialized) {
    logger->Error("CreateMailDef: service is not ready.");
    return false;
  }
  if (m_mailDefFactoryAddr == 0 || m_registerNameAddr == 0) {
    logger->Error("CreateMailDef: factory or register-name address is missing.");
    return false;
  }

  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) {
    logger->Error("CreateMailDef: prism string resolver is not installed.");
    return false;
  }

  // Registry groups names by dot-segment count; the def must not share the
  // mail_text name (one hash slot per name): "mailtext.x" -> "mail.x".
  const std::string textName(mailTextName);
  constexpr const char* kTextPrefix = "mailtext.";
  std::string defName = (textName.rfind(kTextPrefix, 0) == 0) ? "mail." + textName.substr(std::char_traits<char>::length(kTextPrefix)) : "mail." + textName;

  uintptr_t def = 0;
  auto cached = m_customMailDefs.find(defName);
  if (cached != m_customMailDefs.end()) {
    def = cached->second;
    if (def == 0 || IsBadReadPtr(reinterpret_cast<void*>(def), m_mailDefSize)) {
      logger->Warn("CreateMailDef: cached def for '{}' is stale, recreating.", defName);
      m_customMailDefs.erase(cached);
      def = 0;
    }
  }

  if (def == 0) {
    // Engine object for the factory: unit system table[idx]
    // (etalon: ConfirmBankDialogAction 141255fb2..141255fdb).
    const uintptr_t indexAddr = EconomyService::GetInstance().GetUnitSystemIndexAddr();
    const uintptr_t tableAddr = EconomyService::GetInstance().GetUnitSystemTableAddr();
    if (indexAddr == 0 || tableAddr == 0 || IsBadReadPtr(reinterpret_cast<void*>(indexAddr), sizeof(int32_t))) {
      logger->Error("CreateMailDef: unit system index/table address missing.");
      return false;
    }
    const int32_t unitIndex = *reinterpret_cast<int32_t*>(indexAddr);
    if (unitIndex == m_unitSystemInvalidIndex) {
      logger->Error("CreateMailDef: unit system index is invalid.");
      return false;
    }
    const uintptr_t engineSlot = tableAddr + static_cast<uintptr_t>(unitIndex) * sizeof(uintptr_t);
    if (IsBadReadPtr(reinterpret_cast<void*>(engineSlot), sizeof(uintptr_t))) {
      logger->Error("CreateMailDef: unit system table entry out of range (index={}).", unitIndex);
      return false;
    }
    const uintptr_t engine = *reinterpret_cast<uintptr_t*>(engineSlot);
    if (engine == 0) {
      logger->Error("CreateMailDef: engine object is null (index={}).", unitIndex);
      return false;
    }

    def = reinterpret_cast<MailDefFactoryFn>(m_mailDefFactoryAddr)(engine);
    if (def == 0 || IsBadReadPtr(reinterpret_cast<void*>(def), m_mailDefSize)) {
      logger->Error("CreateMailDef: factory returned an invalid unit.");
      return false;
    }

    const char* namePtr = defName.c_str();
    const uintptr_t registered = reinterpret_cast<RegisterMailTextNameFn>(m_registerNameAddr)(def, &namePtr);
    if (registered == 0) {
      logger->Error("CreateMailDef: name registration rejected for '{}'.", defName);
      return false;
    }
    m_customMailDefs.emplace(defName, def);
  }

  // id must reference the mail_text unit: CreateAndDeliverMail copies def+0x18
  // into the instance and Compose resolves the text by that name.
  if (!strRes.Set(reinterpret_cast<uint8_t*>(def) + m_mailDefNameOff, mailTextName)) {
    logger->Error("CreateMailDef: failed to write id for '{}'.", defName);
    return false;
  }
  // Factory leaves +0x14..+0x17 uninit; 0 = inbox (mail_ctrl+0x10, unread++).
  *reinterpret_cast<int32_t*>(def + m_mailDefCategoryOff) = 0;

  outDefName = defName;
  logger->Info("CreateMailDef: '{}' registered at unit 0x{:X} -> mail_text '{}'.", defName, def, mailTextName);
  return true;
}

bool MailService::EnsureCustomMailScaffold() {
  if (!m_isInitialized) return false;

  const auto textIt = m_customMailTexts.find(kCustomMailTextName);
  if (textIt != m_customMailTexts.end() && textIt->second != 0 && !IsBadReadPtr(reinterpret_cast<void*>(textIt->second), m_mailTextSize)) {
    const auto defIt = m_customMailDefs.find(kCustomMailDefName);
    if (defIt != m_customMailDefs.end() && defIt->second != 0 && !IsBadReadPtr(reinterpret_cast<void*>(defIt->second), m_mailDefSize)) {
      return true;
    }
  }

  if (!CreateMailText(kCustomMailTextName, kSpfKeyFrom, kSpfKeySubject, kSpfKeyBody)) return false;
  std::string defName;
  if (!CreateMailDef(kCustomMailTextName, defName)) return false;
  if (defName != kCustomMailDefName) {
    Logging::LoggerFactory::GetInstance().GetLogger("MailService")->Error("EnsureCustomMailScaffold: unexpected def name '{}'.", defName);
    return false;
  }
  return true;
}

bool MailService::EnumerateMailTexts(std::vector<MailTextInfo>& out) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");
  out.clear();

  if (!m_isInitialized) {
    logger->Error("EnumerateMailTexts: service is not ready.");
    return false;
  }
  if (m_loadLocaleAddr == 0 || m_unitArrayDataAddr == 0 || m_unitArrayCountAddr == 0 || m_unitArrayCapacityAddr == 0) {
    logger->Error("EnumerateMailTexts: locale loader or unit array addresses are missing.");
    return false;
  }

  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) {
    logger->Error("EnumerateMailTexts: prism string resolver is not installed.");
    return false;
  }

  // Re-run the game's locale pipeline (parse mails.sii -> filter-collector into
  // DAT_142d894b0/4b8/4c0), same as ComposeText does before the mailbox render.
  auto loadLocale = reinterpret_cast<LoadLocaleFn>(m_loadLocaleAddr);
  const std::string gameLang = GameObjectSessionService::GetInstance().GetGameLanguage();
  loadLocale(gameLang.empty() ? nullptr : gameLang.c_str());

  if (IsBadReadPtr(reinterpret_cast<void*>(m_unitArrayCountAddr), sizeof(uint64_t)) || IsBadReadPtr(reinterpret_cast<void*>(m_unitArrayCapacityAddr), sizeof(uint64_t)) ||
      IsBadReadPtr(reinterpret_cast<void*>(m_unitArrayDataAddr), sizeof(uintptr_t))) {
    logger->Error("EnumerateMailTexts: unit array globals are unreadable.");
    return false;
  }

  const uint64_t count = *reinterpret_cast<uint64_t*>(m_unitArrayCountAddr);
  const uint64_t capacity = *reinterpret_cast<uint64_t*>(m_unitArrayCapacityAddr);
  if (count > capacity || count > kMaxMailTextCount) {
    logger->Error("EnumerateMailTexts: implausible unit array state (count={} capacity={}).", count, capacity);
    return false;
  }
  if (count == 0) {
    logger->Info("EnumerateMailTexts: no mail_text units registered (lang='{}').", gameLang);
    return true;
  }

  const uintptr_t data = *reinterpret_cast<uintptr_t*>(m_unitArrayDataAddr);
  if (data == 0 || IsBadReadPtr(reinterpret_cast<void*>(data), static_cast<size_t>(count) * sizeof(uintptr_t))) {
    logger->Error("EnumerateMailTexts: unit array data pointer invalid (count={}).", count);
    return false;
  }

  out.reserve(static_cast<size_t>(count));
  size_t skipped = 0;
  for (uint64_t i = 0; i < count; ++i) {
    const uintptr_t unit = *reinterpret_cast<uintptr_t*>(data + i * sizeof(uintptr_t));
    if (unit == 0 || IsBadReadPtr(reinterpret_cast<void*>(unit), m_mailTextSize)) {
      ++skipped;
      continue;
    }

    MailTextInfo info;
    info.nameId = *reinterpret_cast<int32_t*>(unit + m_mailTextNameIdOff);

    // Custom units map back through the cache; game units resolve name_id via
    // ResolveUnitName into a stack dynstring builder (etalon FUN_14049b120).
    bool namedFromCache = false;
    for (const auto& entry : m_customMailTexts) {
      if (entry.second == unit) {
        info.name = entry.first;
        namedFromCache = true;
        break;
      }
    }
    if (!namedFromCache && info.nameId != 0) {
      // ScsNameResolver owns the ResolveUnitName call + builder plumbing
      // (ClimateService uses the same path); returns "" on failure.
      const std::string resolved = Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(
          static_cast<uint32_t>(info.nameId));
      if (!resolved.empty() && resolved.size() < 512) {
        info.name = resolved;
      }
    }

    const char* from = strRes.GetBuffer(reinterpret_cast<uint8_t*>(unit) + m_mailTextFromOff);
    const char* subject = strRes.GetBuffer(reinterpret_cast<uint8_t*>(unit) + m_mailTextSubjectOff);
    const char* body = strRes.GetBuffer(reinterpret_cast<uint8_t*>(unit) + m_mailTextBodyOff);
    info.from = (from && !IsBadReadPtr(from, 1)) ? from : "";
    info.subject = (subject && !IsBadReadPtr(subject, 1)) ? subject : "";
    info.body = (body && !IsBadReadPtr(body, 1)) ? body : "";
    out.push_back(std::move(info));
  }

  logger->Info("EnumerateMailTexts: {} units ({} skipped, lang='{}').", out.size(), skipped, gameLang);
  return true;
}

bool MailService::EnumerateDelivered(std::vector<MailInstance>& out) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");
  out.clear();

  if (!m_isInitialized) {
    logger->Error("EnumerateDelivered: service is not ready.");
    return false;
  }
  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) {
    logger->Error("EnumerateDelivered: prism string resolver is not installed.");
    return false;
  }
  const uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) {
    logger->Error("EnumerateDelivered: mail controller is not available.");
    return false;
  }

  for (int pass = 0; pass < 2; ++pass) {
    const bool pending = pass == 1;
    const size_t arrayBase = mailCtrl + (pending ? m_mailCtrlPendingBase : m_mailCtrlInboxBase);
    const uintptr_t dataPtr = arrayBase + m_arrayDataOff;
    const uintptr_t sizePtr = arrayBase + m_arraySizeOff;
    if (IsBadReadPtr(reinterpret_cast<void*>(dataPtr), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(sizePtr), sizeof(uint64_t))) {
      logger->Error("EnumerateDelivered: {} array globals are unreadable.", pending ? "pending" : "inbox");
      return false;
    }
    const uint64_t count = *reinterpret_cast<uint64_t*>(sizePtr);
    if (count > kMaxMailCount) {
      logger->Error("EnumerateDelivered: implausible {} array count ({}).", pending ? "pending" : "inbox", count);
      return false;
    }
    const uintptr_t data = *reinterpret_cast<uintptr_t*>(dataPtr);
    if (!data) {
      continue;
    }

    for (uint64_t i = 0; i < count; ++i) {
      const uintptr_t entryAddr = data + i * kMailEntryStride;
      if (IsBadReadPtr(reinterpret_cast<void*>(entryAddr), kMailEntryStride)) {
        continue;
      }
      const uintptr_t inst = *reinterpret_cast<uintptr_t*>(entryAddr);
      if (!inst || IsBadReadPtr(reinterpret_cast<void*>(inst), m_mailDefSize)) {
        continue;
      }

      MailInstance mail;
      mail.id = *reinterpret_cast<int32_t*>(inst + m_mailDefIdOff);
      mail.read = *reinterpret_cast<uint8_t*>(inst + m_mailDefReadOff) != 0;
      mail.pending = pending;
      mail.index = static_cast<size_t>(i);
      mail.category = *reinterpret_cast<int32_t*>(inst + m_mailDefCategoryOff);
      const char* name = strRes.GetBuffer(reinterpret_cast<uint8_t*>(inst + m_mailDefNameOff));
      mail.name = (name && !IsBadReadPtr(name, 1)) ? name : "";
      ComposeText(inst, &mail.from, &mail.subject, &mail.body);
      CollectInstancePairs(strRes, inst, mail.keys, m_instKeysBase, m_instValuesBase, m_arraySizeOff, m_arrayDataOff, m_tempPrismSize);
      for (const auto& kv : mail.keys) {
        if (kv.first == kSpfKeyOwner) {
          mail.owner = kv.second;
        } else if (kv.first == kSpfKeyTag) {
          mail.tag = kv.second;
        }
      }
      out.push_back(std::move(mail));
    }
  }

  logger->Info("EnumerateDelivered: {} letters.", out.size());
  return true;
}

bool MailService::HasMail(const char* owner, const char* tag) {
  if (!m_isInitialized || !owner || owner[0] == '\0' || !tag || tag[0] == '\0') {
    return false;
  }
  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) {
    return false;
  }
  const uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) {
    return false;
  }
  bool pending = false;
  size_t index = 0;
  int32_t id = -1;
  return FindCustomInstance(mailCtrl, m_mailCtrlInboxBase, m_mailCtrlPendingBase, strRes, owner, tag, pending, index, id, m_mailDefSize, m_mailDefIdOff, m_instKeysBase, m_instValuesBase, m_arraySizeOff, m_arrayDataOff, m_tempPrismSize) != 0;
}

int MailService::UpdateMail(const char* owner, const char* tag, const char* from, const char* subject, const char* body) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");

  if (!owner || owner[0] == '\0' || !tag || tag[0] == '\0') {
    logger->Error("UpdateMail: owner and tag must be non-empty.");
    return -1;
  }
  if (!m_isInitialized) {
    logger->Error("UpdateMail: service is not ready.");
    return -1;
  }
  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) {
    logger->Error("UpdateMail: prism string resolver is not installed.");
    return -1;
  }
  const uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) {
    logger->Error("UpdateMail: mail controller is not available.");
    return -1;
  }

  bool pending = false;
  size_t index = 0;
  int32_t id = -1;
  const uintptr_t inst = FindCustomInstance(mailCtrl, m_mailCtrlInboxBase, m_mailCtrlPendingBase, strRes, owner, tag, pending, index, id, m_mailDefSize, m_mailDefIdOff, m_instKeysBase, m_instValuesBase, m_arraySizeOff, m_arrayDataOff, m_tempPrismSize);
  if (!inst) {
    logger->Error("UpdateMail: no letter with owner='{}' tag='{}'.", owner, tag);
    return -1;
  }

  // Same sanitization as SendCustomMail: user text must not contain the
  // template placeholders (Compose substitutes them in a single pass).
  const std::string texts[3] = {
      StripMailPlaceholders(from),
      StripMailPlaceholders(subject),
      StripMailPlaceholders(body),
  };
  const char* wantedKeys[3] = {kSpfKeyFrom, kSpfKeySubject, kSpfKeyBody};

  const uintptr_t keysBase = inst + m_instKeysBase;
  const uintptr_t valuesBase = inst + m_instValuesBase;
  if (IsBadReadPtr(reinterpret_cast<void*>(keysBase + m_arrayDataOff), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(keysBase + m_arraySizeOff), sizeof(uint64_t)) ||
      IsBadReadPtr(reinterpret_cast<void*>(valuesBase + m_arrayDataOff), sizeof(uintptr_t)) || IsBadReadPtr(reinterpret_cast<void*>(valuesBase + m_arraySizeOff), sizeof(uint64_t))) {
    logger->Error("UpdateMail: instance arrays unreadable (id={}).", id);
    return -1;
  }
  const uint64_t keyCount = *reinterpret_cast<uint64_t*>(keysBase + m_arraySizeOff);
  const uint64_t valueCount = *reinterpret_cast<uint64_t*>(valuesBase + m_arraySizeOff);
  if (keyCount > kMaxMailCount || valueCount > kMaxMailCount) {
    logger->Error("UpdateMail: implausible instance key/value counts (id={}).", id);
    return -1;
  }
  const uintptr_t keysData = *reinterpret_cast<uintptr_t*>(keysBase + m_arrayDataOff);
  const uintptr_t valuesData = *reinterpret_cast<uintptr_t*>(valuesBase + m_arrayDataOff);
  if (!keysData || !valuesData) {
    logger->Error("UpdateMail: instance arrays empty (id={}).", id);
    return -1;
  }

  // Preflight: locate all three keys before writing anything, so a game letter
  // (no SPF keys) can never be half-updated.
  int64_t keyIndex[3] = {-1, -1, -1};
  const uint64_t count = keyCount < valueCount ? keyCount : valueCount;
  for (int slot = 0; slot < 3; ++slot) {
    for (uint64_t i = 0; i < count; ++i) {
      const uintptr_t keyElem = keysData + i * m_tempPrismSize;
      if (IsBadReadPtr(reinterpret_cast<void*>(keyElem), m_tempPrismSize)) {
        continue;
      }
      const char* keyBuf = strRes.GetBuffer(reinterpret_cast<uint8_t*>(keyElem));
      if (keyBuf && std::strcmp(keyBuf, wantedKeys[slot]) == 0) {
        keyIndex[slot] = static_cast<int64_t>(i);
        break;
      }
    }
    if (keyIndex[slot] < 0) {
      logger->Error("UpdateMail: key '{}' missing on letter id={} (not created by SendCustomMail).", wantedKeys[slot], id);
      return -1;
    }
  }

  // In-place value rewrite: id, read flag, inbox position and the shared
  // mailtext.spf template all stay untouched — Compose renders the new values
  // on the next mailbox refresh.
  for (int slot = 0; slot < 3; ++slot) {
    const uintptr_t valueElem = valuesData + static_cast<uint64_t>(keyIndex[slot]) * m_tempPrismSize;
    if (IsBadReadPtr(reinterpret_cast<void*>(valueElem), m_tempPrismSize)) {
      logger->Error("UpdateMail: value element unreadable (key='{}', id={}).", wantedKeys[slot], id);
      return -1;
    }
    if (!strRes.Set(reinterpret_cast<uint8_t*>(valueElem), texts[slot].c_str())) {
      logger->Error("UpdateMail: prism_string Set failed (key='{}', id={}); earlier keys may already be updated.", wantedKeys[slot], id);
      return -1;
    }
  }

  logger->Info("UpdateMail: id={} owner='{}' tag='{}' updated in place ({}).", id, owner, tag, pending ? "pending" : "inbox");
  return id;
}

bool MailService::DeleteMail(const char* owner, const char* tag) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");

  if (!owner || owner[0] == '\0' || !tag || tag[0] == '\0') {
    logger->Error("DeleteMail: owner and tag must be non-empty.");
    return false;
  }
  if (!m_isInitialized) {
    logger->Error("DeleteMail: service is not ready.");
    return false;
  }
  auto& strRes = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!strRes.IsInstalled()) {
    logger->Error("DeleteMail: prism string resolver is not installed.");
    return false;
  }
  const uintptr_t mailCtrl = GetMailCtrlAddr();
  if (!mailCtrl) {
    logger->Error("DeleteMail: mail controller is not available.");
    return false;
  }

  bool pending = false;
  size_t index = 0;
  int32_t id = -1;
  if (FindCustomInstance(mailCtrl, m_mailCtrlInboxBase, m_mailCtrlPendingBase, strRes, owner, tag, pending, index, id, m_mailDefSize, m_mailDefIdOff, m_instKeysBase, m_instValuesBase, m_arraySizeOff, m_arrayDataOff, m_tempPrismSize) == 0) {
    logger->Error("DeleteMail: no letter with owner='{}' tag='{}'.", owner, tag);
    return false;
  }
  return DeleteMail(index, pending);
}

bool MailService::EnumerateMailDefs(std::vector<std::string>& out) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MailService");
  out.clear();

  if (!m_isInitialized) {
    logger->Error("EnumerateMailDefs: service is not ready.");
    return false;
  }

  // Derive the deliverable def name from a registered mail_text unit name:
  // "mailtext.X" / "gameplay.X" -> "mail.X", otherwise "mail.<name>" — the same
  // one-hash-slot-per-dot-segment rule CreateMailDef uses at registration.
  auto deriveDefName = [](const std::string& textName) {
    constexpr const char* kPrefixes[] = {"mailtext.", "gameplay."};
    for (const char* prefix : kPrefixes) {
      const size_t prefixLen = std::char_traits<char>::length(prefix);
      if (textName.rfind(prefix, 0) == 0) {
        return std::string("mail.") + textName.substr(prefixLen);
      }
    }
    return std::string("mail.") + textName;
  };

  // Runtime-registered defs (CreateMailDef) first, sorted for a stable combo.
  std::vector<std::string> custom;
  custom.reserve(m_customMailDefs.size());
  for (const auto& kv : m_customMailDefs) {
    custom.push_back(kv.first);
  }
  std::sort(custom.begin(), custom.end());

  std::unordered_set<std::string> seen;
  for (std::string& name : custom) {
    if (seen.insert(name).second) {
      out.push_back(std::move(name));
    }
  }

  // Defs derived from every registered mail_text unit (locale + CreateMailText).
  std::vector<MailTextInfo> texts;
  if (!EnumerateMailTexts(texts)) {
    return false;  // EnumerateMailTexts logs its own reason
  }
  for (const auto& text : texts) {
    if (text.name.empty()) {
      continue;
    }
    std::string def = deriveDefName(text.name);
    if (seen.insert(def).second) {
      out.push_back(std::move(def));
    }
  }

  logger->Info("EnumerateMailDefs: {} defs.", out.size());
  return true;
}

}  // namespace SPF::Data::GameData
