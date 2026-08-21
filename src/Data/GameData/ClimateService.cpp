#include "SPF/Data/GameData/ClimateService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Hooks/GameTools/ScsNameResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/Vec2.hpp"
#include "SPF/Utils/Vec3.hpp"

#include <cstdint>
#include <cstring>
#include <libloaderapi.h>
#include <memory>
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace Data::GameData {

// ============================================================================
// Singleton / Lifecycle
// ============================================================================

ClimateService::ClimateService() { WorldServiceRegistry::Get().Register(this); }

ClimateService& ClimateService::GetInstance() {
  static ClimateService instance;
  return instance;
}

// ============================================================================
// Initialization / Shutdown / Finders
// ============================================================================

void ClimateService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
  logger->Info("Attempting to initialize ClimateService...");

  RegisterFinders();

  m_isInitialized = false;
  logger->Info("ClimateService initialization finished. Waiting for critical offsets.");
}

void ClimateService::Shutdown() {
  if (m_isInitialized) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
    logger->Info("ClimateService has been shut down.");
    m_isInitialized = false;

    m_updateFnAddr = 0;

    m_weatherModeOffset = 0;
    m_climatePtrOffset = 0;
    m_climateUnitIdOffset = 0;
    m_climateArrayOffset = 0;
    m_climateCountOffset = 0;
    m_weatherBlendFnAddr = 0;
    m_transitionDurationAddr = 0;

    m_lowElevationOffset = 0;
    m_highElevationOffset = 0;
    m_sunDirectionOffset = 0;
    m_temperatureOffset = 0;
    m_skyboxTextureOffset = 0;
    m_skycloudMaskTextureOffset = 0;
    m_lightningMaskOffset = 0;
    m_starsTextureOffset = 0;
    m_mirrorSkyTextureOffset = 0;
    m_ambientOffset = 0;
    m_diffuseOffset = 0;
    m_specularOffset = 0;
    m_envOffset = 0;
    m_envStaticModOffset = 0;
    m_skyColorOffset = 0;
    m_skyBottomColorOffset = 0;
    m_starmapColorOffset = 0;
    m_starsColorOffset = 0;
    m_sunColorOffset = 0;
    m_sunOpacityOffset = 0;
    m_sunHaloColorOffset = 0;
    m_sunShadowStrengthOffset = 0;
    m_moonColorOffset = 0;
    m_moonHaloColorOffset = 0;
    m_moonHaloScaleOffset = 0;
    m_fogColorOffset = 0;
    m_fogColor2Offset = 0;
    m_fogVgradientOffset = 0;
    m_fogOffsetOffset = 0;
    m_fogDensityOffset = 0;
    m_speedCoefOffset = 0;
    m_cloudShadowWeightOffset = 0;
    m_cloudShadowTextureOffset = 0;
    m_cloudShadowAreaSizeOffset = 0;
    m_cloudShadowSpeedOffset = 0;
    m_rainIntensityOffset = 0;
    m_lightningIntensityOffset = 0;
    m_rainMaxWetnessOffset = 0;
    m_rainAdditionalAmbientOffset = 0;
    m_snowIntensityOffset = 0;
    m_snowFlakeSizeRangeOffset = 0;
    m_snowChaosRateOffset = 0;
    m_snowChaosWeightOffset = 0;
    m_snowAdditionalAmbientOffset = 0;
    m_windTypeOffset = 0;
    m_dofStartOffset = 0;
    m_dofTransitionOffset = 0;
    m_dofFilterSizeOffset = 0;
    m_colorBalanceOffset = 0;
    m_colorSaturationOffset = 0;
    m_sunshaftColorOffset = 0;
    m_sunshaftSizeOffset = 0;
    m_lowIntensityMinimumOffset = 0;
    m_lowIntensityMaximumOffset = 0;
    m_lowIntensityColorOffset = 0;
    m_minScaleOffset = 0;
    m_maxScaleOffset = 0;
    m_scaleOverrideOffset = 0;
    m_darkAdaptationSpeedOffset = 0;
    m_brightAdaptationSpeedOffset = 0;
    m_targetGrayOffset = 0;
    m_contrastOffset = 0;
    m_shoulderLengthOffset = 0;
    m_bloomThresholdOffset = 0;
    m_bloomLimitOffset = 0;
    m_bloomIntensityOffset = 0;
    m_bloomStandardDeviationOffset = 0;
    m_stabilityOffset = 0;
    m_weightOffset = 0;

    m_envProfilePtrOffset = 0;
    m_lampsOnElevationOffset = 0;
    m_dayInYearOffset = 0;
    m_summerTimeOffset = 0;
    m_thunderstormProbabilityOffset = 0;

    m_badWeatherFactorPtr = 0;
    m_remainingBadWeatherOffset = 0;
  for (const auto& finder : m_dataFinders) {
    finder->Reset();
  }
  }
}

void ClimateService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::ClimateDataFinder>()); }

uintptr_t ClimateService::ResolveEnvironmentBase() const {
  uintptr_t slot = ManagerCoreService::GetInstance().GetGameplayManagerAddr();
  if (!Utils::PatternFinder::IsValidAddress(slot)) return 0;
  return *(uintptr_t*)slot;
}

bool ClimateService::IsReady() {
  return m_isInitialized && AreAllFindersReady() && ManagerCoreService::GetInstance().IsGameplayManagerReady() &&
         ManagerCoreService::GetInstance().IsEnvObjectOffsetReady();
}

bool ClimateService::IsFinderReady(const char* name) const {
  for (const auto& finder : m_dataFinders) {
    if (strcmp(finder->GetName(), name) == 0) {
      return finder->IsReady();
    }
  }
  return false;
}

bool ClimateService::AreAllFindersReady() const {
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

bool ClimateService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");

  // ClimateService depends on GameplayManager and its Environment Object offset
  // (both resolved by ManagerCoreService). Do not resolve offsets until the
  // manager is available to avoid null dereferences.
  if (!ManagerCoreService::GetInstance().IsGameplayManagerReady() || !ManagerCoreService::GetInstance().IsEnvObjectOffsetReady()) {
    logger->Warn("ClimateService: GameplayManager/EnvObjectOffset not resolved yet. Waiting for ManagerCoreService.");
    return false;
  }

  for (auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (!finder->TryFindOffsets(*this)) {
        return false;
      }
    }
  }

  m_isInitialized = true;
  logger->Info("ClimateService: All offsets found. Service is READY.");
  return true;
}

// ============================================================================
// Climate API — Name / ID Resolution
// ============================================================================

std::string ClimateService::GetCurrentClimateName() {
  if (!m_isInitialized) return "unknown";
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return "unknown";
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return "unknown";

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return "unknown";

  // Climate name UnitID is at m_climateUnitIdOffset
  uint32_t unitId = *(uint32_t*)(climate + m_climateUnitIdOffset);
  return Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(unitId);
}

std::vector<ClimateService::ClimateInfo> ClimateService::GetAvailableClimates() {
  std::vector<ClimateInfo> climates;
  if (!m_isInitialized) return climates;

  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return climates;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env || !m_climateArrayOffset || !m_climateCountOffset) return climates;

  uintptr_t arrayPtr = *(uintptr_t*)(env + m_climateArrayOffset);
  uint64_t count = *(uint64_t*)(env + m_climateCountOffset);

  if (arrayPtr && count > 0 && count < 512) {
    uintptr_t* entries = (uintptr_t*)arrayPtr;
    for (uint64_t i = 0; i < count; ++i) {
      uintptr_t climateObj = entries[i];
      if (climateObj) {
        uint32_t unitId = *(uint32_t*)(climateObj + m_climateUnitIdOffset);
        std::string name = Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(unitId);
        uint64_t shortToken = Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitToken(unitId);

        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
        logger->Info("ClimateService: name {} , shortToken {}.", name, shortToken);

        if (!name.empty()) {
          climates.push_back({name, shortToken});
        }
      }
    }
  }
  return climates;
}

void ClimateService::SetClimate(uint64_t token, bool instant) {
  if (!m_isInitialized || !m_setClimateFnAddr) return;

  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return;

  typedef void(__fastcall * SetClimate_t)(uintptr_t env, uint64_t* pToken, uint8_t instant);
  SetClimate_t fn = (SetClimate_t)m_setClimateFnAddr;

  fn(env, &token, (uint8_t)instant);
}

// ============================================================================
// Sun Profile API
// ============================================================================

int32_t ClimateService::GetActiveSunProfileIndex() {
  if (!m_isInitialized) return -1;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return -1;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return -1;

  int32_t raw0 = (int32_t)*(uint32_t*)(env + m_activeProfileIndexOffset);
  int32_t raw1 = (int32_t)*(uint32_t*)(env + m_nextProfileIndexOffset);
  return (raw0 <= raw1) ? raw0 : raw1;
}

int32_t ClimateService::GetNextSunProfileIndex() {
  if (!m_isInitialized) return -1;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return -1;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return -1;

  int32_t raw0 = (int32_t)*(uint32_t*)(env + m_activeProfileIndexOffset);
  int32_t raw1 = (int32_t)*(uint32_t*)(env + m_nextProfileIndexOffset);
  return (raw0 <= raw1) ? raw1 : raw0;
}

int32_t ClimateService::GetSunProfileCount(bool isBad) {
  uintptr_t container = GetClimateContainer(isBad);
  if (!container) return 0;
  return (int32_t)*(uint64_t*)(container + m_containerCountOffset);
}

std::string ClimateService::GetSunProfileName(int32_t index, bool isBad) {
  if (index < 0) return "unknown";
  uintptr_t container = GetClimateContainer(isBad);
  if (!container) return "unknown";

  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);

  if (profilesArray && (uint64_t)index < count) {
    uintptr_t profile = *(uintptr_t*)(profilesArray + (index * 8));
    if (profile) {
      uint32_t unitId = *(uint32_t*)(profile + m_climateUnitIdOffset);
      return Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(unitId);
    }
  }
  return "none";
}

float ClimateService::GetTransitionProgress() {
  uintptr_t container = GetCurrentClimateContainer();
  if (!container) return 0.0f;

  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);
  if (!profilesArray) return 0.0f;

  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0.0f;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return 0.0f;

  int32_t raw0 = (int32_t)*(uint32_t*)(env + m_activeProfileIndexOffset);
  int32_t raw1 = (int32_t)*(uint32_t*)(env + m_nextProfileIndexOffset);
  if (raw0 < 0 || raw1 < 0 || (uint64_t)raw0 >= count || (uint64_t)raw1 >= count) return 0.0f;

  uintptr_t p0 = *(uintptr_t*)(profilesArray + (raw0 * 8));
  uintptr_t p1 = *(uintptr_t*)(profilesArray + (raw1 * 8));
  if (!p0 || !p1) return 0.0f;

  float elev0 = *(float*)(p0 + m_lowElevationOffset);
  float elev1 = *(float*)(p1 + m_lowElevationOffset);
  float sunAngle = GetSunAngle();

  float result;
  float range = elev1 - elev0;
  if (range != 0.0f)
    result = (sunAngle - elev0) / range;
  else
    result = 0.0f;

  int32_t dir = GetSunDirection(ProfileRef{static_cast<uint64_t>(raw0), false});
  if (dir < 0 && elev0 < elev1)
    result = 1.0f - result;

  if (result < 0.0f) return 0.0f;
  if (result > 1.0f) return 1.0f;
  return result;
}

float ClimateService::GetSunProfileElevation(int32_t index) {
  if (index < 0) return 0.0f;
  uintptr_t container = GetCurrentClimateContainer();
  if (!container) return 0.0f;

  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);
  if (!profilesArray || (uint64_t)index >= count) return 0.0f;

  uintptr_t profile = *(uintptr_t*)(profilesArray + (index * 8));
  if (!profile) return 0.0f;

  return *(float*)(profile + m_highElevationOffset);
}

float ClimateService::GetSunAngle() {
  if (!m_isInitialized) return 0.0f;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0.0f;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return 0.0f;
  return *(float*)(env + m_sunAngleOffset);
}

float ClimateService::GetWeatherBlendProgress() {
  if (!m_isInitialized || !m_weatherBlendFnAddr) return 0.0f;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0.0f;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return 0.0f;
  typedef float(__fastcall * BlendFn)(uintptr_t env);
  return ((BlendFn)m_weatherBlendFnAddr)(env);
}

void ClimateService::SetTransitionDuration(int32_t minutes) {
  if (!m_transitionDurationAddr) return;
  *(uint32_t*)m_transitionDurationAddr = (uint32_t)minutes * 60000 * 1000;
}

void ClimateService::DumpVec3ToLog(intptr_t offset, const char* name) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
  ProfileRef prof = ActiveProfile();
  std::string profileName = GetSunProfileName(static_cast<int32_t>(prof.index), prof.isBad);
  uint64_t varCount = GetProfileCount(offset, prof);
  uint64_t activeVar = GetActiveVariationIndex();
  uint64_t nextVar = GetNextVariationIndex();

  logger->Debug("--- {} Dump ---", name);
  logger->Debug("Container mode: activeProfile={} (idx={}, bad={})", profileName, prof.index, prof.isBad);
  logger->Debug("Variations: {} total, active={}, next={}", varCount, activeVar, nextVar);

  for (uint64_t i = 0; i < varCount; ++i) {
    Utils::Vector3 v = GetProfileVec3ByIndex(offset, prof, i);
    uint32_t hx, hy, hz;
    memcpy(&hx, &v.x, sizeof(hx));
    memcpy(&hy, &v.y, sizeof(hy));
    memcpy(&hz, &v.z, sizeof(hz));
    logger->Debug("{}[{}]: (&{:08x}, &{:08x}, &{:08x})", name, i, hx, hy, hz);
  }

  logger->Debug("--- {} Dump End ---", name);
}

// ============================================================================
// Env Profile API
// ============================================================================

uintptr_t ClimateService::GetEnvProfileData() {
  uintptr_t env = GetEnvObject();
  if (!env || !m_envProfilePtrOffset) return 0;
  return *(uintptr_t*)(env + m_envProfilePtrOffset);
}

float ClimateService::GetLampsOnElevation() {
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_lampsOnElevationOffset) return 0.0f;
  return *(float*)(profData + m_lampsOnElevationOffset);
}

void ClimateService::SetLampsOnElevation(float val) {
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_lampsOnElevationOffset) return;
  *(float*)(profData + m_lampsOnElevationOffset) = val;
  UpdateEnvironment(GetEnvObject());
}

float ClimateService::GetDayInYear() {
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_dayInYearOffset) return 0.0f;
  return *(float*)(profData + m_dayInYearOffset);
}

void ClimateService::SetDayInYear(float val) {
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_dayInYearOffset) return;
  *(float*)(profData + m_dayInYearOffset) = val;
  UpdateEnvironment(GetEnvObject());
}

float ClimateService::GetSummerTime() {
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_summerTimeOffset) return 0.0f;
  return *(float*)(profData + m_summerTimeOffset);
}

void ClimateService::SetSummerTime(float val) {
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_summerTimeOffset) return;
  *(float*)(profData + m_summerTimeOffset) = val;
  UpdateEnvironment(GetEnvObject());
}

float ClimateService::GetThunderstormProbability() {
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_thunderstormProbabilityOffset) return 0.0f;
  return *(float*)(profData + m_thunderstormProbabilityOffset);
}

void ClimateService::SetThunderstormProbability(float val) {
  if (val < 0.0f) val = 0.0f;
  if (val > 1.0f) val = 1.0f;
  uintptr_t profData = GetEnvProfileData();
  if (!profData || !m_thunderstormProbabilityOffset) return;
  *(float*)(profData + m_thunderstormProbabilityOffset) = val;
  UpdateEnvironment(GetEnvObject());
}

// ============================================================================
// Bad Weather Factor & Timer
// ============================================================================

float ClimateService::GetBadWeatherFactor() {
  if (!m_isInitialized || !m_badWeatherFactorPtr) return 0.07f;
  return *(float*)(m_badWeatherFactorPtr + 0x118);
}

void ClimateService::SetBadWeatherFactor(float val) {
  if (!m_isInitialized || !m_badWeatherFactorPtr) return;
  *(float*)(m_badWeatherFactorPtr + 0x118) = val;

  if (!m_setWeatherModeFnAddr) return;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return;

  uint32_t curMode = *(uint32_t*)(env + m_weatherModeOffset);
  int32_t mode = (val <= 0.0f) ? 0 : (val >= 1.0f) ? 1 : (curMode == 0) ? 1 : 0;
  typedef void(__fastcall * ForceWeather_t)(uintptr_t, int32_t, char);
  ((ForceWeather_t)m_setWeatherModeFnAddr)(env, mode, 1);
}

uint32_t ClimateService::GetBadWeatherMode() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return 0;
  return *(uint32_t*)(env + m_nextWeatherModeOffset);
}

float ClimateService::GetRemainingBadWeatherTime() {
  if (!m_isInitialized) return 0.0f;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0.0f;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return 0.0f;
  if (!m_remainingBadWeatherOffset) return 0.0f;
  return *(float*)(env + m_remainingBadWeatherOffset);
}

// ============================================================================
// Weather & Environment — Scalar API
// ============================================================================

int32_t ClimateService::GetWeatherMode() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0;

  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env || !m_weatherModeOffset) return 0;

  return *(int32_t*)(env + m_weatherModeOffset);
}

int32_t ClimateService::GetNextWeatherMode() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env || !m_nextWeatherModeOffset) return 0;
  return *(int32_t*)(env + m_nextWeatherModeOffset);
}

void ClimateService::SetWeatherMode(int32_t mode, bool instant) {  // del ?
  if (!m_isInitialized || !m_setWeatherModeFnAddr) return;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return;
  typedef void(__fastcall * SetWeatherModeFn)(uintptr_t env, int32_t mode, char instant);
  ((SetWeatherModeFn)m_setWeatherModeFnAddr)(env, mode, (char)instant);
}

// ============================================================================
// ProfileRef Helpers
// ============================================================================

ProfileRef ClimateService::ActiveProfile() {
  return {static_cast<uint64_t>(GetActiveSunProfileIndex()), GetWeatherMode() != 0};
}

ProfileRef ClimateService::NextProfile() {
  return {static_cast<uint64_t>(GetNextSunProfileIndex()), GetNextWeatherMode() != 0};
}

ProfileRef ClimateService::Profile(uint64_t index, bool isBad) {
  return {index, isBad};
}

// ============================================================================
// Profile Data Helpers
// ============================================================================

uintptr_t ClimateService::GetEnvObject() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0;
  return *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
}

void ClimateService::UpdateEnvironment(uintptr_t env) {
  if (env && m_updateFnAddr) {
    typedef void(__fastcall* UpdateEnv_t)(uintptr_t);
    ((UpdateEnv_t)m_updateFnAddr)(env);
  }
}

uint64_t ClimateService::GetProfileCount(intptr_t offset, ProfileRef prof) {
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return 0;
  return *(uint64_t*)(profile + offset + 0x10);
}

float* ClimateService::GetProfileArray(intptr_t offset, ProfileRef prof) {
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return nullptr;
  return *(float**)(profile + offset + 8);
}

float ClimateService::GetProfileScalar(intptr_t offset, ProfileRef prof) {
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return 0.0f;
  return *(float*)(profile + offset);
}

void ClimateService::SetProfileScalar(intptr_t offset, ProfileRef prof, float val) {
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  *(float*)(profile + offset) = val;
}

// ── Scalar (rad↔deg) ──────────────────────────────────────────────

float ClimateService::GetLowElevation(ProfileRef prof) {
  float rad = GetProfileScalar(m_lowElevationOffset, prof);
  return rad * (180.0f / 3.14159265358979323846f);
}

void ClimateService::SetLowElevation(ProfileRef prof, float deg) {
  float rad = deg * (3.14159265358979323846f / 180.0f);
  SetProfileScalar(m_lowElevationOffset, prof, rad);
}

float ClimateService::GetHighElevation(ProfileRef prof) {
  float rad = GetProfileScalar(m_highElevationOffset, prof);
  return rad * (180.0f / 3.14159265358979323846f);
}

void ClimateService::SetHighElevation(ProfileRef prof, float deg) {
  float rad = deg * (3.14159265358979323846f / 180.0f);
  SetProfileScalar(m_highElevationOffset, prof, rad);
}

int32_t ClimateService::GetSunDirection(ProfileRef prof) {
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !m_sunDirectionOffset) return 0;
  return *(int32_t*)(profile + m_sunDirectionOffset);
}

void ClimateService::SetSunDirection(ProfileRef prof, int32_t val) {
  if (val < -1 || val > 1) return;
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !m_sunDirectionOffset) return;
  *(int32_t*)(profile + m_sunDirectionOffset) = val;
}

// ── Variant Float ─────────────────────────────────────────────────

float ClimateService::GetProfileFloat(intptr_t offset, ProfileRef prof) {
  float* arr = GetProfileArray(offset, prof);
  if (!arr) return 0.0f;
  uint64_t varIdx = GetActiveVariationIndex();
  uint64_t count = GetProfileCount(offset, prof);
  if (varIdx >= count) return 0.0f;
  return arr[varIdx];
}

float ClimateService::GetProfileFloatByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx) {
  float* arr = GetProfileArray(offset, prof);
  if (!arr || varIdx >= GetProfileCount(offset, prof)) return 0.0f;
  return arr[varIdx];
}

void ClimateService::SetProfileFloat(intptr_t offset, ProfileRef prof, float val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  uint64_t varIdx = GetActiveVariationIndex();
  float* arr = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (arr && varIdx < cnt) arr[varIdx] = val;
  UpdateEnvironment(env);
}

void ClimateService::SetProfileFloatByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, float val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  float* arr = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (arr && varIdx < cnt) arr[varIdx] = val;
  UpdateEnvironment(env);
}

// ── Variant Int32 ─────────────────────────────────────────────────

int32_t ClimateService::GetProfileInt(intptr_t offset, ProfileRef prof) {
  float* arr = GetProfileArray(offset, prof);
  if (!arr) return 0;
  uint64_t varIdx = GetActiveVariationIndex();
  uint64_t count = GetProfileCount(offset, prof);
  if (varIdx >= count) return 0;
  return reinterpret_cast<int32_t*>(arr)[varIdx];
}

int32_t ClimateService::GetProfileIntByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx) {
  float* arr = GetProfileArray(offset, prof);
  if (!arr || varIdx >= GetProfileCount(offset, prof)) return 0;
  return reinterpret_cast<int32_t*>(arr)[varIdx];
}

void ClimateService::SetProfileInt(intptr_t offset, ProfileRef prof, int32_t val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  uint64_t varIdx = GetActiveVariationIndex();
  float* arr = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (arr && varIdx < cnt) reinterpret_cast<int32_t*>(arr)[varIdx] = val;
  UpdateEnvironment(env);
}

void ClimateService::SetProfileIntByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, int32_t val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  float* arr = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (arr && varIdx < cnt) reinterpret_cast<int32_t*>(arr)[varIdx] = val;
  UpdateEnvironment(env);
}

// ── Variant Vec3 ──────────────────────────────────────────────────

Utils::Vector3 ClimateService::GetProfileVec3(intptr_t offset, ProfileRef prof) {
  float* base = GetProfileArray(offset, prof);
  uint64_t varIdx = GetActiveVariationIndex();
  uint64_t count = GetProfileCount(offset, prof);
  if (!base || varIdx >= count) return {};
  float* data = (float*)(base + varIdx * 3);
  return {data[0], data[1], data[2]};
}

Utils::Vector3 ClimateService::GetProfileVec3ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx) {
  float* base = GetProfileArray(offset, prof);
  if (!base || varIdx >= GetProfileCount(offset, prof)) return {};
  float* data = (float*)(base + varIdx * 3);
  return {data[0], data[1], data[2]};
}

void ClimateService::SetProfileVec3(intptr_t offset, ProfileRef prof, const Utils::Vector3& val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  uint64_t varIdx = GetActiveVariationIndex();
  float* base = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (base && varIdx < cnt) {
    float* data = (float*)(base + varIdx * 3);
    data[0] = val.x; data[1] = val.y; data[2] = val.z;
  }
  UpdateEnvironment(env);
}

void ClimateService::SetProfileVec3ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, const Utils::Vector3& val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  float* base = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (base && varIdx < cnt) {
    float* data = (float*)(base + varIdx * 3);
    data[0] = val.x; data[1] = val.y; data[2] = val.z;
  }
  UpdateEnvironment(env);
}

// ── Variant Vec2 ──────────────────────────────────────────────────

Utils::Vec2f ClimateService::GetProfileVec2(intptr_t offset, ProfileRef prof) {
  float* base = GetProfileArray(offset, prof);
  uint64_t varIdx = GetActiveVariationIndex();
  uint64_t count = GetProfileCount(offset, prof);
  if (!base || varIdx >= count) return {};
  float* data = base + varIdx * 2;
  return {data[0], data[1]};
}

Utils::Vec2f ClimateService::GetProfileVec2ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx) {
  float* base = GetProfileArray(offset, prof);
  if (!base || varIdx >= GetProfileCount(offset, prof)) return {};
  float* data = base + varIdx * 2;
  return {data[0], data[1]};
}

void ClimateService::SetProfileVec2(intptr_t offset, ProfileRef prof, const Utils::Vec2f& val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  uint64_t varIdx = GetActiveVariationIndex();
  float* base = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (base && varIdx < cnt) {
    float* data = base + varIdx * 2;
    data[0] = val.x; data[1] = val.y;
  }
  UpdateEnvironment(env);
}

void ClimateService::SetProfileVec2ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, const Utils::Vec2f& val) {
  uintptr_t env = GetEnvObject();
  uintptr_t profile = GetProfilePtr(prof);
  if (!profile || !offset) return;
  float* base = *(float**)(profile + offset + 8);
  uint64_t cnt = *(uint64_t*)(profile + offset + 0x10);
  if (base && varIdx < cnt) {
    float* data = base + varIdx * 2;
    data[0] = val.x; data[1] = val.y;
  }
  UpdateEnvironment(env);
}

// ── Variant Texture ───────────────────────────────────────────────

std::string ClimateService::GetProfileTexture(intptr_t offset, ProfileRef prof) {
  uintptr_t base = (uintptr_t)GetProfileArray(offset, prof);
  uint64_t varIdx = GetActiveVariationIndex();
  uint64_t count = GetProfileCount(offset, prof);
  if (!base || varIdx >= count) return "";
  const char* str = *(const char**)(base + varIdx * 32 + 8);
  return str ? std::string(str) : "";
}

std::string ClimateService::GetProfileTextureByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx) {
  uintptr_t base = (uintptr_t)GetProfileArray(offset, prof);
  if (!base || varIdx >= GetProfileCount(offset, prof)) return "";
  const char* str = *(const char**)(base + varIdx * 32 + 8);
  return str ? std::string(str) : "";
}

void ClimateService::SetProfileTexture(intptr_t offset, ProfileRef prof, const std::string& val) {
  (void)offset;
  (void)prof;
  (void)val;
}

void ClimateService::SetProfileTextureByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, const std::string& val) {
  (void)offset;
  (void)prof;
  (void)varIdx;
  (void)val;
}

// ============================================================================
// Blended Profile Data Helpers
// ============================================================================

float ClimateService::GetBlendedFloat(intptr_t offset) {
  auto activeProf = ActiveProfile();
  auto nextProf = NextProfile();
  float progress = GetTransitionProgress();
  uint64_t activeVar = GetActiveVariationIndex();
  uint64_t nextVar = GetNextVariationIndex();
  float av = GetProfileFloatByIndex(offset, activeProf, activeVar);
  float nv = GetProfileFloatByIndex(offset, nextProf, nextVar);
  return av * (1.0f - progress) + nv * progress;
}

void ClimateService::SetBlendedFloat(intptr_t offset, float blendedVal, float minVal, float maxVal) {
  auto activeProf = ActiveProfile();
  auto nextProf = NextProfile();
  float progress = GetTransitionProgress();
  uint64_t activeVar = GetActiveVariationIndex();
  uint64_t nextVar = GetNextVariationIndex();
  float av = GetProfileFloatByIndex(offset, activeProf, activeVar);
  float nv = GetProfileFloatByIndex(offset, nextProf, nextVar);

  float origBlended = av * (1.0f - progress) + nv * progress;

  float na, nn;
  if (blendedVal <= origBlended) {
    float t = (origBlended - blendedVal) / (origBlended - minVal + 1e-10f);
    na = av * (1.0f - t) + minVal * t;
    nn = nv * (1.0f - t) + minVal * t;
  } else {
    float t = (blendedVal - origBlended) / (maxVal - origBlended + 1e-10f);
    na = av * (1.0f - t) + maxVal * t;
    nn = nv * (1.0f - t) + maxVal * t;
  }

  SetProfileFloatByIndex(offset, activeProf, activeVar, na);
  SetProfileFloatByIndex(offset, nextProf, nextVar, nn);
}

Utils::Vector3 ClimateService::GetBlendedVec3(intptr_t offset) {
  auto activeProf = ActiveProfile();
  auto nextProf = NextProfile();
  float progress = GetTransitionProgress();
  uint64_t activeVar = GetActiveVariationIndex();
  uint64_t nextVar = GetNextVariationIndex();
  Utils::Vector3 av = GetProfileVec3ByIndex(offset, activeProf, activeVar);
  Utils::Vector3 nv = GetProfileVec3ByIndex(offset, nextProf, nextVar);
  return {av.x * (1.0f - progress) + nv.x * progress, av.y * (1.0f - progress) + nv.y * progress, av.z * (1.0f - progress) + nv.z * progress};
}

void ClimateService::SetBlendedVec3(intptr_t offset, const Utils::Vector3& blendedVal, float maxComponent) {
  auto activeProf = ActiveProfile();
  auto nextProf = NextProfile();
  float progress = GetTransitionProgress();
  uint64_t activeVar = GetActiveVariationIndex();
  uint64_t nextVar = GetNextVariationIndex();
  Utils::Vector3 av = GetProfileVec3ByIndex(offset, activeProf, activeVar);
  Utils::Vector3 nv = GetProfileVec3ByIndex(offset, nextProf, nextVar);

  Utils::Vector3 origBlended = {
      av.x * (1.0f - progress) + nv.x * progress,
      av.y * (1.0f - progress) + nv.y * progress,
      av.z * (1.0f - progress) + nv.z * progress};

  auto blendComponent = [&](float cur, float target, float ref) -> float {
    if (target <= ref) {
      float t = (ref - target) / (ref - 0.0f + 1e-10f);
      return cur * (1.0f - t) + 0.0f * t;
    }
    float t = (target - ref) / (maxComponent - ref + 1e-10f);
    return cur * (1.0f - t) + maxComponent * t;
  };

  SetProfileVec3ByIndex(offset, activeProf, activeVar,
      {blendComponent(av.x, blendedVal.x, origBlended.x),
       blendComponent(av.y, blendedVal.y, origBlended.y),
       blendComponent(av.z, blendedVal.z, origBlended.z)});
  SetProfileVec3ByIndex(offset, nextProf, nextVar,
      {blendComponent(nv.x, blendedVal.x, origBlended.x),
       blendComponent(nv.y, blendedVal.y, origBlended.y),
       blendComponent(nv.z, blendedVal.z, origBlended.z)});
}

Utils::Vec2f ClimateService::GetBlendedVec2(intptr_t offset) {
  auto activeProf = ActiveProfile();
  auto nextProf = NextProfile();
  float progress = GetTransitionProgress();
  uint64_t activeVar = GetActiveVariationIndex();
  uint64_t nextVar = GetNextVariationIndex();
  Utils::Vec2f av = GetProfileVec2ByIndex(offset, activeProf, activeVar);
  Utils::Vec2f nv = GetProfileVec2ByIndex(offset, nextProf, nextVar);
  return {av.x * (1.0f - progress) + nv.x * progress, av.y * (1.0f - progress) + nv.y * progress};
}

void ClimateService::SetBlendedVec2(intptr_t offset, const Utils::Vec2f& blendedVal, float maxComponent) {
  auto activeProf = ActiveProfile();
  auto nextProf = NextProfile();
  float progress = GetTransitionProgress();
  uint64_t activeVar = GetActiveVariationIndex();
  uint64_t nextVar = GetNextVariationIndex();
  Utils::Vec2f av = GetProfileVec2ByIndex(offset, activeProf, activeVar);
  Utils::Vec2f nv = GetProfileVec2ByIndex(offset, nextProf, nextVar);

  Utils::Vec2f origBlended = {
      av.x * (1.0f - progress) + nv.x * progress,
      av.y * (1.0f - progress) + nv.y * progress};

  auto blendComponent = [&](float cur, float target, float ref) -> float {
    if (target <= ref) {
      float t = (ref - target) / (ref - 0.0f + 1e-10f);
      return cur * (1.0f - t) + 0.0f * t;
    }
    float t = (target - ref) / (maxComponent - ref + 1e-10f);
    return cur * (1.0f - t) + maxComponent * t;
  };

  SetProfileVec2ByIndex(offset, activeProf, activeVar,
      {blendComponent(av.x, blendedVal.x, origBlended.x),
       blendComponent(av.y, blendedVal.y, origBlended.y)});
  SetProfileVec2ByIndex(offset, nextProf, nextVar,
      {blendComponent(nv.x, blendedVal.x, origBlended.x),
       blendComponent(nv.y, blendedVal.y, origBlended.y)});
}

// ============================================================================
// Variation Index
// ============================================================================

uint64_t ClimateService::GetActiveVariationIndex() {
  if (!m_isInitialized) return 0;
  uintptr_t container = GetCurrentClimateContainer();
  if (!container) return 0;
  uintptr_t varIdxPtr = *(uintptr_t*)(container + 0x30);
  uint64_t varIdxCount = *(uint64_t*)(container + 0x38);
  int32_t profileIdx = GetActiveSunProfileIndex();
  if (varIdxPtr && profileIdx >= 0 && (uint64_t)profileIdx < varIdxCount) {
    return *(uint64_t*)(varIdxPtr + profileIdx * 8);
  }
  return 0;
}

void ClimateService::SetActiveVariationIndex(uint64_t varIdx) {
  if (!m_isInitialized) return;
  uintptr_t container = GetCurrentClimateContainer();
  if (!container) return;
  uintptr_t varIdxPtr = *(uintptr_t*)(container + 0x30);
  uint64_t varIdxCount = *(uint64_t*)(container + 0x38);
  int32_t profileIdx = GetActiveSunProfileIndex();
  if (varIdxPtr && profileIdx >= 0 && (uint64_t)profileIdx < varIdxCount) {
    *(uint64_t*)(varIdxPtr + profileIdx * 8) = varIdx;
  }
}

uint64_t ClimateService::GetNextVariationIndex() {
  if (!m_isInitialized) return 0;
  uintptr_t container = GetClimateContainer(GetNextWeatherMode() != 0);
  if (!container) return 0;
  uintptr_t varIdxPtr = *(uintptr_t*)(container + 0x30);
  uint64_t varIdxCount = *(uint64_t*)(container + 0x38);
  int32_t profileIdx = GetNextSunProfileIndex();
  if (varIdxPtr && profileIdx >= 0 && (uint64_t)profileIdx < varIdxCount) {
    return *(uint64_t*)(varIdxPtr + profileIdx * 8);
  }
  return 0;
}

void ClimateService::SetNextVariationIndex(uint64_t varIdx) {
  if (!m_isInitialized) return;
  uintptr_t container = GetClimateContainer(GetNextWeatherMode() != 0);
  if (!container) return;
  uintptr_t varIdxPtr = *(uintptr_t*)(container + 0x30);
  uint64_t varIdxCount = *(uint64_t*)(container + 0x38);
  int32_t profileIdx = GetNextSunProfileIndex();
  if (varIdxPtr && profileIdx >= 0 && (uint64_t)profileIdx < varIdxCount) {
    *(uint64_t*)(varIdxPtr + profileIdx * 8) = varIdx;
  }
}

// ============================================================================
// Internal Helpers
// ============================================================================

uintptr_t ClimateService::GetClimateContainer(bool isBad) {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return 0;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate || !m_containerNiceOffset || !m_containerBadOffset) return 0;

  return climate + (isBad ? m_containerBadOffset : m_containerNiceOffset);
}

uintptr_t ClimateService::GetCurrentClimateContainer() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0;
  uintptr_t env = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!env) return 0;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate || !m_weatherModeOffset || !m_containerNiceOffset || !m_containerBadOffset) return 0;

  int32_t sel = *(int32_t*)(env + m_weatherModeOffset);
  return climate + (sel ? m_containerBadOffset : m_containerNiceOffset);
}

uintptr_t ClimateService::GetProfilePtr(ProfileRef prof) {
  uintptr_t container = GetClimateContainer(prof.isBad);
  if (!container) return 0;

  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);

  if (profilesArray && prof.index < count) {
    return *(uintptr_t*)(profilesArray + (prof.index * 8));
  }
  return 0;
}

}  // namespace Data::GameData
SPF_NS_END
