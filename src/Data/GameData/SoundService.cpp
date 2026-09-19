#include "SPF/Data/GameData/SoundService.hpp"

#include "SPF/Data/GameData/Finders/SoundDataFinder.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Fmod/FmodApi.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <synchapi.h>
#include <unordered_set>
#include <utility>
#include <vector>
#include <windows.h>

namespace SPF::Data::GameData {

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
  m_eventCache.clear();
  m_pluginBanks.clear();
  m_guidToPath.clear();

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
  ResolveFmodFunctions();
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
      int retrieved = 0;
      if (fnGetPath && fnGetPath(descs[i], pathBuf, sizeof(pathBuf), &retrieved) == FMOD_OK && retrieved > 0) {
        ev.eventPath = pathBuf;
      }

      bool bVal = false;
      if (fnIs3DFn && fnIs3DFn(descs[i], &bVal) == FMOD_OK) { ev.is3D = bVal; ev.hasIs3D = true; }
      if (fnIsOneshot && fnIsOneshot(descs[i], &bVal) == FMOD_OK) { ev.isOneshot = bVal; ev.hasIsOneshot = true; }
      if (fnIsStream && fnIsStream(descs[i], &bVal) == FMOD_OK) { ev.isStream = bVal; ev.hasIsStream = true; }
      if (fnIsSnapshot && fnIsSnapshot(descs[i], &bVal) == FMOD_OK) { ev.isSnapshot = bVal; ev.hasIsSnapshot = true; }

      uint32_t len = 0;
      if (fnGetLength && fnGetLength(descs[i], &len) == FMOD_OK) { ev.durationMs = len; ev.hasDuration = true; }

      float minD = 0.0f, maxD = 0.0f;
      if (fnGetMinMax && fnGetMinMax(descs[i], &minD, &maxD) == FMOD_OK) { ev.minDistance = minD; ev.maxDistance = maxD; }

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
              if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_BOOLEAN) up.boolValue = prop.boolvalue != 0;
              else if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_INTEGER) up.intValue = prop.intvalue;
              else if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_FLOAT) up.floatValue = prop.floatvalue;
              else if (prop.type == FMOD_STUDIO_USER_PROPERTY_TYPE_STRING) up.stringValue = prop.stringvalue ? prop.stringvalue : "";
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
              if (fnGetPath(parent, parentPath, sizeof(parentPath), &parentRetrieved) == FMOD_OK && parentRetrieved > 0)
                bus.parentPath = parentPath;
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
    auto updateRc = fnUpdate(studioSys);
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
    fnUpdate(studioSys);
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

    auto parseHex = [](const std::string& s, int pos, int len) -> uint16_t {
      return static_cast<uint16_t>(std::stoul(s.substr(pos, len), nullptr, 16));
    };

    std::array<uint8_t, 16> guid{};
    uint32_t d1 = std::stoul(uuidStr.substr(0, 8), nullptr, 16);
    uint16_t d2 = parseHex(uuidStr, 9, 4);
    uint16_t d3 = parseHex(uuidStr, 14, 4);
    guid[0]  = static_cast<uint8_t>(d1 & 0xFF);
    guid[1]  = static_cast<uint8_t>((d1 >> 8) & 0xFF);
    guid[2]  = static_cast<uint8_t>((d1 >> 16) & 0xFF);
    guid[3]  = static_cast<uint8_t>((d1 >> 24) & 0xFF);
    guid[4]  = static_cast<uint8_t>(d2 & 0xFF);
    guid[5]  = static_cast<uint8_t>((d2 >> 8) & 0xFF);
    guid[6]  = static_cast<uint8_t>(d3 & 0xFF);
    guid[7]  = static_cast<uint8_t>((d3 >> 8) & 0xFF);
    static const int d4Pos[] = {19, 21, 24, 26, 28, 30, 32, 34};
    for (int i = 0; i < 8; ++i) {
      guid[8 + i] = static_cast<uint8_t>(std::stoul(uuidStr.substr(d4Pos[i], 2), nullptr, 16));
    }

    auto pathStart = line.find(' ', closingBrace + 1);
    if (pathStart == std::string::npos) continue;
    ++pathStart;
    m_guidToPath[guid] = line.substr(pathStart);
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
      if (fnBankGetPath(banks[i], path, sizeof(path), &retrieved) == FMOD_OK)
        info.bankPath = path;
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
  int retrieved = 0;
  fn(static_cast<FMOD::Studio::EventDescription*>(desc), outBuffer, bufferSize, &retrieved);
  return retrieved;
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
}  // namespace SPF::Data::GameData
