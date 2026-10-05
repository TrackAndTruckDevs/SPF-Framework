# SPF Sound API

The SPF Sound API provides complete access to the game's FMOD Studio sound system. Plugins can enumerate sound banks, events, buses, and VCAs; control playback; adjust bus and global parameters; manage listeners; override FMOD parameters at the hook level; suppress game sounds and observe FMOD activity; and reach into the game's own `sound_event_t` objects.

The API has two independent layers. Choose the layer by answering one question: **who should drive the sound — the plugin, or the game?**

- **FMOD layer** — the plugin talks straight to FMOD: loads banks, enumerates events, creates instances, starts/stops playback, sets parameters, mixes buses and VCAs, controls listeners. The game knows nothing about these sounds — no game logic triggers, updates or stops them. Use it when the sound is entirely plugin-owned (custom music, plugin UI sounds, notifications) or when the event does not exist in the game at all. Banks loaded via `SND_LoadBankFile` / `SND_LoadBankMemory` go directly into FMOD — the game cannot see or manage such a bank; the plugin owns its lifetime.
- **Game layer (SoundRef)** — the game maps its own sounds through `.soundref` files and activates them itself (UI clicks, horn, engine, world sounds). The plugin replaces the source the game plays, while the game keeps triggering, timing and stopping it exactly as before.

**Decision rule:** the game should keep calling the sound → SoundRef (game layer); the plugin drives playback / event not in the game → FMOD (direct layer).

## Getting the API

Request the Sound API from the framework in your plugin's `OnActivated` callback.

**Example: OnActivated callback**
```cpp
SPF_Sound_API* s_soundAPI = NULL;

void OnActivated(const SPF_Core_API* core_api) {
    s_soundAPI = core_api->sound;
}
```

## Key Concepts

1. **World-Scoped Lifecycle**: The sound system initializes when the game world loads and shuts down when the world unloads. Always check `SND_IsReady()` before using any other function.
2. **Opaque Handles**: Event instances and banks are represented as `void*` pointers. Do not cast or store them beyond their lifetime — release instances with `SND_ReleaseEvent()` and banks with `SND_UnloadBank()`.
3. **Index-Based Enumeration**: Buses, VCAs, global parameters, events, and SoundRef entries are accessed by zero-based index. Use `SND_GetBusCount()`, `SND_GetEventCount()`, `SND_GetSoundRefCount()`, etc. to determine the range.
4. **String Copy Pattern**: Functions that return strings take an output buffer and size, returning the full string length (excluding null terminator). If the buffer is too small, the string is truncated but the full length is still returned.
5. **Thread Safety**: FMOD-layer functions must be called from the game thread (FMOD's command queue is processed there only; calling from other threads corrupts FMOD state). SoundRef (game layer) functions may be called from any thread — rebinds are applied on the game tick.

## Usage Example

```c
if (!s_soundAPI || !s_soundAPI->SND_IsReady()) return;

// Enumerate buses
int busCount = s_soundAPI->SND_GetBusCount();
for (int i = 0; i < busCount; i++) {
    char path[256];
    s_soundAPI->SND_GetBusPath(i, path, sizeof(path));
    float vol = s_soundAPI->SND_GetBusVolume(i);
    printf("Bus %d: %s (volume: %.2f)\n", i, path, vol);
}

// Find and play an event
int eventIdx = s_soundAPI->SND_FindEventIndexByPath("event:/SFX/Engine");
if (eventIdx >= 0) {
    void* instance = s_soundAPI->SND_CreateEventInstance(eventIdx);
    if (instance) {
        s_soundAPI->SND_StartEvent(instance);
        s_soundAPI->SND_SetEventVolume(instance, 0.8f);
        // ... later:
        s_soundAPI->SND_StopEvent(instance, true);
        s_soundAPI->SND_ReleaseEvent(instance);
    }
}

// Override a parameter globally
s_soundAPI->SND_OverrideParameter("event:/SFX/Engine", "RPM", 2500.0f);
// Later, revert:
s_soundAPI->SND_RemoveParameterOverride("event:/SFX/Engine", "RPM");
```

**Example: Loading a custom plugin bank**
```c
// Load a custom bank with GUID dictionary for path resolution
void* bank = s_soundAPI->SND_LoadBankFile("/path/to/my_bank.bank", "/path/to/my_bank.bank.guids");
if (bank) {
    // Discover events in the bank
    int count = s_soundAPI->SND_GetBankEventCount(bank);
    for (int i = 0; i < count; i++) {
        uint8_t guid[16];
        s_soundAPI->SND_GetBankEventGuid(bank, i, guid);
        char path[256];
        s_soundAPI->SND_GetBankEventPath(bank, i, path, sizeof(path));

        // Find in the global event cache by GUID
        int idx = s_soundAPI->SND_FindEventIndexByGuid(guid);
        if (idx >= 0) {
            void* inst = s_soundAPI->SND_CreateEventInstance(idx);
            s_soundAPI->SND_StartEvent(inst);
            // ...
            s_soundAPI->SND_StopEvent(inst, true);
            s_soundAPI->SND_ReleaseEvent(inst);
        }
    }
    s_soundAPI->SND_UnloadBank(bank);
}
```

## Function Reference

### Service Lifecycle

| Function | Return Type | Description |
|---|---|---|
| **`SND_IsReady()`** | `bool` | Checks if the sound system is initialized and ready. Always call first. |
| **`SND_AreAllOffsetsFound()`** | `bool` | Checks if all FMOD Studio memory pattern offsets (bank list, event list, studio system, etc.) were resolved. Stricter than `SND_IsReady`. |
| **`SND_RefreshOffsets()`** | `bool` | Forces a rescan of FMOD Studio memory patterns. Useful after a bank reload or if offsets become stale. |

---

### Bus Enumeration & Control

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetBusCount()`** | `int` | Returns the number of unique audio buses currently loaded. Each unique bus path is counted once. |
| **`SND_GetBusPath(index, out_buffer, buffer_size)`** | `int` | Copies the FMOD bus path (e.g. `"bus:/Engine/Master"`) into the buffer. Returns full length or `-1`. |
| **`SND_GetBusVolume(index)`** | `float` | Returns the current volume level (linear multiplier, 1.0 = unity, 0.0 = silent). Negative values allowed for phase inversion. Returns `1.0f` on invalid index. |
| **`SND_SetBusVolume(index, volume)`** | `bool` | Sets the volume level of a bus (linear). |
| **`SND_GetBusMute(index)`** | `bool` | Returns whether a bus is muted. |
| **`SND_SetBusMute(index, muted)`** | `bool` | Mutes or unmutes a bus without changing its volume setting. |
| **`SND_GetBusPause(index)`** | `bool` | Returns whether a bus is paused. |
| **`SND_SetBusPause(index, paused)`** | `bool` | Pauses or unpauses a bus, freezing all events routed through it. |

---

### VCA (Volume Control Association)

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetVCACount()`** | `int` | Returns the number of VCAs currently loaded. |
| **`SND_GetVCAPath(index, out_buffer, buffer_size)`** | `int` | Copies the FMOD VCA path (e.g. `"vca:/Engine/Master"`) into the buffer. Returns full length or `-1`. |
| **`SND_GetVCAVolume(index)`** | `float` | Returns the current VCA volume (linear multiplier, applied to all controlled buses). |
| **`SND_SetVCAVolume(index, volume)`** | `bool` | Sets the volume level of a VCA, affecting all controlled buses. |

---

### Global Parameters

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetGlobalParamCount()`** | `int` | Returns the number of global parameters in the FMOD Studio project. |
| **`SND_GetGlobalParamName(index, out_buffer, buffer_size)`** | `int` | Copies the parameter name into the buffer. Returns full length or `-1`. |
| **`SND_GetGlobalParamRange(index, out_minimum, out_maximum)`** | `bool` | Returns the min/max range of a global parameter. Output pointers may be NULL. |
| **`SND_GetGlobalParamValue(param_name)`** | `float` | Returns the current value of a global parameter by name. Returns `0.0f` if not found. |
| **`SND_SetGlobalParamValue(param_name, value)`** | `bool` | Sets the value of a global parameter by name. |

---

### Event Enumeration (Read-Only Metadata)

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetEventCount()`** | `int` | Returns the total number of events across all loaded banks. |
| **`SND_GetEventBankPath(index, out_buffer, buffer_size)`** | `int` | Copies the bank path that contains this event (e.g. `"bank:/SFX"`). |
| **`SND_GetEventPath(index, out_buffer, buffer_size)`** | `int` | Copies the FMOD event path (e.g. `"event:/SFX/Engine"`) into the buffer. |
| **`SND_GetEventGuid(index, out_guid)`** | `bool` | Retrieves the 16-byte GUID of an event. |
| **`SND_IsEvent3D(index)`** | `bool` | Returns whether the event is 3D spatialized. |
| **`SND_IsEventOneshot(index)`** | `bool` | Returns whether the event plays only once (no looping). |
| **`SND_IsEventStream(index)`** | `bool` | Returns whether the event streams from disk rather than loading fully into memory. |
| **`SND_IsEventSnapshot(index)`** | `bool` | Returns whether the event is an FMOD snapshot (mix state capture). |
| **`SND_GetEventDurationMs(index)`** | `uint32_t` | Returns the event duration in milliseconds. |
| **`SND_GetEventMinDistance(index)`** | `float` | Returns the minimum attenuation distance (below this: max volume). |
| **`SND_GetEventMaxDistance(index)`** | `float` | Returns the maximum attenuation distance (above this: inaudible). |
| **`SND_FindEventIndexByPath(event_path)`** | `int` | Searches for an event by exact path. Returns index or `-1` if not found. |
| **`SND_FindEventIndexByPrefix(prefix)`** | `int` | Searches for the first event whose path starts with `prefix` (e.g. `"event:/horn/"`). Returns index or `-1`. |
| **`SND_FindEventIndexByGuid(guid)`** | `int` | Searches for an event by its 16-byte GUID. Useful when path strings are unavailable (e.g. events from plugin-loaded banks). Returns index or `-1`. |
| **`SND_GetEventLiveInstanceCount(event_index)`** | `int` | Returns the number of live (game-created) instances for a given event. These are created by the game engine, not by your plugin. |
| **`SND_GetEventLiveInstance(event_index, instance_index)`** | `void*` | Returns an opaque pointer to a specific live instance. Use with `SND_GetEventPlaybackState()`, `SND_StopEvent()`, etc. Do **not** call `SND_ReleaseEvent()` on game-created instances. |

---

### Event Playback

| Function | Return Type | Description |
|---|---|---|
| **`SND_CreateEventInstance(event_index)`** | `void*` | Creates an independent playable instance of an event. Must be released with `SND_ReleaseEvent()`. |
| **`SND_StartEvent(instance)`** | `bool` | Starts playback of an event instance. |
| **`SND_StopEvent(instance, allow_fadeout)`** | `bool` | Stops playback. Pass `true` for graceful fadeout, `false` for immediate stop. |
| **`SND_PauseEvent(instance, paused)`** | `bool` | Pauses or unpauses an event instance. |
| **`SND_GetEventPlaybackState(instance)`** | `int` | Returns playback state: `0` = Playing, `1` = Sustaining, `2` = Stopped, `3` = Starting, `4` = Stopping, `-1` = invalid. |
| **`SND_ReleaseEvent(instance)`** | `void` | Releases an event instance and frees its resources. Does NOT stop a playing event — call `SND_StopEvent()` first if needed. The pointer becomes invalid. |

---

### Event Instance Properties

| Function | Return Type | Description |
|---|---|---|
| **`SND_SetEventVolume(instance, volume)`** | `bool` | Sets volume (linear multiplier, 1.0 = unity). Applied on top of the bus volume. |
| **`SND_GetEventVolume(instance, out_volume)`** | `bool` | Returns the current volume. Output pointer may be NULL. |
| **`SND_SetEventPitch(instance, pitch)`** | `bool` | Sets pitch (frequency multiplier, 1.0 = original, 2.0 = octave up, 0.5 = octave down). |
| **`SND_GetEventPitch(instance, out_pitch)`** | `bool` | Returns the current pitch. Output pointer may be NULL. |
| **`SND_SetEvent3DAttributes(instance, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, fwd_x, fwd_y, fwd_z, up_x, up_y, up_z)`** | `bool` | Sets 3D position, velocity, and orientation (SCS coordinate system: X=right, Y=up, Z=forward). |
| **`SND_GetEvent3DAttributes(instance, ...)`** | `bool` | Returns current 3D attributes. All output pointers may be NULL. |
| **`SND_SetEventParameter(instance, param_name, value, ignore_seek_speed)`** | `bool` | Sets a named parameter. Pass `true` for immediate change, `false` to seek at the configured rate. |
| **`SND_GetEventParameter(instance, param_name, out_value)`** | `bool` | Returns the current value of a named parameter. Output pointer may be NULL. |
| **`SND_SetEventTimelinePosition(instance, position)`** | `bool` | Sets the timeline position in milliseconds. |
| **`SND_GetEventTimelinePosition(instance)`** | `int` | Returns the current timeline position in ms, or `-1` if invalid. |
| **`SND_SetEventLoop(instance, loop)`** | `bool` | Enables or disables looping. |
| **`SND_GetEventLoopCount(instance)`** | `int` | Returns loop count: `-1` = infinite, `0` = no loop, `N` = play N+1 times. Returns `-2` if invalid. |
| **`SND_SetEventCallback(instance, callback, callback_mask)`** | `bool` | Sets a callback function invoked on the audio thread. Pass NULL to remove. Use `SPF_SND_CALLBACK_*` constants for mask. |

#### Callback Constants

| Constant | Value | Description |
|---|---|---|
| `SPF_SND_CALLBACK_START` | `0x00000001` | Fires when the event starts playing. |
| `SPF_SND_CALLBACK_STOP` | `0x00000002` | Fires when the event stops. |
| `SPF_SND_CALLBACK_RESTART` | `0x00000004` | Fires when the event enters a restart state. |
| `SPF_SND_CALLBACK_VIRTUAL_VOICE` | `0x00000008` | Fires on virtual voice status change. |
| `SPF_SND_CALLBACK_MARKER` | `0x00000010` | Fires when timeline passes a marker or beat. |
| `SPF_SND_CALLBACK_NAMED_MARKER` | `0x00000020` | Fires when timeline passes a named marker. |
| `SPF_SND_CALLBACK_SOUND_DURATION` | `0x00000040` | Fires when sound duration changes. |
| `SPF_SND_CALLBACK_ANY` | `0xFFFFFFFF` | Fires on any of the above events. |

The callback signature is `int (*)(uint32_t type, void* instance, void* parameters)` — return 0 to let FMOD process the callback normally.

---

### Listener Control

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetNumListeners()`** | `int` | Returns the number of active audio listeners (most games use 1; stereo 3D may use 2). Returns `-1` if not ready. |
| **`SND_SetNumListeners(count)`** | `bool` | Sets the number of listeners (typically 1 or 2). |
| **`SND_GetListenerAttributes(index, ...)`** | `bool` | Returns 3D attributes of a listener. All output pointers may be NULL. SCS coordinate system. |
| **`SND_SetListenerAttributes(index, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, fwd_x, fwd_y, fwd_z, up_x, up_y, up_z)`** | `bool` | Sets 3D attributes of a listener. SCS coordinate system. |

---

### Bank Management

| Function | Return Type | Description |
|---|---|---|
| **`SND_LoadBankFile(path, guids_path)`** | `void*` | Loads a bank file from disk synchronously with `FMOD_STUDIO_BANK_LOAD_SAMPLE_DATA`. Pass a `.bank.guids` dictionary path to enable path-based event lookup for plugin-loaded banks; pass NULL for auto-discovery (looks for `{path}.guids` in the same directory). The game cannot see or manage this bank. Returns opaque bank pointer, or NULL on failure. |
| **`SND_LoadBankMemory(data, size, guids_path)`** | `void*` | Loads a bank from a memory buffer (no file on disk). Same GUID dictionary semantics as `SND_LoadBankFile()`. The plugin owns buffer and bank lifetime. Returns opaque bank pointer, or NULL on failure. |
| **`SND_GetBankLoadingState(bank)`** | `int` | Returns loading state: `0` = Unloaded, `1` = Loading, `2` = Loaded, `3` = Error, `-1` = invalid. |
| **`SND_GetBankEventCount(bank)`** | `int` | Returns the number of events defined in a bank. |
| **`SND_GetBankEventGuid(bank, index, out_guid)`** | `int` | Retrieves the 16-byte GUID of an event in a bank. Returns 1 on success, 0 on failure. |
| **`SND_GetBankEventPath(bank, index, out_buffer, buffer_size)`** | `int` | Retrieves the path of an event in a bank. Attempts FMOD's `EventDescription_GetPath` first; falls back to the GUID dictionary if that fails. Returns path length or 0 on failure. |
| **`SND_GetBankCount()`** | `int` | Returns the total number of loaded banks. |
| **`SND_GetBankPath(index, out_buffer, buffer_size)`** | `int` | Copies the path of a loaded bank into the buffer. Returns full length or `-1`. |
| **`SND_UnloadBank(bank)`** | `bool` | Unloads a bank and frees its resources. All events from this bank must be stopped and released first. |

---

### FMOD Hook Overrides

Override parameters, 3D positions, volume, and pitch at the FMOD hook level — the interception layer replaces the game's own FMOD reads/writes. Overrides apply globally to all instances of the specified event and persist until explicitly removed.

| Function | Return Type | Description |
|---|---|---|
| **`SND_OverrideParameter(event_path, param_name, value)`** | `void` | Overrides a named parameter for all instances of an event. Applied on every FMOD read of the parameter. Revert with `SND_RemoveParameterOverride`. |
| **`SND_RemoveParameterOverride(event_path, param_name)`** | `void` | Removes a parameter override, restoring original values. |
| **`SND_Override3DPosition(event_path, pos_x, pos_y, pos_z)`** | `void` | Overrides the 3D position for all instances of an event at the hook level. |
| **`SND_Remove3DOverride(event_path)`** | `void` | Removes the 3D override, restoring original positioning. |
| **`SND_Reset3DToOriginal(event_path)`** | `void` | Resets 3D attributes to the exact values the game last provided (unlike `SND_Remove3DOverride`). |
| **`SND_HasOverrides()`** | `bool` | Returns whether any hook overrides are currently active. |
| **`SND_RemoveAllOverrides()`** | `void` | Removes ALL active overrides (parameters, 3D, volume, pitch). Suppression prefixes are NOT affected — remove them explicitly. |
| **`SND_OverrideEventVolume(event_path, volume)`** | `void` | Forces the FMOD volume of every instance of an event path. Intercepts `EventInstance::setVolume` and replaces the game-requested value at the FMOD boundary; the game's own volume logic keeps running. |
| **`SND_RemoveEventVolumeOverride(event_path)`** | `void` | Removes a volume override installed by `SND_OverrideEventVolume`. |
| **`SND_OverrideEventPitch(event_path, pitch)`** | `void` | Forces the FMOD pitch of every instance of an event path. Same interception model as `SND_OverrideEventVolume`, on `EventInstance::setPitch`. |
| **`SND_RemoveEventPitchOverride(event_path)`** | `void` | Removes a pitch override installed by `SND_OverrideEventPitch`. |

---

### FMOD Interception — Suppression & Activity Observation

The interception layer can block game sounds from ever reaching the speakers and can observe every FMOD call the game makes.

**Suppression** installs a prefix rule on the `EventInstance::start` detour: any start for an event path beginning with the prefix returns `FMOD_OK` without playing — the sound never reaches the speakers from the first sample. Matching is a case-sensitive prefix match and applies to instances created before or after the call.

| Function | Return Type | Description |
|---|---|---|
| **`SND_SetActivityCallback(callback, user_data)`** | `void` | Registers (or with NULL, unregisters) the FMOD activity callback. One callback per process; a new registration replaces the previous one. |
| **`SND_SuppressEventPlayback(path_prefix)`** | `bool` | Blocks playback start for every event path under a prefix (e.g. `"event:/horn/"`). Returns true when the rule was installed, false on empty prefix or unavailable hook. |
| **`SND_UnsuppressEventPlayback(path_prefix)`** | `bool` | Removes a suppression rule. Instances already blocked stay silent until their next start call, which now plays normally. |
| **`SND_GetSuppressedStartCount(path_prefix)`** | `uint64_t` | Returns how many start attempts were suppressed under a prefix. Compare against a stored baseline to detect presses; the value only grows while the rule exists. |
| **`SND_GetLastSuppressedInstance(path_prefix)`** | `void*` | Returns the FMOD instance of the most recent suppressed start. Use it to read per-instance state the game keeps on the silent instance (e.g. the `"play"` parameter) to detect release. Becomes NULL once the game releases it — call every frame, never cache. |

**Activity callback.** Invoked synchronously from inside the intercepting detour on the thread that made the FMOD call (normally the game thread) — there is no queuing. `path` and `param_name` are valid only for the duration of the call; copy what you need. Nested delivery is suppressed, but calling `SND_*` FMOD functions from the callback is still unsafe — buffer the data and act on it later (e.g. in your `OnUpdate`).

Activity codes delivered to the callback:

| Constant | Value | Description |
|---|---|---|
| `SPF_ACTIVITY_EVENT_CREATED` | `1` | `EventDescription::createInstance` returned a new instance. |
| `SPF_ACTIVITY_EVENT_START_SUPPRESSED` | `2` | `EventInstance::start` was blocked by `SND_SuppressEventPlayback`. |
| `SPF_ACTIVITY_EVENT_STARTED` | `3` | `EventInstance::start` executed successfully. |
| `SPF_ACTIVITY_EVENT_STOPPED` | `4` | `EventInstance::stop` called. |
| `SPF_ACTIVITY_EVENT_PAUSED` | `5` | `EventInstance::setPaused(true)` called. |
| `SPF_ACTIVITY_EVENT_UNPAUSED` | `6` | `EventInstance::setPaused(false)` called. |
| `SPF_ACTIVITY_EVENT_RELEASED` | `7` | `EventInstance::release` called (path resolved before release). |
| `SPF_ACTIVITY_EVENT_PARAM_SET` | `8` | `setParameterByName/ByID` called (`param_name`/`param_value` valid). |
| `SPF_ACTIVITY_BANK_LOADED` | `9` | `System::loadBank*` succeeded (path = bank path). |
| `SPF_ACTIVITY_BANK_UNLOADING` | `10` | `Bank::unload` called (path = bank path). |

The callback signature is `void (*)(void* user_data, int activity, const char* path, void* instance, const char* param_name, float param_value)`.

---

### Event Description Introspection

Query event parameter definitions, user properties, and sample state.

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetEventParameterCount(event_index)`** | `int` | Returns the number of parameters defined on an event. |
| **`SND_GetEventParameterByIndex(event_index, param_index, out_name, name_size, out_min, out_max, out_default)`** | `bool` | Returns parameter info by index: name, min, max, and default values. Output pointers may be NULL. |
| **`SND_GetEventUserPropertyCount(event_index)`** | `int` | Returns the number of user properties defined on an event. |
| **`SND_GetEventUserPropertyByIndex(event_index, prop_index, out_name, name_size, out_type)`** | `bool` | Returns user property info by index: name and type (`0`=bool, `1`=int, `2`=float, `3`=string). |
| **`SND_GetEventSoundSize(event_index)`** | `uint32_t` | Returns the compressed sound size of an event in bytes. |
| **`SND_GetEventSampleLoadingState(event_index)`** | `int` | Returns sample loading state: `0` = Not loaded, `1` = Loading, `2` = Loaded, `-1` = invalid. |

---

### Game sound_event Control

Direct handles into the game's own `sound_event_t` objects (the runtime sound events the game creates from `.soundref` paths). **GAME THREAD ONLY** — these calls reach FMOD internals and the game's vtable dispatch. Handles are raw game pointers: valid while the event exists; a bank unload destroys its events, so re-query after.

Event playback state values (`SND_GetGameEventPlaybackState`):

| Constant | Value | Description |
|---|---|---|
| `SPF_SOUND_EVENT_STATE_STOPPED` | `0` | Stopped manually / never started. |
| `SPF_SOUND_EVENT_STATE_PLAYING` | `1` | Playing. |
| `SPF_SOUND_EVENT_STATE_PAUSED` | `3` | Paused. |
| `SPF_SOUND_EVENT_STATE_ENDED` | `4` | Finished on its own. |

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetGameEventCount()`** | `int` | Returns the number of live game `sound_event_t` objects. Walks per-bank event lists and builds a fresh snapshot on every call. |
| **`SND_GetGameEventAt(index)`** | `void*` | Returns the `sound_event_t` handle at `index` within a fresh snapshot, or NULL when out of range. |
| **`SND_FindGameEventByPath(path)`** | `void*` | Finds the first game sound_event whose path equals `path`. Returns NULL if no match. |
| **`SND_FindGameEventBySource(source)`** | `void*` | Finds the first game sound_event whose source equals `source` (`"bank#event"`). Returns NULL if no match. |
| **`SND_FindGameEventByInstance(instance)`** | `void*` | Reverse FMOD→game mapping: finds the game sound_event that owns `instance`. Plugin-created / foreign instances resolve to NULL. |
| **`SND_GetGameEventPlaybackState(event)`** | `uint32_t` | Reads the playback state dword of a game sound_event (one of the `SPF_SOUND_EVENT_STATE_*` values). |
| **`SND_IsGameEventBound(event)`** | `bool` | Reports whether the game event is currently bound (state == 2). |
| **`SND_GameEvent_Activate(event)`** | `bool` | Recreates the event's EventInstance from its current source. Stops and releases the previous instance first (never leaks), then runs the game's own activate path. Does NOT auto-start — call `SND_GameEvent_Start` for that. Game thread only. |
| **`SND_GameEvent_Start(event)`** | `bool` | Starts (or resumes) a game sound_event through its vtable PlaybackControl. |
| **`SND_GameEvent_Stop(event)`** | `bool` | Stops a game sound_event through its vtable Stop; instance is kept until the next `SND_GameEvent_Activate`. |
| **`SND_GameEvent_SetPaused(event, paused)`** | `bool` | Pauses or resumes a game sound_event through its vtable SetPaused. |
| **`SND_GameEvent_SetVolume(event, volume)`** | `bool` | Sets event volume through its vtable SetVolume (also cached by the game at +0x80). Game convention 0..1. |
| **`SND_GameEvent_SetPitch(event, pitch)`** | `bool` | Sets event pitch through its vtable SetPitch (cached at +0x84). 1.0 = normal. |
| **`SND_GameEvent_SetProperty(event, property_id, value)`** | `bool` | Sets a raw FMOD event property through its vtable SetProperty (cached at +0x88). `0` = frequency modulation depth, ... |
| **`SND_GameEvent_Set3DAttributes(event, pos_x, pos_y, pos_z)`** | `bool` | Sets 3D position through its vtable Set3DAttributes. Velocity is zeroed by this overload. |
| **`SND_GameEvent_SetParameterByID(event, id, value)`** | `bool` | Sets a parameter on a game sound_event by its 16-byte FMOD ID. |

---

### SoundRef (Game Sound Binding)

The game binds each sound effect to a bank/event through a `.soundref` path (e.g. `/sound/ui/ui_click.soundref` → `/sound/ui/ui.bank#click`). The SoundRef API lets a plugin rebind that binding at runtime: the game keeps triggering the sound as usual, but it plays from the plugin's bank.

**Catalog.** `SND_GetSoundRefCount()` covers all known soundref paths merged from several sources: the game's static UI/voice-nav tables, `.soundref` path references found in the game binary, files enumerated from the game VFS, and currently live sound events. Use `SND_FindSoundRefIndex()` for lookup by path instead of scanning manually.

**Overrides — two registration modes.** There are two ways to install an override:

1. **`SND_RegisterSoundRefOverride`** — *pure* registration (map only). No side effects at call time: nothing is applied to live events, no bank is loaded, no instance is stopped or started. The override is consulted inside the game's SoundRef load pipeline, so it takes effect on the soundref's **next activation** by the game.
2. **`SND_SoundRef_Replace`** — automatic full-cycle replacement, the one-stop "swap my sound" call. In order it: (1) registers the override (same map as above); (2) rewrites the source field of every LIVE game event with this path; (3) queues a rebind executed on the game thread — stop current instance → release it → activate from the new source → start again if the event was playing/paused before. During activate the game loads the target bank itself through its VFS (the part of the source before `#`), so the bank's VFS directory must be mounted first (`Env_VfsMount`). Events not live right now pick the override up automatically on their next activation.

Overrides persist across the game's own `.soundref` reloads — when the game re-reads the file (e.g. on sound re-activation), the framework re-applies the override automatically. No periodic polling is needed.

**Example: replacing the UI click sound with a custom bank**
```c
// 1. Mount your bank's directory in the game VFS so the game can load it
//    (Env_VfsMount) — required for SND_SoundRef_Replace.

// 2. Swap the sound in one call (game layer)
s_soundAPI->SND_SoundRef_Replace("/sound/ui/ui_click.soundref", "/my_plugin/uimy.bank#click");

// 3. Later — restore the original sound
s_soundAPI->SND_UnregisterSoundRefOverride("/sound/ui/ui_click.soundref");

// 4. Unload your bank (after all overrides are removed)
s_soundAPI->SND_UnloadBank(bank);
```

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetSoundRefCount()`** | `int` | Returns the number of soundref entries in the catalog. |
| **`SND_GetSoundRefPath(index, out_buffer, buffer_size)`** | `int` | Copies the `.soundref` path of a catalog entry into the buffer. Returns full length or `-1`. |
| **`SND_GetSoundRefSource(index, out_buffer, buffer_size)`** | `int` | Copies the current `bank#event` source binding (e.g. `"/sound/ui/ui.bank#click"`). Empty if the entry has no live event. Returns full length or `-1`. |
| **`SND_FindSoundRefIndex(soundref_path)`** | `int` | Searches for a catalog entry by exact `.soundref` path. Returns index or `-1` if not found. |
| **`SND_FindSoundRefBySource(source)`** | `int` | Searches for a catalog entry by its current `bank#event` source. Useful to discover which soundref currently points at a given event. Returns index or `-1`. |
| **`SND_IsSoundRefActive(soundref_path)`** | `bool` | Returns whether a live sound event exists for this soundref path (i.e. it can be overridden right now). |
| **`SND_RegisterSoundRefOverride(soundref_path, source)`** | `bool` | PURE registration — rebinds the soundref to a new `bank#event` source in the override map. Takes effect on the soundref's next activation by the game; no immediate audible change. Returns `false` on invalid input. |
| **`SND_SoundRef_Replace(soundref_path, source)`** | `bool` | AUTOMATIC full-cycle replacement: registers + applies to every live event + queues a game-thread rebind (stop → release → activate → resume). Requires the bank's VFS directory mounted. Returns `false` on invalid input. |
| **`SND_UnregisterSoundRefOverride(soundref_path)`** | `bool` | Removes one override and restores the original source. Live events are restored and rebound on the game thread; a bank auto-loaded by `SND_SoundRef_Replace` stays loaded (silent) — unload it explicitly with `SND_UnloadBank` if needed. Returns `false` if no override exists. |
| **`SND_ClearSoundRefOverrides()`** | `void` | Removes all overrides and restores every original source. |
| **`SND_GetSoundRefOverride(soundref_path, out_buffer, buffer_size)`** | `int` | Copies the registered override source for a soundref path. Returns `0` if no override is registered, full length if one is, `-1` on error. |

---

## ABI Stability

To ensure compatibility with future framework versions without recompilation:
1. The order of existing function pointers will NEVER change.
2. Fields will NEVER be removed from this structure.
3. New functionality is only added by appending to the END of this structure.
