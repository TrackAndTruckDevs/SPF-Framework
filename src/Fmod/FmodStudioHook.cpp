#include "SPF/Fmod/FmodStudioHook.hpp"

#include "SPF/Fmod/FmodApi.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include "MinHook.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <minwindef.h>
#include <mutex>
#include <string>
#include <unordered_map>
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

std::mutex s_mutex;
std::unordered_map<OverrideKey, float, OverrideKeyHash> s_parameterOverrides;
std::unordered_map<Override3DKey, Override3DData, Override3DKeyHash> s_3dOverrides;
std::unordered_map<int, OverrideListenerData> s_listenerOverrides;
std::unordered_map<std::string, std::vector<void*>> s_pathToInstances;
std::unordered_map<void*, std::string> s_eventPathCache;
std::unordered_map<void*, std::string> s_descPathCache;
std::unordered_map<uint64_t, std::string> s_paramIdToName;
inline uint64_t ParamIdKey(FMOD_STUDIO_PARAMETER_ID id) { return (static_cast<uint64_t>(id.data1) << 32) | id.data2; }
SetParameterByName_t s_trampolineSetParameterByName = nullptr;
SetParameterByID_t s_trampolineSetParameterByID = nullptr;
Set3DAttributes_t s_trampolineSet3DAttributes = nullptr;
SetListenerAttributes_t s_trampolineSetListenerAttributes = nullptr;
FMOD::Studio::System* s_studioSystem = nullptr;
GetEventDescription_t s_fnGetDescription = nullptr;
GetEventPath_t s_fnGetPath = nullptr;
GetEventID_t s_fnGetEventID = nullptr;
GetInstanceSystem_t s_fnGetInstanceSystem = nullptr;
LookupPath_t s_fnLookupPath = nullptr;
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
        std::lock_guard lock(s_mutex);
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
  auto descResult = s_fnGetDescription(inst, &desc);
  if (descResult != FMOD_OK || !desc) return {};

  {
    std::lock_guard lock(s_mutex);
    auto it = s_descPathCache.find(desc);
    if (it != s_descPathCache.end()) {
      s_eventPathCache[inst] = it->second;
      PopulateParamCache(desc, it->second, logger);
      return it->second;
    }
  }

  if (s_fnGetPath) {
    char pathBuf[256] = {};
    int retrieved = 0;
    if (s_fnGetPath(desc, pathBuf, sizeof(pathBuf), &retrieved) == FMOD_OK && pathBuf[0]) {
      std::string path(pathBuf);
      std::lock_guard lock(s_mutex);
      s_eventPathCache[inst] = path;
      PopulateParamCache(desc, path, logger);
      return path;
    }
  }

  if (s_fnGetEventID && s_fnGetInstanceSystem && s_fnLookupPath) {
    FMOD_GUID guid = {};
    if (s_fnGetEventID(desc, &guid) == FMOD_OK) {
      FMOD::Studio::System* sys = nullptr;
      if (s_fnGetInstanceSystem(inst, &sys) == FMOD_OK && sys) {
        char pathBuf[256] = {};
        int retrieved = 0;
        if (s_fnLookupPath(sys, &guid, pathBuf, sizeof(pathBuf), &retrieved) == FMOD_OK && pathBuf[0]) {
          std::string path(pathBuf);
          std::lock_guard lock(s_mutex);
          s_eventPathCache[inst] = path;
          PopulateParamCache(desc, path, logger);
          return path;
        }
      }
    }
  }

  return {};
}

FMOD_RESULT WINAPI Detour_SetParameterByName(FMOD::Studio::EventInstance* inst, const char* name, float value) {
  auto& hook = FmodStudioHook::GetInstance();
  if (hook.HasOverrides() && name && inst) {
    std::string eventPath = GetEventPathFromInstance(inst);
    if (!eventPath.empty()) {
      std::lock_guard lock(s_mutex);
      auto it = s_parameterOverrides.find({eventPath, name});
      if (it != s_parameterOverrides.end()) {
        value = it->second;
      }
    }
  }
  return s_trampolineSetParameterByName(inst, name, value);
}

FMOD_RESULT WINAPI Detour_SetParameterByID(FMOD::Studio::EventInstance* inst, FMOD_STUDIO_PARAMETER_ID id, float value) {
  auto& hook = FmodStudioHook::GetInstance();
  if (hook.HasOverrides() && inst) {
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
      if (!paramName.empty()) {
        std::lock_guard lock(s_mutex);
        auto it = s_parameterOverrides.find({eventPath, paramName});
        if (it != s_parameterOverrides.end()) {
          value = it->second;
        }
      }
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

  if (!addrSetParamByName || !addrSetParamByID || !addrSet3D) {
    logger->Error("Could not find all required FMOD functions for '{}'.", m_displayName);
    return false;
  }

  MH_STATUS s1 = MH_CreateHook(addrSetParamByName, reinterpret_cast<void*>(&Detour_SetParameterByName), reinterpret_cast<void**>(&s_trampolineSetParameterByName));
  MH_STATUS s2 = MH_CreateHook(addrSetParamByID, reinterpret_cast<void*>(&Detour_SetParameterByID), reinterpret_cast<void**>(&s_trampolineSetParameterByID));
  MH_STATUS s3 = MH_CreateHook(addrSet3D, reinterpret_cast<void*>(&Detour_Set3DAttributes), reinterpret_cast<void**>(&s_trampolineSet3DAttributes));
  MH_STATUS s4 = addrSetListenerAttrs ? MH_CreateHook(addrSetListenerAttrs, reinterpret_cast<void*>(&Detour_SetListenerAttributes), reinterpret_cast<void**>(&s_trampolineSetListenerAttributes)) : MH_ERROR_FUNCTION_NOT_FOUND;

  if (s1 != MH_OK || s2 != MH_OK || s3 != MH_OK) {
    logger->Error("MH_CreateHook failed for '{}': {} {} {}", m_displayName, MH_StatusToString(s1), MH_StatusToString(s2), MH_StatusToString(s3));
    if (s1 == MH_OK) MH_RemoveHook(addrSetParamByName);
    if (s2 == MH_OK) MH_RemoveHook(addrSetParamByID);
    if (s4 == MH_OK) MH_RemoveHook(addrSetListenerAttrs);
    return false;
  }

  m_hookedAddr1 = reinterpret_cast<uintptr_t>(addrSetParamByName);
  m_hookedAddr2 = reinterpret_cast<uintptr_t>(addrSetParamByID);
  m_hookedAddr3 = reinterpret_cast<uintptr_t>(addrSet3D);
  if (s4 == MH_OK) m_hookedAddr4 = reinterpret_cast<uintptr_t>(addrSetListenerAttrs);

  MH_EnableHook(addrSetParamByName);
  MH_EnableHook(addrSetParamByID);
  MH_EnableHook(addrSet3D);
  if (s4 == MH_OK) MH_EnableHook(addrSetListenerAttrs);

  m_installed = true;
  m_isEnabled = true;

  logger->Info("'{}' installed and enabled: setParameterByName={:#x}, setParameterByID={:#x}, set3DAttributes={:#x}, setListenerAttributes={:#x}", m_displayName, m_hookedAddr1, m_hookedAddr2, m_hookedAddr3, m_hookedAddr4);
  return true;
}

void FmodStudioHook::Uninstall() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
  if (!m_installed) return;

  if (m_hookedAddr1) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr1));
  if (m_hookedAddr2) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr2));
  if (m_hookedAddr3) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr3));
  if (m_hookedAddr4) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr4));

  m_isEnabled = false;

  std::lock_guard lock(s_mutex);
  s_eventPathCache.clear();
  s_paramIdToName.clear();
  s_parameterOverrides.clear();
  s_3dOverrides.clear();
  s_listenerOverrides.clear();
  s_descPathCache.clear();

  logger->Info("'{}' disabled.", m_displayName);
}

void FmodStudioHook::Remove() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("FmodStudioHook");
  if (!m_installed) return;

  if (m_hookedAddr1) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddr1));
  if (m_hookedAddr2) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddr2));
  if (m_hookedAddr3) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddr3));
  if (m_hookedAddr4) MH_RemoveHook(reinterpret_cast<LPVOID>(m_hookedAddr4));

  m_hookedAddr1 = 0;
  m_hookedAddr2 = 0;
  m_hookedAddr3 = 0;
  m_hookedAddr4 = 0;
  s_trampolineSetParameterByName = nullptr;
  s_trampolineSetParameterByID = nullptr;
  s_trampolineSet3DAttributes = nullptr;
  s_trampolineSetListenerAttributes = nullptr;
  s_studioSystem = nullptr;
  m_installed = false;
  m_isEnabled = false;

  std::lock_guard lock(s_mutex);
  s_eventPathCache.clear();
  s_descPathCache.clear();

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
    if (m_hookedAddr1) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddr1));
    if (m_hookedAddr2) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddr2));
    if (m_hookedAddr3) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddr3));
    if (m_hookedAddr4) MH_EnableHook(reinterpret_cast<LPVOID>(m_hookedAddr4));
    logger->Info("'{}' enabled.", m_displayName);
  } else {
    if (m_hookedAddr1) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr1));
    if (m_hookedAddr2) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr2));
    if (m_hookedAddr3) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr3));
    if (m_hookedAddr4) MH_DisableHook(reinterpret_cast<LPVOID>(m_hookedAddr4));
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

}  // namespace SPF::Fmod