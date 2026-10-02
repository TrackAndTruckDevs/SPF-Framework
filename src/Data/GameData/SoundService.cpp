#include "SPF/Data/GameData/SoundService.hpp"

#include "SPF/Data/GameData/Finders/SoundDataFinder.hpp"
#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Fmod/FmodApi.hpp"
#include "SPF/Fmod/FmodStudioHook.hpp"
#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include "MinHook.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <minwindef.h>
#include <mutex>
#include <processthreadsapi.h>
#include <string>
#include <synchapi.h>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <windows.h>

namespace SPF::Data::GameData {

namespace {
thread_local bool tls_insidePluginUpdate = false;
}

SoundService::SoundService() { WorldServiceRegistry::Get().Register(this); }

SoundService& SoundService::GetInstance() {
  static SoundService instance;
  return instance;
}

void SoundService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("Attempting to initialize SoundService...");

  RegisterFinders();

  m_isInitialized = false;
}

void SoundService::Shutdown() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");

  RemoveSoundRefLoadConfigHook();
  RemoveSystemUpdateHook();
  {
    std::lock_guard<std::mutex> lock(m_pendingRebindMutex);
    m_pendingRebinds.clear();
  }
  m_gameThreadId.store(0);

  m_isInitialized = false;
  m_fmodFunctionsResolved = false;
  std::memset(&m_fmodFn, 0, sizeof(m_fmodFn));

  m_bankListLockOffset = 0;
  m_bankListHeadOffset = 0;
  m_bankListSentinelOffset = 0;
  m_bankEventListHeadOffset = 0;
  m_bankPathStringOffset = 0;
  m_eventListTerminatorOffset = 0;
  m_eventPathOffset = 0;
  m_eventGuidOffset = 0;
  m_soundEventListHeadOffset = 0;
  m_soundEventStateOffset = 0;
  m_soundEventBoundLockOffset = 0;
  m_soundEventNodeOffset = 0;
  m_soundEventPathOffset = 0;
  m_soundEventSourceOffset = 0;
  m_soundEventCreateLockAddr = 0;
  m_soundEventActivateFn = 0;
  m_soundEventVtableAddr = 0;
  m_soundRefLoadConfigFn = 0;
  m_uiSoundRefTableAddr = 0;
  m_voiceNavTableAddr = 0;
  m_soundEventBoundState = 0;
  m_uiSoundRefCount = 0;
  m_uiSoundRefEntrySize = 0;
  m_uiWrapperArrayBufferOffset = 0;
  m_uiWrapperArrayCountOffset = 0;
  m_uiWrapperEventOffset = 0;
  m_voiceNavArrayBufferOffset = 0;
  m_voiceNavArrayCountOffset = 0;
  m_voiceNavMaxEntries = 0;
  m_voiceNavEntrySize = 0;
  m_voiceNavEntryStride = 0;
  m_voiceNavEntryEventOffset = 0;
  m_eventCache.clear();
  m_pluginBanks.clear();
  {
    std::lock_guard<std::mutex> lock(m_guidToPathMutex);
    m_guidToPath.clear();
  }
  {
    std::lock_guard<std::mutex> lock(m_soundRefMutex);
    m_soundRefOverrides.clear();
    m_soundRefOriginals.clear();
  }
  m_soundRefRdataCatalog.clear();
  m_soundRefVfsCatalog.clear();
  m_soundRefRdataCatalogBuilt = false;
  m_soundRefVfsCatalogBuilt = false;
  {
    std::lock_guard<std::mutex> lock(m_soundRefSnapshotMutex);
    m_soundRefSnapshot.clear();
    m_soundRefSnapshotIndex.clear();
    m_soundRefSnapshotValid.store(false, std::memory_order_relaxed);
  }
  m_soundRefSnapshotDirty.store(true, std::memory_order_relaxed);

  for (const auto& finder : m_dataFinders) {
    finder->Reset();
  }

  logger->Info("SoundService has been shut down.");
}

void SoundService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::SoundDataFinder>()); }

bool SoundService::ResolveFmodFunctions() {
  if (m_fmodFunctionsResolved) return true;

  auto& fmodApi = Fmod::FmodApi::GetInstance();
  if (!fmodApi.IsReady()) return false;

  m_fmodFn.System_GetBus = fmodApi.Find("System::getBus");
  m_fmodFn.System_GetVCA = fmodApi.Find("System::getVCA");
  m_fmodFn.System_GetEventByID = fmodApi.Find("System::getEventByID");
  m_fmodFn.System_GetParameterByName = fmodApi.Find("System::getParameterByName");
  m_fmodFn.System_SetParameterByName = fmodApi.Find("System::setParameterByName");
  m_fmodFn.System_GetNumParameters = fmodApi.Find("System::getNumParameters");
  m_fmodFn.System_GetParameterDescriptionByName = fmodApi.Find("System::getParameterDescriptionByName");
  m_fmodFn.System_GetParameterDescriptionByID = fmodApi.Find("System::getParameterDescriptionByID");
  m_fmodFn.System_GetParameterDescriptionCount = fmodApi.Find("System::getParameterDescriptionCount");
  m_fmodFn.System_GetParameterDescriptionList = fmodApi.Find("System::getParameterDescriptionList");
  m_fmodFn.System_GetNumListeners = fmodApi.Find("System::getNumListeners");
  m_fmodFn.System_SetNumListeners = fmodApi.Find("System::setNumListeners");
  m_fmodFn.System_GetListenerAttributes = fmodApi.Find("System::getListenerAttributes");
  m_fmodFn.System_SetListenerAttributes = fmodApi.Find("System::setListenerAttributes");
  m_fmodFn.System_LoadBankFile = fmodApi.Find("System::loadBankFile");
  m_fmodFn.System_LoadBankMemory = fmodApi.Find("System::loadBankMemory");
  m_fmodFn.System_Update = fmodApi.Find("System::update");

  m_fmodFn.EventDescription_CreateInstance = fmodApi.Find("EventDescription::createInstance");
  m_fmodFn.EventDescription_GetLength = fmodApi.Find("EventDescription::getLength");
  m_fmodFn.EventDescription_Is3D = fmodApi.Find("EventDescription::is3D");
  m_fmodFn.EventDescription_IsOneshot = fmodApi.Find("EventDescription::isOneshot");
  m_fmodFn.EventDescription_IsStream = fmodApi.Find("EventDescription::isStream");
  m_fmodFn.EventDescription_IsSnapshot = fmodApi.Find("EventDescription::isSnapshot");
  m_fmodFn.EventDescription_IsDopplerEnabled = fmodApi.Find("EventDescription::isDopplerEnabled");
  m_fmodFn.EventDescription_HasSustainPoint = fmodApi.Find("EventDescription::hasSustainPoint");
  m_fmodFn.EventDescription_GetMinMaxDistance = fmodApi.Find("EventDescription::getMinMaxDistance");
  m_fmodFn.EventDescription_GetID = fmodApi.Find("EventDescription::getID");
  m_fmodFn.EventDescription_GetPath = fmodApi.Find("EventDescription::getPath");
  m_fmodFn.EventDescription_GetInstanceCount = fmodApi.Find("EventDescription::getInstanceCount");
  m_fmodFn.EventDescription_GetInstanceList = fmodApi.Find("EventDescription::getInstanceList");
  m_fmodFn.EventDescription_GetParameterDescriptionCount = fmodApi.Find("EventDescription::getParameterDescriptionCount");
  m_fmodFn.EventDescription_GetParameterDescriptionByName = fmodApi.Find("EventDescription::getParameterDescriptionByName");
  m_fmodFn.EventDescription_GetParameterDescriptionByIndex = fmodApi.Find("EventDescription::getParameterDescriptionByIndex");
  m_fmodFn.EventDescription_GetSampleLoadingState = fmodApi.Find("EventDescription::getSampleLoadingState");
  m_fmodFn.EventDescription_GetSoundSize = fmodApi.Find("EventDescription::getSoundSize");
  m_fmodFn.EventDescription_GetUserPropertyCount = fmodApi.Find("EventDescription::getUserPropertyCount");
  m_fmodFn.EventDescription_GetUserPropertyByIndex = fmodApi.Find("EventDescription::getUserPropertyByIndex");

  m_fmodFn.EventInstance_Start = fmodApi.Find("EventInstance::start");
  m_fmodFn.EventInstance_Stop = fmodApi.Find("EventInstance::stop");
  m_fmodFn.EventInstance_SetPaused = fmodApi.Find("EventInstance::setPaused");
  m_fmodFn.EventInstance_GetPlaybackState = fmodApi.Find("EventInstance::getPlaybackState");
  m_fmodFn.EventInstance_Release = fmodApi.Find("EventInstance::release");
  m_fmodFn.EventInstance_SetVolume = fmodApi.Find("EventInstance::setVolume");
  m_fmodFn.EventInstance_GetVolume = fmodApi.Find("EventInstance::getVolume");
  m_fmodFn.EventInstance_SetPitch = fmodApi.Find("EventInstance::setPitch");
  m_fmodFn.EventInstance_GetPitch = fmodApi.Find("EventInstance::getPitch");
  m_fmodFn.EventInstance_Set3DAttributes = fmodApi.Find("EventInstance::set3DAttributes");
  m_fmodFn.EventInstance_Get3DAttributes = fmodApi.Find("EventInstance::get3DAttributes");
  m_fmodFn.EventInstance_SetParameterByName = fmodApi.Find("EventInstance::setParameterByName");
  m_fmodFn.EventInstance_GetParameterByName = fmodApi.Find("EventInstance::getParameterByName");
  m_fmodFn.EventInstance_SetParameterByID = fmodApi.Find("EventInstance::setParameterByID");
  m_fmodFn.EventInstance_GetParameterByID = fmodApi.Find("EventInstance::getParameterByID");
  m_fmodFn.EventInstance_SetTimelinePosition = fmodApi.Find("EventInstance::setTimelinePosition");
  m_fmodFn.EventInstance_GetTimelinePosition = fmodApi.Find("EventInstance::getTimelinePosition");
  m_fmodFn.EventInstance_GetDescription = fmodApi.Find("EventInstance::getDescription");
  m_fmodFn.EventInstance_SetCallback = fmodApi.Find("EventInstance::setCallback");
  m_fmodFn.EventInstance_SetLoopCount = fmodApi.Find("EventInstance::setLoopCount");
  m_fmodFn.EventInstance_GetLoopCount = fmodApi.Find("EventInstance::getLoopCount");

  m_fmodFn.Bus_GetVolume = fmodApi.Find("Bus::getVolume");
  m_fmodFn.Bus_SetVolume = fmodApi.Find("Bus::setVolume");
  m_fmodFn.Bus_GetMute = fmodApi.Find("Bus::getMute");
  m_fmodFn.Bus_SetMute = fmodApi.Find("Bus::setMute");
  m_fmodFn.Bus_SetPaused = fmodApi.Find("Bus::setPaused");
  m_fmodFn.Bus_GetPaused = fmodApi.Find("Bus::getPaused");

  m_fmodFn.VCA_GetVolume = fmodApi.Find("VCA::getVolume");
  m_fmodFn.VCA_SetVolume = fmodApi.Find("VCA::setVolume");
  m_fmodFn.VCA_GetPath = fmodApi.Find("VCA::getPath");

  m_fmodFn.Bank_GetLoadingState = fmodApi.Find("Bank::getLoadingState");
  m_fmodFn.Bank_GetSampleLoadingState = fmodApi.Find("Bank::getSampleLoadingState");
  m_fmodFn.Bank_LoadSampleData = fmodApi.Find("Bank::loadSampleData");
  m_fmodFn.Bank_UnloadSampleData = fmodApi.Find("Bank::unloadSampleData");
  m_fmodFn.Bank_Unload = fmodApi.Find("Bank::unload");
  m_fmodFn.Bank_GetEventCount = fmodApi.Find("Bank::getEventCount");
  m_fmodFn.Bank_GetEventList = fmodApi.Find("Bank::getEventList");
  m_fmodFn.Bank_GetBusCount = fmodApi.Find("Bank::getBusCount");
  m_fmodFn.Bank_GetBusList = fmodApi.Find("Bank::getBusList");
  m_fmodFn.Bank_GetVCACount = fmodApi.Find("Bank::getVCACount");
  m_fmodFn.Bank_GetVCAList = fmodApi.Find("Bank::getVCAList");
  m_fmodFn.Bus_GetPath = fmodApi.Find("Bus::getPath");
  m_fmodFn.Bus_GetParent = fmodApi.Find("Bus::getParent");
  m_fmodFn.Bus_GetFaderLevel = fmodApi.Find("Bus::getFaderLevel");
  m_fmodFn.Bus_IsBypassed = fmodApi.Find("Bus::isBypassed");
  m_fmodFn.System_GetBankCount = fmodApi.Find("System::getBankCount");
  m_fmodFn.System_GetBankList = fmodApi.Find("System::getBankList");
  m_fmodFn.System_GetVCACount = fmodApi.Find("System::getVCACount");
  m_fmodFn.System_GetVCAList = fmodApi.Find("System::getVCAList");
  m_fmodFn.Bank_GetPath = fmodApi.Find("Bank::getPath");
  m_fmodFunctionsResolved = true;
  if (m_fmodFn.System_Update) InstallSystemUpdateHook();
  return true;
}

void* SoundService::GetStudioSystemRaw() {
  uintptr_t soundSystem = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundSystem) return nullptr;
  return *reinterpret_cast<void**>(soundSystem + m_studioSystemOffset);
}

bool SoundService::TryFindAllOffsets() {
  if (m_isInitialized) return true;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");

  if (!ManagerCoreService::GetInstance().IsSoundManagerReady()) {
    logger->Warn("SoundService: SoundManager not resolved yet. Waiting for ManagerCoreService.");
    return false;
  }

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("[Success] Finder '{}' completed successfully.", finder->GetName());
      } else {
        logger->Warn("[Failed] Finder '{}' could not resolve all patterns. Will retry on next tick.", finder->GetName());
        return false;
      }
    }
  }

  m_isInitialized = true;
  RegisterPathResolver();
  InstallSoundRefLoadConfigHook();
  ResolveFmodFunctions();
  InstallSystemUpdateHook();
  logger->Info("SoundService: All offsets found. Service is READY.");
  return true;
}

bool SoundService::IsReady() { return m_isInitialized; }

std::vector<SoundBankGroup> SoundService::GetSoundBankGroups() {
  std::vector<SoundBankGroup> result;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");

  if (!m_isInitialized) {
    logger->Warn("GetSoundBankGroups: Service not initialized.");
    return result;
  }

  uintptr_t soundSystem = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundSystem) {
    logger->Warn("GetSoundBankGroups: SoundManager address is null.");
    return result;
  }

  uintptr_t lockAddr = soundSystem + m_bankListLockOffset;
  AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  uintptr_t bankListHead = *reinterpret_cast<uintptr_t*>(soundSystem + m_bankListHeadOffset);
  uintptr_t bankSentinel = soundSystem + m_bankListSentinelOffset;

  uintptr_t bankNode = bankListHead;

  while (bankNode != bankSentinel) {
    const char* bankPath = *reinterpret_cast<const char**>(bankNode + m_bankPathStringOffset);
    SoundBankGroup group;
    group.bankPath = bankPath ? bankPath : "";

    uintptr_t eventNode = *reinterpret_cast<uintptr_t*>(bankNode + m_bankEventListHeadOffset);
    uintptr_t eventSentinel = bankNode + m_bankEventListHeadOffset + m_eventListTerminatorOffset;

    while (eventNode != eventSentinel) {
      const char* eventPath = *reinterpret_cast<const char**>(eventNode + m_eventPathOffset);

      if (!eventPath || strncmp(eventPath, "event:/", 7) != 0) {
        eventNode = *reinterpret_cast<uintptr_t*>(eventNode);
        continue;
      }

      SoundEvent ev;
      ev.bankPath = group.bankPath;
      ev.eventPath = eventPath ? eventPath : "";
      memcpy(ev.guid, reinterpret_cast<void*>(eventNode + m_eventGuidOffset), 16);

      group.events.push_back(std::move(ev));

      eventNode = *reinterpret_cast<uintptr_t*>(eventNode);
    }

    result.push_back(std::move(group));
    bankNode = *reinterpret_cast<uintptr_t*>(bankNode);
  }

  ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  int totalEvents = 0;
  for (const auto& g : result) totalEvents += static_cast<int>(g.events.size());
  return result;
}

void SoundService::RegisterPathResolver() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  Fmod::FmodStudioHook::GetInstance().SetPathResolverCallback(PathResolverCallback, this);
  logger->Info("SoundService: Registered path resolver into FmodStudioHook.");
}

const char* SoundService::PathResolverCallback(void* userData, const uint8_t guid[16], void* instance) {
  (void)instance;
  auto* self = static_cast<SoundService*>(userData);
  return self->ResolveEventPathByGuid(guid);
}

const char* SoundService::ResolveEventPathByGuid(const uint8_t guid[16]) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  if (!m_isInitialized) return nullptr;

  uintptr_t soundSystem = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundSystem) return nullptr;

  uintptr_t lockAddr = soundSystem + m_bankListLockOffset;
  AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  const char* found = nullptr;

  uintptr_t bankNode = *reinterpret_cast<uintptr_t*>(soundSystem + m_bankListHeadOffset);
  uintptr_t bankSentinel = soundSystem + m_bankListSentinelOffset;

  while (bankNode != bankSentinel) {
    uintptr_t eventNode = *reinterpret_cast<uintptr_t*>(bankNode + m_bankEventListHeadOffset);
    uintptr_t eventSentinel = bankNode + m_bankEventListHeadOffset + m_eventListTerminatorOffset;

    while (eventNode != eventSentinel) {
      if (memcmp(reinterpret_cast<void*>(eventNode + m_eventGuidOffset), guid, 16) == 0) {
        const char* eventPath = *reinterpret_cast<const char**>(eventNode + m_eventPathOffset);
        if (eventPath && strncmp(eventPath, "event:/", 7) == 0) {
          found = eventPath;
        }
        break;
      }
      eventNode = *reinterpret_cast<uintptr_t*>(eventNode);
    }

    if (found) break;
    bankNode = *reinterpret_cast<uintptr_t*>(bankNode);
  }

  ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  // Fallback: plugin banks never appear in the game's per-bank lists, but
  // LoadGuidsFile indexed their GUIDs from the sidecar .guids file. Copy under
  // the lock — the caller dereferences the pointer after we return.
  if (!found) {
    std::array<uint8_t, 16> key{};
    std::memcpy(key.data(), guid, 16);
    std::lock_guard<std::mutex> lock(m_guidToPathMutex);
    auto it = m_guidToPath.find(key);
    if (it != m_guidToPath.end()) {
      static thread_local std::string resolvedPath;
      resolvedPath = it->second;
      found = resolvedPath.c_str();
    }
  }

  return found;
}

std::vector<SoundBankGroup> SoundService::GetPluginBankGroups() {
  std::vector<SoundBankGroup> result;
  if (!m_isInitialized || !m_fmodFunctionsResolved) return result;
  if (m_pluginBanks.empty()) return result;

  auto fnGetEventCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetEventCount);
  auto fnGetEventList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, FMOD::Studio::EventDescription**, int, int*)>(m_fmodFn.Bank_GetEventList);
  auto fnGetPath = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, char*, int, int*)>(m_fmodFn.EventDescription_GetPath);
  auto fnGetID = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD_GUID*)>(m_fmodFn.EventDescription_GetID);
  auto fnGetLength = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, uint32_t*)>(m_fmodFn.EventDescription_GetLength);
  auto fnIs3DFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_Is3D);
  auto fnIsOneshot = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsOneshot);
  auto fnIsStream = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsStream);
  auto fnIsSnapshot = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsSnapshot);
  auto fnGetMinMax = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, float*, float*)>(m_fmodFn.EventDescription_GetMinMaxDistance);

  if (!fnGetEventCount || !fnGetEventList || !fnGetID) return result;

  for (auto* rawBank : m_pluginBanks) {
    auto* bank = static_cast<FMOD::Studio::Bank*>(rawBank);
    int count = 0;
    if (fnGetEventCount(bank, &count) != FMOD_OK || count <= 0) continue;

    std::vector<FMOD::Studio::EventDescription*> descs(count);
    int fetched = 0;
    if (fnGetEventList(bank, descs.data(), count, &fetched) != FMOD_OK) continue;

    SoundBankGroup group;
    group.bankPath = "[plugin bank]";

    for (int i = 0; i < fetched; ++i) {
      SoundEvent ev;
      ev.eventDesc = descs[i];

      if (fnGetID(descs[i], reinterpret_cast<FMOD_GUID*>(ev.guid)) == FMOD_OK) {
        std::array<uint8_t, 16> guidArr{};
        std::memcpy(guidArr.data(), ev.guid, 16);
        auto it = m_guidToPath.find(guidArr);
        if (it != m_guidToPath.end()) {
          ev.eventPath = it->second;
        }
      }

      char pathBuf[512] = {};
      int pathRc = -2;  // -2 = EventDescription::getPath unresolved
      if (fnGetPath) {
        FMOD_RESULT rc = fnGetPath(descs[i], pathBuf, sizeof(pathBuf), nullptr);
        pathRc = static_cast<int>(rc);
        if (rc == FMOD_OK && pathBuf[0] != '\0') {
          ev.eventPath = pathBuf;
        }
      }

      if (ev.eventPath.empty()) {
        static std::unordered_set<const void*> s_loggedEmptyPath;
        if (s_loggedEmptyPath.insert(descs[i]).second) {
          auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
          char guidHex[33] = {};
          for (int g = 0; g < 16; ++g) snprintf(guidHex + g * 2, 3, "%02x", static_cast<unsigned char>(ev.guid[g]));
          logger->Warn("GetPluginBankGroups: unresolved path for event desc={} guid={} getPathRc={} — add '{{guid}} event:/path' line to bank.guids",
                       static_cast<const void*>(descs[i]), guidHex, pathRc);
        }
      }

      bool bVal = false;
      if (fnIs3DFn && fnIs3DFn(descs[i], &bVal) == FMOD_OK) {
        ev.is3D = bVal;
        ev.hasIs3D = true;
      }
      if (fnIsOneshot && fnIsOneshot(descs[i], &bVal) == FMOD_OK) {
        ev.isOneshot = bVal;
        ev.hasIsOneshot = true;
      }
      if (fnIsStream && fnIsStream(descs[i], &bVal) == FMOD_OK) {
        ev.isStream = bVal;
        ev.hasIsStream = true;
      }
      if (fnIsSnapshot && fnIsSnapshot(descs[i], &bVal) == FMOD_OK) {
        ev.isSnapshot = bVal;
        ev.hasIsSnapshot = true;
      }

      uint32_t len = 0;
      if (fnGetLength && fnGetLength(descs[i], &len) == FMOD_OK) {
        ev.durationMs = len;
        ev.hasDuration = true;
      }

      float minD = 0.0f, maxD = 0.0f;
      if (fnGetMinMax && fnGetMinMax(descs[i], &minD, &maxD) == FMOD_OK) {
        ev.minDistance = minD;
        ev.maxDistance = maxD;
      }

      group.events.push_back(std::move(ev));
    }

    if (!group.events.empty()) {
      result.push_back(std::move(group));
    }
  }
  return result;
}

void SoundService::EnrichEventsWithFmodData(std::vector<SoundBankGroup>& groups) {
  if (!ResolveFmodFunctions()) return;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetEventByID) return;

  auto fnGetEventByID = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const FMOD_GUID*, FMOD::Studio::EventDescription**)>(m_fmodFn.System_GetEventByID);
  auto fnGetLength = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, uint32_t*)>(m_fmodFn.EventDescription_GetLength);
  auto fnIs3DFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_Is3D);
  auto fnIsSnapshot = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsSnapshot);
  auto fnIsOneshot = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsOneshot);
  auto fnIsStream = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsStream);
  auto fnIsDoppler = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsDopplerEnabled);
  auto fnHasSustain = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_HasSustainPoint);
  auto fnGetMinMax = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, float*, float*)>(m_fmodFn.EventDescription_GetMinMaxDistance);
  auto fnGetSoundSize = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, uint32_t*)>(m_fmodFn.EventDescription_GetSoundSize);
  auto fnGetSampleState = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetSampleLoadingState);
  auto fnGetInstanceCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetInstanceCount);
  auto fnGetUserPropCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetUserPropertyCount);
  auto fnGetUserPropByIndex = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int, FMOD_STUDIO_USER_PROPERTY*)>(m_fmodFn.EventDescription_GetUserPropertyByIndex);

  for (auto& group : groups) {
    for (auto& event : group.events) {
      FMOD_GUID guid;
      std::memcpy(&guid, event.guid, sizeof(guid));

      FMOD::Studio::EventDescription* desc = nullptr;
      if (fnGetEventByID(studioSys, &guid, &desc) != FMOD_OK || !desc) continue;

      if (fnGetLength) {
        uint32_t length = 0;
        if (fnGetLength(desc, &length) == FMOD_OK) {
          event.durationMs = length;
          event.hasDuration = true;
        }
      }
      if (fnIs3DFn) {
        bool val = false;
        if (fnIs3DFn(desc, &val) == FMOD_OK) {
          event.is3D = val;
          event.hasIs3D = true;
        }
      }
      if (fnIsSnapshot) {
        bool val = false;
        if (fnIsSnapshot(desc, &val) == FMOD_OK) {
          event.isSnapshot = val;
          event.hasIsSnapshot = true;
        }
      }
      if (fnIsOneshot) {
        bool val = false;
        if (fnIsOneshot(desc, &val) == FMOD_OK) {
          event.isOneshot = val;
          event.hasIsOneshot = true;
        }
      }
      if (fnIsStream) {
        bool val = false;
        if (fnIsStream(desc, &val) == FMOD_OK) {
          event.isStream = val;
          event.hasIsStream = true;
        }
      }
      if (fnIsDoppler) {
        bool val = false;
        if (fnIsDoppler(desc, &val) == FMOD_OK) {
          event.isDopplerEnabled = val;
          event.hasIsDopplerEnabled = true;
        }
      }
      if (fnHasSustain) {
        bool val = false;
        if (fnHasSustain(desc, &val) == FMOD_OK) {
          event.hasSustainPoint = val;
          event.hasHasSustainPoint = true;
        }
      }
      if (fnGetMinMax) {
        fnGetMinMax(desc, &event.minDistance, &event.maxDistance);
      }
      if (fnGetSoundSize) {
        fnGetSoundSize(desc, &event.soundSize);
      }
      if (fnGetSampleState) {
        fnGetSampleState(desc, &event.sampleLoadingState);
      }
      if (fnGetInstanceCount) {
        int count = 0;
        if (fnGetInstanceCount(desc, &count) == FMOD_OK) event.instanceCount = count;
      }
      event.eventDesc = desc;
      if (fnGetUserPropCount && fnGetUserPropByIndex) {
        int propCount = 0;
        if (fnGetUserPropCount(desc, &propCount) == FMOD_OK && propCount > 0) {
          for (int pi = 0; pi < propCount; ++pi) {
            FMOD_STUDIO_USER_PROPERTY prop = {};
            if (fnGetUserPropByIndex(desc, pi, &prop) == FMOD_OK) {
              SoundUserProperty up;
              up.name = prop.name ? prop.name : "";
              up.type = prop.type;
              if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_BOOLEAN)
                up.boolValue = prop.boolvalue != 0;
              else if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_INTEGER)
                up.intValue = prop.intvalue;
              else if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_FLOAT)
                up.floatValue = prop.floatvalue;
              else if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_STRING)
                up.stringValue = prop.stringvalue ? prop.stringvalue : "";
              event.userProperties.push_back(std::move(up));
            }
          }
        }
      }
    }
  }
}

void SoundService::EnrichEventParameters(std::vector<SoundBankGroup>& groups) {
  if (!ResolveFmodFunctions()) return;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetEventByID) return;

  auto fnGetEventByID = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const FMOD_GUID*, FMOD::Studio::EventDescription**)>(m_fmodFn.System_GetEventByID);
  auto fnGetParamDescCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetParameterDescriptionCount);
  auto fnGetParamDescByIndex = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int, FMOD_STUDIO_PARAMETER_DESCRIPTION*)>(m_fmodFn.EventDescription_GetParameterDescriptionByIndex);
  if (!fnGetParamDescCount || !fnGetParamDescByIndex) return;

  for (auto& group : groups) {
    for (auto& event : group.events) {
      FMOD_GUID guid;
      std::memcpy(&guid, event.guid, sizeof(guid));

      FMOD::Studio::EventDescription* desc = nullptr;
      if (fnGetEventByID(studioSys, &guid, &desc) != FMOD_OK || !desc) continue;

      int paramCount = 0;
      if (fnGetParamDescCount(desc, &paramCount) != FMOD_OK || paramCount <= 0) continue;

      event.parameters.clear();
      event.parameters.reserve(paramCount);
      for (int p = 0; p < paramCount; ++p) {
        FMOD_STUDIO_PARAMETER_DESCRIPTION pd = {};
        if (fnGetParamDescByIndex(desc, p, &pd) == FMOD_OK && pd.name) {
          SoundParameterDescription sp;
          sp.name = pd.name;
          sp.idData1 = pd.id.data1;
          sp.idData2 = pd.id.data2;
          sp.minimum = pd.minimum;
          sp.maximum = pd.maximum;
          sp.defaultvalue = pd.defaultvalue;
          sp.type = static_cast<int32_t>(pd.type);
          event.parameters.push_back(std::move(sp));
        }
      }
    }
  }
}

std::vector<SoundBusEntry> SoundService::GetBuses() {
  std::vector<SoundBusEntry> result;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  if (!ResolveFmodFunctions()) return result;

  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys) return result;

  auto fnGetBankCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int*)>(m_fmodFn.System_GetBankCount);
  auto fnGetBankList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, FMOD::Studio::Bank**, int, int*)>(m_fmodFn.System_GetBankList);
  auto fnGetBusCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetBusCount);
  auto fnGetBusList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, FMOD::Studio::Bus**, int, int*)>(m_fmodFn.Bank_GetBusList);
  auto fnGetPath = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, char*, int, int*)>(m_fmodFn.Bus_GetPath);
  auto fnGetParent = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, FMOD::Studio::Bus**)>(m_fmodFn.Bus_GetParent);
  auto fnGetVolume = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, float*, float*)>(m_fmodFn.Bus_GetVolume);
  auto fnGetMute = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool*)>(m_fmodFn.Bus_GetMute);
  auto fnGetPaused = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool*)>(m_fmodFn.Bus_GetPaused);
  auto fnGetFaderLevel = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, float*)>(m_fmodFn.Bus_GetFaderLevel);
  auto fnIsBypassed = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool*)>(m_fmodFn.Bus_IsBypassed);
  if (!fnGetBankCount || !fnGetBankList || !fnGetBusCount || !fnGetBusList || !fnGetPath) return result;

  int bankCount = 0;
  if (fnGetBankCount(studioSys, &bankCount) != FMOD_OK || bankCount <= 0) return result;

  std::vector<FMOD::Studio::Bank*> banks(bankCount);
  int returned = 0;
  if (fnGetBankList(studioSys, banks.data(), bankCount, &returned) != FMOD_OK) return result;

  std::unordered_set<std::string> seen;
  for (int i = 0; i < returned; ++i) {
    if (!banks[i]) continue;
    int count = 0;
    if (fnGetBusCount(banks[i], &count) != FMOD_OK || count <= 0) continue;

    std::vector<FMOD::Studio::Bus*> buses(count);
    int busReturned = 0;
    if (fnGetBusList(banks[i], buses.data(), count, &busReturned) != FMOD_OK) continue;

    for (int j = 0; j < busReturned; ++j) {
      if (!buses[j]) continue;
      char path[256] = {};
      int retrieved = 0;
      if (fnGetPath(buses[j], path, sizeof(path), &retrieved) == FMOD_OK && retrieved > 0) {
        if (seen.insert(path).second) {
          SoundBusEntry bus;
          bus.busPath = path;
          if (fnGetParent) {
            FMOD::Studio::Bus* parent = nullptr;
            if (fnGetParent(buses[j], &parent) == FMOD_OK && parent) {
              char parentPath[256] = {};
              int parentRetrieved = 0;
              if (fnGetPath(parent, parentPath, sizeof(parentPath), &parentRetrieved) == FMOD_OK && parentRetrieved > 0) bus.parentPath = parentPath;
            }
          }
          if (fnGetVolume) {
            float vol = 0.0f, finalVol = 0.0f;
            if (fnGetVolume(buses[j], &vol, &finalVol) == FMOD_OK) bus.volume = vol;
          }
          if (fnGetMute) {
            bool muted = false;
            if (fnGetMute(buses[j], &muted) == FMOD_OK) bus.isMuted = muted;
          }
          if (fnGetPaused) {
            bool paused = false;
            if (fnGetPaused(buses[j], &paused) == FMOD_OK) bus.isPaused = paused;
          }
          if (fnGetFaderLevel) {
            float fader = 0.0f;
            if (fnGetFaderLevel(buses[j], &fader) == FMOD_OK) bus.faderLevel = fader;
          }
          if (fnIsBypassed) {
            bool bypassed = false;
            if (fnIsBypassed(buses[j], &bypassed) == FMOD_OK) bus.isBypassed = bypassed;
          }
          result.push_back(std::move(bus));
        }
      }
    }
  }

  return result;
}

std::vector<SoundGlobalParameter> SoundService::GetGlobalParameters() {
  std::vector<SoundGlobalParameter> result;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  if (!ResolveFmodFunctions()) return result;

  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys) return result;

  auto fnGetCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int*)>(m_fmodFn.System_GetParameterDescriptionCount);
  auto fnGetList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, FMOD_STUDIO_PARAMETER_DESCRIPTION*, int, int*)>(m_fmodFn.System_GetParameterDescriptionList);
  if (!fnGetCount || !fnGetList) {
    logger->Warn("GetGlobalParameters: FMOD API not available.");
    return result;
  }

  int count = 0;
  if (fnGetCount(studioSys, &count) != FMOD_OK || count <= 0) {
    return result;
  }

  std::vector<FMOD_STUDIO_PARAMETER_DESCRIPTION> descs(count);
  int returned = 0;
  if (fnGetList(studioSys, descs.data(), count, &returned) != FMOD_OK) {
    logger->Warn("GetGlobalParameters: failed to get parameter list.");
    return result;
  }

  for (int i = 0; i < returned; ++i) {
    SoundGlobalParameter gp;
    gp.paramPath = descs[i].name ? descs[i].name : "";
    gp.minimum = descs[i].minimum;
    gp.maximum = descs[i].maximum;
    gp.defaultvalue = descs[i].defaultvalue;
    gp.type = static_cast<int>(descs[i].type);
    gp.isEditable = (descs[i].type == FMOD_STUDIO_PARAMETER_TYPE::GAME_CONTROLLED && gp.minimum < gp.maximum);
    result.push_back(std::move(gp));
  }

  logger->Info("GetGlobalParameters: {} global parameters found via FMOD API.", result.size());
  return result;
}
// --- Bus controls ---

bool SoundService::GetBusInfo(const std::string& busPath, SoundBusInfo& outInfo) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetBus) return false;

  auto fnGetBus = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, FMOD::Studio::Bus**)>(m_fmodFn.System_GetBus);
  auto fnBusGetVolume = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, float*, float*)>(m_fmodFn.Bus_GetVolume);
  auto fnBusGetMute = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool*)>(m_fmodFn.Bus_GetMute);
  auto fnBusGetPaused = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool*)>(m_fmodFn.Bus_GetPaused);
  if (!fnGetBus || !fnBusGetVolume || !fnBusGetMute) return false;

  FMOD::Studio::Bus* bus = nullptr;
  if (fnGetBus(studioSys, busPath.c_str(), &bus) != FMOD_OK || !bus) return false;

  outInfo.busPath = busPath;
  float vol = 1.0f;
  float finalVol = 1.0f;
  bool muted = false;
  fnBusGetVolume(bus, &vol, &finalVol);
  fnBusGetMute(bus, &muted);
  outInfo.volume = vol;
  outInfo.isMuted = muted;
  if (fnBusGetPaused) fnBusGetPaused(bus, &outInfo.isPaused);
  return true;
}

bool SoundService::SetBusVolume(const std::string& busPath, float volume) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetBus) return false;

  auto fnGetBus = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, FMOD::Studio::Bus**)>(m_fmodFn.System_GetBus);
  auto fnBusSetVolume = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, float)>(m_fmodFn.Bus_SetVolume);
  if (!fnGetBus || !fnBusSetVolume) return false;

  FMOD::Studio::Bus* bus = nullptr;
  if (fnGetBus(studioSys, busPath.c_str(), &bus) != FMOD_OK || !bus) return false;
  return fnBusSetVolume(bus, volume) == FMOD_OK;
}

bool SoundService::SetBusMute(const std::string& busPath, bool muted) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetBus) return false;

  auto fnGetBus = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, FMOD::Studio::Bus**)>(m_fmodFn.System_GetBus);
  auto fnBusSetMute = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool)>(m_fmodFn.Bus_SetMute);
  if (!fnGetBus || !fnBusSetMute) return false;

  FMOD::Studio::Bus* bus = nullptr;
  if (fnGetBus(studioSys, busPath.c_str(), &bus) != FMOD_OK || !bus) return false;
  return fnBusSetMute(bus, muted) == FMOD_OK;
}

bool SoundService::SetBusPause(const std::string& busPath, bool paused) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetBus) return false;

  auto fnGetBus = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, FMOD::Studio::Bus**)>(m_fmodFn.System_GetBus);
  auto fnBusSetPaused = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool)>(m_fmodFn.Bus_SetPaused);
  if (!fnGetBus || !fnBusSetPaused) return false;

  FMOD::Studio::Bus* bus = nullptr;
  if (fnGetBus(studioSys, busPath.c_str(), &bus) != FMOD_OK || !bus) return false;
  return fnBusSetPaused(bus, paused) == FMOD_OK;
}

bool SoundService::GetBusPause(const std::string& busPath, bool& outPaused) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetBus) return false;

  auto fnGetBus = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, FMOD::Studio::Bus**)>(m_fmodFn.System_GetBus);
  auto fnBusGetPaused = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bus*, bool*)>(m_fmodFn.Bus_GetPaused);
  if (!fnGetBus || !fnBusGetPaused) return false;

  FMOD::Studio::Bus* bus = nullptr;
  if (fnGetBus(studioSys, busPath.c_str(), &bus) != FMOD_OK || !bus) return false;
  return fnBusGetPaused(bus, &outPaused) == FMOD_OK;
}

// --- Global parameter controls ---

bool SoundService::GetGlobalParamValue(const std::string& paramName, float& outValue) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetParameterByName) return false;

  auto fnGet = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, float*, float*)>(m_fmodFn.System_GetParameterByName);
  float finalVal = 0.0f;
  return fnGet(studioSys, paramName.c_str(), &outValue, &finalVal) == FMOD_OK;
}

bool SoundService::SetGlobalParamValue(const std::string& paramName, float value) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_SetParameterByName) return false;

  auto fnSet = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, float, bool)>(m_fmodFn.System_SetParameterByName);
  return fnSet(studioSys, paramName.c_str(), value, false) == FMOD_OK;
}

// --- Event playback ---

void* SoundService::CreateEventInstance(const uint8_t guid[16]) {
  if (!ResolveFmodFunctions()) return nullptr;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetEventByID || !m_fmodFn.EventDescription_CreateInstance) return nullptr;

  auto fnGetEventByID = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const FMOD_GUID*, FMOD::Studio::EventDescription**)>(m_fmodFn.System_GetEventByID);
  auto fnCreateInstance = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD::Studio::EventInstance**)>(m_fmodFn.EventDescription_CreateInstance);

  FMOD_GUID fmodGuid;
  std::memcpy(&fmodGuid, guid, sizeof(fmodGuid));

  FMOD::Studio::EventDescription* desc = nullptr;
  if (fnGetEventByID(studioSys, &fmodGuid, &desc) != FMOD_OK || !desc) return nullptr;

  FMOD::Studio::EventInstance* instance = nullptr;
  if (fnCreateInstance(desc, &instance) != FMOD_OK) return nullptr;
  return instance;
}

bool SoundService::StartEvent(void* instance) {
  if (!instance || !m_fmodFn.EventInstance_Start) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*)>(m_fmodFn.EventInstance_Start);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance)) == FMOD_OK;
}

bool SoundService::StopEvent(void* instance, bool allowFadeout) {
  if (!instance || !m_fmodFn.EventInstance_Stop) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD_STUDIO_STOP_MODE)>(m_fmodFn.EventInstance_Stop);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), allowFadeout ? FMOD_STUDIO_STOP_MODE::ALLOWFADEOUT : FMOD_STUDIO_STOP_MODE::IMMEDIATE) == FMOD_OK;
}

bool SoundService::PauseEvent(void* instance, bool paused) {
  if (!instance || !m_fmodFn.EventInstance_SetPaused) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, bool)>(m_fmodFn.EventInstance_SetPaused);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), paused) == FMOD_OK;
}

int SoundService::GetEventPlaybackState(void* instance) {
  if (!instance || !m_fmodFn.EventInstance_GetPlaybackState) return -1;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, int*)>(m_fmodFn.EventInstance_GetPlaybackState);
  int state = -1;
  fn(static_cast<FMOD::Studio::EventInstance*>(instance), &state);
  return state;
}

void SoundService::ReleaseEventInstance(void* instance) {
  if (!instance || !m_fmodFn.EventInstance_Release) return;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*)>(m_fmodFn.EventInstance_Release);
  fn(static_cast<FMOD::Studio::EventInstance*>(instance));
}

// --- EventInstance control ---

bool SoundService::SetEventVolume(void* instance, float volume) {
  if (!instance || !m_fmodFn.EventInstance_SetVolume) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, float)>(m_fmodFn.EventInstance_SetVolume);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), volume) == FMOD_OK;
}

bool SoundService::GetEventVolume(void* instance, float& outVolume, float& outFinalVolume) {
  if (!instance || !m_fmodFn.EventInstance_GetVolume) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, float*, float*)>(m_fmodFn.EventInstance_GetVolume);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), &outVolume, &outFinalVolume) == FMOD_OK;
}

bool SoundService::SetEventPitch(void* instance, float pitch) {
  if (!instance || !m_fmodFn.EventInstance_SetPitch) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, float)>(m_fmodFn.EventInstance_SetPitch);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), pitch) == FMOD_OK;
}

bool SoundService::GetEventPitch(void* instance, float& outPitch, float& outFinalPitch) {
  if (!instance || !m_fmodFn.EventInstance_GetPitch) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, float*, float*)>(m_fmodFn.EventInstance_GetPitch);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), &outPitch, &outFinalPitch) == FMOD_OK;
}

bool SoundService::SetEvent3DAttributes(void* instance, float posX, float posY, float posZ, float velX, float velY, float velZ, float fwdX, float fwdY, float fwdZ, float upX, float upY, float upZ) {
  if (!instance || !m_fmodFn.EventInstance_Set3DAttributes) return false;
  FMOD_3D_ATTRIBUTES attrs = {};
  attrs.position = {posX, posY, posZ};
  attrs.velocity = {velX, velY, velZ};
  attrs.forward = {fwdX, fwdY, fwdZ};
  attrs.up = {upX, upY, upZ};
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD_3D_ATTRIBUTES*)>(m_fmodFn.EventInstance_Set3DAttributes);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), &attrs) == FMOD_OK;
}

bool SoundService::GetEvent3DAttributes(void* instance, float& posX, float& posY, float& posZ, float& velX, float& velY, float& velZ, float& fwdX, float& fwdY, float& fwdZ, float& upX, float& upY, float& upZ) {
  if (!instance || !m_fmodFn.EventInstance_Get3DAttributes) return false;
  FMOD_3D_ATTRIBUTES attrs = {};
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD_3D_ATTRIBUTES*)>(m_fmodFn.EventInstance_Get3DAttributes);
  FMOD_RESULT res = fn(static_cast<FMOD::Studio::EventInstance*>(instance), &attrs);
  if (res != FMOD_OK) return false;
  posX = attrs.position.x;
  posY = attrs.position.y;
  posZ = attrs.position.z;
  velX = attrs.velocity.x;
  velY = attrs.velocity.y;
  velZ = attrs.velocity.z;
  fwdX = attrs.forward.x;
  fwdY = attrs.forward.y;
  fwdZ = attrs.forward.z;
  upX = attrs.up.x;
  upY = attrs.up.y;
  upZ = attrs.up.z;
  return true;
}

bool SoundService::SetEventParameterByName(void* instance, const char* name, float value, bool ignoreSeekSpeed) {
  if (!instance || !m_fmodFn.EventInstance_SetParameterByName) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, const char*, float, bool)>(m_fmodFn.EventInstance_SetParameterByName);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), name, value, ignoreSeekSpeed) == FMOD_OK;
}

bool SoundService::GetEventParameterByName(void* instance, const char* name, float& outValue, float& outFinalValue) {
  if (!instance || !m_fmodFn.EventInstance_GetParameterByName) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, const char*, float*, float*)>(m_fmodFn.EventInstance_GetParameterByName);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), name, &outValue, &outFinalValue) == FMOD_OK;
}

bool SoundService::SetEventParameterByID(void* instance, uint32_t idData1, uint32_t idData2, float value, bool ignoreSeekSpeed) {
  if (!instance || !m_fmodFn.EventInstance_SetParameterByID) return false;
  FMOD_STUDIO_PARAMETER_ID pid = {idData1, idData2};
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD_STUDIO_PARAMETER_ID, float, bool)>(m_fmodFn.EventInstance_SetParameterByID);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), pid, value, ignoreSeekSpeed) == FMOD_OK;
}

bool SoundService::GetEventParameterByID(void* instance, uint32_t idData1, uint32_t idData2, float& outValue, float& outFinalValue) {
  if (!instance || !m_fmodFn.EventInstance_GetParameterByID) return false;
  FMOD_STUDIO_PARAMETER_ID pid = {idData1, idData2};
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD_STUDIO_PARAMETER_ID, float*, float*)>(m_fmodFn.EventInstance_GetParameterByID);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), pid, &outValue, &outFinalValue) == FMOD_OK;
}

bool SoundService::SetEventTimelinePosition(void* instance, int position) {
  if (!instance || !m_fmodFn.EventInstance_SetTimelinePosition) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, int)>(m_fmodFn.EventInstance_SetTimelinePosition);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), position) == FMOD_OK;
}

bool SoundService::GetEventTimelinePosition(void* instance, int& outPosition) {
  if (!instance || !m_fmodFn.EventInstance_GetTimelinePosition) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, int*)>(m_fmodFn.EventInstance_GetTimelinePosition);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), &outPosition) == FMOD_OK;
}

bool SoundService::GetEventDescriptionFromInstance(void* instance, void** outDesc) {
  if (!instance || !m_fmodFn.EventInstance_GetDescription || !outDesc) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD::Studio::EventDescription**)>(m_fmodFn.EventInstance_GetDescription);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), reinterpret_cast<FMOD::Studio::EventDescription**>(outDesc)) == FMOD_OK;
}

bool SoundService::SetEventCallback(void* instance, EventCallbackFn callback, uint32_t callbackMask) {
  if (!instance || !m_fmodFn.EventInstance_SetCallback) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, void*, uint32_t)>(m_fmodFn.EventInstance_SetCallback);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), reinterpret_cast<void*>(callback), callbackMask) == FMOD_OK;
}

bool SoundService::SetEventLoopCount(void* instance, int loopCount) {
  if (!instance || !m_fmodFn.EventInstance_SetLoopCount) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, int)>(m_fmodFn.EventInstance_SetLoopCount);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), loopCount) == FMOD_OK;
}

bool SoundService::GetEventLoopCount(void* instance, int& outCount) {
  if (!instance || !m_fmodFn.EventInstance_GetLoopCount) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventInstance*, int*)>(m_fmodFn.EventInstance_GetLoopCount);
  return fn(static_cast<FMOD::Studio::EventInstance*>(instance), &outCount) == FMOD_OK;
}

bool SoundService::SetEventLoop(void* instance, bool loop) {
  if (!instance) return false;
  if (loop) {
    if (!SetEventLoopCount(instance, -1)) return false;
  } else {
    if (!SetEventLoopCount(instance, 0)) return false;
  }
  return true;
}

// --- Listener ---

int SoundService::GetNumListeners() {
  if (!ResolveFmodFunctions()) return 1;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetNumListeners) return 1;

  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int*)>(m_fmodFn.System_GetNumListeners);
  int num = 1;
  fn(studioSys, &num);
  return num;
}

bool SoundService::SetNumListeners(int numListeners) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_SetNumListeners) return false;

  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int)>(m_fmodFn.System_SetNumListeners);
  return fn(studioSys, numListeners) == FMOD_OK;
}

bool SoundService::GetListenerAttributes(int index, float& posX, float& posY, float& posZ, float& velX, float& velY, float& velZ, float& fwdX, float& fwdY, float& fwdZ, float& upX, float& upY, float& upZ) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetListenerAttributes) return false;

  FMOD_3D_ATTRIBUTES attrs = {};
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int, FMOD_3D_ATTRIBUTES*, FMOD_VECTOR*)>(m_fmodFn.System_GetListenerAttributes);
  FMOD_RESULT res = fn(studioSys, index, &attrs, nullptr);
  if (res != FMOD_OK) return false;
  posX = attrs.position.x;
  posY = attrs.position.y;
  posZ = attrs.position.z;
  velX = attrs.velocity.x;
  velY = attrs.velocity.y;
  velZ = attrs.velocity.z;
  fwdX = attrs.forward.x;
  fwdY = attrs.forward.y;
  fwdZ = attrs.forward.z;
  upX = attrs.up.x;
  upY = attrs.up.y;
  upZ = attrs.up.z;
  return true;
}

bool SoundService::SetListenerAttributes(int index, float posX, float posY, float posZ, float velX, float velY, float velZ, float fwdX, float fwdY, float fwdZ, float upX, float upY, float upZ) {
  if (!ResolveFmodFunctions()) return false;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_SetListenerAttributes) return false;

  FMOD_3D_ATTRIBUTES attrs = {};
  attrs.position = {posX, posY, posZ};
  attrs.velocity = {velX, velY, velZ};
  attrs.forward = {fwdX, fwdY, fwdZ};
  attrs.up = {upX, upY, upZ};
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int, const FMOD_3D_ATTRIBUTES*, const FMOD_VECTOR*)>(m_fmodFn.System_SetListenerAttributes);
  return fn(studioSys, index, &attrs, nullptr) == FMOD_OK;
}

// --- Bank Management ---

void* SoundService::LoadBankFile(const char* bankPath, const char* guidsPath) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  if (!ResolveFmodFunctions() || !bankPath) {
    logger->Warn("LoadBankFile: failed — ResolveFmodFunctions={} path={}", ResolveFmodFunctions(), bankPath ? bankPath : "null");
    return nullptr;
  }
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_LoadBankFile) {
    logger->Warn("LoadBankFile: failed — studioSys={} hasLoadBankFn={}", static_cast<void*>(studioSys), m_fmodFn.System_LoadBankFile != nullptr);
    return nullptr;
  }

  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, uint32_t, FMOD::Studio::Bank**)>(m_fmodFn.System_LoadBankFile);
  FMOD::Studio::Bank* bank = nullptr;
  auto rc = fn(studioSys, bankPath, 2, &bank);
  if (rc != FMOD_OK) {
    logger->Warn("LoadBankFile: System_LoadBankFile failed rc={} path={}", static_cast<int>(rc), bankPath);
    return nullptr;
  }
  m_pluginBanks.push_back(bank);
  m_eventCache.clear();

  if (guidsPath && guidsPath[0]) {
    LoadGuidsFile(guidsPath);
  } else {
    std::string autoPath = std::string(bankPath) + ".guids";
    LoadGuidsFile(autoPath.c_str());
  }

  if (m_fmodFn.System_Update) {
    auto fnUpdate = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*)>(m_fmodFn.System_Update);
    tls_insidePluginUpdate = true;
    auto updateRc = fnUpdate(studioSys);
    tls_insidePluginUpdate = false;
  }
  return bank;
}

void* SoundService::LoadBankMemory(const void* data, uint32_t size, const char* guidsPath) {
  if (!ResolveFmodFunctions() || !data) return nullptr;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_LoadBankMemory) return nullptr;

  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, int, void*, uint32_t, uint32_t, FMOD::Studio::Bank**)>(m_fmodFn.System_LoadBankMemory);
  FMOD::Studio::Bank* bank = nullptr;
  if (fn(studioSys, reinterpret_cast<const char*>(data), 0, const_cast<void*>(data), 0, size, &bank) != FMOD_OK) return nullptr;
  m_pluginBanks.push_back(bank);
  m_eventCache.clear();

  if (guidsPath && guidsPath[0]) {
    LoadGuidsFile(guidsPath);
  }

  if (m_fmodFn.System_Update) {
    auto fnUpdate = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*)>(m_fmodFn.System_Update);
    tls_insidePluginUpdate = true;
    fnUpdate(studioSys);
    tls_insidePluginUpdate = false;
  }
  return bank;
}

bool SoundService::UnloadBank(void* bank) {
  if (!bank || !m_fmodFn.Bank_Unload) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*)>(m_fmodFn.Bank_Unload);
  auto result = fn(static_cast<FMOD::Studio::Bank*>(bank));
  if (result == FMOD_OK) {
    m_pluginBanks.erase(std::remove(m_pluginBanks.begin(), m_pluginBanks.end(), bank), m_pluginBanks.end());
    m_eventCache.clear();
  }
  return result == FMOD_OK;
}

int SoundService::GetBankLoadingState(void* bank) {
  if (!bank || !m_fmodFn.Bank_GetLoadingState) return -1;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetLoadingState);
  int state = -1;
  fn(static_cast<FMOD::Studio::Bank*>(bank), &state);
  return state;
}

int SoundService::GetBankSampleLoadingState(void* bank) {
  if (!bank || !m_fmodFn.Bank_GetSampleLoadingState) return -1;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetSampleLoadingState);
  int state = -1;
  fn(static_cast<FMOD::Studio::Bank*>(bank), &state);
  return state;
}

bool SoundService::LoadBankSampleData(void* bank) {
  if (!bank || !m_fmodFn.Bank_LoadSampleData) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*)>(m_fmodFn.Bank_LoadSampleData);
  return fn(static_cast<FMOD::Studio::Bank*>(bank)) == FMOD_OK;
}

bool SoundService::UnloadBankSampleData(void* bank) {
  if (!bank || !m_fmodFn.Bank_UnloadSampleData) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*)>(m_fmodFn.Bank_UnloadSampleData);
  return fn(static_cast<FMOD::Studio::Bank*>(bank)) == FMOD_OK;
}

int SoundService::GetBankEventCount(void* bank) {
  if (!bank || !m_fmodFn.Bank_GetEventCount) return 0;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetEventCount);
  int count = 0;
  fn(static_cast<FMOD::Studio::Bank*>(bank), &count);
  return count;
}

int SoundService::GetBankEventList(void* bank, void** outEvents, int maxCount) {
  if (!bank || !m_fmodFn.Bank_GetEventList || !outEvents) return 0;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, FMOD::Studio::EventDescription**, int, int*)>(m_fmodFn.Bank_GetEventList);
  int count = 0;
  fn(static_cast<FMOD::Studio::Bank*>(bank), reinterpret_cast<FMOD::Studio::EventDescription**>(outEvents), maxCount, &count);
  return count;
}

int SoundService::GetBankEventGuid(void* bank, int index, uint8_t outGuid[16]) {
  if (!bank || !outGuid || !m_fmodFn.Bank_GetEventList || !m_fmodFn.EventDescription_GetID) return 0;
  auto getCountFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetEventCount);
  int count = 0;
  getCountFn(static_cast<FMOD::Studio::Bank*>(bank), &count);
  if (index < 0 || index >= count) return 0;
  auto getListFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, FMOD::Studio::EventDescription**, int, int*)>(m_fmodFn.Bank_GetEventList);
  std::vector<FMOD::Studio::EventDescription*> descs(count);
  int got = 0;
  getListFn(static_cast<FMOD::Studio::Bank*>(bank), descs.data(), count, &got);
  if (index >= got || !descs[index]) return 0;
  auto idFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD_GUID*)>(m_fmodFn.EventDescription_GetID);
  FMOD_GUID guid{};
  if (idFn(descs[index], &guid) != FMOD_OK) return 0;
  std::memcpy(outGuid, &guid, 16);
  return 1;
}

int SoundService::GetBankEventPath(void* bank, int index, char* outBuffer, int bufferSize) {
  if (!bank || !outBuffer || bufferSize <= 0) return 0;
  if (!m_fmodFn.Bank_GetEventList || !m_fmodFn.EventDescription_GetPath) return 0;
  auto getCountFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetEventCount);
  int count = 0;
  getCountFn(static_cast<FMOD::Studio::Bank*>(bank), &count);
  if (index < 0 || index >= count) return 0;
  auto getListFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, FMOD::Studio::EventDescription**, int, int*)>(m_fmodFn.Bank_GetEventList);
  std::vector<FMOD::Studio::EventDescription*> descs(count);
  int got = 0;
  getListFn(static_cast<FMOD::Studio::Bank*>(bank), descs.data(), count, &got);
  if (index >= got || !descs[index]) return 0;
  auto pathFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, char*, int, int*)>(m_fmodFn.EventDescription_GetPath);
  char dllBuf[256]{};
  int dllLen = sizeof(dllBuf);
  auto rc = pathFn(descs[index], dllBuf, dllLen, nullptr);
  if (rc != FMOD_OK || dllBuf[0] == '\0') {
    std::array<uint8_t, 16> guid{};
    if (GetBankEventGuid(bank, index, guid.data())) {
      auto it = m_guidToPath.find(guid);
      if (it != m_guidToPath.end()) {
        auto len = std::min(static_cast<int>(it->second.size()), bufferSize - 1);
        std::memcpy(outBuffer, it->second.data(), len);
        outBuffer[len] = '\0';
        return len;
      }
    }
    return 0;
  }
  auto len = static_cast<int>(std::strlen(dllBuf));
  auto copyLen = std::min(len, bufferSize - 1);
  std::memcpy(outBuffer, dllBuf, copyLen);
  outBuffer[copyLen] = '\0';
  return copyLen;
}

void SoundService::LoadGuidsFile(const char* guidsPath) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  if (!guidsPath || !guidsPath[0]) return;

  std::ifstream file(guidsPath);
  if (!file.is_open()) {
    logger->Warn("LoadGuidsFile: cannot open {}", guidsPath);
    return;
  }

  int loaded = 0;
  std::string line;
  while (std::getline(file, line)) {
    if (line.size() < 3 || line[0] != '{') continue;
    auto closingBrace = line.find('}');
    if (closingBrace == std::string::npos) continue;
    std::string uuidStr = line.substr(1, closingBrace - 1);
    if (uuidStr.size() != 36) continue;

    auto parseHex = [](const std::string& s, int pos, int len) -> uint16_t { return static_cast<uint16_t>(std::stoul(s.substr(pos, len), nullptr, 16)); };

    std::array<uint8_t, 16> guid{};
    uint32_t d1 = std::stoul(uuidStr.substr(0, 8), nullptr, 16);
    uint16_t d2 = parseHex(uuidStr, 9, 4);
    uint16_t d3 = parseHex(uuidStr, 14, 4);
    guid[0] = static_cast<uint8_t>(d1 & 0xFF);
    guid[1] = static_cast<uint8_t>((d1 >> 8) & 0xFF);
    guid[2] = static_cast<uint8_t>((d1 >> 16) & 0xFF);
    guid[3] = static_cast<uint8_t>((d1 >> 24) & 0xFF);
    guid[4] = static_cast<uint8_t>(d2 & 0xFF);
    guid[5] = static_cast<uint8_t>((d2 >> 8) & 0xFF);
    guid[6] = static_cast<uint8_t>(d3 & 0xFF);
    guid[7] = static_cast<uint8_t>((d3 >> 8) & 0xFF);
    static const int d4Pos[] = {19, 21, 24, 26, 28, 30, 32, 34};
    for (int i = 0; i < 8; ++i) {
      guid[8 + i] = static_cast<uint8_t>(std::stoul(uuidStr.substr(d4Pos[i], 2), nullptr, 16));
    }

    auto pathStart = line.find(' ', closingBrace + 1);
    if (pathStart == std::string::npos) continue;
    ++pathStart;
    {
      std::lock_guard<std::mutex> lock(m_guidToPathMutex);
      m_guidToPath[guid] = line.substr(pathStart);
    }
    ++loaded;
  }
}

std::vector<SoundBankLoadInfo> SoundService::GetLoadedBanksInfo() {
  std::vector<SoundBankLoadInfo> result;
  if (!m_isInitialized || !m_fmodFunctionsResolved) return result;

  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys) return result;

  auto fnGetBankCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int*)>(m_fmodFn.System_GetBankCount);
  auto fnGetBankList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, FMOD::Studio::Bank**, int, int*)>(m_fmodFn.System_GetBankList);
  auto fnBankGetLoadingState = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetLoadingState);
  auto fnBankGetSampleLoadingState = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetSampleLoadingState);
  auto fnBankGetEventCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetEventCount);
  auto fnBankGetBusCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetBusCount);
  auto fnBankGetVCACount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetVCACount);
  auto fnBankGetPath = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, char*, int, int*)>(m_fmodFn.Bank_GetPath);
  if (!fnGetBankCount || !fnGetBankList) return result;

  int bankCount = 0;
  if (fnGetBankCount(studioSys, &bankCount) != FMOD_OK || bankCount <= 0) return result;

  std::vector<FMOD::Studio::Bank*> banks(bankCount);
  int returned = 0;
  if (fnGetBankList(studioSys, banks.data(), bankCount, &returned) != FMOD_OK) return result;

  for (int i = 0; i < returned; ++i) {
    if (!banks[i]) continue;
    SoundBankLoadInfo info;

    if (fnBankGetPath) {
      char path[256] = {};
      int retrieved = 0;
      if (fnBankGetPath(banks[i], path, sizeof(path), &retrieved) == FMOD_OK) info.bankPath = path;
    }
    if (fnBankGetLoadingState) {
      int state = 0;
      if (fnBankGetLoadingState(banks[i], &state) == FMOD_OK) info.loadingState = state;
    }
    if (fnBankGetSampleLoadingState) {
      int state = 0;
      if (fnBankGetSampleLoadingState(banks[i], &state) == FMOD_OK) info.sampleLoadingState = state;
    }
    if (fnBankGetEventCount) {
      int count = 0;
      if (fnBankGetEventCount(banks[i], &count) == FMOD_OK) info.eventCount = count;
    }
    if (fnBankGetBusCount) {
      int count = 0;
      if (fnBankGetBusCount(banks[i], &count) == FMOD_OK) info.busCount = count;
    }
    if (fnBankGetVCACount) {
      int count = 0;
      if (fnBankGetVCACount(banks[i], &count) == FMOD_OK) info.vcaCount = count;
    }
    result.push_back(std::move(info));
  }

  return result;
}

// --- VCA ---

void* SoundService::GetVCAByPath(const char* path) {
  if (!ResolveFmodFunctions() || !path) return nullptr;
  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys || !m_fmodFn.System_GetVCA) return nullptr;

  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const char*, FMOD::Studio::VCA**)>(m_fmodFn.System_GetVCA);
  FMOD::Studio::VCA* vca = nullptr;
  if (fn(studioSys, path, &vca) != FMOD_OK) return nullptr;
  return vca;
}

bool SoundService::SetVCAVolume(void* vca, float volume) {
  if (!vca || !m_fmodFn.VCA_SetVolume) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::VCA*, float)>(m_fmodFn.VCA_SetVolume);
  return fn(static_cast<FMOD::Studio::VCA*>(vca), volume) == FMOD_OK;
}

bool SoundService::GetVCAVolume(void* vca, float& outVolume, float& outFinalVolume) {
  if (!vca || !m_fmodFn.VCA_GetVolume) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::VCA*, float*, float*)>(m_fmodFn.VCA_GetVolume);
  return fn(static_cast<FMOD::Studio::VCA*>(vca), &outVolume, &outFinalVolume) == FMOD_OK;
}

int SoundService::GetVCAPath(void* vca, char* outBuffer, int bufferSize) {
  if (!vca || !m_fmodFn.VCA_GetPath || !outBuffer) return 0;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::VCA*, char*, int, int*)>(m_fmodFn.VCA_GetPath);
  int retrieved = 0;
  fn(static_cast<FMOD::Studio::VCA*>(vca), outBuffer, bufferSize, &retrieved);
  return retrieved;
}

std::vector<SoundVCAEntry> SoundService::GetVCAs() {
  std::vector<SoundVCAEntry> result;
  if (!ResolveFmodFunctions()) return result;

  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  if (!studioSys) return result;

  auto fnGetVCACount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, int*)>(m_fmodFn.System_GetVCACount);
  auto fnGetVCAList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, FMOD::Studio::VCA**, int, int*)>(m_fmodFn.System_GetVCAList);
  auto fnGetPath = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::VCA*, char*, int, int*)>(m_fmodFn.VCA_GetPath);
  if (!fnGetVCACount || !fnGetVCAList || !fnGetPath) return result;

  int count = 0;
  if (fnGetVCACount(studioSys, &count) != FMOD_OK || count <= 0) return result;

  std::vector<FMOD::Studio::VCA*> vcas(count);
  int returned = 0;
  if (fnGetVCAList(studioSys, vcas.data(), count, &returned) != FMOD_OK) return result;

  for (int i = 0; i < returned; ++i) {
    if (!vcas[i]) continue;
    char path[256] = {};
    int retrieved = 0;
    if (fnGetPath(vcas[i], path, sizeof(path), &retrieved) == FMOD_OK && retrieved > 0) {
      SoundVCAEntry vca;
      vca.vcaPath = path;
      result.push_back(std::move(vca));
    }
  }

  return result;
}

// --- EventDescription queries ---

bool SoundService::IsEventSnapshot(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_IsSnapshot) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsSnapshot);
  bool val = false;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &val);
  return val;
}

bool SoundService::IsEventDopplerEnabled(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_IsDopplerEnabled) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsDopplerEnabled);
  bool val = false;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &val);
  return val;
}

bool SoundService::GetEventMinMaxDistance(void* desc, float& outMin, float& outMax) {
  if (!desc || !m_fmodFn.EventDescription_GetMinMaxDistance) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, float*, float*)>(m_fmodFn.EventDescription_GetMinMaxDistance);
  return fn(static_cast<FMOD::Studio::EventDescription*>(desc), &outMin, &outMax) == FMOD_OK;
}

bool SoundService::GetEventSoundSize(void* desc, uint32_t& outSize) {
  if (!desc || !m_fmodFn.EventDescription_GetSoundSize) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, uint32_t*)>(m_fmodFn.EventDescription_GetSoundSize);
  return fn(static_cast<FMOD::Studio::EventDescription*>(desc), &outSize) == FMOD_OK;
}

bool SoundService::GetEventSampleLoadingState(void* desc, int& outState) {
  if (!desc || !m_fmodFn.EventDescription_GetSampleLoadingState) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetSampleLoadingState);
  return fn(static_cast<FMOD::Studio::EventDescription*>(desc), &outState) == FMOD_OK;
}

bool SoundService::IsEvent3D(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_Is3D) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_Is3D);
  bool val = false;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &val);
  return val;
}

bool SoundService::IsEventOneshot(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_IsOneshot) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsOneshot);
  bool val = false;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &val);
  return val;
}

bool SoundService::IsEventStream(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_IsStream) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_IsStream);
  bool val = false;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &val);
  return val;
}

bool SoundService::EventHasSustainPoint(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_HasSustainPoint) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, bool*)>(m_fmodFn.EventDescription_HasSustainPoint);
  bool val = false;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &val);
  return val;
}

bool SoundService::GetEventLength(void* desc, uint32_t& outLength) {
  if (!desc || !m_fmodFn.EventDescription_GetLength) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, uint32_t*)>(m_fmodFn.EventDescription_GetLength);
  return fn(static_cast<FMOD::Studio::EventDescription*>(desc), &outLength) == FMOD_OK;
}

bool SoundService::GetEventID(void* desc, uint8_t outGuid[16]) {
  if (!desc || !m_fmodFn.EventDescription_GetID) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD_GUID*)>(m_fmodFn.EventDescription_GetID);
  FMOD_GUID g = {};
  if (fn(static_cast<FMOD::Studio::EventDescription*>(desc), &g) != FMOD_OK) return false;
  std::memcpy(outGuid, &g, 16);
  return true;
}

int SoundService::GetEventPathFromDesc(void* desc, char* outBuffer, int bufferSize) {
  if (!desc || !m_fmodFn.EventDescription_GetPath || !outBuffer) return 0;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, char*, int, int*)>(m_fmodFn.EventDescription_GetPath);
  if (fn(static_cast<FMOD::Studio::EventDescription*>(desc), outBuffer, bufferSize, nullptr) != FMOD_OK) return 0;
  return static_cast<int>(std::strlen(outBuffer));
}

int SoundService::GetEventInstanceCount(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_GetInstanceCount) return 0;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetInstanceCount);
  int count = 0;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &count);
  return count;
}

std::vector<void*> SoundService::GetEventInstanceList(void* desc) {
  std::vector<void*> result;
  if (!desc || !m_fmodFn.EventDescription_GetInstanceList) return result;
  int count = GetEventInstanceCount(desc);
  if (count <= 0) return result;
  result.resize(count);
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD::Studio::EventInstance**, int, int*)>(m_fmodFn.EventDescription_GetInstanceList);
  int retrieved = 0;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), reinterpret_cast<FMOD::Studio::EventInstance**>(result.data()), count, &retrieved);
  result.resize(retrieved);
  return result;
}

void* SoundService::GetEventInstance(void* desc, int index) {
  if (!desc || index < 0 || !m_fmodFn.EventDescription_GetInstanceList) return nullptr;
  auto fnCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetInstanceCount);
  auto fnList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD::Studio::EventInstance**, int, int*)>(m_fmodFn.EventDescription_GetInstanceList);
  int count = 0;
  fnCount(static_cast<FMOD::Studio::EventDescription*>(desc), &count);
  if (index >= count) return nullptr;
  std::vector<FMOD::Studio::EventInstance*> instances(count);
  int retrieved = 0;
  fnList(static_cast<FMOD::Studio::EventDescription*>(desc), instances.data(), count, &retrieved);
  if (index >= retrieved) return nullptr;
  return instances[index];
}

int SoundService::GetEventParameterDescriptionCount(void* desc) {
  if (!desc || !m_fmodFn.EventDescription_GetParameterDescriptionCount) return 0;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetParameterDescriptionCount);
  int count = 0;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), &count);
  return count;
}

bool SoundService::GetEventParameterByIndex(void* desc, int index, char* outName, int nameSize, float& outMin, float& outMax, float& outDefault) {
  if (!desc || !m_fmodFn.EventDescription_GetParameterDescriptionByIndex) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int, FMOD_STUDIO_PARAMETER_DESCRIPTION*)>(m_fmodFn.EventDescription_GetParameterDescriptionByIndex);
  FMOD_STUDIO_PARAMETER_DESCRIPTION pd = {};
  FMOD_RESULT res = fn(static_cast<FMOD::Studio::EventDescription*>(desc), index, &pd);
  if (res != FMOD_OK) return false;
  if (outName && nameSize > 0) {
    if (pd.name) {
      strncpy(outName, pd.name, nameSize - 1);
      outName[nameSize - 1] = '\0';
    } else {
      outName[0] = '\0';
    }
  }
  outMin = pd.minimum;
  outMax = pd.maximum;
  outDefault = pd.defaultvalue;
  return true;
}

bool SoundService::GetEventUserPropertyCount(void* desc, int& outCount) {
  if (!desc || !m_fmodFn.EventDescription_GetUserPropertyCount) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*)>(m_fmodFn.EventDescription_GetUserPropertyCount);
  return fn(static_cast<FMOD::Studio::EventDescription*>(desc), &outCount) == FMOD_OK;
}

bool SoundService::GetEventUserPropertyByIndex(void* desc, int index, char* outName, int nameSize, int& outType) {
  if (!desc || !m_fmodFn.EventDescription_GetUserPropertyByIndex || !outName) return false;
  auto fn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int, void*)>(m_fmodFn.EventDescription_GetUserPropertyByIndex);
  struct UserProp {
    char name[256];
    int type;
    union {
      int i;
      float f;
      bool b;
    } value;
  } prop = {};
  FMOD_RESULT res = fn(static_cast<FMOD::Studio::EventDescription*>(desc), index, &prop);
  if (res != FMOD_OK) return false;
  strncpy(outName, prop.name, nameSize - 1);
  outName[nameSize - 1] = '\0';
  outType = prop.type;
  return true;
}

// --- Dump (refactored to use cached pointers) ---

void SoundService::DumpAllEventsToLog() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  auto& fmodApi = Fmod::FmodApi::GetInstance();

  logger->Info("===== FMOD Sound Dump START =====");
  logger->Info("FmodApi ready={}", fmodApi.IsReady());
  logger->Info("Available FMOD functions: {}", fmodApi.GetExportCount());

  if (!m_isInitialized) {
    logger->Info("SoundService not initialized. Dump aborted.");
    return;
  }

  if (!ResolveFmodFunctions()) {
    logger->Info("FMOD functions not resolved. Dump aborted.");
    return;
  }

  uintptr_t soundSystem = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundSystem) {
    logger->Info("SoundManager address is null. Dump aborted.");
    return;
  }

  auto* studioSys = static_cast<FMOD::Studio::System*>(GetStudioSystemRaw());
  logger->Info("Studio::System* = 0x{:X}", reinterpret_cast<uintptr_t>(studioSys));

  auto* coreSys = *reinterpret_cast<FMOD::System**>(soundSystem + 0x1d0);
  logger->Info("Core::System*   = 0x{:X}", reinterpret_cast<uintptr_t>(coreSys));

  if (!studioSys || !m_fmodFn.System_GetEventByID) {
    logger->Info("Studio::System* is null or getEventByID not found. Dump aborted.");
    return;
  }

  auto fnGetEventByID = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::System*, const FMOD_GUID*, FMOD::Studio::EventDescription**)>(m_fmodFn.System_GetEventByID);

  uintptr_t lockAddr = soundSystem + m_bankListLockOffset;
  AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  uintptr_t bankListHead = *reinterpret_cast<uintptr_t*>(soundSystem + m_bankListHeadOffset);
  uintptr_t bankSentinel = soundSystem + m_bankListSentinelOffset;

  int bankIndex = 0;
  int eventIndex = 0;
  int resolvedCount = 0;
  int unresolvedCount = 0;
  int skippedCount = 0;

  uintptr_t bankNode = bankListHead;
  while (bankNode != bankSentinel) {
    const char* bankPath = *reinterpret_cast<const char**>(bankNode + m_bankPathStringOffset);
    logger->Info("");
    logger->Info("--- Bank #{}: {} ---", bankIndex, bankPath ? bankPath : "<null>");

    uintptr_t eventNode = *reinterpret_cast<uintptr_t*>(bankNode + m_bankEventListHeadOffset);
    uintptr_t eventSentinel = bankNode + m_bankEventListHeadOffset + m_eventListTerminatorOffset;

    int localEventIdx = 0;
    while (eventNode != eventSentinel) {
      const char* eventPath = *reinterpret_cast<const char**>(eventNode + m_eventPathOffset);
      const uint8_t* rawGuid = reinterpret_cast<const uint8_t*>(eventNode + m_eventGuidOffset);
      char guidHex[33];
      for (int i = 0; i < 16; ++i) snprintf(guidHex + i * 2, 3, "%02x", rawGuid[i]);
      guidHex[32] = '\0';

      logger->Info("  [{}] eventPath={} guid={} raw=0x{:X}", localEventIdx, eventPath ? eventPath : "<null>", guidHex, eventNode);

      eventIndex++;
      localEventIdx++;

      bool isEvent = eventPath && strncmp(eventPath, "event:/", 7) == 0;
      if (!isEvent) {
        skippedCount++;
        eventNode = *reinterpret_cast<uintptr_t*>(eventNode);
        continue;
      }

      FMOD_GUID guid;
      std::memcpy(&guid, rawGuid, 16);
      FMOD::Studio::EventDescription* desc = nullptr;
      FMOD_RESULT res = fnGetEventByID(studioSys, &guid, &desc);

      if (res != FMOD_OK || !desc) {
        logger->Info("    getEventByID FAILED (res={})", static_cast<int>(res));
        unresolvedCount++;
      } else {
        resolvedCount++;
        logger->Info("    EventDescription* = 0x{:X}", reinterpret_cast<uintptr_t>(desc));

        logger->Info("    is3D={}", IsEvent3D(desc));
        uint32_t eventLength = 0;
        if (GetEventLength(desc, eventLength)) logger->Info("    length={}ms", eventLength);
        logger->Info("    isSnapshot={}", IsEventSnapshot(desc));
        logger->Info("    isOneshot={}", IsEventOneshot(desc));
        logger->Info("    isStream={}", IsEventStream(desc));
        logger->Info("    isDopplerEnabled={}", IsEventDopplerEnabled(desc));
        logger->Info("    hasSustainPoint={}", EventHasSustainPoint(desc));
        float minD = 0, maxD = 0;
        if (GetEventMinMaxDistance(desc, minD, maxD)) logger->Info("    minDistance={} maxDistance={}", minD, maxD);
        uint8_t idGuid[16];
        if (GetEventID(desc, idGuid)) {
          char idHex[33];
          for (int i = 0; i < 16; ++i) snprintf(idHex + i * 2, 3, "%02x", idGuid[i]);
          idHex[32] = '\0';
          logger->Info("    getID={}", idHex);
        }
        char pathBuf[512] = {};
        GetEventPathFromDesc(desc, pathBuf, sizeof(pathBuf));
        logger->Info("    getPath='{}'", pathBuf[0] ? pathBuf : "<empty>");
        logger->Info("    instanceCount={}", GetEventInstanceCount(desc));
        int paramCount = GetEventParameterDescriptionCount(desc);
        logger->Info("    parameterCount={}", paramCount);
        if (paramCount > 0 && m_fmodFn.EventDescription_GetParameterDescriptionByIndex) {
          for (int p = 0; p < paramCount; ++p) {
            FMOD_STUDIO_PARAMETER_DESCRIPTION pd = {};
            FMOD_RESULT pr = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int, FMOD_STUDIO_PARAMETER_DESCRIPTION*)>(m_fmodFn.EventDescription_GetParameterDescriptionByIndex)(desc, p, &pd);
            if (pr == FMOD_OK && pd.name) {
              logger->Info("    param[{}] name='{}' id={:08X}:{:08X} min={} max={} default={} type={}", p, pd.name, pd.id.data1, pd.id.data2, pd.minimum, pd.maximum, pd.defaultvalue, static_cast<int32_t>(pd.type));
            }
          }
        }
        int sampleState = 0;
        if (GetEventSampleLoadingState(desc, sampleState)) logger->Info("    sampleLoadingState={}", sampleState);
        uint32_t soundSize = 0;
        if (GetEventSoundSize(desc, soundSize)) logger->Info("    soundSize={} bytes", soundSize);
      }

      eventNode = *reinterpret_cast<uintptr_t*>(eventNode);
    }

    bankNode = *reinterpret_cast<uintptr_t*>(bankNode);
    bankIndex++;
  }

  ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  logger->Info("");
  logger->Info("===== FMOD Sound Dump END: {} banks, {} entries ({} events resolved, {} events unresolved, {} buses/params skipped) =====", bankIndex, eventIndex, resolvedCount, unresolvedCount, skippedCount);
}

bool SoundService::FindEventGuidByPath(const char* eventPath, uint8_t outGuid[16]) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  if (!m_isInitialized || !eventPath) return false;

  uintptr_t soundSystem = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundSystem) return false;

  uintptr_t lockAddr = soundSystem + m_bankListLockOffset;
  AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  uintptr_t bankListHead = *reinterpret_cast<uintptr_t*>(soundSystem + m_bankListHeadOffset);
  uintptr_t bankSentinel = soundSystem + m_bankListSentinelOffset;

  bool found = false;
  uintptr_t bankNode = bankListHead;
  while (bankNode != bankSentinel) {
    uintptr_t eventNode = *reinterpret_cast<uintptr_t*>(bankNode + m_bankEventListHeadOffset);
    uintptr_t eventSentinel = bankNode + m_bankEventListHeadOffset + m_eventListTerminatorOffset;

    while (eventNode != eventSentinel) {
      const char* path = *reinterpret_cast<const char**>(eventNode + m_eventPathOffset);
      if (path && strcmp(path, eventPath) == 0) {
        std::memcpy(outGuid, reinterpret_cast<void*>(eventNode + m_eventGuidOffset), 16);
        found = true;
        break;
      }
      eventNode = *reinterpret_cast<uintptr_t*>(eventNode);
    }
    if (found) break;
    bankNode = *reinterpret_cast<uintptr_t*>(bankNode);
  }

  ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  if (found) {
    char guidHex[33];
    for (int i = 0; i < 16; ++i) snprintf(guidHex + i * 2, 3, "%02x", outGuid[i]);
    guidHex[32] = '\0';
  } else {
    logger->Warn("FindEventGuidByPath: event '{}' not found in any loaded bank", eventPath);
  }
  return found;
}

bool SoundService::BuildEventCache() {
  if (!m_eventCache.empty()) return true;
  if (!m_isInitialized) return false;

  auto groups = GetSoundBankGroups();
  EnrichEventsWithFmodData(groups);

  m_eventCache.clear();
  for (const auto& group : groups) {
    for (const auto& ev : group.events) {
      EventCacheEntry entry;
      entry.bankPath = ev.bankPath;
      entry.eventPath = ev.eventPath;
      std::memcpy(entry.guid, ev.guid, 16);
      entry.eventDesc = ev.eventDesc;
      entry.is3D = ev.is3D;
      entry.isOneshot = ev.isOneshot;
      entry.isStream = ev.isStream;
      entry.isSnapshot = ev.isSnapshot;
      entry.durationMs = ev.durationMs;
      entry.minDistance = ev.minDistance;
      entry.maxDistance = ev.maxDistance;
      m_eventCache.push_back(std::move(entry));
    }
  }

  if (!m_pluginBanks.empty() && m_fmodFn.Bank_GetEventCount && m_fmodFn.Bank_GetEventList && m_fmodFn.EventDescription_GetID) {
    auto fnGetEventCount = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, int*)>(m_fmodFn.Bank_GetEventCount);
    auto fnGetEventList = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::Bank*, FMOD::Studio::EventDescription**, int, int*)>(m_fmodFn.Bank_GetEventList);
    auto fnGetID = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD_GUID*)>(m_fmodFn.EventDescription_GetID);

    for (auto* bank : m_pluginBanks) {
      int count = 0;
      auto rc = fnGetEventCount(static_cast<FMOD::Studio::Bank*>(bank), &count);
      if (rc != FMOD_OK || count <= 0) continue;

      std::vector<FMOD::Studio::EventDescription*> descs(count);
      int fetched = 0;
      rc = fnGetEventList(static_cast<FMOD::Studio::Bank*>(bank), descs.data(), count, &fetched);
      if (rc != FMOD_OK) continue;

      for (int i = 0; i < fetched; i++) {
        FMOD_GUID guid{};
        if (fnGetID(descs[i], &guid) != FMOD_OK) continue;

        std::array<uint8_t, 16> guidArr{};
        std::memcpy(guidArr.data(), &guid, 16);
        std::string resolvedPath;
        auto it = m_guidToPath.find(guidArr);
        if (it != m_guidToPath.end()) resolvedPath = it->second;

        if (resolvedPath.empty() && m_fmodFn.EventDescription_GetPath) {
          auto pathFn = reinterpret_cast<FMOD_RESULT (*)(FMOD::Studio::EventDescription*, char*, int, int*)>(m_fmodFn.EventDescription_GetPath);
          char pathBuf[512] = {};
          if (pathFn(descs[i], pathBuf, sizeof(pathBuf), nullptr) == FMOD_OK && pathBuf[0] != '\0') {
            resolvedPath = pathBuf;
          }
        }

        EventCacheEntry entry;
        entry.eventPath = resolvedPath;
        entry.eventDesc = descs[i];
        std::memcpy(entry.guid, &guid, 16);
        entry.is3D = IsEvent3D(descs[i]);
        entry.isOneshot = IsEventOneshot(descs[i]);
        entry.isStream = IsEventStream(descs[i]);
        entry.isSnapshot = IsEventSnapshot(descs[i]);
        GetEventLength(descs[i], entry.durationMs);
        GetEventMinMaxDistance(descs[i], entry.minDistance, entry.maxDistance);
        m_eventCache.push_back(std::move(entry));
      }
    }
  }

  return true;
}

bool SoundService::BuildBusCache() {
  if (!m_busCache.empty()) return true;
  if (!m_isInitialized) return false;

  auto buses = GetBuses();
  m_busCache.clear();
  m_busCache.reserve(buses.size());
  for (const auto& bus : buses) {
    BusCacheEntry entry;
    entry.busPath = bus.busPath;
    entry.busPtr = nullptr;
    m_busCache.push_back(std::move(entry));
  }
  return true;
}

bool SoundService::BuildVCACache() {
  if (!m_vcaCache.empty()) return true;
  if (!m_isInitialized) return false;

  auto vcas = GetVCAs();
  m_vcaCache.clear();
  m_vcaCache.reserve(vcas.size());
  for (const auto& vca : vcas) {
    VCACacheEntry entry;
    entry.vcaPath = vca.vcaPath;
    m_vcaCache.push_back(std::move(entry));
  }
  return true;
}

namespace {

struct SrwLockGuard {
  PSRWLOCK lock;
  explicit SrwLockGuard(PSRWLOCK l) : lock(l) {
    if (lock) AcquireSRWLockExclusive(lock);
  }
  ~SrwLockGuard() {
    if (lock) ReleaseSRWLockExclusive(lock);
  }
  SrwLockGuard(const SrwLockGuard&) = delete;
  SrwLockGuard& operator=(const SrwLockGuard&) = delete;
};

bool IsSoundEventValid(const void* event) { return event != nullptr && Utils::PatternFinder::IsValidAddress(reinterpret_cast<uintptr_t>(event)); }

constexpr uint64_t kMaxArrayEntries = 64;
constexpr size_t kMaxSoundEventWalk = 4096;
// Hygienic upper bound for the SoundManager object size — not a binary-derived
// offset. Only the list sentinel (+0x80) lives inside the manager; real events
// are heap-allocated. Bounds: sentinel stays far below this even if the
// manager grows in a future game patch.
constexpr uintptr_t kSoundManagerSanityBound = 0x1000;

}  // namespace

void SoundService::ForEachSoundEvent(const std::function<void(void*)>& fn) {
  if (!m_isInitialized || !fn) return;
  if (m_soundEventListHeadOffset == 0 || m_soundEventNodeOffset == 0) return;

  uintptr_t soundManager = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!Utils::PatternFinder::IsValidAddress(soundManager)) return;

  SrwLockGuard createGuard(reinterpret_cast<PSRWLOCK>(m_soundEventCreateLockAddr));

  std::unordered_set<void*> visited;
  auto visit = [&](void* event) {
    if (!IsSoundEventValid(event)) return;
    if (!visited.insert(event).second) return;
    fn(event);
  };
  // The list sentinel node lives inside the SoundManager object itself
  // (Ghidra SoundEvent_CreateAndInsert: sentinel at SoundManager+0x80,
  // head pointer at +0x88). Real sound_event_t objects are heap-allocated,
  // never inside the manager — skip in-manager "events" instead of breaking.
  auto inManager = [&](uintptr_t event) { return event >= soundManager && event < soundManager + kSoundManagerSanityBound; };

  uintptr_t head = *reinterpret_cast<uintptr_t*>(soundManager + m_soundEventListHeadOffset);
  if (Utils::PatternFinder::IsValidAddress(head)) {
    // Link node lives at event+m_soundEventNodeOffset (event+8); head stores that address.
    // node.next is at [node+0], node.prev at [node+8]. Walk both directions:
    // forward may dead-end at the sentinel, backward covers the rest of the cycle.
    auto walk = [&](uintptr_t start, bool forward) {
      uintptr_t node = start;
      size_t iterations = 0;
      while (iterations++ < kMaxSoundEventWalk) {
        uintptr_t event = node - m_soundEventNodeOffset;
        if (!inManager(event)) visit(reinterpret_cast<void*>(event));
        uintptr_t next = forward ? *reinterpret_cast<uintptr_t*>(node) : *reinterpret_cast<uintptr_t*>(node + 8);
        if (next == 0 || next == start || !Utils::PatternFinder::IsValidAddress(next)) break;
        node = next;
      }
    };
    walk(head, true);
    walk(head, false);
  }

  // UI sound_event wrappers (Ghidra FUN_140442180): array of wrapper*, each wrapper[1] = sound_event*.
  if (m_uiWrapperArrayBufferOffset != 0 && m_uiWrapperArrayCountOffset != 0 && m_uiWrapperEventOffset != 0) {
    uintptr_t uiBuffer = *reinterpret_cast<uintptr_t*>(soundManager + m_uiWrapperArrayBufferOffset);
    uint64_t uiCount = *reinterpret_cast<uint64_t*>(soundManager + m_uiWrapperArrayCountOffset);
    if (Utils::PatternFinder::IsValidAddress(uiBuffer) && uiCount > 0 && uiCount <= kMaxArrayEntries) {
      for (uint64_t i = 0; i < uiCount; ++i) {
        uintptr_t slot = uiBuffer + i * sizeof(uintptr_t);
        if (!Utils::PatternFinder::IsValidAddress(slot)) break;
        uintptr_t wrapper = *reinterpret_cast<uintptr_t*>(slot);
        if (!Utils::PatternFinder::IsValidAddress(wrapper)) continue;
        uintptr_t event = *reinterpret_cast<uintptr_t*>(wrapper + m_uiWrapperEventOffset);
        visit(reinterpret_cast<void*>(event));
      }
    }
  }

  // Voice-nav runtime entries (Ghidra FUN_140442180): stride 0x28, embedded wrapper at +0x18,
  // sound_event* at entry+0x20 (may be null until the prompt is first created).
  if (m_voiceNavArrayBufferOffset != 0 && m_voiceNavArrayCountOffset != 0) {
    uintptr_t vnBuffer = *reinterpret_cast<uintptr_t*>(soundManager + m_voiceNavArrayBufferOffset);
    uint64_t vnCount = *reinterpret_cast<uint64_t*>(soundManager + m_voiceNavArrayCountOffset);
    if (Utils::PatternFinder::IsValidAddress(vnBuffer) && vnCount > 0 && vnCount <= kMaxArrayEntries) {
      for (uint64_t i = 0; i < vnCount; ++i) {
        uintptr_t entry = vnBuffer + i * m_voiceNavEntryStride;
        if (!Utils::PatternFinder::IsValidAddress(entry + m_voiceNavEntryEventOffset)) break;
        uintptr_t event = *reinterpret_cast<uintptr_t*>(entry + m_voiceNavEntryEventOffset);
        visit(reinterpret_cast<void*>(event));
      }
    }
  }
}

std::string SoundService::GetSoundEventPath(void* event) {
  if (!IsSoundEventValid(event) || m_soundEventPathOffset == 0) return {};
  auto& prism = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!prism.IsInstalled()) return {};
  return Hooks::GameTools::PrismStringResolver::GetInstance().ReadString(reinterpret_cast<char*>(event) + m_soundEventPathOffset);
}

std::string SoundService::GetSoundEventSource(void* event) {
  if (!IsSoundEventValid(event) || m_soundEventSourceOffset == 0) return {};
  auto& prism = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!prism.IsInstalled()) return {};
  return Hooks::GameTools::PrismStringResolver::GetInstance().ReadString(reinterpret_cast<char*>(event) + m_soundEventSourceOffset);
}

bool SoundService::SetSoundEventPath(void* event, const std::string& newPath) {
  if (!IsSoundEventValid(event) || m_soundEventPathOffset == 0) return false;
  auto& prism = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!prism.IsInstalled()) return false;

  void* field = reinterpret_cast<char*>(event) + m_soundEventPathOffset;

  // Ghidra SoundEvent_CreateAndInsert: state at [SoundManager+0x1c0], bound lock at
  // [SoundManager+0x1d8] — both are SoundManager fields (RBP base), not sound_event_t fields.
  PSRWLOCK perEventLock = nullptr;
  if (m_soundEventStateOffset != 0 && m_soundEventBoundLockOffset != 0) {
    uintptr_t soundManager = ManagerCoreService::GetInstance().GetSoundManagerAddr();
    if (Utils::PatternFinder::IsValidAddress(soundManager)) {
      uint32_t state = *reinterpret_cast<uint32_t*>(soundManager + m_soundEventStateOffset);
      if (state == m_soundEventBoundState) {
        perEventLock = reinterpret_cast<PSRWLOCK>(soundManager + m_soundEventBoundLockOffset);
      }
    }
  }

  bool ok = false;
  {
    SrwLockGuard lockGuard(perEventLock);
    ok = prism.Set(field, newPath.c_str());
  }
  // No manual activation here: activate() acquires SoundManager SRW locks and
  // must only run on the game thread (AV from the UI thread). The game's own
  // re-activation re-runs LoadConfig, whose detour re-applies overrides.
  if (ok) InvalidateSoundRefSnapshot();
  return ok;
}

bool SoundService::SetSoundEventSource(void* event, const std::string& newSource) {
  if (!IsSoundEventValid(event) || m_soundEventSourceOffset == 0) return false;
  auto& prism = Hooks::GameTools::PrismStringResolver::GetInstance();
  if (!prism.IsInstalled()) return false;

  void* field = reinterpret_cast<char*>(event) + m_soundEventSourceOffset;

  // Ghidra SoundEvent_CreateAndInsert: state at [SoundManager+0x1c0], bound lock at
  // [SoundManager+0x1d8] — both are SoundManager fields (RBP base), not sound_event_t fields.
  PSRWLOCK perEventLock = nullptr;
  if (m_soundEventStateOffset != 0 && m_soundEventBoundLockOffset != 0) {
    uintptr_t soundManager = ManagerCoreService::GetInstance().GetSoundManagerAddr();
    if (Utils::PatternFinder::IsValidAddress(soundManager)) {
      uint32_t state = *reinterpret_cast<uint32_t*>(soundManager + m_soundEventStateOffset);
      if (state == m_soundEventBoundState) {
        perEventLock = reinterpret_cast<PSRWLOCK>(soundManager + m_soundEventBoundLockOffset);
      }
    }
  }

  bool ok = false;
  {
    SrwLockGuard lockGuard(perEventLock);
    ok = prism.Set(field, newSource.c_str());
  }
  // No manual activation here: activate() acquires SoundManager SRW locks and
  // must only run on the game thread (AV from the UI thread). The game's own
  // re-activation re-runs LoadConfig, whose detour re-applies overrides.
  if (ok) InvalidateSoundRefSnapshot();
  return ok;
}

std::vector<SoundRefEntry> SoundService::GetUiSoundRefEntries() {
  std::vector<SoundRefEntry> entries;
  if (!Utils::PatternFinder::IsValidAddress(m_uiSoundRefTableAddr)) return entries;

  entries.reserve(m_uiSoundRefCount);
  for (uint32_t i = 0; i < m_uiSoundRefCount; ++i) {
    uintptr_t entryAddr = m_uiSoundRefTableAddr + i * m_uiSoundRefEntrySize;
    if (!Utils::PatternFinder::IsValidAddress(entryAddr + m_uiSoundRefEntrySize)) break;

    uintptr_t pathPtr = *reinterpret_cast<uintptr_t*>(entryAddr + 0x00);
    SoundRefEntry entry;
    entry.path = Utils::PatternFinder::ReadBoundedCString(pathPtr);
    if (entry.path.empty()) continue;
    entry.category = *reinterpret_cast<uint32_t*>(entryAddr + m_uiSoundRefCategoryOffset);
    entry.enabled = *reinterpret_cast<uint8_t*>(entryAddr + m_uiSoundRefEnabledOffset) != 0;
    entry.index = 0;  // UI table has no index field
    entries.push_back(std::move(entry));
  }
  return entries;
}

std::vector<SoundRefEntry> SoundService::GetVoiceNavEntries() {
  std::vector<SoundRefEntry> entries;
  if (!Utils::PatternFinder::IsValidAddress(m_voiceNavTableAddr)) return entries;

  entries.reserve(16);
  for (uint32_t i = 0; i < m_voiceNavMaxEntries; ++i) {
    uintptr_t entryAddr = m_voiceNavTableAddr + i * m_voiceNavEntrySize;
    if (!Utils::PatternFinder::IsValidAddress(entryAddr + m_voiceNavEntrySize)) break;

    // Ghidra: +0x00 category dword, +0x04 index dword, +0x08 enabled byte, +0x10 char* path
    uintptr_t pathPtr = *reinterpret_cast<uintptr_t*>(entryAddr + m_voiceNavPathOffset);
    SoundRefEntry entry;
    entry.path = Utils::PatternFinder::ReadBoundedCString(pathPtr);
    if (entry.path.empty()) continue;
    entry.category = *reinterpret_cast<uint32_t*>(entryAddr + m_voiceNavCategoryOffset);
    entry.enabled = *reinterpret_cast<uint8_t*>(entryAddr + m_voiceNavEnabledOffset) != 0;
    entry.index = static_cast<uint8_t>(*reinterpret_cast<uint32_t*>(entryAddr + m_voiceNavIndexOffset) & 0xFF);
    entries.push_back(std::move(entry));
  }
  return entries;
}

void SoundService::BuildSoundRefRdataCatalog() {
  m_soundRefRdataCatalogBuilt = true;
  m_soundRefRdataCatalog.clear();

  Utils::PatternFinder::StringScanRules rules;
  rules.suffix = ".soundref";
  rules.requiredStart = "/";
  rules.maxBackScan = 512;
  rules.requireNullTerminated = true;

  m_soundRefRdataCatalog = Utils::PatternFinder::FindAllStringsByRules(rules);

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("SoundRef rdata catalog: {} paths", m_soundRefRdataCatalog.size());
}

void SoundService::BuildSoundRefVfsCatalog() {
  m_soundRefVfsCatalog.clear();
  m_soundRefVfsCatalogBuilt = true;

  static constexpr char kExt[] = ".soundref";

  auto& fsService = GameObjectFileSystemService::GetInstance();
  if (!fsService.AreAllFindersReady()) {
    m_soundRefVfsCatalogBuilt = false;
    return;
  }

  namespace fs = std::filesystem;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");

  // Mount walk mirrors PathManager::ResolveUfsVirtualPath (src/System/PathManager.cpp):
  //   array   = *GetDevicesArrayAddr()              -> uintptr_t[ *GetManagersCountAddr() ]
  //   anchor  = manager + GetMountListHeadOffset()  -> sentinel
  //   next    = *(uintptr_t*)node
  //   vpath   = *(char**)(node + NodeVPathOffset + StringBufferOffset)
  //   device  = *(node + NodeDeviceOffset); phys = *(char**)(device + PhysDevicePathOffset)
  uintptr_t managersArrayPtr = *reinterpret_cast<uintptr_t*>(fsService.GetDevicesArrayAddr());
  uint32_t managersCount = *reinterpret_cast<uint32_t*>(fsService.GetManagersCountAddr());
  if (managersArrayPtr == 0 || managersCount == 0) return;
  uintptr_t* pManagers = reinterpret_cast<uintptr_t*>(managersArrayPtr);

  size_t mountCount = 0;
  for (uint32_t m = 0; m < managersCount; ++m) {
    uintptr_t ufsManager = pManagers[m];
    if (IsBadReadPtr(reinterpret_cast<void*>(ufsManager), sizeof(uintptr_t))) continue;

    uintptr_t anchorAddr = ufsManager + fsService.GetMountListHeadOffset();
    if (IsBadReadPtr(reinterpret_cast<void*>(anchorAddr), sizeof(uintptr_t))) continue;
    uintptr_t node = *reinterpret_cast<uintptr_t*>(anchorAddr);
    if (node == 0 || node == anchorAddr) continue;

    for (int i = 0; i < 512; ++i) {
      if (node == anchorAddr || IsBadReadPtr(reinterpret_cast<void*>(node), sizeof(uintptr_t))) break;
      ++mountCount;

      const char* vpathStr = nullptr;
      uintptr_t vpathSlot = node + fsService.GetNodeVPathOffset() + fsService.GetStringBufferOffset();
      if (!IsBadReadPtr(reinterpret_cast<void*>(vpathSlot), sizeof(uintptr_t))) {
        vpathStr = *reinterpret_cast<const char**>(vpathSlot);
      }

      const char* physStr = nullptr;
      uintptr_t deviceAddr = 0;
      uintptr_t deviceSlot = node + fsService.GetNodeDeviceOffset();
      if (!IsBadReadPtr(reinterpret_cast<void*>(deviceSlot), sizeof(uintptr_t))) {
        deviceAddr = *reinterpret_cast<uintptr_t*>(deviceSlot);
      }
      if (deviceAddr && !IsBadReadPtr(reinterpret_cast<void*>(deviceAddr), sizeof(uintptr_t))) {
        uintptr_t physSlot = deviceAddr + fsService.GetPhysicalDevicePathOffset();
        if (!IsBadReadPtr(reinterpret_cast<void*>(physSlot), sizeof(uintptr_t))) {
          physStr = *reinterpret_cast<const char**>(physSlot);
        }
      }

      if (physStr != nullptr && !IsBadReadPtr(const_cast<char*>(physStr), 1)) {
        std::error_code ec;
        fs::path phys(physStr);
        if (fs::is_directory(phys, ec) && !ec) {
          std::string vpath = (vpathStr != nullptr && !IsBadReadPtr(const_cast<char*>(vpathStr), 1)) ? vpathStr : "";
          fs::recursive_directory_iterator it(phys, fs::directory_options::skip_permission_denied, ec);
          size_t budget = 100000;
          for (; it != fs::recursive_directory_iterator() && budget > 0; it.increment(ec)) {
            if (ec) {
              ec.clear();
              continue;
            }
            --budget;
            const auto& entry = *it;
            if (!entry.is_regular_file(ec) || ec) {
              ec.clear();
              continue;
            }
            std::string ext = entry.path().extension().string();
            if (ext.size() != 9) continue;
            bool match = true;
            for (size_t k = 0; k < 9; ++k) {
              char a = ext[k];
              if (a >= 'A' && a <= 'Z') a = static_cast<char>(a + 32);
              if (a != kExt[k]) {
                match = false;
                break;
              }
            }
            if (!match) continue;
            std::string rel = fs::relative(entry.path(), phys, ec).generic_string();
            if (ec || rel.empty()) {
              ec.clear();
              continue;
            }
            std::string gamePath = vpath;
            if (gamePath.empty() || gamePath.back() != '/') gamePath += '/';
            gamePath += rel;
            m_soundRefVfsCatalog.push_back(std::move(gamePath));
          }
        }
      }

      node = *reinterpret_cast<uintptr_t*>(node);
    }
  }

  std::sort(m_soundRefVfsCatalog.begin(), m_soundRefVfsCatalog.end());
  m_soundRefVfsCatalog.erase(std::unique(m_soundRefVfsCatalog.begin(), m_soundRefVfsCatalog.end()), m_soundRefVfsCatalog.end());
  logger->Info("SoundRef VFS catalog: {} paths ({} mount nodes scanned)", m_soundRefVfsCatalog.size(), mountCount);
}

std::vector<SoundRefEntry> SoundService::BuildSoundRefSnapshot() {
  std::vector<SoundRefEntry> out;
  if (!m_isInitialized) return out;

  std::unordered_map<std::string, size_t> indexByPath;
  auto upsert = [&](SoundRefEntry&& entry, uint8_t origin) {
    if (entry.path.empty()) return;
    auto [it, inserted] = indexByPath.emplace(entry.path, out.size());
    if (inserted) {
      out.push_back(std::move(entry));
      out.back().origin |= origin;
      return;
    }
    SoundRefEntry& dst = out[it->second];
    dst.origin |= origin;
    if (dst.category == 0 && entry.category != 0) dst.category = entry.category;
    if (!dst.enabled && entry.enabled) dst.enabled = entry.enabled;
    if (dst.index == 0 && entry.index != 0) dst.index = entry.index;
    if (entry.hasEvent) {
      dst.hasEvent = true;
      if (!entry.source.empty()) dst.source = std::move(entry.source);
    }
  };

  for (SoundRefEntry& entry : GetUiSoundRefEntries()) upsert(std::move(entry), SOUNDREF_ORIGIN_UI);
  for (SoundRefEntry& entry : GetVoiceNavEntries()) upsert(std::move(entry), SOUNDREF_ORIGIN_VN);

  ForEachSoundEvent([&](void* event) {
    SoundRefEntry entry;
    entry.path = GetSoundEventPath(event);
    if (entry.path.empty()) return;
    entry.hasEvent = true;
    entry.source = GetSoundEventSource(event);
    upsert(std::move(entry), SOUNDREF_ORIGIN_LIVE);
  });

  if (!m_soundRefVfsCatalogBuilt) BuildSoundRefVfsCatalog();
  for (const std::string& path : m_soundRefVfsCatalog) {
    SoundRefEntry entry;
    entry.path = path;
    upsert(std::move(entry), SOUNDREF_ORIGIN_VFS);
  }

  if (!m_soundRefRdataCatalogBuilt) BuildSoundRefRdataCatalog();
  for (const std::string& path : m_soundRefRdataCatalog) {
    SoundRefEntry entry;
    entry.path = path;
    upsert(std::move(entry), SOUNDREF_ORIGIN_RDATA);
  }

  return out;
}

std::vector<SoundRefEntry> SoundService::GetSoundRefEntries() {
  // Full refresh semantics for the SoundWindow Refresh/Dump buttons.
  InvalidateSoundRefSnapshot();
  if (!EnsureSoundRefSnapshot()) return {};
  std::lock_guard<std::mutex> lock(m_soundRefSnapshotMutex);
  return m_soundRefSnapshot;
}

bool SoundService::EnsureSoundRefSnapshot() {
  if (!m_isInitialized) return false;
  if (!m_soundRefSnapshotDirty.load(std::memory_order_acquire) && m_soundRefSnapshotValid.load(std::memory_order_acquire)) {
    return true;
  }

  // Clear before walking the game state: any change from this point re-sets
  // the dirty flag, so a concurrent invalidation is never lost (worst case:
  // one redundant rebuild on the next read).
  m_soundRefSnapshotDirty.store(false, std::memory_order_release);

  std::vector<SoundRefEntry> fresh = BuildSoundRefSnapshot();
  std::unordered_map<std::string, int> freshIndex;
  freshIndex.reserve(fresh.size());
  for (size_t i = 0; i < fresh.size(); ++i) freshIndex.emplace(fresh[i].path, static_cast<int>(i));

  std::lock_guard<std::mutex> lock(m_soundRefSnapshotMutex);
  m_soundRefSnapshot = std::move(fresh);
  m_soundRefSnapshotIndex = std::move(freshIndex);
  m_soundRefSnapshotValid.store(true, std::memory_order_release);
  return true;
}

int SoundService::GetSoundRefCount() {
  if (!EnsureSoundRefSnapshot()) return 0;
  std::lock_guard<std::mutex> lock(m_soundRefSnapshotMutex);
  return static_cast<int>(m_soundRefSnapshot.size());
}

bool SoundService::GetSoundRefEntryByIndex(int index, SoundRefEntry& out) {
  if (!EnsureSoundRefSnapshot()) return false;
  std::lock_guard<std::mutex> lock(m_soundRefSnapshotMutex);
  if (index < 0 || index >= static_cast<int>(m_soundRefSnapshot.size())) return false;
  out = m_soundRefSnapshot[index];
  return true;
}

int SoundService::FindSoundRefIndex(const char* soundrefPath) {
  if (soundrefPath == nullptr || !EnsureSoundRefSnapshot()) return -1;
  std::lock_guard<std::mutex> lock(m_soundRefSnapshotMutex);
  auto it = m_soundRefSnapshotIndex.find(soundrefPath);
  if (it == m_soundRefSnapshotIndex.end()) return -1;
  return it->second;
}

int SoundService::FindSoundRefIndexBySource(const char* source) {
  if (source == nullptr || !EnsureSoundRefSnapshot()) return -1;
  std::lock_guard<std::mutex> lock(m_soundRefSnapshotMutex);
  for (int i = 0; i < static_cast<int>(m_soundRefSnapshot.size()); ++i) {
    if (m_soundRefSnapshot[i].source == source) return i;
  }
  return -1;
}

void SoundService::InstallSoundRefLoadConfigHook() {
  if (m_loadConfigHookInstalled) return;
  if (m_soundRefLoadConfigFn == 0) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  MH_STATUS initStatus = MH_Initialize();
  if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
    logger->Warn("SoundRef_LoadConfig hook: MH_Initialize failed: {}", static_cast<int>(initStatus));
    return;
  }
  MH_STATUS status = MH_CreateHook(reinterpret_cast<LPVOID>(m_soundRefLoadConfigFn), reinterpret_cast<LPVOID>(&SoundService::HookedSoundRefLoadConfig), reinterpret_cast<LPVOID*>(&m_loadConfigTrampoline));
  if (status != MH_OK) {
    logger->Error("MH_CreateHook(SoundRef_LoadConfig) failed: {}", static_cast<int>(status));
    m_loadConfigTrampoline = nullptr;
    return;
  }
  status = MH_EnableHook(reinterpret_cast<LPVOID>(m_soundRefLoadConfigFn));
  if (status != MH_OK) {
    logger->Error("MH_EnableHook(SoundRef_LoadConfig) failed: {}", static_cast<int>(status));
    MH_RemoveHook(reinterpret_cast<LPVOID>(m_soundRefLoadConfigFn));
    m_loadConfigTrampoline = nullptr;
    return;
  }
  m_loadConfigHookInstalled = true;
  logger->Info("SoundRef_LoadConfig detour installed (addr=0x{:X})", m_soundRefLoadConfigFn);
}

void SoundService::RemoveSoundRefLoadConfigHook() {
  if (!m_loadConfigHookInstalled) return;
  MH_DisableHook(reinterpret_cast<LPVOID>(m_soundRefLoadConfigFn));
  MH_RemoveHook(reinterpret_cast<LPVOID>(m_soundRefLoadConfigFn));
  m_loadConfigTrampoline = nullptr;
  m_loadConfigHookInstalled = false;
}

void SoundService::QueueSoundRefRebinds(const std::vector<void*>& events) {
  if (events.empty()) return;
  {
    std::lock_guard<std::mutex> lock(m_pendingRebindMutex);
    for (void* event : events) {
      if (std::find(m_pendingRebinds.begin(), m_pendingRebinds.end(), event) == m_pendingRebinds.end()) {
        m_pendingRebinds.push_back(event);
      }
    }
  }
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("SoundRef rebind queued: {} event(s) pending", events.size());
}

// --- L2: game sound_event control (handles are game sound_event_t*, game thread only) ---

namespace {
constexpr uint32_t kEventStatePlaying = 1;
constexpr uint32_t kEventStatePaused = 3;
constexpr uint32_t kEventStateBound = 2;
}  // namespace

std::vector<void*> SoundService::GetGameEventSnapshot() {
  std::vector<void*> out;
  if (!m_isInitialized || m_soundEventPathOffset == 0) return out;
  out.reserve(512);
  ForEachSoundEvent([&out](void* event) { out.push_back(event); });
  return out;
}

int SoundService::GetGameEventCount() { return static_cast<int>(GetGameEventSnapshot().size()); }

void* SoundService::GetGameEventAt(int index) {
  if (index < 0) return nullptr;
  auto snapshot = GetGameEventSnapshot();
  return index < static_cast<int>(snapshot.size()) ? snapshot[static_cast<size_t>(index)] : nullptr;
}

void* SoundService::FindGameEventByPath(const char* path) {
  if (!path || !*path) return nullptr;
  auto matches = FindSoundEventsByPath(path);
  return matches.empty() ? nullptr : matches.front();
}

void* SoundService::FindGameEventBySource(const char* source) {
  if (!source || !*source) return nullptr;
  void* found = nullptr;
  ForEachSoundEvent([&](void* event) {
    if (!found && GetSoundEventSource(event) == source) found = event;
  });
  return found;
}

void* SoundService::FindGameEventByInstance(void* instance) {
  if (!instance || m_soundEventInstanceOffset == 0) return nullptr;
  void* found = nullptr;
  ForEachSoundEvent([&](void* event) {
    if (!found) {
      auto instanceField = reinterpret_cast<void**>(static_cast<char*>(event) + m_soundEventInstanceOffset);
      if (*instanceField == instance) found = event;
    }
  });
  return found;
}

uint32_t SoundService::GetGameEventPlaybackState(void* event) {
  if (!IsSoundEventValid(event) || m_soundEventPlaybackStateOffset == 0) return 0;
  return *reinterpret_cast<const uint32_t*>(static_cast<const char*>(event) + m_soundEventPlaybackStateOffset);
}

bool SoundService::IsGameEventBound(void* event) {
  if (!IsSoundEventValid(event) || m_soundEventBoundFieldOffset == 0) return false;
  return *reinterpret_cast<const uint32_t*>(static_cast<const char*>(event) + m_soundEventBoundFieldOffset) == kEventStateBound;
}

SoundService::ActivateOutcome SoundService::RecreateGameEventInstance(void* event) {
  ActivateOutcome out;
  if (!IsSoundEventValid(event) || m_soundEventActivateFn == 0) return out;

  out.lifecycleManaged = m_soundEventInstanceOffset != 0 && m_soundEventPlaybackStateOffset != 0;
  if (out.lifecycleManaged) {
    auto instanceField = reinterpret_cast<void**>(static_cast<char*>(event) + m_soundEventInstanceOffset);
    void* oldInstance = *instanceField;
    uint32_t state = *reinterpret_cast<const uint32_t*>(static_cast<const char*>(event) + m_soundEventPlaybackStateOffset);
    out.wasActive = state == kEventStatePlaying || state == kEventStatePaused;
    if (oldInstance) {
      auto stopFn = reinterpret_cast<bool (*)(void*)>(GetSoundEventStopFn());
      if (out.wasActive && stopFn) stopFn(event);  // state -> 0, keeps instance pointer
      *instanceField = nullptr;
      ReleaseEventInstance(oldInstance);
      out.oldInstance = oldInstance;
    }
  }

  uintptr_t soundManager = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundManager) return out;
  auto activate = reinterpret_cast<bool (*)(void*, void*)>(m_soundEventActivateFn);
  out.ok = activate(event, reinterpret_cast<void*>(soundManager));
  if (out.lifecycleManaged) {
    out.instance = *reinterpret_cast<void**>(static_cast<char*>(event) + m_soundEventInstanceOffset);
  }
  return out;
}

bool SoundService::GameEventActivate(void* event) { return RecreateGameEventInstance(event).ok; }

bool SoundService::GameEventStart(void* event) {
  if (!IsSoundEventValid(event)) return false;
  auto playback = reinterpret_cast<bool (*)(void*, int)>(GetSoundEventPlaybackControlFn());
  return playback && playback(event, 0);
}

bool SoundService::GameEventStop(void* event) {
  if (!IsSoundEventValid(event)) return false;
  auto stopFn = reinterpret_cast<bool (*)(void*)>(GetSoundEventStopFn());
  return stopFn && stopFn(event);
}

bool SoundService::GameEventSetPaused(void* event, bool paused) {
  if (!IsSoundEventValid(event)) return false;
  auto fn = reinterpret_cast<bool (*)(void*, int)>(GetSoundEventSetPausedFn());
  return fn && fn(event, paused ? 1 : 0);
}

bool SoundService::GameEventSetVolume(void* event, float volume) {
  if (!IsSoundEventValid(event)) return false;
  auto fn = reinterpret_cast<bool (*)(void*, float)>(GetSoundEventSetVolumeFn());
  return fn && fn(event, volume);
}

bool SoundService::GameEventSetPitch(void* event, float pitch) {
  if (!IsSoundEventValid(event)) return false;
  auto fn = reinterpret_cast<bool (*)(void*, float)>(GetSoundEventSetPitchFn());
  return fn && fn(event, pitch);
}

bool SoundService::GameEventSetProperty(void* event, int property_id, float value) {
  if (!IsSoundEventValid(event)) return false;
  auto fn = reinterpret_cast<bool (*)(void*, int, float)>(GetSoundEventSetPropertyFn());
  return fn && fn(event, property_id, value);
}

bool SoundService::GameEventSet3DAttributes(void* event, float pos_x, float pos_y, float pos_z) {
  if (!IsSoundEventValid(event)) return false;
  auto fn = reinterpret_cast<bool (*)(void*, float, float, float)>(GetSoundEventSet3DAttributesFn());
  return fn && fn(event, pos_x, pos_y, pos_z);
}

bool SoundService::GameEventSetParameterByID(void* event, const uint8_t id[16], float value) {
  if (!IsSoundEventValid(event) || !id) return false;
  auto fn = reinterpret_cast<bool (*)(void*, const uint8_t*, float)>(GetSoundEventSetParameterByIDFn());
  return fn && fn(event, id, value);
}

void SoundService::ProcessPendingSoundRefRebinds() {
  if (!m_isInitialized || m_soundEventActivateFn == 0 || m_soundEventPathOffset == 0) return;

  std::vector<void*> pending;
  {
    std::lock_guard<std::mutex> lock(m_pendingRebindMutex);
    if (m_pendingRebinds.empty()) return;
  }

  const uint32_t gameTid = m_gameThreadId.load();
  if (gameTid == 0) {
    static bool s_warnedNoTid = false;
    if (!s_warnedNoTid) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
      logger->Warn("SoundRef rebind pending, but game-thread marker not captured yet - waiting");
      s_warnedNoTid = true;
    }
    return;
  }
  if (gameTid != ::GetCurrentThreadId()) {
    static bool s_warnedWrongTid = false;
    if (!s_warnedWrongTid) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
      logger->Warn("SoundRef rebind deferred: update ran on tid={} (game tid={})", ::GetCurrentThreadId(), gameTid);
      s_warnedWrongTid = true;
    }
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_pendingRebindMutex);
    pending.swap(m_pendingRebinds);
  }
  if (pending.empty()) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  auto activate = reinterpret_cast<bool (*)(void*, void*)>(m_soundEventActivateFn);
  uintptr_t soundManager = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundManager) {
    logger->Warn("SoundRef rebind: SoundManager unavailable, {} event(s) dropped", pending.size());
    return;
  }

  for (void* event : pending) {
    if (!IsSoundEventValid(event)) {
      logger->Info("SoundRef rebind: skip event={}, invalid", event);
      continue;
    }
    std::string path = GetSoundEventPath(event);
    std::string source = GetSoundEventSource(event);
    ActivateOutcome r = RecreateGameEventInstance(event);
    if (!r.lifecycleManaged) {
      logger->Warn("SoundRef rebind '{}': instance/state offsets unavailable, cannot manage instance lifecycle", path);
    }
    if (!r.ok || !r.instance) {
      logger->Info("SoundRef rebind '{}' source='{}': activate={} instance={} (no live instance yet)", path, source, r.ok, r.instance);
      continue;
    }
    bool restarted = false;
    if (r.wasActive) {
      if (auto playback = reinterpret_cast<bool (*)(void*, int)>(GetSoundEventPlaybackControlFn())) {
        restarted = playback(event, 0);
      }
    }
    logger->Info("SoundRef rebind '{}' source='{}': oldInstance={} released={} wasPlaying={} restarted={} instance={}",
                 path, source, r.oldInstance, r.oldInstance != nullptr, r.wasActive, restarted, r.instance);
  }
}

void SoundService::InstallSystemUpdateHook() {
  if (m_systemUpdateHookInstalled) return;
  uintptr_t updateFn = reinterpret_cast<uintptr_t>(m_fmodFn.System_Update);
  if (updateFn == 0) {
    updateFn = reinterpret_cast<uintptr_t>(Fmod::FmodApi::GetInstance().Find("System::update"));
  }
  if (updateFn == 0) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  MH_STATUS initStatus = MH_Initialize();
  if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
    logger->Warn("System::update hook: MH_Initialize failed: {}", static_cast<int>(initStatus));
    return;
  }
  MH_STATUS status = MH_CreateHook(reinterpret_cast<LPVOID>(updateFn), reinterpret_cast<LPVOID>(&SoundService::HookedSystemUpdate), reinterpret_cast<LPVOID*>(&m_systemUpdateTrampoline));
  if (status != MH_OK) {
    logger->Error("MH_CreateHook(System::update) failed: {}", static_cast<int>(status));
    m_systemUpdateTrampoline = nullptr;
    return;
  }
  status = MH_EnableHook(reinterpret_cast<LPVOID>(updateFn));
  if (status != MH_OK) {
    logger->Error("MH_EnableHook(System::update) failed: {}", static_cast<int>(status));
    MH_RemoveHook(reinterpret_cast<LPVOID>(updateFn));
    m_systemUpdateTrampoline = nullptr;
    return;
  }
  m_systemUpdateFnAddr = updateFn;
  m_systemUpdateHookInstalled = true;
  logger->Info("FMOD System::update detour installed (addr=0x{:X}) - SoundRef rebind tick active", updateFn);
}

void SoundService::RemoveSystemUpdateHook() {
  if (!m_systemUpdateHookInstalled) return;
  MH_DisableHook(reinterpret_cast<LPVOID>(m_systemUpdateFnAddr));
  MH_RemoveHook(reinterpret_cast<LPVOID>(m_systemUpdateFnAddr));
  m_systemUpdateTrampoline = nullptr;
  m_systemUpdateFnAddr = 0;
  m_systemUpdateHookInstalled = false;
}

void SoundService::HookedSystemUpdate(void* system) {
  SoundService& self = GetInstance();
  if (self.m_systemUpdateTrampoline != nullptr) {
    reinterpret_cast<void (*)(void*)>(self.m_systemUpdateTrampoline)(system);
  }
  if (tls_insidePluginUpdate) return;
  self.m_gameThreadId.store(::GetCurrentThreadId());
  self.ProcessPendingSoundRefRebinds();
}

void SoundService::HookedSoundRefLoadConfig(void* event) {
  SoundService& self = GetInstance();
  if (self.m_loadConfigTrampoline != nullptr) {
    reinterpret_cast<void (*)(void*)>(self.m_loadConfigTrampoline)(event);
  }
  self.m_gameThreadId.store(::GetCurrentThreadId());
  // The original LoadConfig always rewrites event+source from the .soundref
  // file — invalidate regardless of whether an override is registered below.
  self.InvalidateSoundRefSnapshot();

  if (!self.m_isInitialized || event == nullptr) return;
  if (self.m_soundEventSourceOffset == 0) return;
  if (!Hooks::GameTools::PrismStringResolver::GetInstance().IsInstalled()) return;

  std::string path = self.GetSoundEventPath(event);
  if (path.empty()) return;

  std::string overrideSource;
  std::string originalSource;
  bool originalUnknown = false;
  {
    std::lock_guard<std::mutex> lock(self.m_soundRefMutex);
    auto it = self.m_soundRefOverrides.find(path);
    if (it == self.m_soundRefOverrides.end()) return;
    overrideSource = it->second;
    auto histIt = self.m_soundRefOriginals.find(path);
    if (histIt == self.m_soundRefOriginals.end()) {
      originalSource = self.GetSoundEventSource(event);
      if (originalSource.empty() || originalSource == overrideSource) {
        originalSource.clear();
        originalUnknown = true;
      } else {
        self.m_soundRefOriginals.emplace(path, originalSource);
      }
    } else {
      originalSource = histIt->second;
    }
  }
  if (overrideSource.empty()) return;

  void* field = reinterpret_cast<char*>(event) + self.m_soundEventSourceOffset;
  auto& prism = Hooks::GameTools::PrismStringResolver::GetInstance();
  uint32_t rawLen = prism.GetLength(field);
  bool bufOk = prism.IsBufferReadable(field);
  bool ok = prism.Set(field, overrideSource.c_str());

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("SoundRef LoadConfig hook '{}' preRawLen={} preBufOk={} orig='{}' -> '{}' set={}", path, rawLen, bufOk, originalSource, overrideSource, ok);
  if (originalUnknown) logger->Warn("SoundRef LoadConfig hook '{}': original source not recorded (current equals override)", path);
}

std::vector<void*> SoundService::FindSoundEventsByPath(const std::string& soundrefPath) {
  std::vector<void*> matches;
  if (!m_isInitialized || m_soundEventPathOffset == 0) return matches;
  ForEachSoundEvent([&](void* event) {
    if (GetSoundEventPath(event) == soundrefPath) matches.push_back(event);
  });
  return matches;
}

std::vector<void*> SoundService::ApplyOverrideToEvents(const std::string& soundrefPath, const std::string& source) {
  std::vector<void*> matches;
  if (!m_isInitialized) return matches;
  if (m_soundEventPathOffset == 0 || m_soundEventSourceOffset == 0) return matches;
  if (!Hooks::GameTools::PrismStringResolver::GetInstance().IsInstalled()) return matches;

  matches = FindSoundEventsByPath(soundrefPath);
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  if (matches.empty()) {
    logger->Info("SoundRef apply '{}': no live event, will apply on activation", soundrefPath);
    return matches;
  }

  auto& prism = Hooks::GameTools::PrismStringResolver::GetInstance();
  for (void* event : matches) {
    std::string original;
    bool recordOriginal = false;
    {
      std::lock_guard<std::mutex> lock(m_soundRefMutex);
      auto it = m_soundRefOriginals.find(soundrefPath);
      if (it != m_soundRefOriginals.end()) {
        original = it->second;
      } else {
        recordOriginal = true;
      }
    }
    if (recordOriginal) {
      original = GetSoundEventSource(event);
      std::lock_guard<std::mutex> lock(m_soundRefMutex);
      auto it = m_soundRefOriginals.find(soundrefPath);
      if (it != m_soundRefOriginals.end()) {
        original = it->second;
      } else {
        m_soundRefOriginals.emplace(soundrefPath, original);
      }
    }

    void* field = reinterpret_cast<char*>(event) + m_soundEventSourceOffset;
    const char* rawBuf = prism.GetBuffer(field);
    uint32_t rawLen = prism.GetLength(field);
    bool bufOk = rawBuf != nullptr && Utils::PatternFinder::IsValidAddress(reinterpret_cast<uintptr_t>(rawBuf));
    bool ok = SetSoundEventSource(event, source);

    logger->Info("SoundRef apply '{}' event={}: rawLen={} bufOk={} orig='{}' -> '{}' set={}", soundrefPath, event, rawLen, bufOk, original, source, ok);
  }
  return matches;
}

bool SoundService::RegisterSoundRefOverride(const std::string& soundrefPath, const std::string& source) {
  if (soundrefPath.empty() || source.empty()) return false;
  {
    std::lock_guard<std::mutex> lock(m_soundRefMutex);
    m_soundRefOverrides.insert_or_assign(soundrefPath, source);
  }
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("SoundRef override registered (map only, no lifecycle) path='{}' source='{}'", soundrefPath, source);
  return true;
}

bool SoundService::SoundRefReplace(const std::string& soundrefPath, const std::string& source) {
  if (soundrefPath.empty() || source.empty()) return false;
  {
    std::lock_guard<std::mutex> lock(m_soundRefMutex);
    m_soundRefOverrides.insert_or_assign(soundrefPath, source);
  }

  std::vector<void*> matches = ApplyOverrideToEvents(soundrefPath, source);
  QueueSoundRefRebinds(matches);

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("SoundRef auto-replace path='{}' source='{}': {} live event(s) queued (stop->activate->resume; bank loads via game VFS activate)",
               soundrefPath, source, matches.size());
  return true;
}

bool SoundService::UnregisterSoundRefOverride(const std::string& soundrefPath) {
  {
    std::lock_guard<std::mutex> lock(m_soundRefMutex);
    if (m_soundRefOverrides.erase(soundrefPath) == 0) return false;
  }

  std::string original;
  {
    std::lock_guard<std::mutex> lock(m_soundRefMutex);
    auto it = m_soundRefOriginals.find(soundrefPath);
    if (it != m_soundRefOriginals.end()) {
      original = it->second;
      m_soundRefOriginals.erase(it);
    }
  }

  size_t restored = 0;
  std::vector<void*> rebinds;
  if (!original.empty()) {
    for (void* event : FindSoundEventsByPath(soundrefPath)) {
      if (!IsSoundEventValid(event)) continue;
      if (GetSoundEventSource(event) != original) SetSoundEventSource(event, original);
      ++restored;
      rebinds.push_back(event);
    }
  }
  QueueSoundRefRebinds(rebinds);

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("SoundRef override unregistered path='{}': restored {} event(s)", soundrefPath, restored);
  if (original.empty()) logger->Warn("SoundRef unregister '{}': no recorded original source", soundrefPath);
  return true;
}

std::unordered_map<std::string, std::string> SoundService::GetSoundRefOverrides() const {
  std::lock_guard<std::mutex> lock(m_soundRefMutex);
  return m_soundRefOverrides;
}

void SoundService::ApplySoundRefOverrides() {
  if (!m_isInitialized) return;
  if (m_soundEventPathOffset == 0 || m_soundEventSourceOffset == 0) return;
  if (!Hooks::GameTools::PrismStringResolver::GetInstance().IsInstalled()) return;

  std::unordered_map<std::string, std::string> overrides;
  {
    std::lock_guard<std::mutex> lock(m_soundRefMutex);
    if (m_soundRefOverrides.empty()) return;
    overrides = m_soundRefOverrides;
  }

  std::vector<void*> all;
  for (const auto& [soundrefPath, source] : overrides) {
    std::vector<void*> matches = ApplyOverrideToEvents(soundrefPath, source);
    all.insert(all.end(), matches.begin(), matches.end());
  }
  QueueSoundRefRebinds(all);
}

void SoundService::ClearSoundRefOverrides() {
  std::unordered_map<std::string, std::string> originals;
  {
    std::lock_guard<std::mutex> lock(m_soundRefMutex);
    if (m_soundRefOriginals.empty() && m_soundRefOverrides.empty()) return;
    originals = std::move(m_soundRefOriginals);
    m_soundRefOriginals.clear();
    m_soundRefOverrides.clear();
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  size_t restored = 0;
  std::vector<void*> rebinds;
  for (const auto& [soundrefPath, original] : originals) {
    if (original.empty()) {
      logger->Warn("SoundRef clear: no recorded original for '{}', skipped", soundrefPath);
      continue;
    }
    for (void* event : FindSoundEventsByPath(soundrefPath)) {
      if (!IsSoundEventValid(event)) continue;
      if (GetSoundEventSource(event) != original) {
        if (SetSoundEventSource(event, original)) ++restored;
      } else {
        ++restored;
      }
      rebinds.push_back(event);
      logger->Info("SoundRef clear: restored path='{}' source='{}'", soundrefPath, original);
    }
  }
  QueueSoundRefRebinds(rebinds);
  logger->Info("SoundRef overrides cleared: {} event(s) restored across {} path(s)", restored, originals.size());
}

}  // namespace SPF::Data::GameData
