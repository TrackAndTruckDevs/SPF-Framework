#pragma once

/**
 * @file SPF_Sound_API.h
 * @brief C-style API for the sound system, exposed to plugins.
 *
 * @details This header provides access to the game's sound system through TWO
 *          independent layers. Choose the layer by answering one question:
 *          WHO should drive the sound — the plugin, or the game?
 *
 * ================================================================================================
 * ACCESS LAYERS
 * ================================================================================================
 *
 * [FMOD layer] — direct FMOD Studio access (implemented via FmodApi / FmodStudioHook).
 *   The plugin talks straight to FMOD: loads banks, enumerates events, creates
 *   instances, starts/stops playback, sets parameters, mixes buses and VCAs,
 *   controls listeners. THE GAME KNOWS NOTHING about these sounds — no game
 *   logic triggers, updates or stops them. Use this layer when:
 *     - the sound is entirely plugin-owned (custom music, plugin UI sounds,
 *       notifications, footsteps of a custom character);
 *     - the event does not exist in the game at all (e.g. a footsteps event
 *       the game never had);
 *     - the plugin must control playback timing itself.
 *   Banks loaded via SND_LoadBankFile / SND_LoadBankMemory go directly into FMOD:
 *   the game cannot see or manage such a bank — it never appears in the game's
 *   bank list and the game will not unload it. The plugin owns its lifetime.
 *
 * [Game layer] — SoundRef rebinding (".soundref" -> "bank#event").
 *   The game maps its own sounds through ".soundref" files and activates them
 *   itself (UI clicks, horn, engine, world sounds). Use this layer when the
 *   plugin wants to REPLACE a sound the game plays, while the game keeps
 *   triggering, timing and stopping it exactly as before:
 *     1. SND_RegisterSoundRefOverride("/sound/ui/ui_click.soundref",
 *                                      "my_bank#ui/click_v2");
 *     2. the game — including already-created events — switches to the new
 *        "bank#event";
 *     3. SND_UnregisterSoundRefOverride() / SND_ClearSoundRefOverrides()
 *        restores the originals.
 *   No plugin-side playback control is involved — the game remains the caller.
 *
 * Decision rule:
 *   - the game should keep calling the sound              -> SoundRef (game layer)
 *   - the plugin drives playback / event not in the game  -> FMOD (direct layer)
 *
 * ================================================================================================
 * THREAD SAFETY
 * ================================================================================================
 *
 * FMOD-layer functions must be called from the game thread (FMOD's command queue
 * is processed there only; calling from other threads corrupts FMOD state).
 * SoundRef (game layer) functions may be called from any thread; rebinds are
 * applied on the game tick.
 *
 * ================================================================================================
 * LIFECYCLE
 * ================================================================================================
 *
 * The sound system is world-scoped — it initializes when the game world loads and shuts down
 * when the world unloads. Always check SND_IsReady() before using any other function.
 *
 * Event workflow:
 *   1. Enumerate events: SND_GetEventCount() + SND_GetEventPath()
 *   2. Find a specific event: SND_FindEventIndexByPath()
 *   3. Create an instance: SND_CreateEventInstance()
 *   4. Control playback: SND_StartEvent() / SND_StopEvent() / SND_PauseEvent()
 *   5. Adjust properties: SND_SetEventVolume() / SND_SetEventPitch() / etc.
 *   6. Release when done: SND_ReleaseEvent()
 *
 * Bus/VCA workflow:
 *   1. Enumerate: SND_GetBusCount() + SND_GetBusPath() / SND_GetVCACount() + SND_GetVCAPath()
 *   2. Control: SND_SetBusVolume() / SND_SetVCAVolume() / SND_SetBusMute()
 *
 * Bank management:
 *   1. Load: SND_LoadBankFile() / SND_LoadBankMemory()
 *   2. Query: SND_GetBankLoadingState() / SND_GetBankEventCount()
 *   3. Discover events: SND_GetBankEventGuid() + SND_FindEventIndexByGuid()
 *   4. Unload: SND_UnloadBank()
 *
 * SoundRef rebinding (game layer):
 *   1. Enumerate: SND_GetSoundRefCount() + SND_GetSoundRefPath() / SND_GetSoundRefSource()
 *   2. Find: SND_FindSoundRefIndex() / SND_FindSoundRefBySource()
 *   3. Rebind: SND_RegisterSoundRefOverride()
 *   4. Inspect: SND_GetSoundRefOverride() / SND_IsSoundRefActive()
 *   5. Restore: SND_UnregisterSoundRefOverride() / SND_ClearSoundRefOverrides()
 *
 * ================================================================================================
 * ABI STABILITY
 * ================================================================================================
 *
 * To ensure compatibility with future framework versions without recompilation:
 * 1. The order of existing function pointers will NEVER change.
 * 2. Fields will NEVER be removed from this structure.
 * 3. New functionality is only added by appending to the END of this structure.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// CALLBACK MASK CONSTANTS
// =================================================================================================

/** @brief Callback fires when the event starts playing. */
#define SPF_SND_CALLBACK_START 0x00000001
/** @brief Callback fires when the event stops. */
#define SPF_SND_CALLBACK_STOP 0x00000002
/** @brief Callback fires when the event enters a "restart" state. */
#define SPF_SND_CALLBACK_RESTART 0x00000004
/** @brief Callback fires when the event loses or gains virtual voice status. */
#define SPF_SND_CALLBACK_VIRTUAL_VOICE 0x00000008
/** @brief Callback fires when the event's timeline passes a marker or beat. */
#define SPF_SND_CALLBACK_MARKER 0x00000010
/** @brief Callback fires when the event's timeline passes a named marker. */
#define SPF_SND_CALLBACK_NAMED_MARKER 0x00000020
/** @brief Callback fires when the event's sound duration changes. */
#define SPF_SND_CALLBACK_SOUND_DURATION 0x00000040
/** @brief Callback fires on any of the above events. */
#define SPF_SND_CALLBACK_ANY 0xFFFFFFFF

/**
 * @brief Event callback function type.
 *
 * @param type The callback type (one of SPF_SND_CALLBACK_* constants).
 * @param instance Opaque pointer to the FMOD::Studio::EventInstance.
 * @param parameters Event-specific callback data.
 * @return 0 to allow FMOD to process the callback normally.
 */
typedef int (*SPF_SND_EventCallbackFn)(uint32_t type, void* instance, void* parameters);

// =================================================================================================
// SERVICE LIFECYCLE
// =================================================================================================

/**
 * @brief Checks whether the sound system is initialized and ready to use.
 *
 * @details The sound system becomes ready when the game world is loaded and
 *          FMOD Studio offsets are resolved. Always call this before any other
 *          sound API function.
 *
 * @return true if the sound system is ready, false otherwise.
 */
typedef bool (*SPF_SND_IsReady_t)();

/**
 * @brief Checks whether all FMOD Studio memory pattern offsets have been found.
 *
 * @details This is a stricter check than SND_IsReady — it verifies that every
 *          required offset (bank list, event list, studio system, etc.) was
 *          successfully resolved from game memory.
 *
 * @return true if all offsets are found, false otherwise.
 */
typedef bool (*SPF_SND_AreAllOffsetsFound_t)();

/**
 * @brief Forces a rescan of FMOD Studio memory patterns.
 *
 * @details Re-runs the pattern scanner to find all FMOD offsets in game memory.
 *          Useful after a bank reload or if offsets become stale.
 *
 * @return true if all offsets were successfully resolved, false on failure.
 */
typedef bool (*SPF_SND_RefreshOffsets_t)();

// =================================================================================================
// BUS ENUMERATION & CONTROL [FMOD LAYER]
// =================================================================================================

/**
 * @brief Returns the number of unique audio buses currently loaded.
 *
 * @details The count is derived from all loaded banks. Each unique bus path
 *          (e.g. "bus:/Engine/Master") is counted once.
 *
 * @return Bus count, or 0 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetBusCount_t)();

/**
 * @brief Copies the path of a bus into the provided buffer.
 *
 * @details The path is the FMOD Studio bus path (e.g. "bus:/Engine/Master").
 *          If the path is longer than the buffer, it is truncated but the
 *          full length is still returned.
 *
 * @param index Zero-based bus index (0 to SND_GetBusCount()-1).
 * @param out_buffer Buffer to receive the bus path string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full path length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetBusPath_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Returns the current volume level of a bus.
 *
 * @details Volume is a linear multiplier. 1.0 = unity (no change), 0.0 = silent.
 *          Negative values are allowed for phase inversion.
 *
 * @param index Zero-based bus index.
 * @return The current volume, or 1.0f if the index is invalid.
 */
typedef float (*SPF_SND_GetBusVolume_t)(int index);

/**
 * @brief Sets the volume level of a bus.
 *
 * @details Volume is a linear multiplier. 1.0 = unity, 0.0 = silent.
 *
 * @param index Zero-based bus index.
 * @param volume New volume value (linear).
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_SetBusVolume_t)(int index, float volume);

/**
 * @brief Returns whether a bus is muted.
 *
 * @param index Zero-based bus index.
 * @return true if the bus is muted, false otherwise or if the index is invalid.
 */
typedef bool (*SPF_SND_GetBusMute_t)(int index);

/**
 * @brief Mutes or unmutes a bus.
 *
 * @details Muting silences the bus output without changing its volume setting.
 *
 * @param index Zero-based bus index.
 * @param muted true to mute, false to unmute.
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_SetBusMute_t)(int index, bool muted);

/**
 * @brief Returns whether a bus is paused.
 *
 * @param index Zero-based bus index.
 * @return true if the bus is paused, false otherwise or if the index is invalid.
 */
typedef bool (*SPF_SND_GetBusPause_t)(int index);

/**
 * @brief Pauses or unpauses a bus.
 *
 * @details Pausing a bus freezes all events routed through it.
 *
 * @param index Zero-based bus index.
 * @param paused true to pause, false to unpause.
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_SetBusPause_t)(int index, bool paused);

// =================================================================================================
// VCA (VOLUME CONTROL ASSOCIATION) [FMOD LAYER]
// =================================================================================================

/**
 * @brief Returns the number of VCAs currently loaded.
 *
 * @return VCA count, or 0 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetVCACount_t)();

/**
 * @brief Copies the path of a VCA into the provided buffer.
 *
 * @details The path is the FMOD Studio VCA path (e.g. "vca:/Engine/Master").
 *
 * @param index Zero-based VCA index (0 to SND_GetVCACount()-1).
 * @param out_buffer Buffer to receive the VCA path string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full path length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetVCAPath_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Returns the current volume level of a VCA.
 *
 * @details VCA volume is a linear multiplier applied to all buses controlled
 *          by this VCA.
 *
 * @param index Zero-based VCA index.
 * @return The current volume, or 1.0f if the index is invalid.
 */
typedef float (*SPF_SND_GetVCAVolume_t)(int index);

/**
 * @brief Sets the volume level of a VCA.
 *
 * @param index Zero-based VCA index.
 * @param volume New volume value (linear).
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_SetVCAVolume_t)(int index, float volume);

// =================================================================================================
// GLOBAL PARAMETERS [FMOD LAYER]
// =================================================================================================

/**
 * @brief Returns the number of global parameters defined in the FMOD Studio project.
 *
 * @return Global parameter count, or 0 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetGlobalParamCount_t)();

/**
 * @brief Copies the name of a global parameter into the provided buffer.
 *
 * @details The name is the FMOD Studio parameter path (e.g. "Speed").
 *
 * @param index Zero-based parameter index.
 * @param out_buffer Buffer to receive the parameter name.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full name length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetGlobalParamName_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Returns the minimum and maximum range of a global parameter.
 *
 * @param index Zero-based parameter index.
 * @param out_minimum Pointer to receive the minimum value. May be NULL.
 * @param out_maximum Pointer to receive the maximum value. May be NULL.
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_GetGlobalParamRange_t)(int index, float* out_minimum, float* out_maximum);

/**
 * @brief Returns the current value of a global parameter by name.
 *
 * @param param_name FMOD Studio parameter name (e.g. "Speed").
 * @return The current value, or 0.0f if the parameter is not found.
 */
typedef float (*SPF_SND_GetGlobalParamValue_t)(const char* param_name);

/**
 * @brief Sets the value of a global parameter by name.
 *
 * @param param_name FMOD Studio parameter name (e.g. "Speed").
 * @param value New parameter value.
 * @return true on success, false if the parameter is not found.
 */
typedef bool (*SPF_SND_SetGlobalParamValue_t)(const char* param_name, float value);

// =================================================================================================
// EVENT ENUMERATION [FMOD LAYER]
// =================================================================================================

/**
 * @brief Returns the total number of events across all loaded banks.
 *
 * @details This enumerates every event discovered during bank loading.
 *          Events are indexed sequentially; use SND_GetEventPath() to
 *          retrieve the path of each event by its index.
 *
 * @return Total event count, or 0 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetEventCount_t)();

/**
 * @brief Returns the bank path that contains the event at the given index.
 *
 * @details The bank path is the FMOD Studio bank path (e.g. "bank:/SFX").
 *
 * @param index Zero-based event index (0 to SND_GetEventCount()-1).
 * @param out_buffer Buffer to receive the bank path string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full path length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetEventBankPath_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Copies the path of an event into the provided buffer.
 *
 * @details The path is the FMOD Studio event path (e.g. "event:/SFX/Engine").
 *
 * @param index Zero-based event index.
 * @param out_buffer Buffer to receive the event path string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full path length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetEventPath_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Retrieves the GUID of an event.
 *
 * @param index Zero-based event index.
 * @param out_guid Buffer to receive the 16-byte GUID.
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_GetEventGuid_t)(int index, uint8_t out_guid[16]);

/**
 * @brief Returns whether the event is a 3D spatialized event.
 *
 * @param index Zero-based event index.
 * @return true if the event is 3D, false otherwise.
 */
typedef bool (*SPF_SND_IsEvent3D_t)(int index);

/**
 * @brief Returns whether the event plays only once (no looping).
 *
 * @param index Zero-based event index.
 * @return true if the event is a oneshot, false otherwise.
 */
typedef bool (*SPF_SND_IsEventOneshot_t)(int index);

/**
 * @brief Returns whether the event streams from disk rather than loading fully into memory.
 *
 * @param index Zero-based event index.
 * @return true if the event is streamed, false otherwise.
 */
typedef bool (*SPF_SND_IsEventStream_t)(int index);

/**
 * @brief Returns whether the event is an FMOD snapshot (mix state capture).
 *
 * @param index Zero-based event index.
 * @return true if the event is a snapshot, false otherwise.
 */
typedef bool (*SPF_SND_IsEventSnapshot_t)(int index);

/**
 * @brief Returns the duration of the event in milliseconds.
 *
 * @param index Zero-based event index.
 * @return Duration in ms, or 0 if unknown or the index is invalid.
 */
typedef uint32_t (*SPF_SND_GetEventDurationMs_t)(int index);

/**
 * @brief Returns the minimum attenuation distance of a 3D event.
 *
 * @details Below this distance, the event volume is at its maximum.
 *
 * @param index Zero-based event index.
 * @return Minimum distance in meters, or 0.0f if unknown.
 */
typedef float (*SPF_SND_GetEventMinDistance_t)(int index);

/**
 * @brief Returns the maximum attenuation distance of a 3D event.
 *
 * @details Above this distance, the event is inaudible.
 *
 * @param index Zero-based event index.
 * @return Maximum distance in meters, or 0.0f if unknown.
 */
typedef float (*SPF_SND_GetEventMaxDistance_t)(int index);

/**
 * @brief Searches for an event by its FMOD Studio path.
 *
 * @details Performs a linear search across all loaded events.
 *
 * @param event_path FMOD Studio event path (e.g. "event:/SFX/Engine").
 * @return Zero-based event index, or -1 if not found.
 */
typedef int (*SPF_SND_FindEventIndexByPath_t)(const char* event_path);

/**
 * @brief Searches for the first event whose path starts with the given prefix.
 *
 * @details Useful for finding all events matching a pattern (e.g. "event:/horn/").
 *          Performs a linear search across all loaded events.
 *
 * @param prefix Event path prefix to match (e.g. "event:/horn/").
 * @return Zero-based event index of the first match, or -1 if not found.
 */
typedef int (*SPF_SND_FindEventIndexByPrefix_t)(const char* prefix);

/**
 * @brief Searches for an event by its 16-byte GUID.
 *
 * @details Useful when path strings are unavailable (e.g. events from plugin-loaded banks).
 *          Performs a linear search across all loaded events.
 *
 * @param guid 16-byte FMOD GUID.
 * @return Zero-based event index, or -1 if not found.
 */
typedef int (*SPF_SND_FindEventIndexByGuid_t)(const uint8_t guid[16]);

/**
 * @brief Returns the number of live (game-created) instances for a given event.
 *
 * @details This queries FMOD's EventDescription for the current instance count.
 *          Live instances are created by the game engine, not by the plugin.
 *          Use SND_GetEventLiveInstance() to get individual instance pointers,
 *          then SND_GetEventPlaybackState() to check if they are playing.
 *
 * @param event_index Zero-based event index from enumeration.
 * @return Number of live instances, or 0 if none or index is invalid.
 */
typedef int (*SPF_SND_GetEventLiveInstanceCount_t)(int event_index);

/**
 * @brief Returns a pointer to a specific live instance of an event.
 *
 * @details The returned pointer can be used with SND_GetEventPlaybackState(),
 *          SND_StopEvent(), SND_PauseEvent(), and other instance functions.
 *          This exposes game-created instances — do not call SND_ReleaseEvent()
 *          on them.
 *
 * @param event_index Zero-based event index from enumeration.
 * @param instance_index Zero-based instance index (0 to LiveInstanceCount-1).
 * @return Opaque event instance pointer, or NULL on failure.
 */
typedef void* (*SPF_SND_GetEventLiveInstance_t)(int event_index, int instance_index);

// =================================================================================================
// EVENT PLAYBACK [FMOD LAYER]
// =================================================================================================

/**
 * @brief Creates a playable instance of an event.
 *
 * @details Each call creates a new independent instance. The returned pointer
 *          must be released with SND_ReleaseEvent() when no longer needed.
 *          Multiple instances of the same event can play simultaneously.
 *
 * @param event_index Zero-based event index from enumeration.
 * @return Opaque event instance pointer, or NULL on failure.
 */
typedef void* (*SPF_SND_CreateEventInstance_t)(int event_index);

/**
 * @brief Starts playback of an event instance.
 *
 * @param instance Opaque event instance pointer from SND_CreateEventInstance().
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_StartEvent_t)(void* instance);

/**
 * @brief Stops playback of an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param allow_fadeout If true, the event fades out gracefully. If false, it stops immediately.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_StopEvent_t)(void* instance, bool allow_fadeout);

/**
 * @brief Pauses or unpauses an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param paused true to pause, false to unpause.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_PauseEvent_t)(void* instance, bool paused);

/**
 * @brief Returns the current playback state of an event instance.
 *
 * @return Playback state:
 *         - 0: FMOD_STUDIO_PLAYBACK_PLAYING
 *         - 1: FMOD_STUDIO_PLAYBACK_SUSTAINING
 *         - 2: FMOD_STUDIO_PLAYBACK_STOPPED
 *         - 3: FMOD_STUDIO_STARTING
 *         - 4: FMOD_STUDIO_STOPPING
 *         - -1: invalid instance.
 */
typedef int (*SPF_SND_GetEventPlaybackState_t)(void* instance);

/**
 * @brief Releases an event instance and frees its resources.
 *
 * @details After calling this, the instance pointer is invalid and must not be used.
 *          This does NOT stop a playing event — call SND_StopEvent() first if needed.
 *
 * @param instance Opaque event instance pointer.
 */
typedef void (*SPF_SND_ReleaseEvent_t)(void* instance);

// =================================================================================================
// EVENT INSTANCE PROPERTIES [FMOD LAYER]
// =================================================================================================

/**
 * @brief Sets the volume of an event instance.
 *
 * @details Volume is a linear multiplier applied on top of the bus volume.
 *          1.0 = unity gain, 0.0 = silent.
 *
 * @param instance Opaque event instance pointer.
 * @param volume New volume value (linear).
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_SetEventVolume_t)(void* instance, float volume);

/**
 * @brief Returns the current volume of an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param out_volume Pointer to receive the raw volume value. May be NULL.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_GetEventVolume_t)(void* instance, float* out_volume);

/**
 * @brief Sets the pitch of an event instance.
 *
 * @details Pitch is a frequency multiplier. 1.0 = original pitch,
 *          2.0 = one octave up, 0.5 = one octave down.
 *
 * @param instance Opaque event instance pointer.
 * @param pitch New pitch value (frequency multiplier).
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_SetEventPitch_t)(void* instance, float pitch);

/**
 * @brief Returns the current pitch of an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param out_pitch Pointer to receive the raw pitch value. May be NULL.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_GetEventPitch_t)(void* instance, float* out_pitch);

/**
 * @brief Sets the 3D position, velocity, and orientation of an event instance.
 *
 * @details All vectors use the SCS coordinate system (X=right, Y=up, Z=forward).
 *
 * @param instance Opaque event instance pointer.
 * @param pos_x Position X component.
 * @param pos_y Position Y component.
 * @param pos_z Position Z component.
 * @param vel_x Velocity X component.
 * @param vel_y Velocity Y component.
 * @param vel_z Velocity Z component.
 * @param fwd_x Forward direction X component.
 * @param fwd_y Forward direction Y component.
 * @param fwd_z Forward direction Z component.
 * @param up_x Up direction X component.
 * @param up_y Up direction Y component.
 * @param up_z Up direction Z component.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_SetEvent3DAttributes_t)(void* instance, float pos_x, float pos_y, float pos_z, float vel_x, float vel_y, float vel_z, float fwd_x, float fwd_y, float fwd_z, float up_x, float up_y, float up_z);

/**
 * @brief Returns the current 3D position, velocity, and orientation of an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param out_pos_x Position X. May be NULL.
 * @param out_pos_y Position Y. May be NULL.
 * @param out_pos_z Position Z. May be NULL.
 * @param out_vel_x Velocity X. May be NULL.
 * @param out_vel_y Velocity Y. May be NULL.
 * @param out_vel_z Velocity Z. May be NULL.
 * @param out_fwd_x Forward X. May be NULL.
 * @param out_fwd_y Forward Y. May be NULL.
 * @param out_fwd_z Forward Z. May be NULL.
 * @param out_up_x Up X. May be NULL.
 * @param out_up_y Up Y. May be NULL.
 * @param out_up_z Up Z. May be NULL.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_GetEvent3DAttributes_t)(void* instance, float* out_pos_x, float* out_pos_y, float* out_pos_z, float* out_vel_x, float* out_vel_y, float* out_vel_z, float* out_fwd_x, float* out_fwd_y, float* out_fwd_z, float* out_up_x, float* out_up_y, float* out_up_z);

/**
 * @brief Sets a named parameter on an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param param_name Parameter name as defined in FMOD Studio (e.g. "RPM").
 * @param value New parameter value.
 * @param ignore_seek_speed If true, the value changes immediately. If false, it seeks at the configured rate.
 * @return true on success, false if the instance or parameter is invalid.
 */
typedef bool (*SPF_SND_SetEventParameter_t)(void* instance, const char* param_name, float value, bool ignore_seek_speed);

/**
 * @brief Returns the current value of a named parameter on an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param param_name Parameter name as defined in FMOD Studio.
 * @param out_value Pointer to receive the current value. May be NULL.
 * @return true on success, false if the instance or parameter is invalid.
 */
typedef bool (*SPF_SND_GetEventParameter_t)(void* instance, const char* param_name, float* out_value);

/**
 * @brief Sets the timeline playback position of an event instance.
 *
 * @details Position is in milliseconds from the start of the event's timeline.
 *
 * @param instance Opaque event instance pointer.
 * @param position Timeline position in milliseconds.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_SetEventTimelinePosition_t)(void* instance, int position);

/**
 * @brief Returns the current timeline position of an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @return Timeline position in milliseconds, or -1 if the instance is invalid.
 */
typedef int (*SPF_SND_GetEventTimelinePosition_t)(void* instance);

/**
 * @brief Enables or disables looping on an event instance.
 *
 * @param instance Opaque event instance pointer.
 * @param loop true to enable looping, false to disable.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_SetEventLoop_t)(void* instance, bool loop);

/**
 * @brief Returns the loop count of an event instance.
 *
 * @details -1 means infinite looping, 0 means no loop, N means play N+1 times.
 *
 * @param instance Opaque event instance pointer.
 * @return Loop count, or -2 if the instance is invalid.
 */
typedef int (*SPF_SND_GetEventLoopCount_t)(void* instance);

/**
 * @brief Sets a callback on an event instance.
 *
 * @details The callback is invoked by FMOD on the audio thread. Use the
 *          SPF_SND_CALLBACK_* constants to specify which events trigger the callback.
 *
 * @param instance Opaque event instance pointer.
 * @param callback The callback function. Pass NULL to remove the callback.
 * @param callback_mask Bitmask of SPF_SND_CALLBACK_* constants specifying which events to listen for.
 * @return true on success, false if the instance is invalid.
 */
typedef bool (*SPF_SND_SetEventCallback_t)(void* instance, SPF_SND_EventCallbackFn callback, uint32_t callback_mask);

// =================================================================================================
// LISTENER CONTROL [FMOD LAYER]
// =================================================================================================

/**
 * @brief Returns the number of active audio listeners.
 *
 * @details Most games use 1 listener. Stereo 3D setups may use 2.
 *
 * @return Listener count, or -1 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetNumListeners_t)();

/**
 * @brief Sets the number of active audio listeners.
 *
 * @param count Number of listeners (typically 1 or 2).
 * @return true on success, false if the count is out of range.
 */
typedef bool (*SPF_SND_SetNumListeners_t)(int count);

/**
 * @brief Returns the 3D attributes of a listener.
 *
 * @details Listeners are indexed from 0. All vectors use the SCS coordinate system.
 *
 * @param index Listener index (0 to SND_GetNumListeners()-1).
 * @param out_pos_x Position X. May be NULL.
 * @param out_pos_y Position Y. May be NULL.
 * @param out_pos_z Position Z. May be NULL.
 * @param out_vel_x Velocity X. May be NULL.
 * @param out_vel_y Velocity Y. May be NULL.
 * @param out_vel_z Velocity Z. May be NULL.
 * @param out_fwd_x Forward X. May be NULL.
 * @param out_fwd_y Forward Y. May be NULL.
 * @param out_fwd_z Forward Z. May be NULL.
 * @param out_up_x Up X. May be NULL.
 * @param out_up_y Up Y. May be NULL.
 * @param out_up_z Up Z. May be NULL.
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_GetListenerAttributes_t)(int index, float* out_pos_x, float* out_pos_y, float* out_pos_z, float* out_vel_x, float* out_vel_y, float* out_vel_z, float* out_fwd_x, float* out_fwd_y, float* out_fwd_z, float* out_up_x, float* out_up_y, float* out_up_z);

/**
 * @brief Sets the 3D attributes of a listener.
 *
 * @param index Listener index.
 * @param pos_x Position X.
 * @param pos_y Position Y.
 * @param pos_z Position Z.
 * @param vel_x Velocity X.
 * @param vel_y Velocity Y.
 * @param vel_z Velocity Z.
 * @param fwd_x Forward X.
 * @param fwd_y Forward Y.
 * @param fwd_z Forward Z.
 * @param up_x Up X.
 * @param up_y Up Y.
 * @param up_z Up Z.
 * @return true on success, false if the index is invalid.
 */
typedef bool (*SPF_SND_SetListenerAttributes_t)(int index, float pos_x, float pos_y, float pos_z, float vel_x, float vel_y, float vel_z, float fwd_x, float fwd_y, float fwd_z, float up_x, float up_y, float up_z);

// =================================================================================================
// BANK MANAGEMENT [FMOD LAYER]
// =================================================================================================

/**
 * @brief Loads a bank file from disk and optionally resolves event GUIDs from a .guids dictionary.
 *
 * @details The bank is loaded synchronously with FMOD_STUDIO_BANK_LOAD_SAMPLE_DATA.
 *          If guids_path is non-null and non-empty, the file is parsed as a
 *          GUID-to-path dictionary ({UUID} type:/path format). If guids_path is
 *          null, the function auto-discovers a file named "{bank_path}.guids"
 *          in the same directory.
 *
 *          The dictionary enables path-based event lookup (SND_FindEventIndexByPath)
 *          for plugin-bank events that FMOD cannot resolve internally.
 *
 *          FMOD layer: the game cannot see or manage this bank — it does not
 *          appear in the game bank list and the game will not unload it.
 *
 * @param bank_path Filesystem path to the .bank file.
 * @param guids_path Path to the GUIDs dictionary file, or null for auto-discovery.
 * @return Opaque bank pointer, or NULL on failure.
 */
typedef void* (*SPF_SND_LoadBankFile_t)(const char* bank_path, const char* guids_path);

/**
 * @brief Loads a bank from a memory buffer and optionally resolves event GUIDs from a dictionary.
 *
 * @details FMOD layer: the bank is loaded directly into FMOD — the game does not
 *          see it, does not list it and will not unload it. The plugin owns the
 *          buffer lifetime and the bank lifetime (unload with SND_UnloadBank()).
 *          Same GUID dictionary semantics as SND_LoadBankFile().
 *
 * @param data Pointer to the raw .bank file bytes.
 * @param size Size of the buffer in bytes.
 * @param guids_path Path to the GUIDs dictionary file, or null for auto-discovery.
 * @return Opaque bank pointer, or NULL on failure.
 */
typedef void* (*SPF_SND_LoadBankMemory_t)(const void* data, uint32_t size, const char* guids_path);

/**
 * @brief Returns the loading state of a bank.
 *
 * @return Loading state:
 *         - 0: FMOD_STUDIO_BANK_STATE_UNLOADED
 *         - 1: FMOD_STUDIO_BANK_STATE_LOADING
 *         - 2: FMOD_STUDIO_BANK_STATE_LOADED
 *         - 3: FMOD_STUDIO_BANK_STATE_ERROR
 *         - -1: invalid bank pointer.
 */
typedef int (*SPF_SND_GetBankLoadingState_t)(void* bank);

/**
 * @brief Returns the number of events defined in a bank.
 *
 * @param bank Opaque bank pointer from SND_LoadBankFile().
 * @return Event count, or -1 if the bank pointer is invalid.
 */
typedef int (*SPF_SND_GetBankEventCount_t)(void* bank);

/**
 * @brief Retrieves the GUID of an event in a bank by index.
 *
 * @param bank Opaque bank pointer from SND_LoadBankFile().
 * @param index Zero-based event index within the bank.
 * @param out_guid Buffer of at least 16 bytes to receive the event GUID.
 * @return 1 on success, 0 if the index is invalid or the bank pointer is invalid.
 */
typedef int (*SPF_SND_GetBankEventGuid_t)(void* bank, int index, uint8_t out_guid[16]);

/**
 * @brief Retrieves the path of an event in a bank by index.
 *
 * @details Attempts FMOD's EventDescription_GetPath first. If that fails
 *          (common for plugin-loaded banks), falls back to the GUID dictionary
 *          loaded via SND_LoadBankFile().
 *
 * @param bank Opaque bank pointer from SND_LoadBankFile().
 * @param index Zero-based event index within the bank.
 * @param out_buffer Buffer to receive the event path string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The path length excluding the null terminator, or 0 on failure.
 */
typedef int (*SPF_SND_GetBankEventPath_t)(void* bank, int index, char* out_buffer, int buffer_size);

/**
 * @brief Returns the total number of loaded banks.
 *
 * @return Bank count, or 0 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetBankCount_t)();

/**
 * @brief Copies the path of a loaded bank into the provided buffer.
 *
 * @param index Zero-based bank index.
 * @param out_buffer Buffer to receive the bank path string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full path length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetBankPath_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Unloads a bank and frees its resources.
 *
 * @details All events from this bank must be stopped and released before unloading.
 *
 * @param bank Opaque bank pointer.
 * @return true on success, false if the bank pointer is invalid.
 */
typedef bool (*SPF_SND_UnloadBank_t)(void* bank);

// =================================================================================================
// EVENT DESCRIPTION INTROSPECTION [FMOD LAYER]
// =================================================================================================

/**
 * @brief Returns the number of parameters defined on an event.
 *
 * @param event_index Zero-based event index from enumeration.
 * @return Parameter count, or 0 if the index is invalid.
 */
typedef int (*SPF_SND_GetEventParameterCount_t)(int event_index);

/**
 * @brief Returns information about a parameter by index.
 *
 * @param event_index Zero-based event index from enumeration.
 * @param param_index Zero-based parameter index.
 * @param out_name Buffer to receive the parameter name. May be NULL.
 * @param name_size Size of the name buffer in bytes.
 * @param out_min Pointer to receive the minimum value. May be NULL.
 * @param out_max Pointer to receive the maximum value. May be NULL.
 * @param out_default Pointer to receive the default value. May be NULL.
 * @return true on success, false if index is invalid.
 */
typedef bool (*SPF_SND_GetEventParameterByIndex_t)(int event_index, int param_index, char* out_name, int name_size, float* out_min, float* out_max, float* out_default);

/**
 * @brief Returns the number of user properties defined on an event.
 *
 * @param event_index Zero-based event index from enumeration.
 * @return User property count, or 0 if the index is invalid.
 */
typedef int (*SPF_SND_GetEventUserPropertyCount_t)(int event_index);

/**
 * @brief Returns information about a user property by index.
 *
 * @param event_index Zero-based event index from enumeration.
 * @param prop_index Zero-based user property index.
 * @param out_name Buffer to receive the property name. May be NULL.
 * @param name_size Size of the name buffer in bytes.
 * @param out_type Pointer to receive the property type (0=bool, 1=int, 2=float, 3=string). May be NULL.
 * @return true on success, false if index is invalid.
 */
typedef bool (*SPF_SND_GetEventUserPropertyByIndex_t)(int event_index, int prop_index, char* out_name, int name_size, int* out_type);

/**
 * @brief Returns the compressed sound size of an event in bytes.
 *
 * @param event_index Zero-based event index from enumeration.
 * @return Sound size in bytes, or 0 if unknown or index is invalid.
 */
typedef uint32_t (*SPF_SND_GetEventSoundSize_t)(int event_index);

/**
 * @brief Returns the sample loading state of an event.
 *
 * @details Sample loading state indicates whether the event's audio samples
 *          are loaded into memory, still loading, or not loaded.
 *
 * @param event_index Zero-based event index from enumeration.
 * @return Sample loading state:
 *         - 0: Not loaded
 *         - 1: Loading
 *         - 2: Loaded
 *         - -1: invalid index.
 */
typedef int (*SPF_SND_GetEventSampleLoadingState_t)(int event_index);

// =================================================================================================
// FMOD HOOK OVERRIDES [FMOD LAYER — INTERCEPTS THE GAME'S OWN FMOD READS]
// =================================================================================================

/**
 * @brief Overrides a named parameter for all instances of a specific event.
 *
 * @details This operates at the FMOD hook level — the override is applied every
 *          time the event's parameter is read by FMOD, regardless of which instance
 *          triggered the read. Use SND_RemoveParameterOverride() to revert.
 *
 * @param event_path FMOD Studio event path (e.g. "event:/SFX/Engine").
 * @param param_name Parameter name to override.
 * @param value Override value.
 */
typedef void (*SPF_SND_OverrideParameter_t)(const char* event_path, const char* param_name, float value);

/**
 * @brief Removes a parameter override for a specific event.
 *
 * @param event_path FMOD Studio event path.
 * @param param_name Parameter name to stop overriding.
 */
typedef void (*SPF_SND_RemoveParameterOverride_t)(const char* event_path, const char* param_name);

/**
 * @brief Overrides the 3D position for all instances of a specific event.
 *
 * @details This operates at the FMOD hook level, overriding spatial positioning
 *          for every read of the event's 3D attributes.
 *
 * @param event_path FMOD Studio event path.
 * @param pos_x Position X.
 * @param pos_y Position Y.
 * @param pos_z Position Z.
 */
typedef void (*SPF_SND_Override3DPosition_t)(const char* event_path, float pos_x, float pos_y, float pos_z);

/**
 * @brief Removes the 3D override for a specific event, restoring original positioning.
 *
 * @param event_path FMOD Studio event path.
 */
typedef void (*SPF_SND_Remove3DOverride_t)(const char* event_path);

/**
 * @brief Resets 3D attributes for a specific event to the original game values.
 *
 * @details Unlike SND_Remove3DOverride(), this restores the exact values the game
 *          last provided, rather than just removing the hook override.
 *
 * @param event_path FMOD Studio event path.
 */
typedef void (*SPF_SND_Reset3DToOriginal_t)(const char* event_path);

/**
 * @brief Returns whether any FMOD hook overrides are currently active.
 *
 * @return true if at least one override exists, false otherwise.
 */
typedef bool (*SPF_SND_HasOverrides_t)();

/**
 * @brief Removes ALL active FMOD hook overrides (parameters, 3D, volume, pitch).
 *
 * @details After this call, all events revert to the game's original parameter,
 *          3D attribute, volume and pitch values. Suppression prefixes
 *          (SND_SuppressEventPlayback) are NOT affected — remove them explicitly.
 */
typedef void (*SPF_SND_RemoveAllOverrides_t)();

/**
 * @brief Forces the FMOD volume of every instance of an event path (FMOD hook layer).
 *
 * @details Intercepts EventInstance::setVolume for instances whose resolved
 *          event path equals @p event_path and replaces the value the game
 *          requested. The game's own volume logic keeps running — its writes
 *          are just replaced at the FMOD boundary. Remove the override to let
 *          the game's values through again.
 *
 * @param event_path Exact FMOD event path (e.g. "event:/horn/truck").
 * @param volume Forced volume (0.0 = silent, 1.0 = full).
 */
typedef void (*SPF_SND_OverrideEventVolume_t)(const char* event_path, float volume);

/**
 * @brief Removes a volume override installed by SND_OverrideEventVolume.
 *
 * @param event_path Exact FMOD event path the override was installed for.
 */
typedef void (*SPF_SND_RemoveEventVolumeOverride_t)(const char* event_path);

/**
 * @brief Forces the FMOD pitch of every instance of an event path (FMOD hook layer).
 *
 * @details Same interception model as SND_OverrideEventVolume, on
 *          EventInstance::setPitch.
 *
 * @param event_path Exact FMOD event path.
 * @param pitch Forced pitch multiplier (1.0 = unchanged).
 */
typedef void (*SPF_SND_OverrideEventPitch_t)(const char* event_path, float pitch);

/**
 * @brief Removes a pitch override installed by SND_OverrideEventPitch.
 *
 * @param event_path Exact FMOD event path the override was installed for.
 */
typedef void (*SPF_SND_RemoveEventPitchOverride_t)(const char* event_path);

// =================================================================================================
// FMOD INTERCEPTION [SUPPRESSION / ACTIVITY OBSERVATION] (FMOD hook layer)
// =================================================================================================

// Activity codes delivered to SPF_SND_ActivityCallback_t.
#define SPF_ACTIVITY_EVENT_CREATED          1   // EventDescription::createInstance returned a new instance
#define SPF_ACTIVITY_EVENT_START_SUPPRESSED 2   // EventInstance::start was blocked by SND_SuppressEventPlayback
#define SPF_ACTIVITY_EVENT_STARTED          3   // EventInstance::start executed successfully
#define SPF_ACTIVITY_EVENT_STOPPED          4   // EventInstance::stop called
#define SPF_ACTIVITY_EVENT_PAUSED           5   // EventInstance::setPaused(true) called
#define SPF_ACTIVITY_EVENT_UNPAUSED         6   // EventInstance::setPaused(false) called
#define SPF_ACTIVITY_EVENT_RELEASED         7   // EventInstance::release called (path resolved before release)
#define SPF_ACTIVITY_EVENT_PARAM_SET        8   // setParameterByName/ByID called (param_name/param_value valid)
#define SPF_ACTIVITY_BANK_LOADED            9   // System::loadBank* succeeded (path = bank path)
#define SPF_ACTIVITY_BANK_UNLOADING         10  // Bank::unload called (path = bank path)

/**
 * @brief Callback observing FMOD activity — the interception layer's event feed.
 *
 * @details Invoked synchronously from inside the intercepting detour, on the
 *          thread that made the FMOD call (normally the game thread) — there is
 *          no queuing. @p path and @p param_name are valid only for the
 *          duration of the call; copy what you need. Nested delivery is
 *          suppressed (activity fired from inside your own callback is
 *          dropped), but calling SND_* FMOD functions from the callback is
 *          still unsafe — buffer the data and act on it later (e.g. in your
 *          OnUpdate).
 *
 * @param user_data Pointer passed to SND_SetActivityCallback.
 * @param activity SPF_ACTIVITY_* code.
 * @param path Event path, or bank path for SPF_ACTIVITY_BANK_*; "" if unresolvable.
 * @param instance FMOD instance involved (NULL for bank activities).
 * @param param_name Parameter name for SPF_ACTIVITY_EVENT_PARAM_SET, else NULL.
 * @param param_value Effective value passed to FMOD for SPF_ACTIVITY_EVENT_PARAM_SET, else 0.
 */
typedef void (*SPF_SND_ActivityCallback_t)(void* user_data, int activity, const char* path,
                                           void* instance, const char* param_name, float param_value);

/**
 * @brief Registers (or with NULL, unregisters) the FMOD activity callback.
 *
 * @details One callback per process; a new registration replaces the previous
 *          one. Survives until replaced or the hook is uninstalled.
 *
 * @param callback Callback function, or NULL to unregister.
 * @param user_data Opaque pointer delivered back to @p callback.
 */
typedef void (*SPF_SND_SetActivityCallback_t)(SPF_SND_ActivityCallback_t callback, void* user_data);

/**
 * @brief Blocks playback start for every event path under a prefix (FMOD hook layer).
 *
 * @details Installs a prefix rule on the EventInstance::start detour: any start
 *          call for an event path beginning with @p path_prefix returns FMOD_OK
 *          without playing — the game's horn never reaches the speakers, from
 *          the first sample. The blocked start still counts: watch
 *          SND_GetSuppressedStartCount for press edges. The game may keep
 *          calling setParameter on the silent instance (e.g. "play") — those
 *          writes pass through untouched. Matching is case-sensitive prefix
 *          match; the rule applies to instances created before or after the
 *          call. Suppression rules survive until SND_UnsuppressEventPlayback.
 *
 * @param path_prefix Event path prefix (e.g. "event:/horn/").
 * @return true when the start hook is active and the rule installed; false on
 *         empty prefix or when the hook is unavailable.
 */
typedef bool (*SPF_SND_SuppressEventPlayback_t)(const char* path_prefix);

/**
 * @brief Removes a suppression rule installed by SND_SuppressEventPlayback.
 *
 * @details Instances already blocked stay silent until their next start call,
 *          which now plays normally.
 *
 * @param path_prefix The prefix rule to remove.
 * @return true when a rule was found and removed; false otherwise.
 */
typedef bool (*SPF_SND_UnsuppressEventPlayback_t)(const char* path_prefix);

/**
 * @brief Returns how many start attempts were suppressed under a prefix.
 *
 * @details The counter resets when the prefix rule is removed or the hook
 *          reinstalled. Compare against a stored baseline to detect presses —
 *          the value only grows while the rule exists.
 *
 * @param path_prefix The suppression prefix.
 * @return Suppressed-start attempt count, or 0 when no rule exists.
 */
typedef uint64_t (*SPF_SND_GetSuppressedStartCount_t)(const char* path_prefix);

/**
 * @brief Returns the FMOD instance of the most recent suppressed start under a prefix.
 *
 * @details Use it to read per-instance state the game keeps on the silent
 *          instance (e.g. the "play" parameter) to detect release. The pointer
 *          becomes NULL once the game releases that instance; never cache it
 *          across frames — call this every frame instead. The instance is
 *          stopped (its start was blocked) but alive and queryable.
 *
 * @param path_prefix The suppression prefix.
 * @return FMOD instance pointer, or NULL when none / already released.
 */
typedef void* (*SPF_SND_GetLastSuppressedInstance_t)(const char* path_prefix);

// =================================================================================================
// GAME sound_event CONTROL [GAME LAYER]
// =================================================================================================

/**
 * @brief Returns the number of live game sound_event_t objects.
 *
 * @details Walks per-bank event lists and builds a fresh snapshot on every
 *          call. Handles obtained via SND_GetGameEventAt stay valid while the
 *          event exists — prefer fetching handles once over repeated counts.
 *
 * @return Event count, or 0 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetGameEventCount_t)();

/**
 * @brief Returns the sound_event_t handle at @p index within a fresh snapshot.
 *
 * @param index Zero-based index (0 to SND_GetGameEventCount()-1 of the same snapshot).
 * @return Opaque handle, or NULL when out of range.
 */
typedef void* (*SPF_SND_GetGameEventAt_t)(int index);

/**
 * @brief Finds the first game sound_event whose path equals @p path.
 *
 * @param path Exact event path string (game's own path field).
 * @return Opaque handle, or NULL if no event matches.
 */
typedef void* (*SPF_SND_FindGameEventByPath_t)(const char* path);

/**
 * @brief Finds the first game sound_event whose source equals @p source.
 *
 * @param source Exact "bank#event" source string.
 * @return Opaque handle, or NULL if no event matches.
 */
typedef void* (*SPF_SND_FindGameEventBySource_t)(const char* source);

/**
 * @brief Finds the game sound_event that owns @p instance.
 *
 * Reverse FMOD→game mapping: matches the instance field of every live
 * sound_event against @p instance.
 *
 * @param instance FMOD EventInstance* handle.
 * @return Opaque game event handle, or NULL if no game event owns it
 *         (plugin-created / foreign instances resolve to NULL).
 */
typedef void* (*SPF_SND_FindGameEventByInstance_t)(void* instance);

/**
 * @brief Reads the playback state dword of a game sound_event.
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @return SPF_SOUND_EVENT_STATE_* value; 0 if unavailable.
 */
typedef uint32_t (*SPF_SND_GetGameEventPlaybackState_t)(void* event);

/**
 * @brief Reports whether the game event is currently bound (state == 2).
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @return true when bound, false otherwise.
 */
typedef bool (*SPF_SND_IsGameEventBound_t)(void* event);

/**
 * @brief Recreates the event's EventInstance from its current source.
 *
 * @details Stops and releases the previous instance first (never leaks), then
 *          runs the game's own activate path. Does NOT auto-start playback —
 *          call SND_GameEvent_Start for that. Game thread only.
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @return true when activate succeeded.
 */
typedef bool (*SPF_SND_GameEvent_Activate_t)(void* event);

/**
 * @brief Starts (or resumes) a game sound_event through its vtable PlaybackControl.
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_Start_t)(void* event);

/**
 * @brief Stops a game sound_event through its vtable Stop; instance is kept
 *        until the next SND_GameEvent_Activate.
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_Stop_t)(void* event);

/**
 * @brief Pauses or resumes a game sound_event through its vtable SetPaused.
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @param paused true to pause, false to resume.
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_SetPaused_t)(void* event, bool paused);

/**
 * @brief Sets event volume through its vtable SetVolume (also cached by the game at +0x80).
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @param volume New volume (game convention 0..1).
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_SetVolume_t)(void* event, float volume);

/**
 * @brief Sets event pitch through its vtable SetPitch (cached by the game at +0x84).
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @param pitch New pitch multiplier (1.0 = normal).
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_SetPitch_t)(void* event, float pitch);

/**
 * @brief Sets a raw FMOD event property through its vtable SetProperty (cached at +0x88).
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @param property_id FMOD event property id (0 = frequency modulation depth, ...).
 * @param value New property value.
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_SetProperty_t)(void* event, int property_id, float value);

/**
 * @brief Sets 3D position through its vtable Set3DAttributes.
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @param pos_x,pos_y,pos_z World position; velocity is zeroed by this overload.
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_Set3DAttributes_t)(void* event, float pos_x, float pos_y, float pos_z);

/**
 * @brief Sets a parameter on a game sound_event by its 16-byte FMOD ID.
 *
 * @param event Handle from SND_FindGameEventByPath/BySource/At.
 * @param id 16-byte parameter GUID (same layout as SPD_GET_PARAMETER_ID_GUID).
 * @param value New parameter value.
 * @return true on success.
 */
typedef bool (*SPF_SND_GameEvent_SetParameterByID_t)(void* event, const uint8_t id[16], float value);

// SoundEvent playback state (sound_event_t+0x30) — SPF_SND_GetGameEventPlaybackState values
#define SPF_SOUND_EVENT_STATE_STOPPED 0u  // stopped manually / never started
#define SPF_SOUND_EVENT_STATE_PLAYING 1u
#define SPF_SOUND_EVENT_STATE_PAUSED 3u
#define SPF_SOUND_EVENT_STATE_ENDED 4u    // finished on its own

// =================================================================================================
// SOUNDREF CATALOG & REBINDING [GAME LAYER]
// =================================================================================================

/**
 * @brief Returns the number of known .soundref paths.
 *
 * @details The catalog merges all sources: static game tables (UI, voice-nav),
 *          ".soundref" path references found in the game binary, files enumerated
 *          from the game VFS, and currently live sound events. Entries with a live
 *          event additionally expose their current "bank#event" source.
 *
 * @return SoundRef count, or 0 if the sound system is not ready.
 */
typedef int (*SPF_SND_GetSoundRefCount_t)();

/**
 * @brief Copies the .soundref path of a catalog entry into the provided buffer.
 *
 * @param index Zero-based catalog index (0 to SND_GetSoundRefCount()-1).
 * @param out_buffer Buffer to receive the path string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full path length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetSoundRefPath_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Copies the current "bank#event" source of a catalog entry.
 *
 * @details Non-empty only when the game has a live sound_event for this path
 *          (SND_IsSoundRefActive). Reflects any registered override once applied.
 *
 * @param index Zero-based catalog index.
 * @param out_buffer Buffer to receive the source string.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The full source length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_SND_GetSoundRefSource_t)(int index, char* out_buffer, int buffer_size);

/**
 * @brief Searches the catalog for a ".soundref" path.
 *
 * @param soundref_path Full .soundref path (e.g. "/sound/ui/ui_click.soundref").
 * @return Zero-based catalog index, or -1 if not found.
 */
typedef int (*SPF_SND_FindSoundRefIndex_t)(const char* soundref_path);

/**
 * @brief Searches for the first catalog entry whose live source equals the given "bank#event".
 *
 * @details Useful to discover which soundref currently points at a given event,
 *          e.g. before rebinding everything that plays "music/main_menu" from a
 *          specific bank.
 *
 * @param source "bank#event" string (e.g. "sound/ui/ui.bank#click").
 * @return Zero-based catalog index, or -1 if not found.
 */
typedef int (*SPF_SND_FindSoundRefBySource_t)(const char* source);

/**
 * @brief Returns whether the game currently has a live sound_event for the path.
 *
 * @param soundref_path Full .soundref path.
 * @return true if a live event exists, false otherwise.
 */
typedef bool (*SPF_SND_IsSoundRefActive_t)(const char* soundref_path);

/**
 * @brief PURE registration of a ".soundref" -> "bank#event" override (map only).
 *
 * @details No side effects at call time: nothing is applied to live events, no
 *          bank is loaded, no instance is stopped or started. The override is
 *          consulted inside the game's SoundRef load pipeline, so it takes
 *          effect on the soundref's NEXT activation by the game. For immediate
 *          audible replacement use SND_SoundRef_Replace.
 *
 * @param soundref_path Full .soundref path to rebind.
 * @param source New source in "bank#event" format (e.g. "my_bank#ui/click_v2").
 * @return true if the override was registered, false on invalid arguments.
 */
typedef bool (*SPF_SND_RegisterSoundRefOverride_t)(const char* soundref_path, const char* source);

/**
 * @brief AUTOMATIC full-cycle replacement — the one-stop "swap my sound" call.
 *
 * @details Single call does everything, in order:
 *          1) registers the override (same map as SND_RegisterSoundRefOverride);
 *          2) rewrites the source field of every LIVE game event with this path;
 *          3) queues a rebind executed on the game thread: stop current instance
 *             -> release it -> activate from the new source -> start again if the
 *             event was playing/paused before. During activate the GAME loads the
 *             target bank itself through its VFS (part of source before '#'), so
 *             the bank's VFS directory must be mounted first (Env_VfsMount).
 *          Events not live right now pick the override up automatically on their
 *          next activation (same as pure SND_RegisterSoundRefOverride).
 *
 * @param soundref_path Full .soundref path to rebind.
 * @param source New source in "bank#event" format; before '#' is the VFS bank path.
 * @return true if registered and the apply pass ran, false on invalid arguments.
 */
typedef bool (*SPF_SND_SoundRef_Replace_t)(const char* soundref_path, const char* source);

/**
 * @brief Removes one override, restoring the original "bank#event" source.
 *
 * @details Live events with this path are restored to their original source and
 *          rebound on the game thread with the same stop -> activate -> resume
 *          lifecycle, so a playing replacement stops and the original resumes.
 *          A bank that was auto-loaded by SND_SoundRef_Replace stays loaded
 *          (silent); unload it explicitly with SND_UnloadBank if needed.
 *
 * @param soundref_path Full .soundref path.
 * @return true if an override existed and was removed, false otherwise.
 */
typedef bool (*SPF_SND_UnregisterSoundRefOverride_t)(const char* soundref_path);

/**
 * @brief Removes ALL registered SoundRef overrides, restoring every original source.
 */
typedef void (*SPF_SND_ClearSoundRefOverrides_t)();

/**
 * @brief Copies the currently registered override source for a path.
 *
 * @param soundref_path Full .soundref path.
 * @param out_buffer Buffer to receive the "bank#event" source.
 * @param buffer_size Size of the output buffer in bytes.
 * @return The source length (0 if no override is registered), or -1 on invalid arguments.
 */
typedef int (*SPF_SND_GetSoundRefOverride_t)(const char* soundref_path, char* out_buffer, int buffer_size);

// =================================================================================================
// API STRUCTURE
// =================================================================================================

/**
 * @struct SPF_Sound_API
 * @brief Complete sound system API for plugins.
 *
 * @details This structure provides access to the FMOD Studio sound system. A pointer
 *          to it is available in the SPF_Core_API::sound field after OnActivated().
 *
 *          All functions require the sound system to be ready (SND_IsReady() returns true).
 *          Using functions before the system is ready will return safe default values
 *          (false, 0, -1, or empty strings).
 *
 * **ABI Rule**: New function pointers are only appended to the end of this structure.
 */
typedef struct SPF_Sound_API {
  /**
   * @brief Service lifecycle.
   * @{
   */
  SPF_SND_IsReady_t SND_IsReady;
  SPF_SND_AreAllOffsetsFound_t SND_AreAllOffsetsFound;
  SPF_SND_RefreshOffsets_t SND_RefreshOffsets;
  /** @} */

  /**
   * @brief Bus enumeration and control (FMOD layer).
   * @{
   */
  SPF_SND_GetBusCount_t SND_GetBusCount;
  SPF_SND_GetBusPath_t SND_GetBusPath;
  SPF_SND_GetBusVolume_t SND_GetBusVolume;
  SPF_SND_SetBusVolume_t SND_SetBusVolume;
  SPF_SND_GetBusMute_t SND_GetBusMute;
  SPF_SND_SetBusMute_t SND_SetBusMute;
  SPF_SND_GetBusPause_t SND_GetBusPause;
  SPF_SND_SetBusPause_t SND_SetBusPause;
  /** @} */

  /**
   * @brief VCA (Volume Control Association) enumeration and control (FMOD layer).
   * @{
   */
  SPF_SND_GetVCACount_t SND_GetVCACount;
  SPF_SND_GetVCAPath_t SND_GetVCAPath;
  SPF_SND_GetVCAVolume_t SND_GetVCAVolume;
  SPF_SND_SetVCAVolume_t SND_SetVCAVolume;
  /** @} */

  /**
   * @brief Global parameter enumeration and control (FMOD layer).
   * @{
   */
  SPF_SND_GetGlobalParamCount_t SND_GetGlobalParamCount;
  SPF_SND_GetGlobalParamName_t SND_GetGlobalParamName;
  SPF_SND_GetGlobalParamRange_t SND_GetGlobalParamRange;
  SPF_SND_GetGlobalParamValue_t SND_GetGlobalParamValue;
  SPF_SND_SetGlobalParamValue_t SND_SetGlobalParamValue;
  /** @} */

  /**
   * @brief Event enumeration — bank catalog metadata (FMOD layer).
   * @{
   */
  SPF_SND_GetEventCount_t SND_GetEventCount;
  SPF_SND_GetEventBankPath_t SND_GetEventBankPath;
  SPF_SND_GetEventPath_t SND_GetEventPath;
  SPF_SND_GetEventGuid_t SND_GetEventGuid;
  SPF_SND_IsEvent3D_t SND_IsEvent3D;
  SPF_SND_IsEventOneshot_t SND_IsEventOneshot;
  SPF_SND_IsEventStream_t SND_IsEventStream;
  SPF_SND_IsEventSnapshot_t SND_IsEventSnapshot;
  SPF_SND_GetEventDurationMs_t SND_GetEventDurationMs;
  SPF_SND_GetEventMinDistance_t SND_GetEventMinDistance;
  SPF_SND_GetEventMaxDistance_t SND_GetEventMaxDistance;
  SPF_SND_FindEventIndexByPath_t SND_FindEventIndexByPath;
  SPF_SND_FindEventIndexByPrefix_t SND_FindEventIndexByPrefix;
  SPF_SND_FindEventIndexByGuid_t SND_FindEventIndexByGuid;
  SPF_SND_GetEventLiveInstanceCount_t SND_GetEventLiveInstanceCount;
  SPF_SND_GetEventLiveInstance_t SND_GetEventLiveInstance;
  /** @} */

  /**
   * @brief Event instance playback control — plugin-driven playback (FMOD layer).
   * @{
   */
  SPF_SND_CreateEventInstance_t SND_CreateEventInstance;
  SPF_SND_StartEvent_t SND_StartEvent;
  SPF_SND_StopEvent_t SND_StopEvent;
  SPF_SND_PauseEvent_t SND_PauseEvent;
  SPF_SND_GetEventPlaybackState_t SND_GetEventPlaybackState;
  SPF_SND_ReleaseEvent_t SND_ReleaseEvent;
  /** @} */

  /**
   * @brief Event instance property control (FMOD layer).
   * @{
   */
  SPF_SND_SetEventVolume_t SND_SetEventVolume;
  SPF_SND_GetEventVolume_t SND_GetEventVolume;
  SPF_SND_SetEventPitch_t SND_SetEventPitch;
  SPF_SND_GetEventPitch_t SND_GetEventPitch;
  SPF_SND_SetEvent3DAttributes_t SND_SetEvent3DAttributes;
  SPF_SND_GetEvent3DAttributes_t SND_GetEvent3DAttributes;
  SPF_SND_SetEventParameter_t SND_SetEventParameter;
  SPF_SND_GetEventParameter_t SND_GetEventParameter;
  SPF_SND_SetEventTimelinePosition_t SND_SetEventTimelinePosition;
  SPF_SND_GetEventTimelinePosition_t SND_GetEventTimelinePosition;
  SPF_SND_SetEventLoop_t SND_SetEventLoop;
  SPF_SND_GetEventLoopCount_t SND_GetEventLoopCount;
  SPF_SND_SetEventCallback_t SND_SetEventCallback;
  /** @} */

  /**
   * @brief Listener control (FMOD layer).
   * @{
   */
  SPF_SND_GetNumListeners_t SND_GetNumListeners;
  SPF_SND_SetNumListeners_t SND_SetNumListeners;
  SPF_SND_GetListenerAttributes_t SND_GetListenerAttributes;
  SPF_SND_SetListenerAttributes_t SND_SetListenerAttributes;
  /** @} */

  /**
   * @brief Bank management — banks loaded here are invisible to the game (FMOD layer).
   * @{
   */
  SPF_SND_LoadBankFile_t SND_LoadBankFile;
  SPF_SND_LoadBankMemory_t SND_LoadBankMemory;
  SPF_SND_GetBankLoadingState_t SND_GetBankLoadingState;
  SPF_SND_GetBankEventCount_t SND_GetBankEventCount;
  SPF_SND_GetBankEventGuid_t SND_GetBankEventGuid;
  SPF_SND_GetBankEventPath_t SND_GetBankEventPath;
  SPF_SND_GetBankCount_t SND_GetBankCount;
  SPF_SND_GetBankPath_t SND_GetBankPath;
  SPF_SND_UnloadBank_t SND_UnloadBank;
  /** @} */

  /**
   * @brief FMOD hook overrides — intercept the game's own FMOD reads (FMOD hook layer).
   * @{
   */
  SPF_SND_OverrideParameter_t SND_OverrideParameter;
  SPF_SND_RemoveParameterOverride_t SND_RemoveParameterOverride;
  SPF_SND_Override3DPosition_t SND_Override3DPosition;
  SPF_SND_Remove3DOverride_t SND_Remove3DOverride;
  SPF_SND_Reset3DToOriginal_t SND_Reset3DToOriginal;
  SPF_SND_HasOverrides_t SND_HasOverrides;
  SPF_SND_RemoveAllOverrides_t SND_RemoveAllOverrides;
  SPF_SND_OverrideEventVolume_t SND_OverrideEventVolume;
  SPF_SND_RemoveEventVolumeOverride_t SND_RemoveEventVolumeOverride;
  SPF_SND_OverrideEventPitch_t SND_OverrideEventPitch;
  SPF_SND_RemoveEventPitchOverride_t SND_RemoveEventPitchOverride;
  /** @} */

  /**
   * @brief FMOD interception — suppression and activity observation (FMOD hook layer).
   * @{
   */
  SPF_SND_SetActivityCallback_t SND_SetActivityCallback;
  SPF_SND_SuppressEventPlayback_t SND_SuppressEventPlayback;
  SPF_SND_UnsuppressEventPlayback_t SND_UnsuppressEventPlayback;
  SPF_SND_GetSuppressedStartCount_t SND_GetSuppressedStartCount;
  SPF_SND_GetLastSuppressedInstance_t SND_GetLastSuppressedInstance;
  /** @} */

  /**
   * @brief Event description introspection — parameters, user properties, sample state (FMOD layer).
   * @{
   */
  SPF_SND_GetEventParameterCount_t SND_GetEventParameterCount;
  SPF_SND_GetEventParameterByIndex_t SND_GetEventParameterByIndex;
  SPF_SND_GetEventUserPropertyCount_t SND_GetEventUserPropertyCount;
  SPF_SND_GetEventUserPropertyByIndex_t SND_GetEventUserPropertyByIndex;
  SPF_SND_GetEventSoundSize_t SND_GetEventSoundSize;
  SPF_SND_GetEventSampleLoadingState_t SND_GetEventSampleLoadingState;
  /** @} */

  /**
   * @brief Game sound_event control — direct handles into the game's own events (game layer).
   * @details GAME THREAD ONLY: these calls reach FMOD internals and the game's
   *          vtable dispatch. Handles are raw game pointers — valid while the
   *          event exists; bank unload destroys its events, re-query after.
   * @{
   */
  SPF_SND_GetGameEventCount_t SND_GetGameEventCount;
  SPF_SND_GetGameEventAt_t SND_GetGameEventAt;
  SPF_SND_FindGameEventByPath_t SND_FindGameEventByPath;
  SPF_SND_FindGameEventBySource_t SND_FindGameEventBySource;
  SPF_SND_FindGameEventByInstance_t SND_FindGameEventByInstance;
  SPF_SND_GetGameEventPlaybackState_t SND_GetGameEventPlaybackState;
  SPF_SND_IsGameEventBound_t SND_IsGameEventBound;
  SPF_SND_GameEvent_Activate_t SND_GameEvent_Activate;
  SPF_SND_GameEvent_Start_t SND_GameEvent_Start;
  SPF_SND_GameEvent_Stop_t SND_GameEvent_Stop;
  SPF_SND_GameEvent_SetPaused_t SND_GameEvent_SetPaused;
  SPF_SND_GameEvent_SetVolume_t SND_GameEvent_SetVolume;
  SPF_SND_GameEvent_SetPitch_t SND_GameEvent_SetPitch;
  SPF_SND_GameEvent_SetProperty_t SND_GameEvent_SetProperty;
  SPF_SND_GameEvent_Set3DAttributes_t SND_GameEvent_Set3DAttributes;
  SPF_SND_GameEvent_SetParameterByID_t SND_GameEvent_SetParameterByID;
  /** @} */

  /**
   * @brief SoundRef catalog & rebinding — the game keeps driving the sound (game layer).
   * @{
   */
  SPF_SND_GetSoundRefCount_t SND_GetSoundRefCount;
  SPF_SND_GetSoundRefPath_t SND_GetSoundRefPath;
  SPF_SND_GetSoundRefSource_t SND_GetSoundRefSource;
  SPF_SND_FindSoundRefIndex_t SND_FindSoundRefIndex;
  SPF_SND_FindSoundRefBySource_t SND_FindSoundRefBySource;
  SPF_SND_IsSoundRefActive_t SND_IsSoundRefActive;
  SPF_SND_RegisterSoundRefOverride_t SND_RegisterSoundRefOverride;
  SPF_SND_SoundRef_Replace_t SND_SoundRef_Replace;
  SPF_SND_UnregisterSoundRefOverride_t SND_UnregisterSoundRefOverride;
  SPF_SND_ClearSoundRefOverrides_t SND_ClearSoundRefOverrides;
  SPF_SND_GetSoundRefOverride_t SND_GetSoundRefOverride;
  /** @} */
} SPF_Sound_API;

#ifdef __cplusplus
}
#endif
