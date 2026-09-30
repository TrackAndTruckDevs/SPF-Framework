#pragma once

#include "SPF/Data/GameData/Finders/ISoundDataFinder.hpp"
#include "SPF/Data/GameData/IWorldScopedService.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace SPF::Data::GameData {

struct SoundParameterDescription {
  std::string name;
  uint32_t idData1 = 0;
  uint32_t idData2 = 0;
  float minimum = 0.0f;
  float maximum = 0.0f;
  float defaultvalue = 0.0f;
  int32_t type = 0;
};

struct SoundUserProperty {
  std::string name;
  int type = 0;
  bool boolValue = false;
  int intValue = 0;
  float floatValue = 0.0f;
  std::string stringValue;
};

struct SoundEvent {
  std::string bankPath;
  std::string eventPath;
  uint8_t guid[16];
  void* eventDesc = nullptr;
  uint32_t durationMs = 0;
  bool hasDuration = false;
  bool is3D = false;
  bool hasIs3D = false;
  bool isOneshot = false;
  bool hasIsOneshot = false;
  bool isStream = false;
  bool hasIsStream = false;
  bool hasSustainPoint = false;
  bool hasHasSustainPoint = false;
  bool isSnapshot = false;
  bool hasIsSnapshot = false;
  bool isDopplerEnabled = false;
  bool hasIsDopplerEnabled = false;
  float minDistance = 0.0f;
  float maxDistance = 0.0f;
  uint32_t soundSize = 0;
  int sampleLoadingState = 0;
  int instanceCount = 0;
  std::vector<SoundParameterDescription> parameters;
  std::vector<SoundUserProperty> userProperties;
};

struct SoundBankGroup {
  std::string bankPath;
  std::vector<SoundEvent> events;
};

struct SoundBusEntry {
  std::string busPath;
  std::string parentPath;
  float volume = 1.0f;
  float faderLevel = 1.0f;
  bool isMuted = false;
  bool isPaused = false;
  bool isBypassed = false;
};

struct SoundBusInfo {
  std::string busPath;
  float volume = 1.0f;
  float faderLevel = 1.0f;
  bool isMuted = false;
  bool isPaused = false;
  bool isBypassed = false;
};

struct SoundGlobalParameter {
  std::string paramPath;
  float minimum = 0.0f;
  float maximum = 0.0f;
  float defaultvalue = 0.0f;
  int type = 0;
  bool isEditable = false;
};

struct SoundGlobalParamValue {
  std::string name;
  float value = 0.0f;
};

struct SoundVCAEntry {
  std::string vcaPath;
};

struct SoundVCAInfo {
  std::string vcaPath;
  float volume = 1.0f;
};

struct SoundBankLoadInfo {
  std::string bankPath;
  int loadingState = 0;
  int sampleLoadingState = 0;
  int eventCount = 0;
  int busCount = 0;
  int vcaCount = 0;
};

using EventCallbackFn = int (*)(int type, void* instance, void* parameters);

enum SoundRefOrigin : uint8_t {
  SOUNDREF_ORIGIN_NONE = 0,
  SOUNDREF_ORIGIN_UI = 1 << 0,
  SOUNDREF_ORIGIN_VN = 1 << 1,
  SOUNDREF_ORIGIN_RDATA = 1 << 2,
  SOUNDREF_ORIGIN_VFS = 1 << 3,
  SOUNDREF_ORIGIN_LIVE = 1 << 4,
};

struct SoundRefEntry {
  std::string path;
  uint32_t category = 0;
  bool enabled = false;
  uint8_t index = 0;
  std::string source;
  bool hasEvent = false;
  uint8_t origin = SOUNDREF_ORIGIN_NONE;  // SoundRefOrigin bitflags
};

class SoundService : public IWorldScopedService {
 public:
  static SoundService& GetInstance();

  SoundService(const SoundService&) = delete;
  void operator=(const SoundService&) = delete;

 private:
  SoundService();
  ~SoundService() = default;

 public:
  void Initialize();
  void Shutdown();
  bool IsReady();
  bool TryFindAllOffsets();

  std::vector<SoundBankGroup> GetSoundBankGroups();
  std::vector<SoundBankGroup> GetPluginBankGroups();
  void EnrichEventsWithFmodData(std::vector<SoundBankGroup>& groups);
  void EnrichEventParameters(std::vector<SoundBankGroup>& groups);
  std::vector<SoundBusEntry> GetBuses();
  std::vector<SoundGlobalParameter> GetGlobalParameters();

  bool GetBusInfo(const std::string& busPath, SoundBusInfo& outInfo);
  bool SetBusVolume(const std::string& busPath, float volume);
  bool SetBusMute(const std::string& busPath, bool muted);
  bool SetBusPause(const std::string& busPath, bool paused);
  bool GetBusPause(const std::string& busPath, bool& outPaused);

  bool GetGlobalParamValue(const std::string& paramName, float& outValue);
  bool SetGlobalParamValue(const std::string& paramName, float value);

  void* CreateEventInstance(const uint8_t guid[16]);
  bool StartEvent(void* instance);
  bool StopEvent(void* instance, bool allowFadeout);
  bool PauseEvent(void* instance, bool paused);
  int GetEventPlaybackState(void* instance);
  void ReleaseEventInstance(void* instance);

  bool SetEventVolume(void* instance, float volume);
  bool GetEventVolume(void* instance, float& outVolume, float& outFinalVolume);
  bool SetEventPitch(void* instance, float pitch);
  bool GetEventPitch(void* instance, float& outPitch, float& outFinalPitch);
  bool SetEvent3DAttributes(void* instance, float posX, float posY, float posZ, float velX, float velY, float velZ, float fwdX, float fwdY, float fwdZ, float upX, float upY, float upZ);
  bool GetEvent3DAttributes(void* instance, float& posX, float& posY, float& posZ, float& velX, float& velY, float& velZ, float& fwdX, float& fwdY, float& fwdZ, float& upX, float& upY, float& upZ);
  bool SetEventParameterByName(void* instance, const char* name, float value, bool ignoreSeekSpeed);
  bool GetEventParameterByName(void* instance, const char* name, float& outValue, float& outFinalValue);
  bool SetEventParameterByID(void* instance, uint32_t idData1, uint32_t idData2, float value, bool ignoreSeekSpeed);
  bool GetEventParameterByID(void* instance, uint32_t idData1, uint32_t idData2, float& outValue, float& outFinalValue);
  bool SetEventTimelinePosition(void* instance, int position);
  bool GetEventTimelinePosition(void* instance, int& outPosition);
  bool GetEventDescriptionFromInstance(void* instance, void** outDesc);
  bool SetEventCallback(void* instance, EventCallbackFn callback, uint32_t callbackMask);
  bool SetEventLoopCount(void* instance, int loopCount);
  bool GetEventLoopCount(void* instance, int& outCount);
  bool SetEventLoop(void* instance, bool loop);

  int GetNumListeners();
  bool SetNumListeners(int numListeners);
  bool GetListenerAttributes(int index, float& posX, float& posY, float& posZ, float& velX, float& velY, float& velZ, float& fwdX, float& fwdY, float& fwdZ, float& upX, float& upY, float& upZ);
  bool SetListenerAttributes(int index, float posX, float posY, float posZ, float velX, float velY, float velZ, float fwdX, float fwdY, float fwdZ, float upX, float upY, float upZ);

  void* LoadBankFile(const char* bankPath, const char* guidsPath);
  void* LoadBankMemory(const void* data, uint32_t size, const char* guidsPath);
  bool UnloadBank(void* bank);
  int GetBankLoadingState(void* bank);
  int GetBankSampleLoadingState(void* bank);
  bool LoadBankSampleData(void* bank);
  bool UnloadBankSampleData(void* bank);
  int GetBankEventCount(void* bank);
  int GetBankEventList(void* bank, void** outEvents, int maxCount);
  std::vector<SoundBankLoadInfo> GetLoadedBanksInfo();

  // Flat event cache for plugin API
  struct EventCacheEntry {
    std::string bankPath;
    std::string eventPath;
    uint8_t guid[16]{};
    void* eventDesc = nullptr;
    bool is3D = false;
    bool isOneshot = false;
    bool isStream = false;
    bool isSnapshot = false;
    uint32_t durationMs = 0;
    float minDistance = 0.0f;
    float maxDistance = 0.0f;
  };
  bool BuildEventCache();
  const std::vector<EventCacheEntry>& GetEventCache() const { return m_eventCache; }
  void InvalidateEventCache() { m_eventCache.clear(); }

  struct BusCacheEntry {
    std::string busPath;
    void* busPtr = nullptr;
  };
  bool BuildBusCache();
  const std::vector<BusCacheEntry>& GetBusCache() const { return m_busCache; }
  void InvalidateBusCache() { m_busCache.clear(); }

  struct VCACacheEntry {
    std::string vcaPath;
  };
  bool BuildVCACache();
  const std::vector<VCACacheEntry>& GetVCACache() const { return m_vcaCache; }
  void InvalidateVCACache() { m_vcaCache.clear(); }

  void* GetVCAByPath(const char* path);
  bool SetVCAVolume(void* vca, float volume);
  bool GetVCAVolume(void* vca, float& outVolume, float& outFinalVolume);
  int GetVCAPath(void* vca, char* outBuffer, int bufferSize);
  std::vector<SoundVCAEntry> GetVCAs();
  bool IsEvent3D(void* desc);
  bool IsEventSnapshot(void* desc);
  bool IsEventDopplerEnabled(void* desc);
  bool IsEventOneshot(void* desc);
  bool IsEventStream(void* desc);
  bool EventHasSustainPoint(void* desc);
  bool GetEventLength(void* desc, uint32_t& outLength);
  bool GetEventMinMaxDistance(void* desc, float& outMin, float& outMax);
  bool GetEventSoundSize(void* desc, uint32_t& outSize);
  bool GetEventSampleLoadingState(void* desc, int& outState);
  bool GetEventID(void* desc, uint8_t outGuid[16]);
  int GetEventPathFromDesc(void* desc, char* outBuffer, int bufferSize);
  int GetEventInstanceCount(void* desc);
  std::vector<void*> GetEventInstanceList(void* desc);
  void* GetEventInstance(void* desc, int index);
  int GetEventParameterDescriptionCount(void* desc);
  bool GetEventParameterByIndex(void* desc, int index, char* outName, int nameSize, float& outMin, float& outMax, float& outDefault);
  bool GetEventUserPropertyCount(void* desc, int& outCount);
  bool GetEventUserPropertyByIndex(void* desc, int index, char* outName, int nameSize, int& outType);
  void DumpAllEventsToLog();
  bool FindEventGuidByPath(const char* eventPath, uint8_t outGuid[16]);

  void LoadGuidsFile(const char* guidsPath);
  int GetBankEventGuid(void* bank, int index, uint8_t outGuid[16]);
  int GetBankEventPath(void* bank, int index, char* outBuffer, int bufferSize);

  const char* GetName() const override { return "SoundService"; }
  void ResetForWorldReload() override { Shutdown(); }
  bool TryFinalizeWorldInit() override { return TryFindAllOffsets(); }
  std::vector<void*> m_eventInstanceList;

  void ForEachSoundEvent(const std::function<void(void*)>& fn);
  std::string GetSoundEventPath(void* event);
  std::string GetSoundEventSource(void* event);

 private:
  std::vector<SoundRefEntry> GetUiSoundRefEntries();
  std::vector<SoundRefEntry> GetVoiceNavEntries();
  bool SetSoundEventPath(void* event, const std::string& newPath);
  bool SetSoundEventSource(void* event, const std::string& newSource);
  void ApplySoundRefOverrides();
  bool EnsureSoundRefSnapshot();
  std::vector<SoundRefEntry> BuildSoundRefSnapshot();

 public:
  bool RegisterSoundRefOverride(const std::string& soundrefPath, const std::string& source);
  bool UnregisterSoundRefOverride(const std::string& soundrefPath);
  std::unordered_map<std::string, std::string> GetSoundRefOverrides() const;
  void ClearSoundRefOverrides();
  void ProcessPendingSoundRefRebinds();
  void QueueSoundRefRebinds(const std::vector<void*>& events);
  std::vector<void*> FindSoundEventsByPath(const std::string& soundrefPath);
  std::vector<void*> ApplyOverrideToEvents(const std::string& soundrefPath, const std::string& source);

  std::vector<SoundRefEntry> GetSoundRefEntries();
  // Snapshot cache: full catalog rebuilt lazily only after invalidation;
  // all reads below are O(1) (or O(n) scan without rebuild for BySource).
  int GetSoundRefCount();
  bool GetSoundRefEntryByIndex(int index, SoundRefEntry& out);
  int FindSoundRefIndex(const char* soundrefPath);
  int FindSoundRefIndexBySource(const char* source);
  void InvalidateSoundRefSnapshot() { m_soundRefSnapshotDirty.store(true, std::memory_order_relaxed); }
  void BuildSoundRefRdataCatalog();
  void BuildSoundRefVfsCatalog();
  void InstallSoundRefLoadConfigHook();
  void RemoveSoundRefLoadConfigHook();
  static void HookedSoundRefLoadConfig(void* event);
  void InstallSystemUpdateHook();
  void RemoveSystemUpdateHook();
  static void HookedSystemUpdate(void* system);

  uint32_t GetBankListLockOffset() const { return m_bankListLockOffset; }
  uint32_t GetBankListHeadOffset() const { return m_bankListHeadOffset; }
  uint32_t GetBankListSentinelOffset() const { return m_bankListSentinelOffset; }
  uint32_t GetBankEventListHeadOffset() const { return m_bankEventListHeadOffset; }
  uint32_t GetBankPathStringOffset() const { return m_bankPathStringOffset; }
  uint32_t GetEventListTerminatorOffset() const { return m_eventListTerminatorOffset; }
  uint32_t GetEventPathOffset() const { return m_eventPathOffset; }
  uint32_t GetEventGuidOffset() const { return m_eventGuidOffset; }
  uint32_t GetSoundEventListHeadOffset() const { return m_soundEventListHeadOffset; }
  uint32_t GetSoundEventStateOffset() const { return m_soundEventStateOffset; }
  uint32_t GetSoundEventBoundLockOffset() const { return m_soundEventBoundLockOffset; }
  uint32_t GetSoundEventNodeOffset() const { return m_soundEventNodeOffset; }
  uint32_t GetSoundEventPathOffset() const { return m_soundEventPathOffset; }
  uint32_t GetSoundEventSourceOffset() const { return m_soundEventSourceOffset; }
  uintptr_t GetSoundEventCreateLockAddr() const { return m_soundEventCreateLockAddr; }
  uintptr_t GetSoundEventActivateFn() const { return m_soundEventActivateFn; }
  uintptr_t GetSoundRefLoadConfigFn() const { return m_soundRefLoadConfigFn; }
  uintptr_t GetUiSoundRefTableAddr() const { return m_uiSoundRefTableAddr; }
  uintptr_t GetVoiceNavTableAddr() const { return m_voiceNavTableAddr; }
  uint32_t GetUiWrapperArrayCountOffset() const { return m_uiWrapperArrayCountOffset; }
  uint32_t GetUiWrapperArrayBufferOffset() const { return m_uiWrapperArrayBufferOffset; }
  uint32_t GetUiWrapperEventOffset() const { return m_uiWrapperEventOffset; }
  uint32_t GetVoiceNavArrayCountOffset() const { return m_voiceNavArrayCountOffset; }
  uint32_t GetVoiceNavArrayBufferOffset() const { return m_voiceNavArrayBufferOffset; }
  uint32_t GetVoiceNavMaxEntries() const { return m_voiceNavMaxEntries; }
  uint32_t GetUiSoundRefCount() const { return m_uiSoundRefCount; }
  uint32_t GetUiSoundRefEntrySize() const { return m_uiSoundRefEntrySize; }
  uint32_t GetUiSoundRefCategoryOffset() const { return m_uiSoundRefCategoryOffset; }
  uint32_t GetUiSoundRefEnabledOffset() const { return m_uiSoundRefEnabledOffset; }
  uint32_t GetVoiceNavEntrySize() const { return m_voiceNavEntrySize; }
  uint32_t GetVoiceNavEntryStride() const { return m_voiceNavEntryStride; }
  uint32_t GetVoiceNavEntryEventOffset() const { return m_voiceNavEntryEventOffset; }
  uint32_t GetVoiceNavPathOffset() const { return m_voiceNavPathOffset; }
  uint32_t GetVoiceNavEnabledOffset() const { return m_voiceNavEnabledOffset; }
  uint32_t GetVoiceNavIndexOffset() const { return m_voiceNavIndexOffset; }
  uint32_t GetVoiceNavCategoryOffset() const { return m_voiceNavCategoryOffset; }
  uint32_t GetSoundEventBoundState() const { return m_soundEventBoundState; }
  uint32_t GetStudioSystemOffset() const { return m_studioSystemOffset; }

  void SetBankListLockOffset(uint32_t off) { m_bankListLockOffset = off; }
  void SetBankListHeadOffset(uint32_t off) { m_bankListHeadOffset = off; }
  void SetBankListSentinelOffset(uint32_t off) { m_bankListSentinelOffset = off; }
  void SetBankEventListHeadOffset(uint32_t off) { m_bankEventListHeadOffset = off; }
  void SetBankPathStringOffset(uint32_t off) { m_bankPathStringOffset = off; }
  void SetEventListTerminatorOffset(uint32_t off) { m_eventListTerminatorOffset = off; }
  void SetEventPathOffset(uint32_t off) { m_eventPathOffset = off; }
  void SetEventGuidOffset(uint32_t off) { m_eventGuidOffset = off; }
  void SetSoundEventListHeadOffset(uint32_t off) { m_soundEventListHeadOffset = off; }
  void SetSoundEventStateOffset(uint32_t off) { m_soundEventStateOffset = off; }
  void SetSoundEventBoundLockOffset(uint32_t off) { m_soundEventBoundLockOffset = off; }
  void SetSoundEventNodeOffset(uint32_t off) { m_soundEventNodeOffset = off; }
  void SetSoundEventPathOffset(uint32_t off) { m_soundEventPathOffset = off; }
  void SetSoundEventSourceOffset(uint32_t off) { m_soundEventSourceOffset = off; }
  void SetSoundEventCreateLockAddr(uintptr_t addr) { m_soundEventCreateLockAddr = addr; }
  void SetSoundEventActivateFn(uintptr_t fn) { m_soundEventActivateFn = fn; }
  void SetSoundRefLoadConfigFn(uintptr_t fn) { m_soundRefLoadConfigFn = fn; }
  void SetUiSoundRefTableAddr(uintptr_t addr) { m_uiSoundRefTableAddr = addr; }
  void SetVoiceNavTableAddr(uintptr_t addr) { m_voiceNavTableAddr = addr; }
  void SetUiWrapperArrayCountOffset(uint32_t off) { m_uiWrapperArrayCountOffset = off; }
  void SetUiWrapperArrayBufferOffset(uint32_t off) { m_uiWrapperArrayBufferOffset = off; }
  void SetUiWrapperEventOffset(uint32_t off) { m_uiWrapperEventOffset = off; }
  void SetVoiceNavArrayCountOffset(uint32_t off) { m_voiceNavArrayCountOffset = off; }
  void SetVoiceNavArrayBufferOffset(uint32_t off) { m_voiceNavArrayBufferOffset = off; }
  void SetVoiceNavMaxEntries(uint32_t count) { m_voiceNavMaxEntries = count; }
  void SetUiSoundRefCount(uint32_t count) { m_uiSoundRefCount = count; }
  void SetUiSoundRefEntrySize(uint32_t size) { m_uiSoundRefEntrySize = size; }
  void SetUiSoundRefCategoryOffset(uint32_t off) { m_uiSoundRefCategoryOffset = off; }
  void SetUiSoundRefEnabledOffset(uint32_t off) { m_uiSoundRefEnabledOffset = off; }
  void SetVoiceNavEntrySize(uint32_t size) { m_voiceNavEntrySize = size; }
  void SetVoiceNavEntryStride(uint32_t stride) { m_voiceNavEntryStride = stride; }
  void SetVoiceNavEntryEventOffset(uint32_t off) { m_voiceNavEntryEventOffset = off; }
  void SetVoiceNavPathOffset(uint32_t off) { m_voiceNavPathOffset = off; }
  void SetVoiceNavEnabledOffset(uint32_t off) { m_voiceNavEnabledOffset = off; }
  void SetVoiceNavIndexOffset(uint32_t off) { m_voiceNavIndexOffset = off; }
  void SetVoiceNavCategoryOffset(uint32_t off) { m_voiceNavCategoryOffset = off; }
  void SetSoundEventBoundState(uint32_t state) { m_soundEventBoundState = state; }
  void SetStudioSystemOffset(uint32_t off) { m_studioSystemOffset = off; }

  void RegisterFinders();
  bool ResolveFmodFunctions();
  void* GetStudioSystemRaw();

  bool m_isInitialized = false;
  bool m_fmodFunctionsResolved = false;
  std::vector<std::unique_ptr<ISoundDataFinder>> m_dataFinders;

  uint32_t m_bankListLockOffset = 0;
  uint32_t m_bankListHeadOffset = 0;
  uint32_t m_bankListSentinelOffset = 0;
  uint32_t m_bankEventListHeadOffset = 0;
  uint32_t m_bankPathStringOffset = 0;
  uint32_t m_eventListTerminatorOffset = 0;
  uint32_t m_eventPathOffset = 0;
  uint32_t m_eventGuidOffset = 0;
  uint32_t m_soundEventListHeadOffset = 0;
  uint32_t m_soundEventStateOffset = 0;
  uint32_t m_soundEventBoundLockOffset = 0;
  uint32_t m_soundEventNodeOffset = 0;
  uint32_t m_soundEventPathOffset = 0;
  uint32_t m_soundEventSourceOffset = 0;
  uintptr_t m_soundEventCreateLockAddr = 0;
  uintptr_t m_soundEventActivateFn = 0;
  uintptr_t m_soundRefLoadConfigFn = 0;
  uintptr_t m_uiSoundRefTableAddr = 0;
  uintptr_t m_voiceNavTableAddr = 0;
  uint32_t m_uiWrapperArrayCountOffset = 0;
  uint32_t m_uiWrapperArrayBufferOffset = 0;
  uint32_t m_uiWrapperEventOffset = 0;
  uint32_t m_voiceNavArrayCountOffset = 0;
  uint32_t m_voiceNavArrayBufferOffset = 0;
  uint32_t m_voiceNavMaxEntries = 0;
  uint32_t m_uiSoundRefCount = 0;
  uint32_t m_uiSoundRefEntrySize = 0;
  uint32_t m_uiSoundRefCategoryOffset = 0;
  uint32_t m_uiSoundRefEnabledOffset = 0;
  uint32_t m_voiceNavEntrySize = 0;
  uint32_t m_voiceNavEntryStride = 0;
  uint32_t m_voiceNavEntryEventOffset = 0;
  uint32_t m_voiceNavPathOffset = 0;
  uint32_t m_voiceNavEnabledOffset = 0;
  uint32_t m_voiceNavIndexOffset = 0;
  uint32_t m_voiceNavCategoryOffset = 0;
  uint32_t m_soundEventBoundState = 0;
  std::vector<EventCacheEntry> m_eventCache;
  std::vector<void*> m_pluginBanks;

  struct GuidHash {
    size_t operator()(const std::array<uint8_t, 16>& g) const {
      size_t h = 0;
      for (int i = 0; i < 16; ++i) h = h * 31 + g[i];
      return h;
    }
  };
  struct GuidEqual {
    bool operator()(const std::array<uint8_t, 16>& a, const std::array<uint8_t, 16>& b) const {
      return std::memcmp(a.data(), b.data(), 16) == 0;
    }
  };
  std::unordered_map<std::array<uint8_t, 16>, std::string, GuidHash, GuidEqual> m_guidToPath;
  std::vector<BusCacheEntry> m_busCache;
  std::vector<VCACacheEntry> m_vcaCache;
  uint32_t m_studioSystemOffset = 0;

  std::unordered_map<std::string, std::string> m_soundRefOverrides;
  std::unordered_map<std::string, std::string> m_soundRefOriginals;  // soundref path -> source before first override
  mutable std::mutex m_soundRefMutex;

  std::vector<std::string> m_soundRefRdataCatalog;
  std::vector<std::string> m_soundRefVfsCatalog;
  bool m_soundRefRdataCatalogBuilt = false;
  bool m_soundRefVfsCatalogBuilt = false;

  std::vector<SoundRefEntry> m_soundRefSnapshot;
  std::unordered_map<std::string, int> m_soundRefSnapshotIndex;
  std::atomic<bool> m_soundRefSnapshotDirty{true};
  std::atomic<bool> m_soundRefSnapshotValid{false};
  mutable std::mutex m_soundRefSnapshotMutex;

  void* m_loadConfigTrampoline = nullptr;
  bool m_loadConfigHookInstalled = false;
  void* m_systemUpdateTrampoline = nullptr;
  uintptr_t m_systemUpdateFnAddr = 0;
  bool m_systemUpdateHookInstalled = false;
  std::vector<void*> m_pendingRebinds;
  std::mutex m_pendingRebindMutex;
  std::atomic<uint32_t> m_gameThreadId{0};

  struct FmodFn {
    void* System_GetBus = nullptr;
    void* System_GetVCA = nullptr;
    void* System_GetEventByID = nullptr;
    void* System_GetParameterByName = nullptr;
    void* System_SetParameterByName = nullptr;
    void* System_GetNumParameters = nullptr;
    void* System_GetParameterDescriptionByName = nullptr;
    void* System_GetParameterDescriptionByID = nullptr;
    void* System_GetParameterDescriptionCount = nullptr;
    void* System_GetParameterDescriptionList = nullptr;
    void* System_GetNumListeners = nullptr;
    void* System_SetNumListeners = nullptr;
    void* System_GetListenerAttributes = nullptr;
    void* System_SetListenerAttributes = nullptr;
    void* System_LoadBankFile = nullptr;
    void* System_LoadBankMemory = nullptr;
    void* System_Update = nullptr;
    void* System_GetBankCount = nullptr;
    void* System_GetBankList = nullptr;
    void* System_GetVCACount = nullptr;
    void* System_GetVCAList = nullptr;
    void* EventDescription_CreateInstance = nullptr;
    void* EventDescription_GetLength = nullptr;
    void* EventDescription_Is3D = nullptr;
    void* EventDescription_IsOneshot = nullptr;
    void* EventDescription_IsStream = nullptr;
    void* EventDescription_IsSnapshot = nullptr;
    void* EventDescription_IsDopplerEnabled = nullptr;
    void* EventDescription_HasSustainPoint = nullptr;
    void* EventDescription_GetMinMaxDistance = nullptr;
    void* EventDescription_GetID = nullptr;
    void* EventDescription_GetPath = nullptr;
    void* EventDescription_GetInstanceCount = nullptr;
    void* EventDescription_GetInstanceList = nullptr;
    void* EventDescription_GetParameterDescriptionCount = nullptr;
    void* EventDescription_GetParameterDescriptionByName = nullptr;
    void* EventDescription_GetParameterDescriptionByIndex = nullptr;
    void* EventDescription_GetSampleLoadingState = nullptr;
    void* EventDescription_GetSoundSize = nullptr;
    void* EventDescription_GetUserPropertyCount = nullptr;
    void* EventDescription_GetUserPropertyByIndex = nullptr;

    void* EventInstance_Start = nullptr;
    void* EventInstance_Stop = nullptr;
    void* EventInstance_SetPaused = nullptr;
    void* EventInstance_GetPlaybackState = nullptr;
    void* EventInstance_Release = nullptr;
    void* EventInstance_SetVolume = nullptr;
    void* EventInstance_GetVolume = nullptr;
    void* EventInstance_SetPitch = nullptr;
    void* EventInstance_GetPitch = nullptr;
    void* EventInstance_Set3DAttributes = nullptr;
    void* EventInstance_Get3DAttributes = nullptr;
    void* EventInstance_SetParameterByName = nullptr;
    void* EventInstance_GetParameterByName = nullptr;
    void* EventInstance_SetParameterByID = nullptr;
    void* EventInstance_GetParameterByID = nullptr;
    void* EventInstance_SetTimelinePosition = nullptr;
    void* EventInstance_GetTimelinePosition = nullptr;
    void* EventInstance_GetDescription = nullptr;
    void* EventInstance_SetCallback = nullptr;
    void* EventInstance_SetLoopCount = nullptr;
    void* EventInstance_GetLoopCount = nullptr;

    void* Bus_GetVolume = nullptr;
    void* Bus_SetVolume = nullptr;
    void* Bus_GetMute = nullptr;
    void* Bus_SetMute = nullptr;
    void* Bus_SetPaused = nullptr;
    void* Bus_GetPaused = nullptr;
    void* Bus_GetPath = nullptr;
    void* Bus_GetID = nullptr;
    void* Bus_GetParent = nullptr;
    void* Bus_GetFaderLevel = nullptr;
    void* Bus_IsBypassed = nullptr;

    void* VCA_GetVolume = nullptr;
    void* VCA_SetVolume = nullptr;
    void* VCA_GetPath = nullptr;

    void* Bank_GetLoadingState = nullptr;
    void* Bank_GetSampleLoadingState = nullptr;
    void* Bank_LoadSampleData = nullptr;
    void* Bank_UnloadSampleData = nullptr;
    void* Bank_Unload = nullptr;
    void* Bank_GetEventCount = nullptr;
    void* Bank_GetEventList = nullptr;
    void* Bank_GetBusCount = nullptr;
    void* Bank_GetBusList = nullptr;
    void* Bank_GetVCACount = nullptr;
    void* Bank_GetVCAList = nullptr;
    void* Bank_GetPath = nullptr;
  } m_fmodFn;
};

}  // namespace SPF::Data::GameData
