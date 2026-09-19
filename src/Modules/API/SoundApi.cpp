#include "SPF/Modules/API/SoundApi.hpp"

#include "SPF/Data/GameData/SoundService.hpp"
#include "SPF/Fmod/FmodApi.hpp"
#include "SPF/Fmod/FmodStudioHook.hpp"
#include "SPF/SPF_API/SPF_Sound_API.h"

#include <cstdint>
#include <cstring>
#include <string>

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
}  // namespace

void SoundApi::FillSoundApi(SPF_Sound_API* sound_api) {
  if (!sound_api) return;

  sound_api->SND_IsReady = &T_SND_IsReady;
  sound_api->SND_AreAllOffsetsFound = &T_SND_AreAllOffsetsFound;
  sound_api->SND_RefreshOffsets = &T_SND_RefreshOffsets;

  sound_api->SND_GetBusCount = &T_SND_GetBusCount;
  sound_api->SND_GetBusPath = &T_SND_GetBusPath;
  sound_api->SND_GetBusVolume = &T_SND_GetBusVolume;
  sound_api->SND_SetBusVolume = &T_SND_SetBusVolume;
  sound_api->SND_GetBusMute = &T_SND_GetBusMute;
  sound_api->SND_SetBusMute = &T_SND_SetBusMute;
  sound_api->SND_GetBusPause = &T_SND_GetBusPause;
  sound_api->SND_SetBusPause = &T_SND_SetBusPause;

  sound_api->SND_GetVCACount = &T_SND_GetVCACount;
  sound_api->SND_GetVCAPath = &T_SND_GetVCAPath;
  sound_api->SND_GetVCAVolume = &T_SND_GetVCAVolume;
  sound_api->SND_SetVCAVolume = &T_SND_SetVCAVolume;

  sound_api->SND_GetGlobalParamCount = &T_SND_GetGlobalParamCount;
  sound_api->SND_GetGlobalParamName = &T_SND_GetGlobalParamName;
  sound_api->SND_GetGlobalParamRange = &T_SND_GetGlobalParamRange;
  sound_api->SND_GetGlobalParamValue = &T_SND_GetGlobalParamValue;
  sound_api->SND_SetGlobalParamValue = &T_SND_SetGlobalParamValue;

  sound_api->SND_GetEventCount = &T_SND_GetEventCount;
  sound_api->SND_GetEventBankPath = &T_SND_GetEventBankPath;
  sound_api->SND_GetEventPath = &T_SND_GetEventPath;
  sound_api->SND_GetEventGuid = &T_SND_GetEventGuid;
  sound_api->SND_IsEvent3D = &T_SND_IsEvent3D;
  sound_api->SND_IsEventOneshot = &T_SND_IsEventOneshot;
  sound_api->SND_IsEventStream = &T_SND_IsEventStream;
  sound_api->SND_IsEventSnapshot = &T_SND_IsEventSnapshot;
  sound_api->SND_GetEventDurationMs = &T_SND_GetEventDurationMs;
  sound_api->SND_GetEventMinDistance = &T_SND_GetEventMinDistance;
  sound_api->SND_GetEventMaxDistance = &T_SND_GetEventMaxDistance;
  sound_api->SND_FindEventIndexByPath = &T_SND_FindEventIndexByPath;
  sound_api->SND_FindEventIndexByPrefix = &T_SND_FindEventIndexByPrefix;
  sound_api->SND_FindEventIndexByGuid = &T_SND_FindEventIndexByGuid;
  sound_api->SND_GetEventLiveInstanceCount = &T_SND_GetEventLiveInstanceCount;
  sound_api->SND_GetEventLiveInstance = &T_SND_GetEventLiveInstance;

  sound_api->SND_CreateEventInstance = &T_SND_CreateEventInstance;
  sound_api->SND_StartEvent = &T_SND_StartEvent;
  sound_api->SND_StopEvent = &T_SND_StopEvent;
  sound_api->SND_PauseEvent = &T_SND_PauseEvent;
  sound_api->SND_GetEventPlaybackState = &T_SND_GetEventPlaybackState;
  sound_api->SND_ReleaseEvent = &T_SND_ReleaseEvent;

  sound_api->SND_SetEventVolume = &T_SND_SetEventVolume;
  sound_api->SND_GetEventVolume = &T_SND_GetEventVolume;
  sound_api->SND_SetEventPitch = &T_SND_SetEventPitch;
  sound_api->SND_GetEventPitch = &T_SND_GetEventPitch;
  sound_api->SND_SetEvent3DAttributes = &T_SND_SetEvent3DAttributes;
  sound_api->SND_GetEvent3DAttributes = &T_SND_GetEvent3DAttributes;
  sound_api->SND_SetEventParameter = &T_SND_SetEventParameter;
  sound_api->SND_GetEventParameter = &T_SND_GetEventParameter;
  sound_api->SND_SetEventTimelinePosition = &T_SND_SetEventTimelinePosition;
  sound_api->SND_GetEventTimelinePosition = &T_SND_GetEventTimelinePosition;
  sound_api->SND_SetEventLoop = &T_SND_SetEventLoop;
  sound_api->SND_GetEventLoopCount = &T_SND_GetEventLoopCount;
  sound_api->SND_SetEventCallback = &T_SND_SetEventCallback;

  sound_api->SND_GetNumListeners = &T_SND_GetNumListeners;
  sound_api->SND_SetNumListeners = &T_SND_SetNumListeners;
  sound_api->SND_GetListenerAttributes = &T_SND_GetListenerAttributes;
  sound_api->SND_SetListenerAttributes = &T_SND_SetListenerAttributes;

  sound_api->SND_LoadBankFile = &T_SND_LoadBankFile;
  sound_api->SND_GetBankLoadingState = &T_SND_GetBankLoadingState;
  sound_api->SND_GetBankEventCount = &T_SND_GetBankEventCount;
  sound_api->SND_GetBankEventGuid = &T_SND_GetBankEventGuid;
  sound_api->SND_GetBankEventPath = &T_SND_GetBankEventPath;
  sound_api->SND_GetBankCount = &T_SND_GetBankCount;
  sound_api->SND_GetBankPath = &T_SND_GetBankPath;
  sound_api->SND_UnloadBank = &T_SND_UnloadBank;

  sound_api->SND_OverrideParameter = &T_SND_OverrideParameter;
  sound_api->SND_RemoveParameterOverride = &T_SND_RemoveParameterOverride;
  sound_api->SND_Override3DPosition = &T_SND_Override3DPosition;
  sound_api->SND_Remove3DOverride = &T_SND_Remove3DOverride;
  sound_api->SND_Reset3DToOriginal = &T_SND_Reset3DToOriginal;
  sound_api->SND_HasOverrides = &T_SND_HasOverrides;
  sound_api->SND_RemoveAllOverrides = &T_SND_RemoveAllOverrides;

  sound_api->SND_GetEventParameterCount = &T_SND_GetEventParameterCount;
  sound_api->SND_GetEventParameterByIndex = &T_SND_GetEventParameterByIndex;
  sound_api->SND_GetEventUserPropertyCount = &T_SND_GetEventUserPropertyCount;
  sound_api->SND_GetEventUserPropertyByIndex = &T_SND_GetEventUserPropertyByIndex;
  sound_api->SND_GetEventSoundSize = &T_SND_GetEventSoundSize;
  sound_api->SND_GetEventSampleLoadingState = &T_SND_GetEventSampleLoadingState;
}

// --- Service Lifecycle ---

bool SoundApi::T_SND_IsReady() { return SoundService::GetInstance().IsReady(); }
bool SoundApi::T_SND_AreAllOffsetsFound() { return SoundService::GetInstance().TryFindAllOffsets(); }
bool SoundApi::T_SND_RefreshOffsets() { return SoundService::GetInstance().TryFindAllOffsets(); }

// --- Bus ---

int SoundApi::T_SND_GetBusCount() {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildBusCache();
  return static_cast<int>(svc.GetBusCache().size());
}

int SoundApi::T_SND_GetBusPath(int index, char* out_buffer, int buffer_size) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return -1;
  svc.BuildBusCache();
  const auto& cache = svc.GetBusCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return -1;
  return SafeCopyToBuffer(cache[index].busPath, out_buffer, buffer_size);
}

float SoundApi::T_SND_GetBusVolume(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 1.0f;
  svc.BuildBusCache();
  const auto& cache = svc.GetBusCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return 1.0f;
  SoundBusInfo info{};
  if (svc.GetBusInfo(cache[index].busPath, info)) return info.volume;
  return 1.0f;
}

bool SoundApi::T_SND_SetBusVolume(int index, float volume) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildBusCache();
  const auto& cache = svc.GetBusCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  return svc.SetBusVolume(cache[index].busPath, volume);
}

bool SoundApi::T_SND_GetBusMute(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildBusCache();
  const auto& cache = svc.GetBusCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  SoundBusInfo info{};
  if (svc.GetBusInfo(cache[index].busPath, info)) return info.isMuted;
  return false;
}

bool SoundApi::T_SND_SetBusMute(int index, bool muted) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildBusCache();
  const auto& cache = svc.GetBusCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  return svc.SetBusMute(cache[index].busPath, muted);
}

bool SoundApi::T_SND_GetBusPause(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildBusCache();
  const auto& cache = svc.GetBusCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  bool paused = false;
  if (svc.GetBusPause(cache[index].busPath, paused)) return paused;
  return false;
}

bool SoundApi::T_SND_SetBusPause(int index, bool paused) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildBusCache();
  const auto& cache = svc.GetBusCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  return svc.SetBusPause(cache[index].busPath, paused);
}

// --- VCA ---

int SoundApi::T_SND_GetVCACount() {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildVCACache();
  return static_cast<int>(svc.GetVCACache().size());
}

int SoundApi::T_SND_GetVCAPath(int index, char* out_buffer, int buffer_size) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return -1;
  svc.BuildVCACache();
  const auto& cache = svc.GetVCACache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return -1;
  return SafeCopyToBuffer(cache[index].vcaPath, out_buffer, buffer_size);
}

float SoundApi::T_SND_GetVCAVolume(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 1.0f;
  svc.BuildVCACache();
  const auto& cache = svc.GetVCACache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return 1.0f;
  void* vca = svc.GetVCAByPath(cache[index].vcaPath.c_str());
  if (!vca) return 1.0f;
  float vol = 0.0f, finalVol = 0.0f;
  if (svc.GetVCAVolume(vca, vol, finalVol)) return vol;
  return 1.0f;
}

bool SoundApi::T_SND_SetVCAVolume(int index, float volume) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildVCACache();
  const auto& cache = svc.GetVCACache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  void* vca = svc.GetVCAByPath(cache[index].vcaPath.c_str());
  if (!vca) return false;
  return svc.SetVCAVolume(vca, volume);
}

// --- Global Parameters ---

int SoundApi::T_SND_GetGlobalParamCount() {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  return static_cast<int>(svc.GetGlobalParameters().size());
}

int SoundApi::T_SND_GetGlobalParamName(int index, char* out_buffer, int buffer_size) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return -1;
  auto params = svc.GetGlobalParameters();
  if (index < 0 || index >= static_cast<int>(params.size())) return -1;
  return SafeCopyToBuffer(params[index].paramPath, out_buffer, buffer_size);
}

bool SoundApi::T_SND_GetGlobalParamRange(int index, float* out_minimum, float* out_maximum) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  auto params = svc.GetGlobalParameters();
  if (index < 0 || index >= static_cast<int>(params.size())) return false;
  if (out_minimum) *out_minimum = params[index].minimum;
  if (out_maximum) *out_maximum = params[index].maximum;
  return true;
}

float SoundApi::T_SND_GetGlobalParamValue(const char* param_name) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady() || !param_name) return 0.0f;
  float value = 0.0f;
  if (svc.GetGlobalParamValue(param_name, value)) return value;
  return 0.0f;
}

bool SoundApi::T_SND_SetGlobalParamValue(const char* param_name, float value) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady() || !param_name) return false;
  return svc.SetGlobalParamValue(param_name, value);
}

// --- Event Enumeration ---

int SoundApi::T_SND_GetEventCount() {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildEventCache();
  return static_cast<int>(svc.GetEventCache().size());
}

int SoundApi::T_SND_GetEventBankPath(int index, char* out_buffer, int buffer_size) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return -1;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return -1;
  return SafeCopyToBuffer(cache[index].bankPath, out_buffer, buffer_size);
}

int SoundApi::T_SND_GetEventPath(int index, char* out_buffer, int buffer_size) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return -1;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return -1;
  return SafeCopyToBuffer(cache[index].eventPath, out_buffer, buffer_size);
}

bool SoundApi::T_SND_GetEventGuid(int index, uint8_t out_guid[16]) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady() || !out_guid) return false;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  std::memcpy(out_guid, cache[index].guid, 16);
  return true;
}

bool SoundApi::T_SND_IsEvent3D(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  return cache[index].is3D;
}

bool SoundApi::T_SND_IsEventOneshot(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  return cache[index].isOneshot;
}

bool SoundApi::T_SND_IsEventStream(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  return cache[index].isStream;
}

bool SoundApi::T_SND_IsEventSnapshot(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return false;
  return cache[index].isSnapshot;
}

uint32_t SoundApi::T_SND_GetEventDurationMs(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return 0;
  return cache[index].durationMs;
}

float SoundApi::T_SND_GetEventMinDistance(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0.0f;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return 0.0f;
  return cache[index].minDistance;
}

float SoundApi::T_SND_GetEventMaxDistance(int index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0.0f;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (index < 0 || index >= static_cast<int>(cache.size())) return 0.0f;
  return cache[index].maxDistance;
}

int SoundApi::T_SND_FindEventIndexByPath(const char* event_path) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady() || !event_path) return -1;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  for (int i = 0; i < static_cast<int>(cache.size()); ++i) {
    if (cache[i].eventPath == event_path) return i;
  }
  return -1;
}

int SoundApi::T_SND_FindEventIndexByPrefix(const char* prefix) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady() || !prefix) return -1;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  for (int i = 0; i < static_cast<int>(cache.size()); ++i) {
    if (cache[i].eventPath.compare(0, strlen(prefix), prefix) == 0) return i;
  }
  return -1;
}

int SoundApi::T_SND_FindEventIndexByGuid(const uint8_t guid[16]) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady() || !guid) return -1;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  for (int i = 0; i < static_cast<int>(cache.size()); ++i) {
    if (std::memcmp(cache[i].guid, guid, 16) == 0) return i;
  }
  return -1;
}

int SoundApi::T_SND_GetEventLiveInstanceCount(int event_index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return 0;
  return svc.GetEventInstanceCount(cache[event_index].eventDesc);
}

void* SoundApi::T_SND_GetEventLiveInstance(int event_index, int instance_index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return nullptr;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return nullptr;
  return svc.GetEventInstance(cache[event_index].eventDesc, instance_index);
}

// --- Event Playback ---

void* SoundApi::T_SND_CreateEventInstance(int event_index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return nullptr;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return nullptr;
  return svc.CreateEventInstance(cache[event_index].guid);
}

bool SoundApi::T_SND_StartEvent(void* instance) { return SoundService::GetInstance().StartEvent(instance); }
bool SoundApi::T_SND_StopEvent(void* instance, bool allow_fadeout) { return SoundService::GetInstance().StopEvent(instance, allow_fadeout); }
bool SoundApi::T_SND_PauseEvent(void* instance, bool paused) { return SoundService::GetInstance().PauseEvent(instance, paused); }
int SoundApi::T_SND_GetEventPlaybackState(void* instance) { return SoundService::GetInstance().GetEventPlaybackState(instance); }
void SoundApi::T_SND_ReleaseEvent(void* instance) { SoundService::GetInstance().ReleaseEventInstance(instance); }

// --- Event Instance Properties ---

bool SoundApi::T_SND_SetEventVolume(void* instance, float volume) { return SoundService::GetInstance().SetEventVolume(instance, volume); }

bool SoundApi::T_SND_GetEventVolume(void* instance, float* out_volume) {
  if (!out_volume) return false;
  float finalVol = 0.0f;
  return SoundService::GetInstance().GetEventVolume(instance, *out_volume, finalVol);
}

bool SoundApi::T_SND_SetEventPitch(void* instance, float pitch) { return SoundService::GetInstance().SetEventPitch(instance, pitch); }

bool SoundApi::T_SND_GetEventPitch(void* instance, float* out_pitch) {
  if (!out_pitch) return false;
  float finalPitch = 0.0f;
  return SoundService::GetInstance().GetEventPitch(instance, *out_pitch, finalPitch);
}

bool SoundApi::T_SND_SetEvent3DAttributes(void* instance, float pos_x, float pos_y, float pos_z, float vel_x, float vel_y, float vel_z, float fwd_x, float fwd_y, float fwd_z, float up_x, float up_y, float up_z) {
  return SoundService::GetInstance().SetEvent3DAttributes(instance, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, fwd_x, fwd_y, fwd_z, up_x, up_y, up_z);
}

bool SoundApi::T_SND_GetEvent3DAttributes(void* instance, float* out_pos_x, float* out_pos_y, float* out_pos_z, float* out_vel_x, float* out_vel_y, float* out_vel_z, float* out_fwd_x, float* out_fwd_y, float* out_fwd_z, float* out_up_x,
                                          float* out_up_y, float* out_up_z) {
  if (!out_pos_x || !out_pos_y || !out_pos_z || !out_vel_x || !out_vel_y || !out_vel_z || !out_fwd_x || !out_fwd_y || !out_fwd_z || !out_up_x || !out_up_y || !out_up_z) return false;
  return SoundService::GetInstance().GetEvent3DAttributes(instance, *out_pos_x, *out_pos_y, *out_pos_z, *out_vel_x, *out_vel_y, *out_vel_z, *out_fwd_x, *out_fwd_y, *out_fwd_z, *out_up_x, *out_up_y, *out_up_z);
}

bool SoundApi::T_SND_SetEventParameter(void* instance, const char* param_name, float value, bool ignore_seek_speed) { return SoundService::GetInstance().SetEventParameterByName(instance, param_name, value, ignore_seek_speed); }

bool SoundApi::T_SND_GetEventParameter(void* instance, const char* param_name, float* out_value) {
  if (!out_value) return false;
  float finalVal = 0.0f;
  return SoundService::GetInstance().GetEventParameterByName(instance, param_name, *out_value, finalVal);
}

bool SoundApi::T_SND_SetEventTimelinePosition(void* instance, int position) { return SoundService::GetInstance().SetEventTimelinePosition(instance, position); }

int SoundApi::T_SND_GetEventTimelinePosition(void* instance) {
  int pos = 0;
  SoundService::GetInstance().GetEventTimelinePosition(instance, pos);
  return pos;
}

bool SoundApi::T_SND_SetEventLoop(void* instance, bool loop) { return SoundService::GetInstance().SetEventLoop(instance, loop); }

int SoundApi::T_SND_GetEventLoopCount(void* instance) {
  int count = 0;
  SoundService::GetInstance().GetEventLoopCount(instance, count);
  return count;
}

bool SoundApi::T_SND_SetEventCallback(void* instance, SPF_SND_EventCallbackFn callback, uint32_t callback_mask) {
  auto internalCb = reinterpret_cast<EventCallbackFn>(callback);
  return SoundService::GetInstance().SetEventCallback(instance, internalCb, callback_mask);
}

// --- Listener ---

int SoundApi::T_SND_GetNumListeners() { return SoundService::GetInstance().GetNumListeners(); }

bool SoundApi::T_SND_SetNumListeners(int count) { return SoundService::GetInstance().SetNumListeners(count); }

bool SoundApi::T_SND_GetListenerAttributes(int index, float* out_pos_x, float* out_pos_y, float* out_pos_z, float* out_vel_x, float* out_vel_y, float* out_vel_z, float* out_fwd_x, float* out_fwd_y, float* out_fwd_z, float* out_up_x, float* out_up_y,
                                           float* out_up_z) {
  if (!out_pos_x || !out_pos_y || !out_pos_z || !out_vel_x || !out_vel_y || !out_vel_z || !out_fwd_x || !out_fwd_y || !out_fwd_z || !out_up_x || !out_up_y || !out_up_z) return false;
  return SoundService::GetInstance().GetListenerAttributes(index, *out_pos_x, *out_pos_y, *out_pos_z, *out_vel_x, *out_vel_y, *out_vel_z, *out_fwd_x, *out_fwd_y, *out_fwd_z, *out_up_x, *out_up_y, *out_up_z);
}

bool SoundApi::T_SND_SetListenerAttributes(int index, float pos_x, float pos_y, float pos_z, float vel_x, float vel_y, float vel_z, float fwd_x, float fwd_y, float fwd_z, float up_x, float up_y, float up_z) {
  return SoundService::GetInstance().SetListenerAttributes(index, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, fwd_x, fwd_y, fwd_z, up_x, up_y, up_z);
}

// --- Banks ---

void* SoundApi::T_SND_LoadBankFile(const char* bank_path, const char* guids_path) { return SoundService::GetInstance().LoadBankFile(bank_path, guids_path); }

int SoundApi::T_SND_GetBankLoadingState(void* bank) { return SoundService::GetInstance().GetBankLoadingState(bank); }

int SoundApi::T_SND_GetBankEventCount(void* bank) { return SoundService::GetInstance().GetBankEventCount(bank); }

int SoundApi::T_SND_GetBankEventGuid(void* bank, int index, uint8_t out_guid[16]) { return SoundService::GetInstance().GetBankEventGuid(bank, index, out_guid); }

int SoundApi::T_SND_GetBankEventPath(void* bank, int index, char* out_buffer, int buffer_size) { return SoundService::GetInstance().GetBankEventPath(bank, index, out_buffer, buffer_size); }

int SoundApi::T_SND_GetBankCount() {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  return static_cast<int>(svc.GetLoadedBanksInfo().size());
}

int SoundApi::T_SND_GetBankPath(int index, char* out_buffer, int buffer_size) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return -1;
  auto banks = svc.GetLoadedBanksInfo();
  if (index < 0 || index >= static_cast<int>(banks.size())) return -1;
  return SafeCopyToBuffer(banks[index].bankPath, out_buffer, buffer_size);
}

bool SoundApi::T_SND_UnloadBank(void* bank) { return SoundService::GetInstance().UnloadBank(bank); }

// --- FMOD Hook Overrides ---

void SoundApi::T_SND_OverrideParameter(const char* event_path, const char* param_name, float value) {
  if (!event_path || !param_name) return;
  Fmod::FmodStudioHook::GetInstance().OverrideParameter(event_path, param_name, value);
}

void SoundApi::T_SND_RemoveParameterOverride(const char* event_path, const char* param_name) {
  if (!event_path || !param_name) return;
  Fmod::FmodStudioHook::GetInstance().RemoveParameterOverride(event_path, param_name);
}

void SoundApi::T_SND_Override3DPosition(const char* event_path, float pos_x, float pos_y, float pos_z) {
  if (!event_path) return;
  FMOD_VECTOR pos{pos_x, pos_y, pos_z};
  Fmod::FmodStudioHook::GetInstance().Override3DPosition(event_path, pos);
}

void SoundApi::T_SND_Remove3DOverride(const char* event_path) {
  if (!event_path) return;
  Fmod::FmodStudioHook::GetInstance().Remove3DOverride(event_path);
}

void SoundApi::T_SND_Reset3DToOriginal(const char* event_path) {
  if (!event_path) return;
  Fmod::FmodStudioHook::GetInstance().Reset3DToOriginal(event_path);
}

bool SoundApi::T_SND_HasOverrides() { return Fmod::FmodStudioHook::GetInstance().HasOverrides(); }

void SoundApi::T_SND_RemoveAllOverrides() { Fmod::FmodStudioHook::GetInstance().RemoveAllOverrides(); }

// --- Event Description Introspection ---

int SoundApi::T_SND_GetEventParameterCount(int event_index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return 0;
  return svc.GetEventParameterDescriptionCount(cache[event_index].eventDesc);
}

bool SoundApi::T_SND_GetEventParameterByIndex(int event_index, int param_index, char* out_name, int name_size, float* out_min, float* out_max, float* out_default) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return false;
  float min = 0.0f, max = 0.0f, def = 0.0f;
  bool ok = svc.GetEventParameterByIndex(cache[event_index].eventDesc, param_index, out_name, name_size, min, max, def);
  if (out_min) *out_min = min;
  if (out_max) *out_max = max;
  if (out_default) *out_default = def;
  return ok;
}

int SoundApi::T_SND_GetEventUserPropertyCount(int event_index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return 0;
  int count = 0;
  svc.GetEventUserPropertyCount(cache[event_index].eventDesc, count);
  return count;
}

bool SoundApi::T_SND_GetEventUserPropertyByIndex(int event_index, int prop_index, char* out_name, int name_size, int* out_type) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return false;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return false;
  int type = 0;
  bool ok = svc.GetEventUserPropertyByIndex(cache[event_index].eventDesc, prop_index, out_name, name_size, type);
  if (out_type) *out_type = type;
  return ok;
}

uint32_t SoundApi::T_SND_GetEventSoundSize(int event_index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return 0;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return 0;
  uint32_t size = 0;
  svc.GetEventSoundSize(cache[event_index].eventDesc, size);
  return size;
}

int SoundApi::T_SND_GetEventSampleLoadingState(int event_index) {
  auto& svc = SoundService::GetInstance();
  if (!svc.IsReady()) return -1;
  svc.BuildEventCache();
  const auto& cache = svc.GetEventCache();
  if (event_index < 0 || event_index >= static_cast<int>(cache.size())) return -1;
  int state = -1;
  svc.GetEventSampleLoadingState(cache[event_index].eventDesc, state);
  return state;
}

}  // namespace SPF::Modules::API
