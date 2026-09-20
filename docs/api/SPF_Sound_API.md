# SPF Sound API

The SPF Sound API provides complete access to the game's FMOD Studio sound system. Plugins can enumerate sound banks, events, buses, and VCAs; control playback; adjust bus and global parameters; manage listeners; and override FMOD parameters at the hook level.

## Getting the API

Request the Sound API from the framework during your plugin's initialization.

**Example: C**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Sound_API.h"

SPF_Sound_API* s_soundAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_soundAPI = (SPF_Sound_API*)params->GetAPI(SPF_API_SOUND);
}
```

**Example: OnActivated callback (Recommended)**
```cpp
SPF_Sound_API* s_soundAPI = NULL;

void OnActivated(const SPF_Core_API* core_api) {
    s_soundAPI = core_api->sound;
}
```

## Key Concepts

1. **World-Scoped Lifecycle**: The sound system initializes when the game world loads and shuts down when the world unloads. Always check `SND_IsReady()` before using any other function.
2. **Opaque Handles**: Event instances and banks are represented as `void*` pointers. Do not cast or store them beyond their lifetime — release instances with `SND_ReleaseEvent()` and banks with `SND_UnloadBank()`.
3. **Index-Based Enumeration**: Buses, VCAs, global parameters, and events are accessed by zero-based index. Use `SND_GetBusCount()`, `SND_GetEventCount()`, etc. to determine the range.
4. **String Copy Pattern**: Functions that return strings take an output buffer and size, returning the full string length (excluding null terminator). If the buffer is too small, the string is truncated but the full length is still returned.

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
| **`SND_AreAllOffsetsFound()`** | `bool` | Checks if all FMOD Studio memory pattern offsets were resolved. |
| **`SND_RefreshOffsets()`** | `bool` | Forces a rescan of FMOD Studio memory patterns. |

---

### Bus Enumeration & Control

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetBusCount()`** | `int` | Returns the number of unique audio buses currently loaded. |
| **`SND_GetBusPath(index, out_buffer, buffer_size)`** | `int` | Copies the FMOD bus path (e.g. `"bus:/Engine/Master"`) into the buffer. Returns full length or `-1`. |
| **`SND_GetBusVolume(index)`** | `float` | Returns the current volume level (linear multiplier, 1.0 = unity). |
| **`SND_SetBusVolume(index, volume)`** | `bool` | Sets the volume level of a bus. |
| **`SND_GetBusMute(index)`** | `bool` | Returns whether a bus is muted. |
| **`SND_SetBusMute(index, muted)`** | `bool` | Mutes or unmutes a bus without changing its volume. |
| **`SND_GetBusPause(index)`** | `bool` | Returns whether a bus is paused. |
| **`SND_SetBusPause(index, paused)`** | `bool` | Pauses or unpauses a bus, freezing all routed events. |

---

### VCA (Volume Control Association)

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetVCACount()`** | `int` | Returns the number of VCAs currently loaded. |
| **`SND_GetVCAPath(index, out_buffer, buffer_size)`** | `int` | Copies the FMOD VCA path into the buffer. Returns full length or `-1`. |
| **`SND_GetVCAVolume(index)`** | `float` | Returns the current VCA volume (linear multiplier). |
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
| **`SND_GetEventPath(index, out_buffer, buffer_size)`** | `int` | Copies the FMOD event path (e.g. `"event:/SFX/Engine"`) into the buffer. |
| **`SND_GetEventBankPath(index, out_buffer, buffer_size)`** | `int` | Copies the bank path that contains this event (e.g. `"bank:/SFX"`). |
| **`SND_GetEventGuid(index, out_guid)`** | `bool` | Retrieves the 16-byte GUID of an event. |
| **`SND_IsEvent3D(index)`** | `bool` | Returns whether the event is 3D spatialized. |
| **`SND_IsEventOneshot(index)`** | `bool` | Returns whether the event plays only once (no looping). |
| **`SND_IsEventStream(index)`** | `bool` | Returns whether the event streams from disk. |
| **`SND_IsEventSnapshot(index)`** | `bool` | Returns whether the event is an FMOD snapshot. |
| **`SND_GetEventDurationMs(index)`** | `uint32_t` | Returns the event duration in milliseconds. |
| **`SND_GetEventMinDistance(index)`** | `float` | Returns the minimum attenuation distance (below this: max volume). |
| **`SND_GetEventMaxDistance(index)`** | `float` | Returns the maximum attenuation distance (above this: inaudible). |
| **`SND_FindEventIndexByPath(event_path)`** | `int` | Searches for an event by exact path. Returns index or `-1` if not found. |
| **`SND_FindEventIndexByPrefix(prefix)`** | `int` | Searches for the first event whose path starts with `prefix`. Useful for pattern matching (e.g. `"event:/horn/"`). Returns index or `-1`. |
| **`SND_FindEventIndexByGuid(guid)`** | `int` | Searches for an event by its 16-byte GUID. Useful when path strings are unavailable (e.g. events from plugin-loaded banks). Returns index or `-1`. |
| **`SND_GetEventLiveInstanceCount(event_index)`** | `int` | Returns the number of live (game-created) instances for a given event. These are instances created by the game engine, not by your plugin. |
| **`SND_GetEventLiveInstance(event_index, instance_index)`** | `void*` | Returns an opaque pointer to a specific live instance. Use with `SND_GetEventPlaybackState()`, `SND_StopEvent()`, etc. Do **not** call `SND_ReleaseEvent()` on game-created instances. |

---

### Event Playback

| Function | Return Type | Description |
|---|---|---|
| **`SND_CreateEventInstance(event_index)`** | `void*` | Creates a playable instance of an event. Must be released with `SND_ReleaseEvent()`. |
| **`SND_StartEvent(instance)`** | `bool` | Starts playback of an event instance. |
| **`SND_StopEvent(instance, allow_fadeout)`** | `bool` | Stops playback. Pass `true` for graceful fadeout, `false` for immediate stop. |
| **`SND_PauseEvent(instance, paused)`** | `bool` | Pauses or unpauses an event instance. |
| **`SND_GetEventPlaybackState(instance)`** | `int` | Returns playback state: `0` = Playing, `1` = Sustaining, `2` = Stopped, `3` = Starting, `4` = Stopping, `-1` = invalid. |
| **`SND_ReleaseEvent(instance)`** | `void` | Releases an event instance and frees its resources. The pointer becomes invalid. |

---

### Event Instance Properties

| Function | Return Type | Description |
|---|---|---|
| **`SND_SetEventVolume(instance, volume)`** | `bool` | Sets volume (linear multiplier, 1.0 = unity). Applied on top of bus volume. |
| **`SND_GetEventVolume(instance, out_volume)`** | `bool` | Returns the current volume. Output pointer may be NULL. |
| **`SND_SetEventPitch(instance, pitch)`** | `bool` | Sets pitch (frequency multiplier, 1.0 = original, 2.0 = octave up). |
| **`SND_GetEventPitch(instance, out_pitch)`** | `bool` | Returns the current pitch. Output pointer may be NULL. |
| **`SND_SetEvent3DAttributes(instance, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, fwd_x, fwd_y, fwd_z, up_x, up_y, up_z)`** | `bool` | Sets 3D position, velocity, and orientation (SCS coordinate system). |
| **`SND_GetEvent3DAttributes(instance, ...)`** | `bool` | Returns current 3D attributes. All output pointers may be NULL. |
| **`SND_SetEventParameter(instance, param_name, value, ignore_seek_speed)`** | `bool` | Sets a named parameter. Pass `true` for immediate change, `false` for seek speed. |
| **`SND_GetEventParameter(instance, param_name, out_value)`** | `bool` | Returns the current value of a named parameter. Output pointer may be NULL. |
| **`SND_SetEventTimelinePosition(instance, position)`** | `bool` | Sets the timeline position in milliseconds. |
| **`SND_GetEventTimelinePosition(instance)`** | `int` | Returns the current timeline position in ms, or `-1` if invalid. |
| **`SND_SetEventLoop(instance, loop)`** | `bool` | Enables or disables looping. |
| **`SND_GetEventLoopCount(instance)`** | `int` | Returns loop count: `-1` = infinite, `0` = no loop, `N` = play N+1 times. |
| **`SND_SetEventCallback(instance, callback, callback_mask)`** | `bool` | Sets a callback function. Pass NULL to remove. Use `SPF_SND_CALLBACK_*` constants for mask. |

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

---

### Listener Control

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetNumListeners()`** | `int` | Returns the number of active audio listeners. |
| **`SND_SetNumListeners(count)`** | `bool` | Sets the number of listeners (typically 1 or 2). |
| **`SND_GetListenerAttributes(index, ...)`** | `bool` | Returns 3D attributes of a listener. All output pointers may be NULL. |
| **`SND_SetListenerAttributes(index, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, fwd_x, fwd_y, fwd_z, up_x, up_y, up_z)`** | `bool` | Sets 3D attributes of a listener. |

---

### Bank Management

| Function | Return Type | Description |
|---|---|---|
| **`SND_LoadBankFile(path, guids_path)`** | `void*` | Loads a bank file from disk. Pass a `.bank.guids` dictionary path to enable path-based event lookup for plugin-loaded banks. Pass NULL for auto-discovery (looks for `{path}.guids` in same directory). Returns opaque bank pointer. |
| **`SND_GetBankLoadingState(bank)`** | `int` | Returns loading state: `0` = Unloaded, `1` = Loading, `2` = Loaded, `3` = Error, `-1` = invalid. |
| **`SND_GetBankEventCount(bank)`** | `int` | Returns the number of events defined in a bank. |
| **`SND_GetBankEventGuid(bank, index, out_guid)`** | `int` | Retrieves the 16-byte GUID of an event in a bank. Returns 1 on success, 0 on failure. |
| **`SND_GetBankEventPath(bank, index, out_buffer, buffer_size)`** | `int` | Retrieves the path of an event in a bank. Falls back to the GUID dictionary if FMOD cannot resolve the path. Returns path length or 0 on failure. |
| **`SND_GetBankCount()`** | `int` | Returns the total number of loaded banks. |
| **`SND_GetBankPath(index, out_buffer, buffer_size)`** | `int` | Copies the path of a loaded bank into the buffer. Returns full length or `-1`. |
| **`SND_UnloadBank(bank)`** | `bool` | Unloads a bank. All events from this bank must be stopped and released first. |

---

### FMOD Hook Overrides

Override parameters and 3D positions at the FMOD hook level. Overrides apply globally to all instances of the specified event and persist until explicitly removed.

| Function | Return Type | Description |
|---|---|---|
| **`SND_OverrideParameter(event_path, param_name, value)`** | `void` | Overrides a named parameter for all instances of an event. |
| **`SND_RemoveParameterOverride(event_path, param_name)`** | `void` | Removes a parameter override, restoring original values. |
| **`SND_Override3DPosition(event_path, pos_x, pos_y, pos_z)`** | `void` | Overrides the 3D position for all instances of an event. |
| **`SND_Remove3DOverride(event_path)`** | `void` | Removes the 3D override, restoring original positioning. |
| **`SND_Reset3DToOriginal(event_path)`** | `void` | Resets 3D attributes to the exact values the game last provided. |
| **`SND_HasOverrides()`** | `bool` | Returns whether any overrides are currently active. |
| **`SND_RemoveAllOverrides()`** | `void` | Removes ALL active overrides (parameters and 3D). All events revert to game values. |

---

### Event Description Introspection

Query event parameter definitions, user properties, and sample state.

| Function | Return Type | Description |
|---|---|---|
| **`SND_GetEventParameterCount(event_index)`** | `int` | Returns the number of parameters defined on an event. |
| **`SND_GetEventParameterByIndex(event_index, param_index, out_name, name_size, out_min, out_max, out_default)`** | `bool` | Returns parameter info by index: name, min, max, and default values. Output pointers may be NULL. |
| **`SND_GetEventUserPropertyCount(event_index)`** | `int` | Returns the number of user properties defined on an event. |
| **`SND_GetEventUserPropertyByIndex(event_index, prop_index, out_name, name_size, out_type)`** | `bool` | Returns user property info by index: name and type (0=bool, 1=int, 2=float, 3=string). |
| **`SND_GetEventSoundSize(event_index)`** | `uint32_t` | Returns the compressed sound size of an event in bytes. |
| **`SND_GetEventSampleLoadingState(event_index)`** | `int` | Returns sample loading state: `0` = Not loaded, `1` = Loading, `2` = Loaded, `-1` = invalid. |
