#pragma once

#include "SPF/SPF_API/SPF_Sound_API.h"

namespace SPF::Modules::API {
class SoundApi {
 public:
  static void FillSoundApi(SPF_Sound_API* sound_api);

 private:
  static bool T_SND_IsReady();
  static bool T_SND_AreAllOffsetsFound();
  static bool T_SND_RefreshOffsets();

  static int T_SND_GetBusCount();
  static int T_SND_GetBusPath(int index, char* out_buffer, int buffer_size);
  static float T_SND_GetBusVolume(int index);
  static bool T_SND_SetBusVolume(int index, float volume);
  static bool T_SND_GetBusMute(int index);
  static bool T_SND_SetBusMute(int index, bool muted);
  static bool T_SND_GetBusPause(int index);
  static bool T_SND_SetBusPause(int index, bool paused);

  static int T_SND_GetVCACount();
  static int T_SND_GetVCAPath(int index, char* out_buffer, int buffer_size);
  static float T_SND_GetVCAVolume(int index);
  static bool T_SND_SetVCAVolume(int index, float volume);

  static int T_SND_GetGlobalParamCount();
  static int T_SND_GetGlobalParamName(int index, char* out_buffer, int buffer_size);
  static bool T_SND_GetGlobalParamRange(int index, float* out_minimum, float* out_maximum);
  static float T_SND_GetGlobalParamValue(const char* param_name);
  static bool T_SND_SetGlobalParamValue(const char* param_name, float value);

  static int T_SND_GetEventCount();
  static int T_SND_GetEventBankPath(int index, char* out_buffer, int buffer_size);
  static int T_SND_GetEventPath(int index, char* out_buffer, int buffer_size);
  static bool T_SND_GetEventGuid(int index, uint8_t out_guid[16]);
  static bool T_SND_IsEvent3D(int index);
  static bool T_SND_IsEventOneshot(int index);
  static bool T_SND_IsEventStream(int index);
  static bool T_SND_IsEventSnapshot(int index);
  static uint32_t T_SND_GetEventDurationMs(int index);
  static float T_SND_GetEventMinDistance(int index);
  static float T_SND_GetEventMaxDistance(int index);
  static int T_SND_FindEventIndexByPath(const char* event_path);
  static int T_SND_FindEventIndexByPrefix(const char* prefix);
  static int T_SND_FindEventIndexByGuid(const uint8_t guid[16]);
  static int T_SND_GetEventLiveInstanceCount(int event_index);
  static void* T_SND_GetEventLiveInstance(int event_index, int instance_index);

  static void* T_SND_CreateEventInstance(int event_index);
  static bool T_SND_StartEvent(void* instance);
  static bool T_SND_StopEvent(void* instance, bool allow_fadeout);
  static bool T_SND_PauseEvent(void* instance, bool paused);
  static int T_SND_GetEventPlaybackState(void* instance);
  static void T_SND_ReleaseEvent(void* instance);

  static bool T_SND_SetEventVolume(void* instance, float volume);
  static bool T_SND_GetEventVolume(void* instance, float* out_volume);
  static bool T_SND_SetEventPitch(void* instance, float pitch);
  static bool T_SND_GetEventPitch(void* instance, float* out_pitch);
  static bool T_SND_SetEvent3DAttributes(void* instance, float pos_x, float pos_y, float pos_z, float vel_x, float vel_y, float vel_z, float fwd_x, float fwd_y, float fwd_z, float up_x, float up_y, float up_z);
  static bool T_SND_GetEvent3DAttributes(void* instance, float* out_pos_x, float* out_pos_y, float* out_pos_z, float* out_vel_x, float* out_vel_y, float* out_vel_z, float* out_fwd_x, float* out_fwd_y, float* out_fwd_z, float* out_up_x, float* out_up_y, float* out_up_z);
  static bool T_SND_SetEventParameter(void* instance, const char* param_name, float value, bool ignore_seek_speed);
  static bool T_SND_GetEventParameter(void* instance, const char* param_name, float* out_value);
  static bool T_SND_SetEventTimelinePosition(void* instance, int position);
  static int T_SND_GetEventTimelinePosition(void* instance);
  static bool T_SND_SetEventLoop(void* instance, bool loop);
  static int T_SND_GetEventLoopCount(void* instance);
  static bool T_SND_SetEventCallback(void* instance, SPF_SND_EventCallbackFn callback, uint32_t callback_mask);

  static int T_SND_GetNumListeners();
  static bool T_SND_SetNumListeners(int count);
  static bool T_SND_GetListenerAttributes(int index, float* out_pos_x, float* out_pos_y, float* out_pos_z, float* out_vel_x, float* out_vel_y, float* out_vel_z, float* out_fwd_x, float* out_fwd_y, float* out_fwd_z, float* out_up_x, float* out_up_y, float* out_up_z);
  static bool T_SND_SetListenerAttributes(int index, float pos_x, float pos_y, float pos_z, float vel_x, float vel_y, float vel_z, float fwd_x, float fwd_y, float fwd_z, float up_x, float up_y, float up_z);

  static void* T_SND_LoadBankFile(const char* bank_path, const char* guids_path);
  static int T_SND_GetBankLoadingState(void* bank);
  static int T_SND_GetBankEventCount(void* bank);
  static int T_SND_GetBankEventGuid(void* bank, int index, uint8_t out_guid[16]);
  static int T_SND_GetBankEventPath(void* bank, int index, char* out_buffer, int buffer_size);
  static int T_SND_GetBankCount();
  static int T_SND_GetBankPath(int index, char* out_buffer, int buffer_size);
  static bool T_SND_UnloadBank(void* bank);

  static void T_SND_OverrideParameter(const char* event_path, const char* param_name, float value);
  static void T_SND_RemoveParameterOverride(const char* event_path, const char* param_name);
  static void T_SND_Override3DPosition(const char* event_path, float pos_x, float pos_y, float pos_z);
  static void T_SND_Remove3DOverride(const char* event_path);
  static void T_SND_Reset3DToOriginal(const char* event_path);
  static bool T_SND_HasOverrides();
  static void T_SND_RemoveAllOverrides();

  static int T_SND_GetEventParameterCount(int event_index);
  static bool T_SND_GetEventParameterByIndex(int event_index, int param_index, char* out_name, int name_size, float* out_min, float* out_max, float* out_default);
  static int T_SND_GetEventUserPropertyCount(int event_index);
  static bool T_SND_GetEventUserPropertyByIndex(int event_index, int prop_index, char* out_name, int name_size, int* out_type);
  static uint32_t T_SND_GetEventSoundSize(int event_index);
  static int T_SND_GetEventSampleLoadingState(int event_index);
};
}  // namespace SPF::Modules::API
