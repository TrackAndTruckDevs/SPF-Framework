/**
 * @file SPF_Mail_API.h
 * @brief API for reading and controlling the game's mail system.
 *
 * @details This API provides plugins with access to the player's mailbox:
 *          deliver letters defined by mail_data defs or fully custom letters
 *          (from/subject/body with owner/tag identity keys), read inbox and
 *          pending queue, compose localized letter text, mark letters as read,
 *          update and delete custom letters in place, and manage runtime
 *          mail_text/mail_def units.
 *
 *          Custom letters (MAIL_SendCustomMail) carry a plugin-chosen
 *          (owner, tag) identity pair that persists in the game save. After a
 *          reload a plugin re-reads its own file, enumerates the inbox and
 *          matches its letters by owner+tag (MAIL_HasMail / MAIL_GetMailInfo).
 *
 * ================================================================================================
 * USAGE EXAMPLE (C++)
 * ================================================================================================
 * @code
 * void MyPlugin_OnActivated(const SPF_Core_API* api) {
 *     if (!api->mail->MAIL_IsReady()) return;
 *
 *     int id = api->mail->MAIL_SendCustomMail("Me", "Hello", "Body text", "myplugin", "welcome");
 *     if (id >= 0) api->mail->MAIL_MarkRead(id);
 *
 *     int count = api->mail->MAIL_GetInboxCount();
 *     for (int i = 0; i < count; ++i) {
 *         SPF_MAIL_MailInfo info;
 *         if (api->mail->MAIL_GetMailInfo(i, false, &info)) {
 *             // info.id, info.subject, info.owner, ...
 *         }
 *     }
 * }
 * @endcode
 *
 * ================================================================================================
 * ABI RULE
 * ================================================================================================
 * New function pointers are only APPENDED to the end of the SPF_Mail_API structure.
 * Never reorder or remove existing entries — plugins compiled against older headers
 * must keep working.
 *
 * ================================================================================================
 * THREAD SAFETY
 * ================================================================================================
 * All functions must be called from the game thread. Calling from other threads may
 * crash due to unsynchronized access to game memory.
 */

#ifndef SPF_MAIL_API_H
#define SPF_MAIL_API_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// 1. CONSTANTS AND STRUCTURES
// =================================================================================================

/** @brief Maximum size (including NUL) of text fields embedded in info structures. */
#define SPF_MAIL_TEXT_MAX 512

/** @brief Metadata of a single inbox/pending mail entry. */
typedef struct SPF_MAIL_MailInfo {
  int32_t id;            /**< Game-assigned mail id. */
  bool read;             /**< Read flag. */
  bool pending;          /**< true = pending queue, false = inbox. */
  uint32_t index;        /**< Position inside its mail_ctrl array. */
  char name[SPF_MAIL_TEXT_MAX];     /**< Mail def name (e.g. "mail.ovs.new_routes"). */
  char subject[SPF_MAIL_TEXT_MAX];  /**< Composed mail_text subject. */
  char owner[SPF_MAIL_TEXT_MAX];    /**< Custom letters: owner identity key; "" for game letters. */
  char tag[SPF_MAIL_TEXT_MAX];      /**< Custom letters: tag identity key; "" for game letters. */
} SPF_MAIL_MailInfo;

/** @brief Snapshot of one delivered letter (inbox then pending queue). */
typedef struct SPF_MAIL_MailInstance {
  int32_t id;            /**< Game id (instance +0x10), stable across sessions. */
  bool read;             /**< Read flag. */
  bool pending;          /**< true = pending queue, false = inbox. */
  uint32_t index;        /**< Position inside its mail_ctrl array. */
  int32_t category;      /**< mail_def category (0 = inbox category). */
  char name[SPF_MAIL_TEXT_MAX];     /**< Mail_text unit name the letter resolves to. */
  char from[SPF_MAIL_TEXT_MAX];     /**< Composed From. */
  char subject[SPF_MAIL_TEXT_MAX];  /**< Composed Subject. */
  char body[SPF_MAIL_TEXT_MAX];     /**< Composed Body. */
  char owner[SPF_MAIL_TEXT_MAX];    /**< Owner identity key ("" when absent). */
  char tag[SPF_MAIL_TEXT_MAX];      /**< Tag identity key ("" when absent). */
} SPF_MAIL_MailInstance;

/** @brief Snapshot of a registered mail_text unit. */
typedef struct SPF_MAIL_TextInfo {
  int32_t nameId;        /**< Registry name id (0 = not registered). */
  char name[SPF_MAIL_TEXT_MAX];     /**< Resolved unit name. */
  char from[SPF_MAIL_TEXT_MAX];     /**< From text. */
  char subject[SPF_MAIL_TEXT_MAX];  /**< Subject text. */
  char body[SPF_MAIL_TEXT_MAX];     /**< Body text. */
} SPF_MAIL_TextInfo;

// =================================================================================================
// 2. FUNCTION POINTER TYPES
// =================================================================================================

/** @brief Returns true if the mail service is fully initialized and all offsets are resolved. */
typedef bool (*SPF_MAIL_IsReady_t)(void);

/** @brief Re-runs pattern discovery for mail functions/offsets. Returns true on success. */
typedef bool (*SPF_MAIL_RefreshOffsets_t)(void);

/**
 * @brief Delivers a mail defined by its mail_data name into the player inbox.
 * @param mail_def_name mail_data def name (e.g. "mail.ovs.new_routes").
 * @param keys Array of placeholder key names (may be NULL when count is 0).
 * @param values Array of placeholder values, same length and order as keys.
 * @param count Number of key/value pairs.
 * @return Mail id (>= 0) on success, -1 on failure.
 */
typedef int (*SPF_MAIL_Send_t)(const char* mail_def_name, const char* const* keys, const char* const* values, int count);

/**
 * @brief Delivers a fully custom letter carrying (owner, tag) identity keys.
 * @param from From text; @param subject Subject text; @param body Body text.
 * @param owner Non-empty plugin-chosen identity key.
 * @param tag Non-empty plugin-chosen identity key.
 * @return Mail id (>= 0) on success, -1 on failure.
 */
typedef int (*SPF_MAIL_SendCustomMail_t)(const char* from, const char* subject, const char* body, const char* owner, const char* tag);

/** @brief Returns true if a letter with the (owner, tag) identity exists in inbox or pending. */
typedef bool (*SPF_MAIL_HasMail_t)(const char* owner, const char* tag);

/**
 * @brief Rewrites from/subject/body of the existing custom letter (owner, tag) IN PLACE.
 * @return Letter id (>= 0) on success, -1 on failure.
 */
typedef int (*SPF_MAIL_UpdateMail_t)(const char* owner, const char* tag, const char* from, const char* subject, const char* body);

/** @brief Deletes the custom letter identified by (owner, tag), inbox or pending. */
typedef bool (*SPF_MAIL_DeleteMail_t)(const char* owner, const char* tag);

/** @brief Number of mails in the inbox (0 on error). */
typedef int (*SPF_MAIL_GetInboxCount_t)(void);

/** @brief Number of mails in the pending queue (0 on error). */
typedef int (*SPF_MAIL_GetPendingCount_t)(void);

/** @brief Reads metadata of the inbox/pending entry at index into out. */
typedef bool (*SPF_MAIL_GetMailInfo_t)(int index, bool pending, SPF_MAIL_MailInfo* out);

/**
 * @brief Composes the localized from/subject/body for a mail by its id.
 * @param out_from/out_subject/out_body Caller buffers; each holds buffer_size bytes.
 * @return true if a mail with that id was found and composed.
 */
typedef bool (*SPF_MAIL_GetMailBody_t)(int mail_id, bool pending, char* out_from, char* out_subject, char* out_body, int buffer_size);

/** @brief Marks the inbox mail with mail_id as read. */
typedef bool (*SPF_MAIL_MarkRead_t)(int mail_id);

/** @brief Removes the inbox/pending entry at index. */
typedef bool (*SPF_MAIL_DeleteMailByIndex_t)(int index, bool pending);

/** @brief Number of delivered letters (inbox then pending). 0 on error. */
typedef int (*SPF_MAIL_GetDeliveredCount_t)(void);

/** @brief Reads the delivered letter at index into out. */
typedef bool (*SPF_MAIL_GetDelivered_t)(int index, SPF_MAIL_MailInstance* out);

/**
 * @brief Creates (or updates) a runtime mail_text unit and registers its name.
 * @param name Unit name, e.g. "mailtext.custom_note" (dot-separated [0-9A-Za-z_]).
 */
typedef bool (*SPF_MAIL_CreateMailText_t)(const char* name, const char* from, const char* subject, const char* body);

/** @brief Number of registered mail_text units. 0 on error. */
typedef int (*SPF_MAIL_GetMailTextCount_t)(void);

/** @brief Reads the registered mail_text unit at index into out. */
typedef bool (*SPF_MAIL_GetMailTextInfo_t)(int index, SPF_MAIL_TextInfo* out);

/**
 * @brief Creates (or updates) a runtime mail_def referencing the mail_text unit.
 * @param mail_text_name Registered mail_text unit name (must exist via MAIL_CreateMailText).
 * @param out_def_name Receives the def name to pass to MAIL_Send (buffer_size bytes).
 */
typedef bool (*SPF_MAIL_CreateMailDef_t)(const char* mail_text_name, char* out_def_name, int buffer_size);

/** @brief Number of deliverable mail def names. 0 on error. */
typedef int (*SPF_MAIL_GetMailDefCount_t)(void);

/** @brief Reads the deliverable mail def name at index into out_buffer. Returns name length. */
typedef int (*SPF_MAIL_GetMailDefName_t)(int index, char* out_buffer, int buffer_size);

// =================================================================================================
// 3. API STRUCTURE
// =================================================================================================

/**
 * @struct SPF_Mail_API
 * @brief Mail API for plugins.
 *
 * @details A pointer to this structure is available in the SPF_Core_API::mail field
 *          after OnActivated().
 *
 *          All functions require MAIL_IsReady() to return true. Using functions before
 *          the service is ready will return safe default values (0 / false / -1).
 *
 * **ABI Rule**: New function pointers are only appended to the end of this structure.
 */
typedef struct SPF_Mail_API {
  /** @brief Service lifecycle. @{ */
  SPF_MAIL_IsReady_t MAIL_IsReady;
  SPF_MAIL_RefreshOffsets_t MAIL_RefreshOffsets;
  /** @} */

  /** @brief Mail delivery. @{ */
  SPF_MAIL_Send_t MAIL_Send;
  SPF_MAIL_SendCustomMail_t MAIL_SendCustomMail;
  /** @} */

  /** @brief Custom letter identity management. @{ */
  SPF_MAIL_HasMail_t MAIL_HasMail;
  SPF_MAIL_UpdateMail_t MAIL_UpdateMail;
  SPF_MAIL_DeleteMail_t MAIL_DeleteMail;
  /** @} */

  /** @brief Inbox/pending read. @{ */
  SPF_MAIL_GetInboxCount_t MAIL_GetInboxCount;
  SPF_MAIL_GetPendingCount_t MAIL_GetPendingCount;
  SPF_MAIL_GetMailInfo_t MAIL_GetMailInfo;
  SPF_MAIL_GetMailBody_t MAIL_GetMailBody;
  SPF_MAIL_MarkRead_t MAIL_MarkRead;
  SPF_MAIL_DeleteMailByIndex_t MAIL_DeleteMailByIndex;
  /** @} */

  /** @brief Full delivered enumeration (inbox then pending). @{ */
  SPF_MAIL_GetDeliveredCount_t MAIL_GetDeliveredCount;
  SPF_MAIL_GetDelivered_t MAIL_GetDelivered;
  /** @} */

  /** @brief Runtime mail_text units. @{ */
  SPF_MAIL_CreateMailText_t MAIL_CreateMailText;
  SPF_MAIL_GetMailTextCount_t MAIL_GetMailTextCount;
  SPF_MAIL_GetMailTextInfo_t MAIL_GetMailTextInfo;
  /** @} */

  /** @brief Runtime mail defs. @{ */
  SPF_MAIL_CreateMailDef_t MAIL_CreateMailDef;
  SPF_MAIL_GetMailDefCount_t MAIL_GetMailDefCount;
  SPF_MAIL_GetMailDefName_t MAIL_GetMailDefName;
  /** @} */
} SPF_Mail_API;

#ifdef __cplusplus
}
#endif

#endif  // SPF_MAIL_API_H
