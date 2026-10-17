# SPF Mail API

The SPF Mail API provides access to the game's mailbox: deliver letters defined by `mail_data` defs or fully custom letters (from/subject/body with owner/tag identity keys), read the inbox and pending queue, compose localized letter text, mark letters as read, update and delete custom letters in place, and manage runtime `mail_text`/`mail_def` units.

Custom letters (`MAIL_SendCustomMail`) carry a plugin-chosen `(owner, tag)` identity pair stored inside the letter instance. This identity persists in the game save — after a reload a plugin re-reads its own file (the content source of truth), enumerates the inbox and matches its letters by owner+tag. The framework keeps no registry of its own.

## Getting the API

Access the Mail API during your plugin's `OnActivated` callback via the core API.

**Example: OnActivated callback (Recommended)**
```cpp
SPF_Mail_API* s_mailAPI = NULL;

void OnActivated(const SPF_Core_API* core_api) {
    s_mailAPI = core_api->mail;
}
```

## Key Concepts

1. **World-Scoped Lifecycle**: The mail system initializes when the game world loads. Always check `MAIL_IsReady()` before using any other function. After loading into the game, call `MAIL_RefreshOffsets()` if the service was not ready at startup.
2. **Two Delivery Paths**: `MAIL_Send(def_name, keys, values, count)` delivers a letter from an existing `mail_data` def with placeholder substitutions. `MAIL_SendCustomMail(from, subject, body, owner, tag)` delivers a fully custom letter built on the framework's shared template — use it when you control the entire text.
3. **Owner/Tag Identity**: Every custom letter carries a non-empty `(owner, tag)` pair (e.g. `"myplugin"`, `"welcome_1"`). Use `MAIL_HasMail` to check existence, `MAIL_UpdateMail` to rewrite text in place, `MAIL_DeleteMail` to remove — all keyed by identity, not by game id, so they survive save/reload.
4. **In-Place Updates**: `MAIL_UpdateMail` only rewrites the instance value strings — game id, read flag, inbox position and the shared template stay untouched. The new text renders on the next mailbox open. All three SPF keys are located before any write, so a game letter (no SPF keys) can never be half-updated.
5. **String Safety**: Info structures embed fixed 512-byte text fields (truncated safely). `MAIL_GetMailBody` / `MAIL_CreateMailDef` / `MAIL_GetMailDefName` use caller-provided buffers with an explicit `buffer_size`.
6. **Mail Ids vs Indexes**: Mail ids are game-assigned and stable across sessions; array indexes shift when letters are deleted. Re-read counts and indexes before every index-based operation. Id-based functions (`MAIL_GetMailBody`, `MAIL_MarkRead`) search both inbox and pending.
7. **Delivered Enumeration**: `MAIL_GetDeliveredCount` / `MAIL_GetDelivered` walk the full mailbox (inbox then pending) with composed text and identity keys — prefer them over per-index `MAIL_GetMailInfo` when you need full letter content.
8. **Runtime Texts and Defs**: `MAIL_CreateMailText` creates (or updates) a registered `mail_text` unit. `MAIL_CreateMailDef` links a def to that unit and returns the def name to pass to `MAIL_Send`. Def naming follows the game's rule: `"mailtext.x"` / `"gameplay.x"` → `"mail.x"`, otherwise `"mail.<name>"`.
9. **Game Thread**: Mail functions execute game delivery/compose logic. Call them from the game thread.

## Usage Example

### Custom Letter Lifecycle

```c
if (!s_mailAPI || !s_mailAPI->MAIL_IsReady()) return;

// Deliver a custom letter with plugin identity
int id = s_mailAPI->MAIL_SendCustomMail("Fleet Office", "Contract ready", "Pick up the cargo at the depot.",
                                        "myplugin", "contract_42");
if (id >= 0) s_mailAPI->MAIL_MarkRead(id);

// After game reload: reconcile by identity (content comes from your own file)
if (!s_mailAPI->MAIL_HasMail("myplugin", "contract_42")) {
    s_mailAPI->MAIL_SendCustomMail("Fleet Office", "Contract ready", "Pick up the cargo at the depot.",
                                   "myplugin", "contract_42");
}

// Rewrite text in place (id, read flag and position stay)
s_mailAPI->MAIL_UpdateMail("myplugin", "contract_42", "Fleet Office", "Contract done", "Good work!");

// Remove the letter
s_mailAPI->MAIL_DeleteMail("myplugin", "contract_42");
```

### Reading the Mailbox

```c
if (!s_mailAPI || !s_mailAPI->MAIL_IsReady()) return;

int count = s_mailAPI->MAIL_GetInboxCount();
for (int i = 0; i < count; ++i) {
    SPF_MAIL_MailInfo info;
    if (s_mailAPI->MAIL_GetMailInfo(i, false, &info)) {
        printf("id=%d subject='%s' owner='%s'\n", info.id, info.subject, info.owner);

        char from[512], subject[512], body[512];
        if (s_mailAPI->MAIL_GetMailBody(info.id, false, from, subject, body, sizeof(from))) {
            printf("body: %s\n", body);
        }
    }
}
```

### Delivering a Def-Based Letter with Placeholders

```c
const char* keys[]   = { "$CITY$", "$DELIVERED$" };
const char* values[] = { "Berlin", "5" };
int id = s_mailAPI->MAIL_Send("mail.ovs.new_routes", keys, values, 2);
```

### Runtime Text + Def

```c
s_mailAPI->MAIL_CreateMailText("mailtext.my_note", "Me", "Subject", "Body text");

char defName[512];
if (s_mailAPI->MAIL_CreateMailDef("mailtext.my_note", defName, sizeof(defName))) {
    s_mailAPI->MAIL_Send(defName, NULL, NULL, 0);
}
```

## Function Reference

### Service Lifecycle

| Function | Return Type | Description |
|---|---|---|
| **`MAIL_IsReady()`** | `bool` | Checks if the mail service is initialized (all mail patterns resolved). Always call first. |
| **`MAIL_RefreshOffsets()`** | `bool` | Forces a rescan of mail memory patterns. Useful if the service was not ready when the plugin activated. |

### Mail Delivery

| Function | Return Type | Description |
|---|---|---|
| **`MAIL_Send(def_name, keys, values, count)`** | `int` | Delivers the letter defined by mail_data def `def_name`, substituting up to `count` placeholder pairs. `keys`/`values` may be NULL when `count` is 0. Returns mail id (≥ 0) or `-1` on failure. |
| **`MAIL_SendCustomMail(from, subject, body, owner, tag)`** | `int` | Delivers a fully custom letter carrying the `(owner, tag)` identity keys. Text is sanitized against template placeholders. Returns mail id (≥ 0) or `-1` on failure. |

### Custom Letter Identity Management

| Function | Return Type | Description |
|---|---|---|
| **`MAIL_HasMail(owner, tag)`** | `bool` | Returns whether a letter with the `(owner, tag)` identity exists in the inbox or pending queue. |
| **`MAIL_UpdateMail(owner, tag, from, subject, body)`** | `int` | Rewrites from/subject/body of the existing custom letter in place. Returns letter id (≥ 0) or `-1` when not found / not ready. |
| **`MAIL_DeleteMail(owner, tag)`** | `bool` | Deletes the custom letter identified by `(owner, tag)`, wherever it currently is. |

### Inbox / Pending Read

| Function | Return Type | Description |
|---|---|---|
| **`MAIL_GetInboxCount()`** | `int` | Number of mails in the inbox. `0` on error. |
| **`MAIL_GetPendingCount()`** | `int` | Number of mails in the pending queue. `0` on error. |
| **`MAIL_GetMailInfo(index, pending, out)`** | `bool` | Fills `SPF_MAIL_MailInfo` for the entry at `index`. `false` if index invalid, `out` is NULL, or read failed. |
| **`MAIL_GetMailBody(mail_id, pending, out_from, out_subject, out_body, buffer_size)`** | `bool` | Composes the localized from/subject/body for the mail with `mail_id` into three caller buffers (each `buffer_size` bytes). Searches inbox (`pending=false`) or pending (`pending=true`). |
| **`MAIL_MarkRead(mail_id)`** | `bool` | Marks the inbox mail with `mail_id` as read. `false` if not found. |
| **`MAIL_DeleteMailByIndex(index, pending)`** | `bool` | Removes the inbox/pending entry at `index`. |

`SPF_MAIL_MailInfo`: `id` (int32), `read` (bool), `pending` (bool), `index` (uint32), `name` / `subject` / `owner` / `tag` (embedded 512-byte fields).

### Full Delivered Enumeration

| Function | Return Type | Description |
|---|---|---|
| **`MAIL_GetDeliveredCount()`** | `int` | Number of delivered letters (inbox then pending). `0` on error. |
| **`MAIL_GetDelivered(index, out)`** | `bool` | Fills `SPF_MAIL_MailInstance` for the delivered letter at `index` (composed text + identity keys). |

`SPF_MAIL_MailInstance`: `id` (int32), `read` (bool), `pending` (bool), `index` (uint32), `category` (int32), `name` / `from` / `subject` / `body` / `owner` / `tag` (embedded 512-byte fields).

### Runtime mail_text Units

| Function | Return Type | Description |
|---|---|---|
| **`MAIL_CreateMailText(name, from, subject, body)`** | `bool` | Creates (or updates) a runtime mail_text unit and registers its name. `name` is dot-separated `[0-9A-Za-z_]`, e.g. `"mailtext.custom_note"`. |
| **`MAIL_GetMailTextCount()`** | `int` | Number of registered mail_text units. `0` on error. |
| **`MAIL_GetMailTextInfo(index, out)`** | `bool` | Fills `SPF_MAIL_TextInfo` for the unit at `index`. |

`SPF_MAIL_TextInfo`: `nameId` (int32, 0 = not registered), `name` / `from` / `subject` / `body` (embedded 512-byte fields).

### Runtime mail Defs

| Function | Return Type | Description |
|---|---|---|
| **`MAIL_CreateMailDef(mail_text_name, out_def_name, buffer_size)`** | `bool` | Creates (or updates) a runtime mail_def referencing the mail_text unit `mail_text_name` and writes the resulting def name into `out_def_name`. |
| **`MAIL_GetMailDefCount()`** | `int` | Number of deliverable mail def names (runtime defs first, then defs derived from registered texts). `0` on error. |
| **`MAIL_GetMailDefName(index, out_buffer, buffer_size)`** | `int` | Copies the def name at `index` into `out_buffer`. Returns the full name length (even when truncated), `-1` on error. |

---

## ABI Stability

To ensure compatibility with future framework versions without recompilation:
1. The order of existing function pointers will NEVER change.
2. Fields will NEVER be removed from this structure.
3. New functionality is only added by appending to the END of this structure.
