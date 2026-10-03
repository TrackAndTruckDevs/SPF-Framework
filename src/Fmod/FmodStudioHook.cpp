#include "SPF/Fmod/FmodStudioHook.hpp"
#include "SPF/Fmod/FmodApi.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "MinHook.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <minwindef.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SPF::Fmod {

namespace {

struct OverrideKey {
  std::string eventPath;
  std::string paramName;
  bool operator==(const OverrideKey& other) const { return eventPath == other.eventPath && paramName == other.paramName; }
};

struct OverrideKeyHash {
  size_t operator()(const OverrideKey& k) const {
    size_t h1 = std::hash<std::string>{}(k.eventPath);
    size_t h2 = std::hash<std::string>{}(k.paramName);
    return h1 ^ (h2 << 1);
  }
};

struct Override3DKey {
  std::string eventPath;
  bool operator==(const Override3DKey& other) const { return eventPath == other.eventPath; }
};

struct Override3DKeyHash {
  size_t operator()(const Override3DKey& k) const { return std::hash<std::string>{}(k.eventPath); }
};

using SetParameterByName_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, const char*, float);
using SetParameterByID_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD_STUDIO_PARAMETER_ID, float);
using Set3DAttributes_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, const FMOD_3D_ATTRIBUTES*);
using SetListenerAttributes_t = FMOD_RESULT (*)(FMOD::Studio::System*, int, const FMOD_3D_ATTRIBUTES*, const FMOD_VECTOR*);
using GetEventDescription_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD::Studio::EventDescription**);
using GetEventPath_t = FMOD_RESULT (*)(FMOD::Studio::EventDescription*, char*, int, int*);
using GetEventID_t = FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD_GUID*);
using GetInstanceSystem_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD::Studio::System**);
using LookupPath_t = FMOD_RESULT (*)(FMOD::Studio::System*, const FMOD_GUID*, char*, int, int*);
using Start_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*);
using Stop_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, FMOD_STUDIO_STOP_MODE);
using CreateInstance_t = FMOD_RESULT (*)(FMOD::Studio::EventDescription*, FMOD::Studio::EventInstance**);
using Release_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*);
using SetPaused_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, bool);
using SetVolume_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, float);
using SetPitch_t = FMOD_RESULT (*)(FMOD::Studio::EventInstance*, float);
using LoadBankFile_t = FMOD_RESULT (*)(FMOD::Studio::System*, const char*, unsigned int, FMOD::Studio::Bank**);
using LoadBankMemory_t = FMOD_RESULT (*)(FMOD::Studio::System*, const char*, int, FMOD_STUDIO_LOAD_MEMORY_MODE, unsigned int, FMOD::Studio::Bank**);
using LoadBankCustom_t = FMOD_RESULT (*)(FMOD::Studio::System*, const FMOD_STUDIO_BANK_INFO*, unsigned int, FMOD::Studio::Bank**);
using BankUnload_t = FMOD_RESULT (*)(FMOD::Studio::Bank*);
using GetBankPath_t = FMOD_RESULT (*)(FMOD::Studio::Bank*, char*, int, int*);
using SystemGetBankCount_t = FMOD_RESULT (*)(FMOD::Studio::System*, int*);
using SystemGetBankList_t = FMOD_RESULT (*)(FMOD::Studio::System*, FMOD::Studio::Bank**, int, int*);

std::mutex s_mutex;
std::unordered_map<OverrideKey, float, OverrideKeyHash> s_parameterOverrides;
std::unordered_map<Override3DKey, Override3DData, Override3DKeyHash> s_3dOverrides;
std::unordered_map<int, OverrideListenerData> s_listenerOverrides;
std::unordered_map<std::string, std::vector<void*>> s_pathToInstances;
std::unordered_map<void*, std::string> s_eventPathCache;
std::unordered_map<void*, std::string> s_descPathCache;
std::unordered_map<uint64_t, std::string> s_paramIdToName;
inline uint64_t ParamIdKey(FMOD_STUDIO_PARAMETER_ID id) { return (static_cast<uint64_t>(id.data1) << 32) | id.data2; }

// Suppression: prefix → suppressed-start attempt count. Instances blocked at
// least once are tracked separately so a re-start of the same instance is
// re-evaluated instead of blindly passed through.
std::unordered_map<std::string, uint64_t> s_suppressionCounts;
std::unordered_set<void*> s_suppressedInstances;
void* s_lastSuppressedInstance = nullptr;
FmodStudioHook::ActivityCallback s_activityCallback = nullptr;
void* s_activityUserData = nullptr;
thread_local bool s_inActivityCallback = false;
FmodStudioHook::PathResolverCallback s_pathResolverCallback = nullptr;
void* s_pathResolverUserData = nullptr;
std::unordered_map<std::string, float> s_volumeOverrides;
std::unordered_map<std::string, float> s_pitchOverrides;
SetParameterByName_t s_trampolineSetParameterByName = nullptr;
SetParameterByID_t s_trampolineSetParameterByID = nullptr;
Set3DAttributes_t s_trampolineSet3DAttributes = nullptr;
SetListenerAttributes_t s_trampolineSetListenerAttributes = nullptr;
Start_t s_trampolineStart = nullptr;
Stop_t s_trampolineStop = nullptr;
CreateInstance_t s_trampolineCreateInstance = nullptr;
Release_t s_trampolineRelease = nullptr;
SetPaused_t s_trampolineSetPaused = nullptr;
SetVolume_t s_trampolineSetVolume = nullptr;
SetPitch_t s_trampolineSetPitch = nullptr;
LoadBankFile_t s_trampolineLoadBankFile = nullptr;
LoadBankMemory_t s_trampolineLoadBankMemory = nullptr;
LoadBankCustom_t s_trampolineLoadBankCustom = nullptr;
BankUnload_t s_trampolineBankUnload = nullptr;
FMOD::Studio::System* s_studioSystem = nullptr;
GetEventDescription_t s_fnGetDescription = nullptr;
GetEventPath_t s_fnGetPath = nullptr;
GetEventID_t s_fnGetEventID = nullptr;
GetInstanceSystem_t s_fnGetInstanceSystem = nullptr;
LookupPath_t s_fnLookupPath = nullptr;
GetBankPath_t s_fnBankGetPath = nullptr;
using BankGetEventCount_t = FMOD_RESULT (*)(FMOD::Studio::Bank*, int*);
using BankGetEventList_t = FMOD_RESULT (*)(FMOD::Studio::Bank*, FMOD::Studio::EventDescription**, int, int*);
BankGetEventCount_t s_fnBankGetEventCount = nullptr;
BankGetEventList_t s_fnBankGetEventList = nullptr;
SystemGetBankCount_t s_fnGetBankCount = nullptr;
SystemGetBankList_t s_fnGetBankList = nullptr;
using GetParamCount_t = FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int*);
using GetParamDescByIndex_t = FMOD_RESULT (*)(FMOD::Studio::EventDescription*, int, FMOD_STUDIO_PARAMETER_DESCRIPTION*);
GetParamCount_t s_fnGetParamCount = nullptr;
GetParamDescByIndex_t s_fnGetParamDescByIndex = nullptr;

void PopulateParamCache(FMOD::Studio::EventDescription* desc, const std::string& path, const std::shared_ptr<Logging::Logger>& logger) {
  if (!desc || !s_fnGetParamCount || !s_fnGetParamDescByIndex || path.empty()) return;
  int numParams = 0;
  if (s_fnGetParamCount(desc, &numParams) == FMOD_OK && numParams > 0) {
    for (int i = 0; i < numParams; ++i) {
      FMOD_STUDIO_PARAMETER_DESCRIPTION pd = {};
      if (s_fnGetParamDescByIndex(desc, i, &pd) == FMOD_OK && pd.name) {
        // Callers (GetEventPathFromInstance) already hold s_mutex — locking
        // again here deadlocks MSVC builds (non-recursive std::mutex).
        s_paramIdToName[ParamIdKey(pd.id)] = pd.name;
      }
    }
  }
}
std::string GetEventPathFromInstance(FMOD::Studio::EventInstance* inst) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
  if (!inst) return {};
  if (!s_fnGetDescription) {
    static bool s_warned = false;
    if (!s_warned) {
      logger->Warn("[PATH] getDescription not resolved — cannot resolve event path");
      s_warned = true;
    }
    return {};
  }

  {
    std::lock_guard lock(s_mutex);
    auto it = s_eventPathCache.find(inst);
    if (it != s_eventPathCache.end()) return it->second;
  }

  FMOD::Studio::EventDescription* desc = nullptr;
  FMOD_RESULT descResult = s_fnGetDescription(inst, &desc);
  if (descResult != FMOD_OK || !desc) {
    static std::atomic<int> s_descFailLogs{0};
    int n = s_descFailLogs.fetch_add(1) + 1;
    if (n <= 10)
      logger->Warn("[PATH] getDescription failed ({}/10): rc={} inst={}", n, (int)descResult,
                   static_cast<const void*>(inst));
    return {};
  }

  {
    std::lock_guard lock(s_mutex);
    auto it = s_descPathCache.find(desc);
    if (it != s_descPathCache.end()) {
      s_eventPathCache[inst] = it->second;
      PopulateParamCache(desc, it->second, logger);
      return it->second;
    }
  }

  // -1 = function pointer not resolved; otherwise the raw FMOD rc. All values
  // feed the failure report at the bottom so a pass-through in Detour_Start
  // names the exact call that broke instead of just "could not be resolved".
  int getPathRc = -1;
  if (s_fnGetPath) {
    char pathBuf[256] = {};
    int retrieved = 0;
    getPathRc = (int)s_fnGetPath(desc, pathBuf, sizeof(pathBuf), &retrieved);
    if (getPathRc == FMOD_OK && pathBuf[0]) {
      std::string path(pathBuf);
      std::lock_guard lock(s_mutex);
      s_eventPathCache[inst] = path;
      s_descPathCache[desc] = path;
      PopulateParamCache(desc, path, logger);
      return path;
    }
  }

  int idRc = -1;
  int sysRc = -1;
  int lookupRc = -1;
  if (s_fnGetEventID && s_fnGetInstanceSystem && s_fnLookupPath) {
    FMOD_GUID guid = {};
    idRc = (int)s_fnGetEventID(desc, &guid);
    if (idRc == FMOD_OK) {
      FMOD::Studio::System* sys = nullptr;
      sysRc = (int)s_fnGetInstanceSystem(inst, &sys);
      if (sysRc == FMOD_OK && sys) {
        char pathBuf[256] = {};
        int retrieved = 0;
        lookupRc = (int)s_fnLookupPath(sys, &guid, pathBuf, sizeof(pathBuf), &retrieved);
        if (lookupRc == FMOD_OK && pathBuf[0]) {
          std::string path(pathBuf);
          std::lock_guard lock(s_mutex);
          s_eventPathCache[inst] = path;
          s_descPathCache[desc] = path;
          PopulateParamCache(desc, path, logger);
          return path;
        }
      }
    }
  }

  // Fallback: FMOD's path table is empty for SCS memory-loaded banks, but the
  // event GUID resolves fine (idRc==0). Ask the registered resolver (normally
  // SoundService backed by game per-bank data) what path this GUID maps to.
  if (s_pathResolverCallback && s_fnGetEventID) {
    FMOD_GUID guid = {};
    if (s_fnGetEventID(desc, &guid) == FMOD_OK) {
      const char* resolved = s_pathResolverCallback(s_pathResolverUserData,
                                                    reinterpret_cast<const uint8_t*>(&guid), inst);
      if (resolved && resolved[0]) {
        std::string path(resolved);
        std::lock_guard lock(s_mutex);
        s_eventPathCache[inst] = path;
        s_descPathCache[desc] = path;
        PopulateParamCache(desc, path, logger);
        logger->Debug("[PATH] resolved via game data: {}", path);
        return path;
      }
    }
  }

  // Total failure: every step's rc in one line — this is the diagnostic
  // companion to Detour_Start's "could NOT be resolved" pass-through.
  // Before the resolver callback is registered (early startup) a miss is
  // expected, not an error — keep it quiet instead of warning.
  static std::atomic<int> s_failLogs{0};
  static std::atomic<int> s_earlyLogs{0};
  if (!s_pathResolverCallback) {
    int m = s_earlyLogs.fetch_add(1) + 1;
    if (m <= 3)
      logger->Debug("[PATH] resolve skipped ({}/3): path resolver not registered yet, inst={}", m,
                    static_cast<const void*>(inst));
    return {};
  }
  int n = s_failLogs.fetch_add(1) + 1;
  if (n <= 10)
    logger->Warn("[PATH] resolve failed ({}/10): getPathRc={} idRc={} sysRc={} lookupRc={} inst={}",
                 n, getPathRc, idRc, sysRc, lookupRc, static_cast<const void*>(inst));

  return {};
}

void FireActivity(int activity, const char* path, void* instance, const char* paramName, float paramValue) {
  FmodStudioHook::ActivityCallback cb = nullptr;
  void* user = nullptr;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    cb = s_activityCallback;
    user = s_activityUserData;
  }
  if (!cb || s_inActivityCallback) return;
  s_inActivityCallback = true;
  cb(user, activity, path ? path : "", instance, paramName, paramValue);
  s_inActivityCallback = false;
}

std::string GetBankPath(FMOD::Studio::Bank* bank) {
  if (!bank || !s_fnBankGetPath) return {};
  char buffer[256] = {};
  int retrieved = 0;
  if (s_fnBankGetPath(bank, buffer, sizeof(buffer), &retrieved) != FMOD_OK || !buffer[0]) return {};
  return buffer;
}

// Warm the desc->path cache at bank-load time so gameplay-time resolutions
// hit the cache instead of walking the game's per-bank lists. Best effort:
// a miss (game bank list not built yet) stays lazy. Runs only when some
// consumer exists — with no overrides/rules/activity registered the paths
// would never be read.
void PrewarmDescPathCache(FMOD::Studio::Bank* bank) {
  if (!bank || !s_fnBankGetEventCount || !s_fnBankGetEventList || !s_fnGetEventID) return;
  FmodStudioHook::PathResolverCallback resolver = nullptr;
  void* resolverUser = nullptr;
  bool consumer = false;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    resolver = s_pathResolverCallback;
    resolverUser = s_pathResolverUserData;
    consumer = s_activityCallback != nullptr || !s_suppressionCounts.empty() ||
               !s_parameterOverrides.empty() || !s_3dOverrides.empty() ||
               !s_volumeOverrides.empty() || !s_pitchOverrides.empty();
  }
  if (!resolver || !consumer) return;
  int count = 0;
  if (s_fnBankGetEventCount(bank, &count) != FMOD_OK || count <= 0) return;
  std::vector<FMOD::Studio::EventDescription*> descs(static_cast<size_t>(count));
  int fetched = 0;
  if (s_fnBankGetEventList(bank, descs.data(), count, &fetched) != FMOD_OK || fetched <= 0) return;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
  int warmed = 0;
  for (int i = 0; i < fetched; ++i) {
    FMOD::Studio::EventDescription* desc = descs[i];
    if (!desc) continue;
    {
      std::lock_guard<std::mutex> lock(s_mutex);
      if (s_descPathCache.find(desc) != s_descPathCache.end()) continue;
    }
    FMOD_GUID guid{};
    if (s_fnGetEventID(desc, &guid) != FMOD_OK) continue;
    // Resolver walks game structures — call it without s_mutex held (same
    // contract as GetEventPathFromInstance), then copy the result.
    const char* resolved = resolver(resolverUser, reinterpret_cast<const uint8_t*>(&guid), nullptr);
    if (!resolved || !resolved[0]) continue;
    std::string path(resolved);
    {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_descPathCache[desc] = path;
      PopulateParamCache(desc, path, logger);
    }
    ++warmed;
  }
  //if (warmed > 0) logger->Debug("[PATH] prewarmed {} event paths at bank load", warmed);
}

// Burst-resolve every already-loaded bank in one pass. Called when a
// suppression rule is armed: at that point bank-load-time prewarm may have
// been skipped (no consumer yet), so the first gameplay starts would walk
// the game's per-bank lists one event at a time. Idempotent — descs already
// in the cache are skipped.
void PrewarmAllLoadedBanks() {
  if (!s_studioSystem || !s_fnGetBankCount || !s_fnGetBankList) return;
  int count = 0;
  if (s_fnGetBankCount(s_studioSystem, &count) != FMOD_OK || count <= 0) return;
  std::vector<FMOD::Studio::Bank*> banks(static_cast<size_t>(count));
  int fetched = 0;
  if (s_fnGetBankList(s_studioSystem, banks.data(), count, &fetched) != FMOD_OK || fetched <= 0) return;
  for (int i = 0; i < fetched; ++i) PrewarmDescPathCache(banks[i]);
}

FMOD_RESULT WINAPI Detour_SetParameterByName(FMOD::Studio::EventInstance* inst, const char* name, float value) {
  auto& hook = FmodStudioHook::GetInstance();
  const bool wantOverride = hook.HasOverrides();
  const bool wantActivity = hook.HasActivityCallback();
  if ((wantOverride || wantActivity) && name && inst) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty()) {
      if (wantOverride) {
        std::lock_guard lock(s_mutex);
        auto it = s_parameterOverrides.find({eventPath, name});
        if (it != s_parameterOverrides.end()) {
          value = it->second;
        }
      }
      if (wantActivity) FireActivity(FmodStudioHook::kActivityParamSet, eventPath.c_str(), inst, name, value);
    }
  }
  return s_trampolineSetParameterByName(inst, name, value);
}

FMOD_RESULT WINAPI Detour_SetParameterByID(FMOD::Studio::EventInstance* inst, FMOD_STUDIO_PARAMETER_ID id, float value) {
  auto& hook = FmodStudioHook::GetInstance();
  const bool wantOverride = hook.HasOverrides();
  const bool wantActivity = hook.HasActivityCallback();
  if ((wantOverride || wantActivity) && inst) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty()) {
      std::string paramName;
      {
        std::lock_guard lock(s_mutex);
        auto idIt = s_paramIdToName.find(ParamIdKey(id));
        if (idIt != s_paramIdToName.end()) {
          paramName = idIt->second;
        }
      }
      if (wantOverride && !paramName.empty()) {
        std::lock_guard lock(s_mutex);
        auto it = s_parameterOverrides.find({eventPath, paramName});
        if (it != s_parameterOverrides.end()) {
          value = it->second;
        }
      }
      if (wantActivity)
        FireActivity(FmodStudioHook::kActivityParamSet, eventPath.c_str(), inst,
                     paramName.empty() ? nullptr : paramName.c_str(), value);
    }
  }
  return s_trampolineSetParameterByID(inst, id, value);
}

FMOD_RESULT WINAPI Detour_Set3DAttributes(FMOD::Studio::EventInstance* inst, const FMOD_3D_ATTRIBUTES* attrs) {
  auto& hook = FmodStudioHook::GetInstance();
  if (hook.HasOverrides() && inst && attrs) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty()) {
      {
        std::lock_guard lock(s_mutex);
        auto& vec = s_pathToInstances[eventPath];
        if (std::find(vec.begin(), vec.end(), inst) == vec.end()) {
          vec.push_back(inst);
        }
      }
      std::lock_guard lock(s_mutex);
      auto it = s_3dOverrides.find({eventPath});
      if (it != s_3dOverrides.end()) {
        if (!it->second.hasOriginal) {
          it->second.original = *attrs;
          it->second.hasOriginal = true;
        }
        return s_trampolineSet3DAttributes(inst, &it->second.replacement);
      }
    }
  }
  return s_trampolineSet3DAttributes(inst, attrs);
}

FMOD_RESULT WINAPI Detour_SetListenerAttributes(FMOD::Studio::System* sys, int index, const FMOD_3D_ATTRIBUTES* attrs, const FMOD_VECTOR* dopplerScale) {
  if (sys) s_studioSystem = sys;
  auto& hook = FmodStudioHook::GetInstance();
  if (hook.HasOverrides() && sys && attrs) {
    std::lock_guard lock(s_mutex);
    auto it = s_listenerOverrides.find(index);
    if (it != s_listenerOverrides.end()) {
      if (!it->second.hasOriginal) {
        it->second.original = *attrs;
        it->second.hasOriginal = true;
      }
      return s_trampolineSetListenerAttributes(sys, index, &it->second.replacement, dopplerScale);
    }
  }
  return s_trampolineSetListenerAttributes(sys, index, attrs, dopplerScale);
}

FMOD_RESULT WINAPI Detour_Start(FMOD::Studio::EventInstance* inst) {
  if (!inst) return s_trampolineStart(inst);
  bool rulesActive = false;
  bool wantActivity = false;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    rulesActive = !s_suppressionCounts.empty();
    wantActivity = s_activityCallback != nullptr;
  }
  // Idle fast path: without suppression rules and an activity callback the
  // resolved path would be discarded — skip the whole resolution chain.
  if (!rulesActive && !wantActivity) return s_trampolineStart(inst);
  std::string eventPath = GetEventPathFromInstance(inst);
  bool suppressed = false;
  uint64_t attempts = 0;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!eventPath.empty()) {
      for (auto& kv : s_suppressionCounts) {
        if (eventPath.rfind(kv.first, 0) == 0) {
          attempts = ++kv.second;
          s_suppressedInstances.insert(inst);
          s_lastSuppressedInstance = inst;
          suppressed = true;
          break;
        }
      }
    }
    if (!suppressed) s_suppressedInstances.erase(inst);
  }
  // A pass-through while suppression is armed is never silent: an unresolved
  // path is an error (rules cannot match what cannot be read); a resolved but
  // unmatched path is logged once per unique path so prefixes can be compared
  // against the real event paths.
  if (rulesActive && !suppressed) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
    if (eventPath.empty()) {
      static std::atomic<int> s_unresolvedLogs{0};
      int n = s_unresolvedLogs.fetch_add(1) + 1;
      if (n <= 10)
        logger->Warn("[Suppression] start passed through: event path could NOT be resolved ({}/10) — rules active but cannot match", n);
    } 
    // else {
    //   static std::mutex s_loggedPathsMutex;
    //   static std::unordered_set<std::string> s_loggedUnmatchedPaths;
    //   bool firstTime = false;
    //   {
    //     std::lock_guard<std::mutex> lk(s_loggedPathsMutex);
    //     firstTime = s_loggedUnmatchedPaths.insert(eventPath).second;
    //   }
    //   if (firstTime) logger->Info("[Suppression] start '{}' not matched by any suppression rule", eventPath);
    // }
  }
  if (suppressed) {
    // Bounded visibility: first attempts of each suppression run at INFO, the
    // per-frame restart storm from a held button stays at DEBUG.
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
    if (attempts <= 3) {
      logger->Info("[FMOD] Suppressed start #{}: {}", attempts, eventPath);
    } else {
      logger->Debug("[FMOD] Suppressed start #{}: {}", attempts, eventPath);
    }
    FireActivity(FmodStudioHook::kActivityStartSuppressed, eventPath.c_str(), inst, nullptr, 0.0f);
    return FMOD_OK;
  }
  FMOD_RESULT result = s_trampolineStart(inst);
  if (result == FMOD_OK && !eventPath.empty())
    FireActivity(FmodStudioHook::kActivityStarted, eventPath.c_str(), inst, nullptr, 0.0f);
  return result;
}

FMOD_RESULT WINAPI Detour_Stop(FMOD::Studio::EventInstance* inst, FMOD_STUDIO_STOP_MODE mode) {
  if (inst && FmodStudioHook::GetInstance().HasActivityCallback()) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty()) FireActivity(FmodStudioHook::kActivityStopped, eventPath.c_str(), inst, nullptr, 0.0f);
  }
  return s_trampolineStop(inst, mode);
}

FMOD_RESULT WINAPI Detour_CreateInstance(FMOD::Studio::EventDescription* desc, FMOD::Studio::EventInstance** instance) {
  FMOD_RESULT result = s_trampolineCreateInstance(desc, instance);
  if (result == FMOD_OK && instance && *instance) {
    {
      // Instance pointers are recycled by FMOD — drop any stale path cached
      // for this address before resolving it fresh.
      std::lock_guard<std::mutex> lock(s_mutex);
      s_eventPathCache.erase(*instance);
    }
    if (FmodStudioHook::GetInstance().HasActivityCallback()) {
      std::string eventPath = GetEventPathFromInstance(*instance);
      FireActivity(FmodStudioHook::kActivityEventCreated, eventPath.c_str(), *instance, nullptr, 0.0f);
    }
  }
  return result;
}

FMOD_RESULT WINAPI Detour_Release(FMOD::Studio::EventInstance* inst) {
  if (inst) {
    bool wantActivity = FmodStudioHook::GetInstance().HasActivityCallback();
    std::string eventPath;
    if (wantActivity) eventPath = GetEventPathFromInstance(inst);
    {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_eventPathCache.erase(inst);
      s_suppressedInstances.erase(inst);
      if (s_lastSuppressedInstance == inst) s_lastSuppressedInstance = nullptr;
    }
    if (wantActivity) FireActivity(FmodStudioHook::kActivityReleased, eventPath.c_str(), inst, nullptr, 0.0f);
  }
  return s_trampolineRelease(inst);
}

FMOD_RESULT WINAPI Detour_SetPaused(FMOD::Studio::EventInstance* inst, bool paused) {
  if (inst && FmodStudioHook::GetInstance().HasActivityCallback()) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty())
      FireActivity(paused ? FmodStudioHook::kActivityPaused : FmodStudioHook::kActivityUnpaused,
                   eventPath.c_str(), inst, nullptr, 0.0f);
  }
  return s_trampolineSetPaused(inst, paused);
}

FMOD_RESULT WINAPI Detour_SetVolume(FMOD::Studio::EventInstance* inst, float volume) {
  bool haveOverrides = false;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    haveOverrides = !s_volumeOverrides.empty();
  }
  if (inst && haveOverrides) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty()) {
      std::lock_guard<std::mutex> lock(s_mutex);
      auto it = s_volumeOverrides.find(eventPath);
      if (it != s_volumeOverrides.end()) volume = it->second;
    }
  }
  return s_trampolineSetVolume(inst, volume);
}

FMOD_RESULT WINAPI Detour_SetPitch(FMOD::Studio::EventInstance* inst, float pitch) {
  bool haveOverrides = false;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    haveOverrides = !s_pitchOverrides.empty();
  }
  if (inst && haveOverrides) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty()) {
      std::lock_guard<std::mutex> lock(s_mutex);
      auto it = s_pitchOverrides.find(eventPath);
      if (it != s_pitchOverrides.end()) pitch = it->second;
    }
  }
  return s_trampolineSetPitch(inst, pitch);
}

FMOD_RESULT WINAPI Detour_LoadBankFile(FMOD::Studio::System* sys, const char* filename, unsigned int flags,
                                        FMOD::Studio::Bank** bank) {
  FMOD_RESULT result = s_trampolineLoadBankFile(sys, filename, flags, bank);
  if (result == FMOD_OK && bank && *bank) {
    PrewarmDescPathCache(*bank);
    std::string bankPath = GetBankPath(*bank);
    if (bankPath.empty() && filename) bankPath = filename;
    FireActivity(FmodStudioHook::kActivityBankLoaded, bankPath.c_str(), nullptr, nullptr, 0.0f);
  }
  return result;
}

FMOD_RESULT WINAPI Detour_LoadBankMemory(FMOD::Studio::System* sys, const char* data, int size,
                                          FMOD_STUDIO_LOAD_MEMORY_MODE mode, unsigned int flags,
                                          FMOD::Studio::Bank** bank) {
  FMOD_RESULT result = s_trampolineLoadBankMemory(sys, data, size, mode, flags, bank);
  if (result == FMOD_OK && bank && *bank) {
    PrewarmDescPathCache(*bank);
    std::string bankPath = GetBankPath(*bank);
    FireActivity(FmodStudioHook::kActivityBankLoaded, bankPath.c_str(), nullptr, nullptr, 0.0f);
  }
  return result;
}

FMOD_RESULT WINAPI Detour_LoadBankCustom(FMOD::Studio::System* sys, const FMOD_STUDIO_BANK_INFO* info,
                                          unsigned int flags, FMOD::Studio::Bank** bank) {
  FMOD_RESULT result = s_trampolineLoadBankCustom(sys, info, flags, bank);
  if (result == FMOD_OK && bank && *bank) {
    PrewarmDescPathCache(*bank);
    std::string bankPath = GetBankPath(*bank);
    FireActivity(FmodStudioHook::kActivityBankLoaded, bankPath.c_str(), nullptr, nullptr, 0.0f);
  }
  return result;
}

FMOD_RESULT WINAPI Detour_BankUnload(FMOD::Studio::Bank* bank) {
  if (bank) {
    std::string bankPath = GetBankPath(bank);
    if (!bankPath.empty()) FireActivity(FmodStudioHook::kActivityBankUnloading, bankPath.c_str(), nullptr, nullptr, 0.0f);
  }
  // EventDescription* pointers die with their bank and FMOD recycles them, so a
  // desc->path cache entry would otherwise resolve a recycled pointer to the old
  // path. Enumerate this bank's descriptions while still valid (before unload)
  // and drop exactly those entries; full clear is only a fallback.
  std::vector<FMOD::Studio::EventDescription*> dyingDescs;
  if (bank && s_fnBankGetEventCount && s_fnBankGetEventList) {
    int count = 0;
    if (s_fnBankGetEventCount(bank, &count) == FMOD_OK && count > 0) {
      dyingDescs.resize(static_cast<size_t>(count));
      int written = 0;
      if (s_fnBankGetEventList(bank, dyingDescs.data(), count, &written) == FMOD_OK) {
        dyingDescs.resize(static_cast<size_t>(written));
      } else {
        dyingDescs.clear();
      }
    }
  }
  FMOD_RESULT result = s_trampolineBankUnload(bank);
  if (result == FMOD_OK) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!dyingDescs.empty()) {
      for (auto* d : dyingDescs) s_descPathCache.erase(d);
    } else {
      s_descPathCache.clear();
    }
  }
  return result;
}

}  // namespace

FmodStudioHook& FmodStudioHook::GetInstance() {
  static FmodStudioHook instance;
  return instance;
}

FmodStudioHook::FmodStudioHook() = default;

bool FmodStudioHook::Install() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");

  if (m_installed) {
    logger->Info("Hook '{}' already installed.", m_displayName);
    return true;
  }

  auto& fmodApi = FmodApi::GetInstance();
  if (!fmodApi.IsReady()) {
    logger->Warn("FmodApi not ready, cannot install '{}'.", m_displayName);
    return false;
  }

  void* addrSetParamByName = fmodApi.Find("EventInstance::setParameterByName");
  void* addrSetParamByID = fmodApi.Find("EventInstance::setParameterByID");
  void* addrSet3D = fmodApi.Find("EventInstance::set3DAttributes");
  void* addrSetListenerAttrs = fmodApi.Find("System::setListenerAttributes");
  s_fnGetDescription = reinterpret_cast<GetEventDescription_t>(fmodApi.Find("EventInstance::getDescription"));
  s_fnGetPath = reinterpret_cast<GetEventPath_t>(fmodApi.Find("EventDescription::getPath"));
  s_fnGetEventID = reinterpret_cast<GetEventID_t>(fmodApi.Find("EventDescription::getID"));
  s_fnGetInstanceSystem = reinterpret_cast<GetInstanceSystem_t>(fmodApi.Find("EventInstance::getSystem"));
  s_fnLookupPath = reinterpret_cast<LookupPath_t>(fmodApi.Find("System::lookupPath"));
  s_fnGetParamCount = reinterpret_cast<GetParamCount_t>(fmodApi.Find("EventDescription::getParameterDescriptionCount"));
  s_fnGetParamDescByIndex = reinterpret_cast<GetParamDescByIndex_t>(fmodApi.Find("EventDescription::getParameterDescriptionByIndex"));
  s_fnBankGetPath = reinterpret_cast<GetBankPath_t>(fmodApi.Find("Bank::getPath"));
  s_fnBankGetEventCount = reinterpret_cast<BankGetEventCount_t>(fmodApi.Find("Bank::getEventCount"));
  s_fnBankGetEventList = reinterpret_cast<BankGetEventList_t>(fmodApi.Find("Bank::getEventList"));
  s_fnGetBankCount = reinterpret_cast<SystemGetBankCount_t>(fmodApi.Find("System::getBankCount"));
  s_fnGetBankList = reinterpret_cast<SystemGetBankList_t>(fmodApi.Find("System::getBankList"));

  if (!addrSetParamByName || !addrSetParamByID || !addrSet3D) {
    logger->Error("Could not find all required FMOD functions for '{}'.", m_displayName);
    return false;
  }

  MH_STATUS createHookParamByName = MH_CreateHook(addrSetParamByName, reinterpret_cast<void*>(&Detour_SetParameterByName), reinterpret_cast<void**>(&s_trampolineSetParameterByName));
  MH_STATUS createHookParamByID = MH_CreateHook(addrSetParamByID, reinterpret_cast<void*>(&Detour_SetParameterByID), reinterpret_cast<void**>(&s_trampolineSetParameterByID));
  MH_STATUS createHookSet3D = MH_CreateHook(addrSet3D, reinterpret_cast<void*>(&Detour_Set3DAttributes), reinterpret_cast<void**>(&s_trampolineSet3DAttributes));
  MH_STATUS createHookListener = addrSetListenerAttrs ? MH_CreateHook(addrSetListenerAttrs, reinterpret_cast<void*>(&Detour_SetListenerAttributes), reinterpret_cast<void**>(&s_trampolineSetListenerAttributes)) : MH_ERROR_FUNCTION_NOT_FOUND;

  if (createHookParamByName != MH_OK || createHookParamByID != MH_OK || createHookSet3D != MH_OK) {
    logger->Error("MH_CreateHook failed for '{}': {} {} {} {}", m_displayName, MH_StatusToString(createHookParamByName), MH_StatusToString(createHookParamByID), MH_StatusToString(createHookSet3D), MH_StatusToString(createHookListener));
    if (createHookParamByName == MH_OK) MH_RemoveHook(addrSetParamByName);
    if (createHookParamByID == MH_OK) MH_RemoveHook(addrSetParamByID);
    if (createHookSet3D == MH_OK) MH_RemoveHook(addrSet3D);
    if (createHookListener == MH_OK) MH_RemoveHook(addrSetListenerAttrs);
    return false;
  }

  m_hookedAddrSetParamByName = reinterpret_cast<uintptr_t>(addrSetParamByName);
  m_hookedAddrSetParamByID = reinterpret_cast<uintptr_t>(addrSetParamByID);
  m_hookedAddrSet3DAttributes = reinterpret_cast<uintptr_t>(addrSet3D);
  if (createHookListener == MH_OK) m_hookedAddrSetListenerAttributes = reinterpret_cast<uintptr_t>(addrSetListenerAttrs);

  MH_EnableHook(addrSetParamByName);
  MH_EnableHook(addrSetParamByID);
  MH_EnableHook(addrSet3D);
  if (createHookListener == MH_OK) MH_EnableHook(addrSetListenerAttrs);

  // Optional interception hooks (suppression / activity / volume / pitch /
  // banks) — best effort: a missing export logs a warning, the required
  // parameter hooks above keep working.
  struct ExtraHookSpec {
    const char* fmodName;
    void* detour;
    void** trampoline;
  };
  const ExtraHookSpec extraHooks[] = {
      {"EventInstance::start", reinterpret_cast<void*>(&Detour_Start), reinterpret_cast<void**>(&s_trampolineStart)},
      {"EventInstance::stop", reinterpret_cast<void*>(&Detour_Stop), reinterpret_cast<void**>(&s_trampolineStop)},
      {"EventDescription::createInstance", reinterpret_cast<void*>(&Detour_CreateInstance), reinterpret_cast<void**>(&s_trampolineCreateInstance)},
      {"EventInstance::release", reinterpret_cast<void*>(&Detour_Release), reinterpret_cast<void**>(&s_trampolineRelease)},
      {"EventInstance::setPaused", reinterpret_cast<void*>(&Detour_SetPaused), reinterpret_cast<void**>(&s_trampolineSetPaused)},
      {"EventInstance::setVolume", reinterpret_cast<void*>(&Detour_SetVolume), reinterpret_cast<void**>(&s_trampolineSetVolume)},
      {"EventInstance::setPitch", reinterpret_cast<void*>(&Detour_SetPitch), reinterpret_cast<void**>(&s_trampolineSetPitch)},
      {"System::loadBankFile", reinterpret_cast<void*>(&Detour_LoadBankFile), reinterpret_cast<void**>(&s_trampolineLoadBankFile)},
      {"System::loadBankMemory", reinterpret_cast<void*>(&Detour_LoadBankMemory), reinterpret_cast<void**>(&s_trampolineLoadBankMemory)},
      {"System::loadBankCustom", reinterpret_cast<void*>(&Detour_LoadBankCustom), reinterpret_cast<void**>(&s_trampolineLoadBankCustom)},
      {"Bank::unload", reinterpret_cast<void*>(&Detour_BankUnload), reinterpret_cast<void**>(&s_trampolineBankUnload)},
  };
  m_extraHookedAddrs.clear();
  int extrasInstalled = 0;
  for (const auto& spec : extraHooks) {
    void* addr = fmodApi.Find(spec.fmodName);
    if (!addr) {
      logger->Warn("[{}] optional hook not found: {}", m_displayName, spec.fmodName);
      continue;
    }
    MH_STATUS status = MH_CreateHook(addr, spec.detour, spec.trampoline);
    if (status != MH_OK) {
      logger->Warn("[{}] MH_CreateHook failed for {}: {}", m_displayName, spec.fmodName, MH_StatusToString(status));
      continue;
    }
    MH_EnableHook(addr);
    m_extraHookedAddrs.push_back(reinterpret_cast<uintptr_t>(addr));
    logger->Info("[{}] hooked {} @ {:#x}", m_displayName, spec.fmodName, reinterpret_cast<uintptr_t>(addr));
    ++extrasInstalled;
  }

  m_installed = true;
  m_isEnabled = true;

  logger->Info("'{}' installed and enabled: setParameterByName={:#x}, setParameterByID={:#x}, set3DAttributes={:#x}, setListenerAttributes={:#x}, interception hooks={}", m_displayName, m_hookedAddrSetParamByName, m_hookedAddrSetParamByID, m_hookedAddrSet3DAttributes, m_hookedAddrSetListenerAttributes, extrasInstalled);
  return true;
}

void FmodStudioHook::Uninstall() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
  if (!m_installed) return;

  if (m_hookedAddrSetParamByName) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByName));
  if (m_hookedAddrSetParamByID) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByID));
  if (m_hookedAddrSet3DAttributes) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSet3DAttributes));
  if (m_hookedAddrSetListenerAttributes) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetListenerAttributes));
  for (uintptr_t addr : m_extraHookedAddrs) MH_DisableHook(reinterpret_cast<LPVOID>(addr));

  m_isEnabled = false;

  std::lock_guard lock(s_mutex);
  s_eventPathCache.clear();
  s_paramIdToName.clear();
  s_parameterOverrides.clear();
  s_3dOverrides.clear();
  s_listenerOverrides.clear();
  s_descPathCache.clear();
  // Value overrides are per-profile state; suppression rules and the activity
  // callback are behavioral registrations that must survive a reinstall.
  s_volumeOverrides.clear();
  s_pitchOverrides.clear();

  logger->Info("'{}' disabled.", m_displayName);
}

void FmodStudioHook::Remove() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
  if (!m_installed) return;

  if (m_hookedAddrSetParamByName) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByName));
  if (m_hookedAddrSetParamByID) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByID));
  if (m_hookedAddrSet3DAttributes) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddrSet3DAttributes));
  if (m_hookedAddrSetListenerAttributes) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddrSetListenerAttributes));
  for (uintptr_t addr : m_extraHookedAddrs) MH_RemoveHook(reinterpret_cast<LPVOID>(addr));
  m_extraHookedAddrs.clear();

  m_hookedAddrSetParamByName = 0;
  m_hookedAddrSetParamByID = 0;
  m_hookedAddrSet3DAttributes = 0;
  m_hookedAddrSetListenerAttributes = 0;
  s_trampolineSetParameterByName = nullptr;
  s_trampolineSetParameterByID = nullptr;
  s_trampolineSet3DAttributes = nullptr;
  s_trampolineSetListenerAttributes = nullptr;
  s_trampolineStart = nullptr;
  s_trampolineStop = nullptr;
  s_trampolineCreateInstance = nullptr;
  s_trampolineRelease = nullptr;
  s_trampolineSetPaused = nullptr;
  s_trampolineSetVolume = nullptr;
  s_trampolineSetPitch = nullptr;
  s_trampolineLoadBankFile = nullptr;
  s_trampolineLoadBankMemory = nullptr;
  s_trampolineLoadBankCustom = nullptr;
  s_trampolineBankUnload = nullptr;
  s_studioSystem = nullptr;
  s_fnBankGetPath = nullptr;
  m_installed = false;
  m_isEnabled = false;

  std::lock_guard lock(s_mutex);
  s_eventPathCache.clear();
  s_descPathCache.clear();
  s_suppressionCounts.clear();
  s_suppressedInstances.clear();
  s_lastSuppressedInstance = nullptr;
  s_activityCallback = nullptr;
  s_activityUserData = nullptr;
  s_volumeOverrides.clear();
  s_pitchOverrides.clear();

  logger->Info("'{}' removed.", m_displayName);
}

void FmodStudioHook::SetEnabled(bool enabled) {
  if (m_isEnabled == enabled) return;
  if (!m_installed) {
    m_isEnabled = enabled;
    return;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");

  if (enabled) {
    if (m_hookedAddrSetParamByName) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByName));
    if (m_hookedAddrSetParamByID) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByID));
    if (m_hookedAddrSet3DAttributes) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddrSet3DAttributes));
    if (m_hookedAddrSetListenerAttributes) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetListenerAttributes));
    for (uintptr_t addr : m_extraHookedAddrs) MH_EnableHook(reinterpret_cast<LPVOID>(addr));
    logger->Info("'{}' enabled.", m_displayName);
  } else {
    if (m_hookedAddrSetParamByName) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByName));
    if (m_hookedAddrSetParamByID) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetParamByID));
    if (m_hookedAddrSet3DAttributes) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSet3DAttributes));
    if (m_hookedAddrSetListenerAttributes) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddrSetListenerAttributes));
    for (uintptr_t addr : m_extraHookedAddrs) MH_DisableHook(reinterpret_cast<LPVOID>(addr));
    logger->Info("'{}' disabled.", m_displayName);
  }
  m_isEnabled = enabled;
}

void FmodStudioHook::OverrideParameter(const std::string& eventPath, const std::string& paramName, float value) {
  std::lock_guard lock(s_mutex);
  s_parameterOverrides[{eventPath, paramName}] = value;
}

void FmodStudioHook::Override3DAttributes(const std::string& eventPath, const FMOD_3D_ATTRIBUTES& attrs) {
  std::lock_guard lock(s_mutex);
  auto& data = s_3dOverrides[{eventPath}];
  if (!data.hasOriginal) {
    data.original = attrs;
    data.hasOriginal = true;
  }
  data.replacement = attrs;
}

void FmodStudioHook::Override3DPosition(const std::string& eventPath, const FMOD_VECTOR& position) {
  std::lock_guard lock(s_mutex);
  auto& data = s_3dOverrides[{eventPath}];
  data.replacement.position = position;
}

void FmodStudioHook::RemoveParameterOverride(const std::string& eventPath, const std::string& paramName) {
  std::lock_guard lock(s_mutex);
  s_parameterOverrides.erase({eventPath, paramName});
}

void FmodStudioHook::Remove3DOverride(const std::string& eventPath) {
  std::lock_guard lock(s_mutex);
  s_3dOverrides.erase({eventPath});
  s_pathToInstances.erase(eventPath);
}

void FmodStudioHook::Reset3DToOriginal(const std::string& eventPath) {
  FMOD_3D_ATTRIBUTES original{};
  bool found = false;
  std::vector<void*> instances;
  {
    std::lock_guard lock(s_mutex);
    auto it = s_3dOverrides.find({eventPath});
    if (it != s_3dOverrides.end() && it->second.hasOriginal) {
      original = it->second.original;
      found = true;
    }
    auto pit = s_pathToInstances.find(eventPath);
    if (pit != s_pathToInstances.end()) {
      instances = pit->second;
    }
    s_3dOverrides.erase({eventPath});
    s_pathToInstances.erase(eventPath);
  }
  if (found && s_trampolineSet3DAttributes) {
    for (auto* instRaw : instances) {
      auto* inst = reinterpret_cast<FMOD::Studio::EventInstance*>(instRaw);
      s_trampolineSet3DAttributes(inst, &original);
    }
  }
}

void FmodStudioHook::OverrideListenerAttributes(int index, const FMOD_3D_ATTRIBUTES& attrs) {
  std::lock_guard lock(s_mutex);
  auto& data = s_listenerOverrides[index];
  if (!data.hasOriginal) {
    data.original = attrs;
    data.hasOriginal = true;
  }
  data.replacement = attrs;
}

void FmodStudioHook::RemoveListenerOverride(int index) {
  std::lock_guard lock(s_mutex);
  s_listenerOverrides.erase(index);
}

void FmodStudioHook::ResetListenerToOriginal(int index) {
  FMOD_3D_ATTRIBUTES original{};
  bool found = false;
  {
    std::lock_guard lock(s_mutex);
    auto it = s_listenerOverrides.find(index);
    if (it != s_listenerOverrides.end() && it->second.hasOriginal) {
      original = it->second.original;
      found = true;
    }
    s_listenerOverrides.erase(index);
  }
  if (found && s_trampolineSetListenerAttributes && s_studioSystem) {
    s_trampolineSetListenerAttributes(s_studioSystem, index, &original, nullptr);
  }
}

void FmodStudioHook::RemoveAllOverrides() {
  std::lock_guard lock(s_mutex);
  s_parameterOverrides.clear();
  s_3dOverrides.clear();
  s_listenerOverrides.clear();
  s_pathToInstances.clear();
  s_eventPathCache.clear();
  s_volumeOverrides.clear();
  s_pitchOverrides.clear();
}

void FmodStudioHook::PopulateDescPathCache(void* desc, const char* path) {
  if (!desc || !path || path[0] == '\0') return;
  std::lock_guard lock(s_mutex);
  s_descPathCache[desc] = path;
}

void FmodStudioHook::ClearDescPathCache() {
  std::lock_guard lock(s_mutex);
  s_descPathCache.clear();
}

bool FmodStudioHook::HasOverrides() const {
  std::lock_guard lock(s_mutex);
  return !s_parameterOverrides.empty() || !s_3dOverrides.empty() || !s_listenerOverrides.empty();
}

bool FmodStudioHook::SuppressEventPlayback(const std::string& pathPrefix) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
  if (pathPrefix.empty()) {
    logger->Warn("SuppressEventPlayback: empty prefix — rejected.");
    return false;
  }
  if (!s_trampolineStart) {
    logger->Warn("EventInstance::start hook unavailable — suppression '{}' not active.", pathPrefix);
    return false;
  }
  {
    std::lock_guard lock(s_mutex);
    s_suppressionCounts.try_emplace(pathPrefix, 0);
  }
  // Arm first (makes the prewarm's consumer check pass), then burst-resolve
  // all loaded banks so gameplay starts hit warm caches instead of walking
  // the game's per-bank lists one event at a time.
  PrewarmAllLoadedBanks();
  logger->Info("Suppression armed: '{}'", pathPrefix);
  return true;
}

bool FmodStudioHook::UnsuppressEventPlayback(const std::string& pathPrefix) {
  std::lock_guard lock(s_mutex);
  return s_suppressionCounts.erase(pathPrefix) > 0;
}

uint64_t FmodStudioHook::GetSuppressedStartCount(const std::string& pathPrefix) const {
  std::lock_guard lock(s_mutex);
  auto it = s_suppressionCounts.find(pathPrefix);
  return it == s_suppressionCounts.end() ? 0 : it->second;
}

void* FmodStudioHook::GetLastSuppressedInstance(const std::string& pathPrefix) const {
  void* candidate = nullptr;
  {
    std::lock_guard lock(s_mutex);
    if (!s_lastSuppressedInstance) return nullptr;
    if (s_suppressedInstances.find(s_lastSuppressedInstance) == s_suppressedInstances.end()) return nullptr;
    candidate = s_lastSuppressedInstance;
  }
  std::string eventPath = GetEventPathFromInstance(reinterpret_cast<FMOD::Studio::EventInstance*>(candidate));
  if (eventPath.empty() || eventPath.rfind(pathPrefix, 0) != 0) return nullptr;
  std::lock_guard lock(s_mutex);
  if (s_suppressedInstances.find(candidate) == s_suppressedInstances.end()) return nullptr;
  return candidate;
}

void FmodStudioHook::SetActivityCallback(ActivityCallback callback, void* userData) {
  std::lock_guard lock(s_mutex);
  s_activityCallback = callback;
  s_activityUserData = userData;
}

bool FmodStudioHook::HasActivityCallback() const {
  std::lock_guard lock(s_mutex);
  return s_activityCallback != nullptr;
}

void FmodStudioHook::SetPathResolverCallback(PathResolverCallback callback, void* userData) {
  std::lock_guard lock(s_mutex);
  s_pathResolverCallback = callback;
  s_pathResolverUserData = userData;
}

bool FmodStudioHook::HasPathResolverCallback() const {
  std::lock_guard lock(s_mutex);
  return s_pathResolverCallback != nullptr;
}

void FmodStudioHook::OverrideEventVolume(const std::string& eventPath, float volume) {
  std::lock_guard lock(s_mutex);
  s_volumeOverrides[eventPath] = volume;
}

void FmodStudioHook::RemoveEventVolumeOverride(const std::string& eventPath) {
  std::lock_guard lock(s_mutex);
  s_volumeOverrides.erase(eventPath);
}

void FmodStudioHook::OverrideEventPitch(const std::string& eventPath, float pitch) {
  std::lock_guard lock(s_mutex);
  s_pitchOverrides[eventPath] = pitch;
}

void FmodStudioHook::RemoveEventPitchOverride(const std::string& eventPath) {
  std::lock_guard lock(s_mutex);
  s_pitchOverrides.erase(eventPath);
}

}  // namespace SPF::Fmod