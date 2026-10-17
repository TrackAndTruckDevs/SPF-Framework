#pragma once

#include "SPF/Data/GameData/Finders/MailDataFinder.hpp"
#include "SPF/Data/GameData/IWorldScopedService.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SPF::Data::GameData {

/**
 * @class MailService
 * @brief Programmatic mail delivery via the game's CreateAndDeliverMail pipeline.
 *
 * Builds the MailRequest (name prism_string + keys/values prism arrays) on the
 * stack exactly as the game does (etalon: DeliverOvsNewRoutesMail), then calls
 * CreateAndDeliverMail discovered by MailDataFinder. Reads the mail controller
 * through economy->mailCtx (offset owned by EconomyService).
 */
class MailService : public IWorldScopedService {
 public:
  static MailService& GetInstance();

  MailService(const MailService&) = delete;
  void operator=(const MailService&) = delete;

  // --- Lifecycle ---
  void Initialize();
  void Reset();
  bool TryFindAllOffsets();

  // --- IWorldScopedService ---
  const char* GetName() const override { return "MailService"; }
  void ResetForWorldReload() override { Reset(); }
  bool TryFinalizeWorldInit() override { return TryFindAllOffsets(); }

  // --- Mail API ---
  using Params = std::vector<std::pair<std::string, std::string>>;

  /**
   * @brief Delivers a mail defined by its mail_data name into the player inbox.
   * @param mailDefName mail_data def name (e.g. "mail.ovs.new_routes").
   * @param params Placeholder key->value substitutions used by the mail text.
   * @return Mail id (>= 0) on success, -1 on failure.
   */
  int Send(const char* mailDefName, const Params& params);

  // Creates mailtext.spf template + mail.spf def; idempotent (cache-checked).
  bool EnsureCustomMailScaffold();

  // Delivers a custom mail carrying owner/tag as instance keys; returns the
  // game-assigned mail id, or -1 on failure.
  int SendCustomMail(const char* from, const char* subject, const char* body, const char* owner, const char* tag);

  /**
   * @brief Creates a runtime mail_text unit and registers its name in the game
   * unit-name registry (factory + RegisterMailTextName), then writes from/subject/body.
   * Repeated calls with the same name update the unit created by the first call.
   * @param name unit name, e.g. "mailtext.custom_note" (dot-separated [0-9A-Za-z_]).
   * @return true when the unit exists in the registry with the requested fields.
   */
  bool CreateMailText(const char* name, const char* from, const char* subject, const char* body);

  /**
   * @brief Creates a runtime mail_def_u referencing the mail_text unit @p mailTextName
   * and registers the def name ("mailtext.x" -> "mail.x", else "mail.<name>").
   * Repeated calls with the same text name update the def created by the first call.
   * @param mailTextName registered mail_text unit name (must exist via CreateMailText).
   * @param outDefName receives the def name to pass to Send().
   * @return true when the def exists in the registry.
   */
  bool CreateMailDef(const char* mailTextName, std::string& outDefName);

  // --- Read API ---

  /** @brief Snapshot of a single inbox/pending mail entry. */
  struct MailInfo {
    int id = -1;
    bool read = false;
    bool pending = false;
    size_t index = 0;
    std::string name;     // mail def name (e.g. "mail.ovs.new_routes")
    std::string subject;  // composed mail_text subject
    std::string owner;    // custom letters: "$SPF_M_OWNER$" instance key value; "" for game letters
    std::string tag;      // custom letters: "$SPF_M_TAG$" instance key value; "" for game letters
  };

  /** @brief Number of mails in the inbox (0 on error). */
  int GetInboxCount() const;

  /** @brief Number of mails in the pending queue (0 on error). */
  int GetPendingCount() const;

  /**
   * @brief Reads metadata of the inbox/pending entry at @p index.
   * @return true if the entry was read successfully.
   */
  bool GetMailInfo(size_t index, bool pending, MailInfo& out);

  /**
   * @brief Composes the localized from/subject/body for a mail by its id.
   * @param pending Search the pending queue instead of the inbox.
   * @return true if a mail with that id was found and composed.
   */
  bool GetMailBody(int mailId, bool pending, std::string& from, std::string& subject, std::string& body);

  /** @brief Snapshot of a registered mail_text unit. */
  struct MailTextInfo {
    int32_t nameId = 0;  // registry name id (0 = not registered)
    std::string name;    // resolved name (filled for units created via CreateMailText)
    std::string from;
    std::string subject;
    std::string body;
  };

  /**
   * @brief Reloads locale mails.sii and copies every registered mail_text unit.
   * @return true when enumeration completed (out may still be empty).
   */
  bool EnumerateMailTexts(std::vector<MailTextInfo>& out);

  /**
   * @brief Snapshot of one delivered letter: game id, read/pending state, fully
   *        composed display text (from/subject/body) and the raw instance
   *        key/value pairs, including the custom identity keys owner/tag.
   */
  struct MailInstance {
    int id = -1;                  // game id (instance +0x10), stable across sessions
    bool read = false;            // read flag (instance +0x88)
    bool pending = false;         // true = pending queue, false = inbox
    size_t index = 0;             // position inside its mail_ctrl array
    int32_t category = 0;         // mail_def+0x14 (0 = inbox category)
    std::string name;             // mail_text unit name the letter resolves to
    std::string from;             // composed From (template + instance values)
    std::string subject;          // composed Subject
    std::string body;             // composed Body
    std::string owner;            // "$SPF_M_OWNER$" value ("" when absent)
    std::string tag;              // "$SPF_M_TAG$" value ("" when absent)
    std::vector<std::pair<std::string, std::string>> keys;  // ALL instance pairs
  };

  /**
   * @brief Enumerates every delivered letter (inbox then pending queue) with
   *        composed text and raw key/value pairs.
   * @details Custom letters (SendCustomMail) carry identity keys
   *          "$SPF_M_OWNER$"/"$SPF_M_TAG$" copied into the letter instance at
   *          delivery; they persist in the game save, so after a reload a plugin
   *          reads its own file (the content source of truth), enumerates the
   *          inbox and matches its letters by owner+tag. The framework keeps no
   *          registry of its own.
   * @return false only when the service is not ready or a mail_ctrl array is
   *         unreadable (reason is logged).
   */
  bool EnumerateDelivered(std::vector<MailInstance>& out);

  /**
   * @brief Marks the inbox mail with @p mailId as read.
   * @return true if the mail was found (and was not already read).
   */
  bool MarkRead(int mailId);

  /**
   * @brief Removes the inbox/pending entry at @p index.
   * @return true if the remove function was invoked.
   */
  bool DeleteMail(size_t index, bool pending);

  /**
   * @brief Checks whether a letter with the (owner, tag) identity exists in the
   *        inbox or pending queue. Used by plugins to reconcile their saved
   *        letter state on game start.
   */
  bool HasMail(const char* owner, const char* tag);

  /**
   * @brief Rewrites from/subject/body of the existing custom letter (owner, tag)
   *        IN PLACE: only the instance value strings change — game id, read
   *        flag, inbox position and the shared mailtext.spf template stay
   *        untouched, and Compose renders the new text on the next mailbox open.
   * @details All three SPF keys are located before any write, so a game letter
   *          (no SPF keys) can never be half-updated. User text is sanitized
   *          through the same placeholder stripping as SendCustomMail.
   * @return letter id (>= 0) on success; -1 when not found / not ready / key
   *         missing (reason is logged).
   */
  int UpdateMail(const char* owner, const char* tag, const char* from, const char* subject, const char* body);

  /**
   * @brief Deletes the custom letter identified by (owner, tag), wherever it
   *        currently is (inbox or pending). Applies to the first match when a
   *        plugin delivered duplicate identities (plugin data error).
   * @return true when found and the removal was invoked; false + logged reason
   *         otherwise.
   */
  bool DeleteMail(const char* owner, const char* tag);

  /**
   * @brief Lists every deliverable mail def name for Send(): runtime-registered
   *        defs (CreateMailDef) first, then "mail.X" derived from each
   *        registered "mailtext.X"/"gameplay.X" unit (locale + CreateMailText).
   * @return false only when the service is not ready (reason is logged).
   */
  bool EnumerateMailDefs(std::vector<std::string>& out);

  bool IsReady() const { return m_isInitialized; }

  // --- Setters used by MailDataFinder (Session pattern) ---
  void SetCreateAndDeliverMailAddr(uintptr_t v) { m_createAndDeliverMailAddr = v; }
  void SetComposeAddr(uintptr_t v) { m_composeAddr = v; }
  void SetRemoveAtIndexAddr(uintptr_t v) { m_removeAtIndexAddr = v; }
  void SetLoadLocaleAddr(uintptr_t v) { m_loadLocaleAddr = v; }
  void SetMailTextFactoryAddr(uintptr_t v) { m_mailTextFactoryAddr = v; }
  void SetMailDefFactoryAddr(uintptr_t v) { m_mailDefFactoryAddr = v; }
  void SetRegisterNameAddr(uintptr_t v) { m_registerNameAddr = v; }
  void SetUnitArrayObjectAddr(uintptr_t v) { m_unitArrayObjectAddr = v; }
  void SetUnitArrayDataAddr(uintptr_t v) { m_unitArrayDataAddr = v; }
  void SetUnitArrayCountAddr(uintptr_t v) { m_unitArrayCountAddr = v; }
  void SetUnitArrayCapacityAddr(uintptr_t v) { m_unitArrayCapacityAddr = v; }
  void SetMailCtrlInboxOffset(int32_t v) { m_mailCtrlInboxBase = v; }
  void SetMailCtrlUnreadOffset(int32_t v) { m_mailCtrlUnreadOff = v; }
  void SetMailCtrlPendingOffset(int32_t v) { m_mailCtrlPendingBase = v; }
  void SetMailDefNameOffset(int32_t v) { m_mailDefNameOff = v; }
  void SetMailDefReadOffset(int32_t v) { m_mailDefReadOff = v; }
  void SetMailDefParamKeysOffset(int32_t v) { m_instKeysBase = v; }
  void SetMailDefParamValuesOffset(int32_t v) { m_instValuesBase = v; }
  void SetMailTextFromOffset(int32_t v) { m_mailTextFromOff = v; }
  void SetMailTextSubjectOffset(int32_t v) { m_mailTextSubjectOff = v; }
  void SetMailTextBodyOffset(int32_t v) { m_mailTextBodyOff = v; }
  void SetCtrlRefcountOff(int32_t v) { m_ctrlRefcountOff = v; }
  void SetCtrlRefcountInc(uint64_t v) { m_ctrlRefcountInc = v; }
  void SetSharedCtrlInnerOff(int32_t v) { m_sharedCtrlInnerOff = v; }
  void SetMailDefSize(int32_t v) { m_mailDefSize = v; }
  void SetMailDefIdOffset(int32_t v) { m_mailDefIdOff = v; }
  void SetMailDefCategoryOffset(int32_t v) { m_mailDefCategoryOff = v; }
  void SetUnitSystemInvalidIndex(int32_t v) { m_unitSystemInvalidIndex = v; }
  void SetRequestKeysOffset(int32_t v) { m_requestKeysOff = v; }
  void SetRequestValuesOffset(int32_t v) { m_requestValuesOff = v; }
  void SetArraySizeOffset(int32_t v) { m_arraySizeOff = v; }
  void SetArrayCapFieldOffset(int32_t v) { m_arrayCapFieldOff = v; }
  void SetArrayCtrlFieldOffset(int32_t v) { m_arrayCtrlFieldOff = v; }
  void SetArrayDataOffset(int32_t v) { m_arrayDataOff = v; }
  void SetTempPrismSize(int32_t v) { m_tempPrismSize = v; }
  void SetMailTextNameIdOffset(int32_t v) { m_mailTextNameIdOff = v; }
  void SetInitialized(bool v) { m_isInitialized = v; }

 private:
  MailService();

  /** @brief Resolves the live mail_ctrl* via economy->mailCtx, or 0. */
  uintptr_t GetMailCtrlAddr() const;

  /**
   * @brief Runs the game Compose (FUN_141258cd0) for @p mailDef.
   * Any non-null output is filled; null outputs are skipped.
   */
  bool ComposeText(uintptr_t mailDef, std::string* from, std::string* subject, std::string* body);

  Finders::MailDataFinder m_finder;
  uintptr_t m_createAndDeliverMailAddr = 0;
  uintptr_t m_composeAddr = 0;
  uintptr_t m_removeAtIndexAddr = 0;
  uintptr_t m_loadLocaleAddr = 0;
  uintptr_t m_mailTextFactoryAddr = 0;
  uintptr_t m_mailDefFactoryAddr = 0;
  uintptr_t m_unitArrayObjectAddr = 0;
  uintptr_t m_unitArrayDataAddr = 0;
  uintptr_t m_unitArrayCountAddr = 0;
  uintptr_t m_unitArrayCapacityAddr = 0;
  uintptr_t m_registerNameAddr = 0;
  int32_t m_mailCtrlInboxBase = 0;
  int32_t m_mailCtrlUnreadOff = 0;
  int32_t m_mailCtrlPendingBase = 0;
  int32_t m_mailDefIdOff = 0x10;
  int32_t m_mailDefNameOff = 0;
  int32_t m_mailDefReadOff = 0;
  int32_t m_mailDefSize = 0;
  int32_t m_instKeysBase = 0;
  int32_t m_instValuesBase = 0;
  int32_t m_mailTextFromOff = 0;
  int32_t m_mailTextSubjectOff = 0;
  int32_t m_mailTextBodyOff = 0;
  int32_t m_mailTextSize = 0;
  int32_t m_ctrlRefcountOff = 0;
  uint64_t m_ctrlRefcountInc = 0;
  int32_t m_sharedCtrlInnerOff = 0;
  int32_t m_mailDefCategoryOff = 0;
  int32_t m_unitSystemInvalidIndex = 0;
  int32_t m_requestKeysOff = 0;
  int32_t m_requestValuesOff = 0;
  int32_t m_arraySizeOff = 0;
  int32_t m_arrayCapFieldOff = 0;
  int32_t m_arrayCtrlFieldOff = 0;
  int32_t m_arrayDataOff = 0;
  int32_t m_tempPrismSize = 0;
  int32_t m_mailTextNameIdOff = 0;
  std::unordered_map<std::string, uintptr_t> m_customMailTexts;
  std::unordered_map<std::string, uintptr_t> m_customMailDefs;
  bool m_isInitialized = false;
};

}  // namespace SPF::Data::GameData
