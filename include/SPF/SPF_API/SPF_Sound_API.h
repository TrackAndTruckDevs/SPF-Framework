#pragma once

/**
 * @file SPF_Sound_API.h
 * @brief C-style API for the FMOD sound system, exposed to plugins.
 *
 * @details This header provides complete access to the game's FMOD Studio sound system.
 *          Plugins can enumerate sound banks, events, buses, and VCAs; control playback;
 *          adjust bus and global parameters; manage listeners; and override FMOD parameters
 *          at the hook level.
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
 *   1. Load: SND_LoadBankFile()
 *   2. Query: SND_GetBankLoadingState() / SND_GetBankEventCount()
 *   3. Discover events: SND_GetBankEventGuid() + SND_FindEventIndexByGuid()
 *   4. Unload: SND_UnloadBank()
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
// BUS ENUMERATION & CONTROL
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
// VCA (VOLUME CONTROL ASSOCIATION)
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
// GLOBAL PARAMETERS
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
// EVENT ENUMERATION
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
// EVENT PLAYBACK
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
// EVENT INSTANCE PROPERTIES
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
// LISTENER CONTROL
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
// BANK MANAGEMENT
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
 * @param bank_path Filesystem path to the .bank file.
 * @param guids_path Path to the GUIDs dictionary file, or null for auto-discovery.
 * @return Opaque bank pointer, or NULL on failure.
 */
typedef void* (*SPF_SND_LoadBankFile_t)(const char* bank_path, const char* guids_path);

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
// EVENT DESCRIPTION INTROSPECTION
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
// FMOD HOOK OVERRIDES
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
 * @brief Removes ALL active FMOD hook overrides (parameters and 3D).
 *
 * @details After this call, all events revert to the game's original parameter
 *          and 3D attribute values.
 */
typedef void (*SPF_SND_RemoveAllOverrides_t)();

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
   * @brief Bus enumeration and control.
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
   * @brief VCA (Volume Control Association) enumeration and control.
   * @{
   */
  SPF_SND_GetVCACount_t SND_GetVCACount;
  SPF_SND_GetVCAPath_t SND_GetVCAPath;
  SPF_SND_GetVCAVolume_t SND_GetVCAVolume;
  SPF_SND_SetVCAVolume_t SND_SetVCAVolume;
  /** @} */

  /**
   * @brief Global parameter enumeration and control.
   * @{
   */
  SPF_SND_GetGlobalParamCount_t SND_GetGlobalParamCount;
  SPF_SND_GetGlobalParamName_t SND_GetGlobalParamName;
  SPF_SND_GetGlobalParamRange_t SND_GetGlobalParamRange;
  SPF_SND_GetGlobalParamValue_t SND_GetGlobalParamValue;
  SPF_SND_SetGlobalParamValue_t SND_SetGlobalParamValue;
  /** @} */

  /**
   * @brief Event enumeration (read-only metadata).
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
   * @brief Event instance playback control.
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
   * @brief Event instance property control.
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
   * @brief Listener control.
   * @{
   */
  SPF_SND_GetNumListeners_t SND_GetNumListeners;
  SPF_SND_SetNumListeners_t SND_SetNumListeners;
  SPF_SND_GetListenerAttributes_t SND_GetListenerAttributes;
  SPF_SND_SetListenerAttributes_t SND_SetListenerAttributes;
  /** @} */

  /**
   * @brief Bank management.
   * @{
   */
  SPF_SND_LoadBankFile_t SND_LoadBankFile;
  SPF_SND_GetBankLoadingState_t SND_GetBankLoadingState;
  SPF_SND_GetBankEventCount_t SND_GetBankEventCount;
  SPF_SND_GetBankEventGuid_t SND_GetBankEventGuid;
  SPF_SND_GetBankEventPath_t SND_GetBankEventPath;
  SPF_SND_GetBankCount_t SND_GetBankCount;
  SPF_SND_GetBankPath_t SND_GetBankPath;
  SPF_SND_UnloadBank_t SND_UnloadBank;
  /** @} */

  /**
   * @brief FMOD hook overrides (parameter and 3D attribute injection).
   * @{
   */
  SPF_SND_OverrideParameter_t SND_OverrideParameter;
  SPF_SND_RemoveParameterOverride_t SND_RemoveParameterOverride;
  SPF_SND_Override3DPosition_t SND_Override3DPosition;
  SPF_SND_Remove3DOverride_t SND_Remove3DOverride;
  SPF_SND_Reset3DToOriginal_t SND_Reset3DToOriginal;
  SPF_SND_HasOverrides_t SND_HasOverrides;
  SPF_SND_RemoveAllOverrides_t SND_RemoveAllOverrides;
  /** @} */

  /**
   * @brief Event description introspection (parameters, user properties, sample state).
   * @{
   */
  SPF_SND_GetEventParameterCount_t SND_GetEventParameterCount;
  SPF_SND_GetEventParameterByIndex_t SND_GetEventParameterByIndex;
  SPF_SND_GetEventUserPropertyCount_t SND_GetEventUserPropertyCount;
  SPF_SND_GetEventUserPropertyByIndex_t SND_GetEventUserPropertyByIndex;
  SPF_SND_GetEventSoundSize_t SND_GetEventSoundSize;
  SPF_SND_GetEventSampleLoadingState_t SND_GetEventSampleLoadingState;
  /** @} */
} SPF_Sound_API;

#ifdef __cplusplus
}
#endif
