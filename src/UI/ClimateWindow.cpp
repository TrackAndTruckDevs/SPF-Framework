#include "SPF/UI/ClimateWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/Utils/Vec2.hpp"
#include "SPF/Utils/Vec3.hpp"

#include "fmt/base.h"
#include "fmt/format.h"
#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace UI {
using namespace Localization;

namespace {

struct VarRenderContext {
  uint64_t activeVar;
  uint64_t nextVar;
};

static void RenderBlendedFloat(const char* label, float val, float minVal, float maxVal, const char* fmt, const std::function<void(float)>& setter) {
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
  ImGui::SetNextItemWidth(180);
  if (ImGui::SliderFloat(label, &val, minVal, maxVal, fmt)) {
    setter(val);
  }
  ImGui::PopStyleColor();
}

static void RenderBlendedVec3(const char* label, const Utils::Vector3& val, float maxVal, const char* fmt, const std::function<void(const Utils::Vector3&)>& setter) {
  float v[3] = {val.x, val.y, val.z};
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
  ImGui::SetNextItemWidth(350);
  if (ImGui::SliderFloat3(label, v, 0.0f, maxVal, fmt)) {
    setter({v[0], v[1], v[2]});
  }
  ImGui::SameLine();
  float norm[3] = {v[0] / maxVal, v[1] / maxVal, v[2] / maxVal};
  if (ImGui::ColorEdit3("##c", norm, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
    setter({norm[0] * maxVal, norm[1] * maxVal, norm[2] * maxVal});
  }
  ImGui::PopStyleColor();
}

static void RenderBlendedVec2(const char* label, const Utils::Vec2f& val, float maxVal, const char* fmt, const std::function<void(const Utils::Vec2f&)>& setter) {
  float v[2] = {val.x, val.y};
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
  ImGui::SetNextItemWidth(280);
  if (ImGui::SliderFloat2(label, v, 0.0f, maxVal, fmt)) {
    setter({v[0], v[1]});
  }
  ImGui::PopStyleColor();
}

static void RenderFloatVar(VarRenderContext& ctx, const char* name, uint64_t count, float minVal, float maxVal, const char* fmt, const std::function<float(uint64_t)>& getter, const std::function<void(uint64_t, float)>& setter, const std::function<void()>& renderNext = {},
                           const std::function<float()>& blendedGetter = {}, const std::function<void(float)>& blendedSetter = {}, float blendProgress = -1.0f) {
  if (count == 0) return;
  ImGui::PushID(name);
  if (!ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_None)) {
    ImGui::PopID();
    return;
  }
  auto& loc = LocalizationManager::GetInstance();

  if (blendedGetter && blendedSetter && blendProgress >= 0.0f) {
    auto _bl = fmt::format(fmt::runtime(loc.Get("climate_window.blend_label")), blendProgress * 100.0f);
    RenderBlendedFloat(_bl.c_str(), blendedGetter(), minVal, maxVal, fmt, blendedSetter);
    ImGui::Separator();
  }

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    float v = getter(i);
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? loc.Get("climate_window.active_tag") : "");

    if (i == ctx.activeVar && i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    } else if (i == ctx.activeVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.47f, 0.2f, 1.0f));
    } else if (i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    }

    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderFloat(label.c_str(), &v, minVal, maxVal, fmt)) {
      setter(i, v);
    }

    if (i == ctx.activeVar || i == ctx.nextVar) {
      ImGui::PopStyleColor();
    }
    ImGui::PopID();
  }

  if (renderNext) {
    ImGui::Separator();
    renderNext();
  }
  ImGui::TreePop();
  ImGui::PopID();
}

static void RenderVec3Var(VarRenderContext& ctx, const char* name, uint64_t count, float maxVal, const char* fmt, const std::function<Utils::Vector3(uint64_t)>& getter, const std::function<void(uint64_t, const Utils::Vector3&)>& setter, const std::function<void()>& renderNext = {},
                          const std::function<Utils::Vector3()>& blendedGetter = {}, const std::function<void(const Utils::Vector3&)>& blendedSetter = {}, float blendProgress = -1.0f) {
  if (count == 0) return;
  ImGui::PushID(name);
  if (!ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_None)) {
    ImGui::PopID();
    return;
  }
  auto& loc = LocalizationManager::GetInstance();

  if (blendedGetter && blendedSetter && blendProgress >= 0.0f) {
    auto _bl = fmt::format(fmt::runtime(loc.Get("climate_window.blend_label")), blendProgress * 100.0f);
    RenderBlendedVec3(_bl.c_str(), blendedGetter(), maxVal, fmt, blendedSetter);
    ImGui::Separator();
  }

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    Utils::Vector3 vec = getter(i);
    float arr[3] = {vec.x, vec.y, vec.z};
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? loc.Get("climate_window.active_tag") : "");

    if (i == ctx.activeVar && i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    } else if (i == ctx.activeVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.47f, 0.2f, 1.0f));
    } else if (i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    }

    ImGui::SetNextItemWidth(350);
    if (ImGui::SliderFloat3(label.c_str(), arr, 0.0f, maxVal, fmt)) {
      setter(i, {arr[0], arr[1], arr[2]});
    }

    ImGui::SameLine();
    float norm[3] = {arr[0] / maxVal, arr[1] / maxVal, arr[2] / maxVal};
    if (ImGui::ColorEdit3("##c", norm, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
      setter(i, {norm[0] * maxVal, norm[1] * maxVal, norm[2] * maxVal});
    }

    if (i == ctx.activeVar || i == ctx.nextVar) {
      ImGui::PopStyleColor();
    }
    ImGui::PopID();
  }

  if (renderNext) {
    ImGui::Separator();
    renderNext();
  }
  ImGui::TreePop();
  ImGui::PopID();
}

static void RenderTextureVar(VarRenderContext& ctx, const char* name, uint64_t count, const std::function<std::string(uint64_t)>& getter, const std::function<void(uint64_t, const std::string&)>& setter, const std::function<void()>& renderNext = {}) {
  if (count == 0) return;
  ImGui::PushID(name);
  if (!ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_None)) {
    ImGui::PopID();
    return;
  }
  auto& loc = LocalizationManager::GetInstance();

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    std::string tex = getter(i);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", tex.c_str());
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? loc.Get("climate_window.active_tag") : "");

    if (i == ctx.activeVar && i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    } else if (i == ctx.activeVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.47f, 0.2f, 1.0f));
    } else if (i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    }

    ImGui::SetNextItemWidth(280);
    if (ImGui::InputText(label.c_str(), buf, sizeof(buf))) {
      setter(i, std::string(buf));
    }

    if (i == ctx.activeVar || i == ctx.nextVar) {
      ImGui::PopStyleColor();
    }
    ImGui::PopID();
  }

  if (renderNext) {
    ImGui::Separator();
    renderNext();
  }
  ImGui::TreePop();
  ImGui::PopID();
}

static void RenderIntVar(VarRenderContext& ctx, const char* name, uint64_t count, int32_t minVal, int32_t maxVal, const std::function<int32_t(uint64_t)>& getter, const std::function<void(uint64_t, int32_t)>& setter, const std::function<void()>& renderNext = {}) {
  if (count == 0) return;
  ImGui::PushID(name);
  if (!ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_None)) {
    ImGui::PopID();
    return;
  }
  auto& loc = LocalizationManager::GetInstance();

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    int32_t v = getter(i);
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? loc.Get("climate_window.active_tag") : "");

    if (i == ctx.activeVar && i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    } else if (i == ctx.activeVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.47f, 0.2f, 1.0f));
    } else if (i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    }

    ImGui::SetNextItemWidth(180);
    if (ImGui::SliderInt(label.c_str(), &v, minVal, maxVal)) {
      setter(i, v);
    }

    if (i == ctx.activeVar || i == ctx.nextVar) {
      ImGui::PopStyleColor();
    }
    ImGui::PopID();
  }

  if (renderNext) {
    ImGui::Separator();
    renderNext();
  }
  ImGui::TreePop();
  ImGui::PopID();
}

static void RenderVec2Var(VarRenderContext& ctx, const char* name, uint64_t count, float maxVal, const char* fmt, const std::function<Utils::Vec2f(uint64_t)>& getter, const std::function<void(uint64_t, const Utils::Vec2f&)>& setter, const std::function<void()>& renderNext = {},
                          const std::function<Utils::Vec2f()>& blendedGetter = {}, const std::function<void(const Utils::Vec2f&)>& blendedSetter = {}, float blendProgress = -1.0f) {
  if (count == 0) return;
  ImGui::PushID(name);
  if (!ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_None)) {
    ImGui::PopID();
    return;
  }
  auto& loc = LocalizationManager::GetInstance();

  if (blendedGetter && blendedSetter && blendProgress >= 0.0f) {
    auto _bl = fmt::format(fmt::runtime(loc.Get("climate_window.blend_label")), blendProgress * 100.0f);
    RenderBlendedVec2(_bl.c_str(), blendedGetter(), maxVal, fmt, blendedSetter);
    ImGui::Separator();
  }

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    Utils::Vec2f vec = getter(i);
    float arr[2] = {vec.x, vec.y};
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? loc.Get("climate_window.active_tag") : "");

    if (i == ctx.activeVar && i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    } else if (i == ctx.activeVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.47f, 0.2f, 1.0f));
    } else if (i == ctx.nextVar) {
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
    }

    ImGui::SetNextItemWidth(280);
    if (ImGui::SliderFloat2(label.c_str(), arr, 0.0f, maxVal, fmt)) {
      setter(i, {arr[0], arr[1]});
    }

    if (i == ctx.activeVar || i == ctx.nextVar) {
      ImGui::PopStyleColor();
    }
    ImGui::PopID();
  }

  if (renderNext) {
    ImGui::Separator();
    renderNext();
  }
  ImGui::TreePop();
  ImGui::PopID();
}

// --- RenderNext helpers ---
static void RenderNextFloat(const char* label, float val, float minVal, float maxVal, const char* fmt, const std::function<void(float)>& setter, uint64_t nv) {
  auto& loc = LocalizationManager::GetInstance();
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  char l[64];
  snprintf(l, sizeof(l), loc.Get("climate_window.next_var_fmt").c_str(), nv, label);
  ImGui::SetNextItemWidth(180);
  if (ImGui::SliderFloat(l, &val, minVal, maxVal, fmt)) setter(val);
  ImGui::PopStyleColor();
}

static void RenderNextVec3(const char* label, const Utils::Vector3& val, float maxVal, const char* fmt, const std::function<void(const Utils::Vector3&)>& setter, uint64_t nv) {
  auto& loc = LocalizationManager::GetInstance();
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  float arr[3] = {val.x, val.y, val.z};
  char l[64];
  snprintf(l, sizeof(l), loc.Get("climate_window.next_var_fmt").c_str(), nv, label);
  ImGui::SetNextItemWidth(350);
  if (ImGui::SliderFloat3(l, arr, 0.0f, maxVal, fmt)) setter({arr[0], arr[1], arr[2]});
  ImGui::SameLine();
  float norm[3] = {arr[0] / maxVal, arr[1] / maxVal, arr[2] / maxVal};
  if (ImGui::ColorEdit3("##c", norm, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) setter({norm[0] * maxVal, norm[1] * maxVal, norm[2] * maxVal});
  ImGui::PopStyleColor();
}

static void RenderNextVec2(const char* label, const Utils::Vec2f& val, float maxVal, const char* fmt, const std::function<void(const Utils::Vec2f&)>& setter, uint64_t nv) {
  auto& loc = LocalizationManager::GetInstance();
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  float arr[2] = {val.x, val.y};
  char l[64];
  snprintf(l, sizeof(l), loc.Get("climate_window.next_var_fmt").c_str(), nv, label);
  ImGui::SetNextItemWidth(280);
  if (ImGui::SliderFloat2(l, arr, 0.0f, maxVal, fmt)) setter({arr[0], arr[1]});
  ImGui::PopStyleColor();
}

static void RenderNextInt(const char* label, int32_t val, int32_t minVal, int32_t maxVal, const std::function<void(int32_t)>& setter, uint64_t nv) {
  auto& loc = LocalizationManager::GetInstance();
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  char l[64];
  snprintf(l, sizeof(l), loc.Get("climate_window.next_var_fmt").c_str(), nv, label);
  ImGui::SetNextItemWidth(180);
  if (ImGui::SliderInt(l, &val, minVal, maxVal)) setter(val);
  ImGui::PopStyleColor();
}

static void RenderNextTexture(const char* label, const std::string& val, const std::function<void(const std::string&)>& setter, uint64_t nv) {
  auto& loc = LocalizationManager::GetInstance();
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", val.c_str());
  char l[64];
  snprintf(l, sizeof(l), loc.Get("climate_window.next_var_fmt").c_str(), nv, label);
  ImGui::SetNextItemWidth(280);
  if (ImGui::InputText(l, buf, sizeof(buf))) setter(std::string(buf));
  ImGui::PopStyleColor();
}

}  // anonymous namespace

ClimateWindow::ClimateWindow(const std::string& componentName, const std::string& windowId, Data::GameData::ClimateService& climateService) : BaseWindow(componentName, windowId), m_climateService(climateService) {
  m_titleLocalizationKey = "climate_window.title";
  RefreshLocalization();
}

void ClimateWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locNotReady = loc.Get("climate_window.not_ready");
  m_locSubtitle = loc.Get("climate_window.subtitle");
  m_locSelectClimate = loc.Get("climate_window.select_climate");
  m_locRefreshList = loc.Get("climate_window.refresh_list");
  m_locActiveClimate = loc.Get("climate_window.active_climate");
  m_locTimeAutostep = loc.Get("climate_window.time_autostep");
  m_locBtnStopMinus = loc.Get("climate_window.btn_stop_minus");
  m_locBtnMinus1Min = loc.Get("climate_window.btn_minus_1min");
  m_locBtnStopPlus = loc.Get("climate_window.btn_stop_plus");
  m_locBtnPlus1Min = loc.Get("climate_window.btn_plus_1min");
  m_locSunProfilesCount = loc.Get("climate_window.sun_profiles_count");
  m_locActiveSunProfile = loc.Get("climate_window.active_sun_profile");
  m_locNextSunProfile = loc.Get("climate_window.next_sun_profile");
  m_locTransitionProgress = loc.Get("climate_window.transition_progress");
  m_locActiveElev = loc.Get("climate_window.active_elev");
  m_locNextElev = loc.Get("climate_window.next_elev");
  m_locSunAngle = loc.Get("climate_window.sun_angle");
  m_locNice = loc.Get("climate_window.nice");
  m_locBad = loc.Get("climate_window.bad");
  m_locNextWeather = loc.Get("climate_window.next_weather");
  m_locInterpolatedChange = loc.Get("climate_window.interpolated_change");
  m_locInterpolatedTooltip = loc.Get("climate_window.interpolated_tooltip");
  m_locWeatherMixing = loc.Get("climate_window.weather_mixing");
  m_locNoTransition = loc.Get("climate_window.no_transition");
  m_locDuration = loc.Get("climate_window.duration");
  m_locBadWeatherFactor = loc.Get("climate_window.bad_weather_factor");
  m_locFactor = loc.Get("climate_window.factor");
  m_locSet = loc.Get("climate_window.set");
  m_locBadWeatherMode = loc.Get("climate_window.bad_weather_mode");
  m_locBadWeatherActive = loc.Get("climate_window.bad_weather_active");
  m_locBadWeatherInactive = loc.Get("climate_window.bad_weather_inactive");
  m_locRemainingTime = loc.Get("climate_window.remaining_time");
  m_locVariationStatus = loc.Get("climate_window.variation_status");
  m_locSetActiveVariation = loc.Get("climate_window.set_active_variation");
  m_locSetNextVariation = loc.Get("climate_window.set_next_variation");
  m_locActiveTag = loc.Get("climate_window.active_tag");
  m_locBlendLabel = loc.Get("climate_window.blend_label");
  m_locNextVarFmt = loc.Get("climate_window.next_var_fmt");
  m_locSectionEnvProfile = loc.Get("climate_window.section_env_profile");
  m_locSectionSun = loc.Get("climate_window.section_sun");
  m_locSectionMoonStars = loc.Get("climate_window.section_moon_stars");
  m_locSectionSkyTextures = loc.Get("climate_window.section_sky_textures");
  m_locSectionCloudShadows = loc.Get("climate_window.section_cloud_shadows");
  m_locSectionTemperature = loc.Get("climate_window.section_temperature");
  m_locSectionRainLightning = loc.Get("climate_window.section_rain_lightning");
  m_locSectionSnow = loc.Get("climate_window.section_snow");
  m_locSectionFog = loc.Get("climate_window.section_fog");
  m_locSectionAmbientEnv = loc.Get("climate_window.section_ambient_env");
  m_locSectionPostProcess = loc.Get("climate_window.section_post_process");
  m_locSectionWindBlending = loc.Get("climate_window.section_wind_blending");
  m_locSunLowElevation = loc.Get("climate_window.sun.low_elevation");
  m_locSunHighElevation = loc.Get("climate_window.sun.high_elevation");
  m_locSunDirection = loc.Get("climate_window.sun.direction");
  m_locSunDirectionForward = loc.Get("climate_window.sun.direction_forward");
  m_locSunDirectionZenith = loc.Get("climate_window.sun.direction_zenith");
  m_locSunDirectionBackward = loc.Get("climate_window.sun.direction_backward");
  m_locSunColor = loc.Get("climate_window.sun.color");
  m_locSunOpacity = loc.Get("climate_window.sun.opacity");
  m_locSunHaloColor = loc.Get("climate_window.sun.halo_color");
  m_locSunShadowStrength = loc.Get("climate_window.sun.shadow_strength");
  m_locSunshaftColor = loc.Get("climate_window.sun.shaft_color");
  m_locSunshaftSize = loc.Get("climate_window.sun.shaft_size");
  m_locMoonColor = loc.Get("climate_window.moon.color");
  m_locMoonHaloColor = loc.Get("climate_window.moon.halo_color");
  m_locMoonHaloScale = loc.Get("climate_window.moon.halo_scale");
  m_locStarmapColor = loc.Get("climate_window.moon.starmap_color");
  m_locStarsColor = loc.Get("climate_window.moon.stars_color");
  m_locStarsTexture = loc.Get("climate_window.moon.stars_texture");
  m_locSkyColor = loc.Get("climate_window.sky.color");
  m_locSkyBottomColor = loc.Get("climate_window.sky.bottom_color");
  m_locSkyboxTexture = loc.Get("climate_window.sky.skybox_texture");
  m_locSkycloudMaskTexture = loc.Get("climate_window.sky.skycloud_mask_texture");
  m_locMirrorSkyTexture = loc.Get("climate_window.sky.mirror_sky_texture");
  m_locCloudShadowWeight = loc.Get("climate_window.cloud_shadows.weight");
  m_locCloudShadowTexture = loc.Get("climate_window.cloud_shadows.texture");
  m_locCloudShadowAreaSize = loc.Get("climate_window.cloud_shadows.area_size");
  m_locCloudShadowSpeed = loc.Get("climate_window.cloud_shadows.speed");
  m_locTemperature = loc.Get("climate_window.temperature.label");
  m_locRainIntensity = loc.Get("climate_window.rain_lightning.rain_intensity");
  m_locLightningIntensity = loc.Get("climate_window.rain_lightning.lightning_intensity");
  m_locLightningMask = loc.Get("climate_window.rain_lightning.lightning_mask");
  m_locRainMaxWetness = loc.Get("climate_window.rain_lightning.rain_max_wetness");
  m_locRainAdditionalAmbient = loc.Get("climate_window.rain_lightning.rain_additional_ambient");
  m_locSnowIntensity = loc.Get("climate_window.snow.intensity");
  m_locSnowFlakeSizeRange = loc.Get("climate_window.snow.flake_size_range");
  m_locSnowAdditionalAmbient = loc.Get("climate_window.snow.additional_ambient");
  m_locSnowChaosRate = loc.Get("climate_window.snow.chaos_rate");
  m_locSnowChaosWeight = loc.Get("climate_window.snow.chaos_weight");
  m_locFogColor = loc.Get("climate_window.fog.color");
  m_locFogColor2 = loc.Get("climate_window.fog.color_2");
  m_locFogVgradient = loc.Get("climate_window.fog.vgradient");
  m_locFogOffset = loc.Get("climate_window.fog.offset");
  m_locFogDensity = loc.Get("climate_window.fog.density");
  m_locAmbient = loc.Get("climate_window.ambient_env.ambient");
  m_locDiffuse = loc.Get("climate_window.ambient_env.diffuse");
  m_locSpecular = loc.Get("climate_window.ambient_env.specular");
  m_locEnv = loc.Get("climate_window.ambient_env.env");
  m_locEnvStaticMod = loc.Get("climate_window.ambient_env.env_static_mod");
  m_locToneMapping = loc.Get("climate_window.post_process.tone_mapping");
  m_locContrast = loc.Get("climate_window.post_process.contrast");
  m_locShoulderLength = loc.Get("climate_window.post_process.shoulder_length");
  m_locColorGrading = loc.Get("climate_window.post_process.color_grading");
  m_locColorBalance = loc.Get("climate_window.post_process.color_balance");
  m_locColorSaturation = loc.Get("climate_window.post_process.color_saturation");
  m_locBloom = loc.Get("climate_window.post_process.bloom");
  m_locBloomThreshold = loc.Get("climate_window.post_process.bloom_threshold");
  m_locBloomLimit = loc.Get("climate_window.post_process.bloom_limit");
  m_locBloomIntensity = loc.Get("climate_window.post_process.bloom_intensity");
  m_locBloomStandardDeviation = loc.Get("climate_window.post_process.bloom_standard_deviation");
  m_locDepthOfField = loc.Get("climate_window.post_process.depth_of_field");
  m_locDofStart = loc.Get("climate_window.post_process.dof_start");
  m_locDofTransition = loc.Get("climate_window.post_process.dof_transition");
  m_locDofFilterSize = loc.Get("climate_window.post_process.dof_filter_size");
  m_locEyeAdaptation = loc.Get("climate_window.post_process.eye_adaptation");
  m_locLowIntensityMin = loc.Get("climate_window.post_process.low_intensity_min");
  m_locLowIntensityMax = loc.Get("climate_window.post_process.low_intensity_max");
  m_locLowIntensityColor = loc.Get("climate_window.post_process.low_intensity_color");
  m_locDarkAdaptationSpeed = loc.Get("climate_window.post_process.dark_adaptation_speed");
  m_locBrightAdaptationSpeed = loc.Get("climate_window.post_process.bright_adaptation_speed");
  m_locTargetGray = loc.Get("climate_window.post_process.target_gray");
  m_locExposureScale = loc.Get("climate_window.post_process.exposure_scale");
  m_locMinScale = loc.Get("climate_window.post_process.min_scale");
  m_locMaxScale = loc.Get("climate_window.post_process.max_scale");
  m_locScaleOverride = loc.Get("climate_window.post_process.scale_override");
  m_locWindType = loc.Get("climate_window.wind_blending.wind_type");
  m_locSpeedCoef = loc.Get("climate_window.wind_blending.speed_coef");
  m_locStability = loc.Get("climate_window.wind_blending.stability");
  m_locBlendWeight = loc.Get("climate_window.wind_blending.blend_weight");
  m_locLampsOnElevation = loc.Get("climate_window.env_profile.lamps_on_elevation");
  m_locDayInYear = loc.Get("climate_window.env_profile.day_in_year");
  m_locSummerTime = loc.Get("climate_window.env_profile.summer_time");
  m_locThunderstormProbability = loc.Get("climate_window.env_profile.thunderstorm_probability");
}

void ClimateWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();
  auto& svc = m_climateService;

  if (!svc.IsReady()) {
    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", m_locNotReady.c_str());
    return;
  }

  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", m_locSubtitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  //--- Climate Selection Dropdown ---
  std::string currentClimate = svc.GetCurrentClimateName();

  static std::vector<Data::GameData::ClimateService::ClimateInfo> climateCache;
  if (climateCache.empty() && svc.IsReady()) {
    climateCache = svc.GetAvailableClimates();
  }

  if (!climateCache.empty()) {
    static int selectedIdx = -1;
    std::vector<const char*> items;
    for (size_t i = 0; i < climateCache.size(); ++i) {
      items.push_back(climateCache[i].name.c_str());
      if (selectedIdx == -1 && climateCache[i].name == currentClimate) {
        selectedIdx = (int)i;
      }
    }

    if (ImGui::Combo(m_locSelectClimate.c_str(), &selectedIdx, items.data(), (int)items.size())) {
      if (selectedIdx >= 0 && selectedIdx < (int)climateCache.size()) {
        svc.SetClimate(climateCache[selectedIdx].token, true);
      }
    }
    if (ImGui::Button(m_locRefreshList.c_str())) {
      climateCache = svc.GetAvailableClimates();
    }
  }

  // --- Profile Information ---
  ImGui::Text(m_locActiveClimate.c_str(), currentClimate.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  // --- Time Auto-Step Controls ---
  static bool stepBack = false;
  static bool stepFwd = false;
  ImGui::Text("%s", m_locTimeAutostep.c_str());
  ImGui::SameLine();
  if (ImGui::Button(stepBack ? m_locBtnStopMinus.c_str() : m_locBtnMinus1Min.c_str())) {
    stepBack = !stepBack;
    stepFwd = false;
  }
  ImGui::SameLine();
  if (ImGui::Button(stepFwd ? m_locBtnStopPlus.c_str() : m_locBtnPlus1Min.c_str())) {
    stepFwd = !stepFwd;
    stepBack = false;
  }
  auto& world = Data::GameData::GameWorldService::GetInstance();
  if (stepFwd) {
    world.SetSimulationTime(world.GetSimulationTime() + 1);
  } else if (stepBack) {
    world.SetSimulationTime(world.GetSimulationTime() - 1);
  }
  ImGui::Separator();
  ImGui::Spacing();

  // --- Sun Profile Info ---
  auto activeProf = svc.ActiveProfile();
  int32_t aIdx = svc.GetActiveSunProfileIndex();
  int32_t bIdx = svc.GetNextSunProfileIndex();
  int32_t profileCount = svc.GetSunProfileCount(activeProf.isBad);
  ImGui::Text(m_locSunProfilesCount.c_str(), profileCount);
  ImGui::Text(m_locActiveSunProfile.c_str(), svc.GetSunProfileName(aIdx, activeProf.isBad).c_str(), aIdx);
  ImGui::Text(m_locNextSunProfile.c_str(), svc.GetSunProfileName(bIdx, activeProf.isBad).c_str(), bIdx);
  ImGui::Separator();
  float transitionProgress = svc.GetTransitionProgress();
  ImGui::Text(m_locTransitionProgress.c_str(), transitionProgress * 100.0f);
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text(m_locActiveElev.c_str(), svc.GetSunProfileElevation(aIdx));
  ImGui::Text(m_locNextElev.c_str(), svc.GetSunProfileElevation(bIdx));
  float sunAngle = svc.GetSunAngle();
  ImGui::Text(m_locSunAngle.c_str(), sunAngle, sunAngle * 180.0f / 3.14159265f);
  static float setSunAngle = 0.0f;
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Weather Mode Selection ---
  int32_t currentWeather = svc.GetWeatherMode();
  static bool interpolatedTransition = false;
  const char* weatherNames[] = {m_locNice.c_str(), m_locBad.c_str()};

  // --- Next Weather Mode ---
  int32_t nextWeather = svc.GetNextWeatherMode();
  static int32_t lastNextWeather = -1;
  ImGui::Text(m_locNextWeather.c_str(), weatherNames[nextWeather], nextWeather);
  ImGui::Spacing();

  const bool isNice = (currentWeather == 0);
  const ImVec2 btnSize(150, 0);
  if (isNice) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
  }
  if (ImGui::Button(m_locNice.c_str(), btnSize)) {
    svc.SetWeatherMode(0, !interpolatedTransition);
  }
  if (isNice) {
    ImGui::PopStyleColor();
  }
  ImGui::SameLine();
  const bool isBad = (currentWeather == 1);
  if (isBad) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
  }
  if (ImGui::Button(m_locBad.c_str(), btnSize)) {
    svc.SetWeatherMode(1, !interpolatedTransition);
  }
  if (isBad) {
    ImGui::PopStyleColor();
  }

  ImGui::SameLine();
  if (ImGui::Checkbox(m_locInterpolatedChange.c_str(), &interpolatedTransition)) {
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", m_locInterpolatedTooltip.c_str());
  }
  // --- Weather Blend Progress ---
  ImGui::Spacing();
  float blendProgress = svc.GetWeatherBlendProgress();
  if (blendProgress <= 1.0f) {
    ImGui::Text(m_locWeatherMixing.c_str(), blendProgress * 100.0f);
  } else {
    ImGui::Text("%s", m_locNoTransition.c_str());
  }
  static int durationMinutes = 20;
  if (ImGui::SliderInt(m_locDuration.c_str(), &durationMinutes, 1, 120)) {
    svc.SetTransitionDuration(durationMinutes);
  }
  ImGui::Separator();
  ImGui::Spacing();

  // --- Bad Weather Factor Panel ---
  float current = svc.GetBadWeatherFactor();
  static float s_desired = current;
  ImGui::Text(m_locBadWeatherFactor.c_str(), current);

  ImGui::SetNextItemWidth(260);
  ImGui::SliderFloat(m_locFactor.c_str(), &s_desired, 0.0f, 1.0f, "%.3f");
  ImGui::SameLine();
  if (ImGui::Button(m_locSet.c_str())) {
    svc.SetBadWeatherFactor(s_desired);
  }

  uint32_t badMode = svc.GetBadWeatherMode();
  ImGui::Text(m_locBadWeatherMode.c_str(), badMode ? m_locBadWeatherActive.c_str() : m_locBadWeatherInactive.c_str(), badMode);

  float remainSec = svc.GetRemainingBadWeatherTime();
  uint32_t remainMin = (uint32_t)(remainSec / 60.0f);
  ImGui::Text(m_locRemainingTime.c_str(), remainSec, remainMin);

  ImGui::Separator();
  ImGui::Spacing();

  auto activeProf2 = svc.ActiveProfile();
  auto nextProf2 = svc.NextProfile();
  uint64_t activeVarCount = svc.GetRainIntensityCount(activeProf2);
  uint64_t nextVarCount = svc.GetRainIntensityCount(nextProf2);
  uint64_t activeVar = svc.GetActiveVariationIndex();
  uint64_t nextVar = svc.GetNextVariationIndex();

  static int setActiveVarIdx = (int)activeVar;
  static int setNextVarIdx = (int)nextVar;

  std::string aName = svc.GetSunProfileName(aIdx, activeProf.isBad);
  std::string bName = svc.GetSunProfileName(bIdx, activeProf.isBad);
  ImGui::Text(m_locVariationStatus.c_str(),
      aName.c_str(), activeVar, activeVarCount - 1,
      bName.c_str(), nextVar, nextVarCount - 1);

  ImGui::Spacing();

  auto actLabel = fmt::format(fmt::runtime(m_locSetActiveVariation), aName);
  ImGui::SetNextItemWidth(250);
  ImGui::SliderInt("##setActive", &setActiveVarIdx, 0, (int)(activeVarCount - 1));
  ImGui::SameLine();
  if (ImGui::Button(actLabel.c_str())) {
    svc.SetActiveVariationIndex((uint64_t)setActiveVarIdx);
  }

  auto nextLabel = fmt::format(fmt::runtime(m_locSetNextVariation), bName);
  ImGui::SetNextItemWidth(250);
  ImGui::SliderInt("##setNext", &setNextVarIdx, 0, (int)(nextVarCount - 1));
  ImGui::SameLine();
  if (ImGui::Button(nextLabel.c_str())) {
    svc.SetNextVariationIndex((uint64_t)setNextVarIdx);
  }
  ImGui::Separator();
  ImGui::Spacing();

  // Env Profile
  if (ImGui::CollapsingHeader(m_locSectionEnvProfile.c_str(), ImGuiTreeNodeFlags_None)) {
    {
      float v = svc.GetLampsOnElevation();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat(m_locLampsOnElevation.c_str(), &v, 0.0f, 90.0f, "%.1f°")) svc.SetLampsOnElevation(v);
    }
    {
      float v = svc.GetDayInYear();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat(m_locDayInYear.c_str(), &v, 1.0f, 366.0f, "%.0f")) svc.SetDayInYear(v);
    }
    {
      float v = svc.GetSummerTime();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat(m_locSummerTime.c_str(), &v, 0.0f, 2.0f, "%.1f")) svc.SetSummerTime(v);
    }
    {
      float v = svc.GetThunderstormProbability();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat(m_locThunderstormProbability.c_str(), &v, 0.0f, 1.0f, "%.3f")) svc.SetThunderstormProbability(v);
    }
  }

  ImGui::Separator();
  ImGui::Spacing();

  // ─── Sun Profile Attributes ────────────────────────────────────
  {
    auto nextProf = svc.NextProfile();
    float progress = svc.GetTransitionProgress();
    VarRenderContext actCtx{activeVar, UINT64_MAX};

    // 1. Sun
    if (ImGui::CollapsingHeader(m_locSectionSun.c_str(), ImGuiTreeNodeFlags_None)) {
      {
        float v = svc.GetLowElevation(activeProf);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat(m_locSunLowElevation.c_str(), &v, -90.0f, 90.0f, "%.1f°")) svc.SetLowElevation(activeProf, v);
      }
      {
        float v = svc.GetHighElevation(activeProf);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat(m_locSunHighElevation.c_str(), &v, -90.0f, 90.0f, "%.1f°")) svc.SetHighElevation(activeProf, v);
      }
      {
        int32_t dir = svc.GetSunDirection(activeProf);
        const char* dirNames[] = {m_locSunDirectionForward.c_str(), m_locSunDirectionZenith.c_str(), m_locSunDirectionBackward.c_str()};
        int combo = 1 - dir;
        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo(m_locSunDirection.c_str(), &combo, dirNames, IM_ARRAYSIZE(dirNames))) svc.SetSunDirection(activeProf, 1 - combo);
      }
      RenderVec3Var(actCtx, m_locSunColor.c_str(), svc.GetSunColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSunColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSunColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locSunColor.c_str(), svc.GetSunColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSunColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSunColor(v, 50); },
          progress);
      RenderFloatVar(actCtx, m_locSunOpacity.c_str(), svc.GetSunOpacityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetSunOpacityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSunOpacityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSunOpacity.c_str(), svc.GetSunOpacityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetSunOpacityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunOpacity(); },
          [&](float v) { svc.SetBlendedSunOpacity(v, 0, 1); },
          progress);
      RenderVec3Var(actCtx, m_locSunHaloColor.c_str(), svc.GetSunHaloColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSunHaloColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSunHaloColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locSunHaloColor.c_str(), svc.GetSunHaloColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSunHaloColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunHaloColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSunHaloColor(v, 50); },
          progress);
      RenderFloatVar(actCtx, m_locSunShadowStrength.c_str(), svc.GetSunShadowStrengthCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetSunShadowStrengthByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSunShadowStrengthByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSunShadowStrength.c_str(), svc.GetSunShadowStrengthByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetSunShadowStrengthByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunShadowStrength(); },
          [&](float v) { svc.SetBlendedSunShadowStrength(v, 0, 1); },
          progress);
      RenderVec3Var(actCtx, m_locSunshaftColor.c_str(), svc.GetSunshaftColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSunshaftColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSunshaftColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locSunshaftColor.c_str(), svc.GetSunshaftColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSunshaftColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunshaftColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSunshaftColor(v, 50); },
          progress);
      RenderFloatVar(actCtx, m_locSunshaftSize.c_str(), svc.GetSunshaftSizeCount(activeProf), 0, 100, "%.3f",
          [&](uint64_t i) { return svc.GetSunshaftSizeByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSunshaftSizeByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSunshaftSize.c_str(), svc.GetSunshaftSizeByIndex(nextProf, nextVar), 0, 100, "%.3f",
                   [&](float v) { svc.SetSunshaftSizeByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunshaftSize(); },
          [&](float v) { svc.SetBlendedSunshaftSize(v, 0, 100); },
          progress);
    }

    // 2. Moon & Stars
    if (ImGui::CollapsingHeader(m_locSectionMoonStars.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, m_locMoonColor.c_str(), svc.GetMoonColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetMoonColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetMoonColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locMoonColor.c_str(), svc.GetMoonColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetMoonColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMoonColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedMoonColor(v, 1); },
          progress);
      RenderVec3Var(actCtx, m_locMoonHaloColor.c_str(), svc.GetMoonHaloColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetMoonHaloColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetMoonHaloColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locMoonHaloColor.c_str(), svc.GetMoonHaloColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetMoonHaloColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMoonHaloColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedMoonHaloColor(v, 1); },
          progress);
      RenderFloatVar(actCtx, m_locMoonHaloScale.c_str(), svc.GetMoonHaloScaleCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetMoonHaloScaleByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetMoonHaloScaleByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locMoonHaloScale.c_str(), svc.GetMoonHaloScaleByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetMoonHaloScaleByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMoonHaloScale(); },
          [&](float v) { svc.SetBlendedMoonHaloScale(v, 0, 1); },
          progress);
      RenderVec3Var(actCtx, m_locStarmapColor.c_str(), svc.GetStarmapColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetStarmapColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetStarmapColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locStarmapColor.c_str(), svc.GetStarmapColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetStarmapColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedStarmapColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedStarmapColor(v, 1); },
          progress);
      RenderVec3Var(actCtx, m_locStarsColor.c_str(), svc.GetStarsColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetStarsColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetStarsColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locStarsColor.c_str(), svc.GetStarsColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetStarsColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedStarsColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedStarsColor(v, 1); },
          progress);
      RenderTextureVar(actCtx, m_locStarsTexture.c_str(), svc.GetStarsTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetStarsTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetStarsTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture(m_locStarsTexture.c_str(), svc.GetStarsTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetStarsTextureByIndex(nextProf, nextVar, v); }, nextVar); });
    }

    // 3. Sky & Textures
    if (ImGui::CollapsingHeader(m_locSectionSkyTextures.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, m_locSkyColor.c_str(), svc.GetSkyColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSkyColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSkyColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locSkyColor.c_str(), svc.GetSkyColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSkyColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSkyColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSkyColor(v, 50); },
          progress);
      RenderVec3Var(actCtx, m_locSkyBottomColor.c_str(), svc.GetSkyBottomColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSkyBottomColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSkyBottomColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locSkyBottomColor.c_str(), svc.GetSkyBottomColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSkyBottomColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSkyBottomColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSkyBottomColor(v, 50); },
          progress);
      RenderTextureVar(actCtx, m_locSkyboxTexture.c_str(), svc.GetSkyboxTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetSkyboxTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetSkyboxTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture(m_locSkyboxTexture.c_str(), svc.GetSkyboxTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetSkyboxTextureByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderTextureVar(actCtx, m_locSkycloudMaskTexture.c_str(), svc.GetSkycloudMaskTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetSkycloudMaskTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetSkycloudMaskTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture(m_locSkycloudMaskTexture.c_str(), svc.GetSkycloudMaskTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetSkycloudMaskTextureByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderFloatVar(actCtx, m_locMirrorSkyTexture.c_str(), svc.GetMirrorSkyTextureCount(activeProf), 0, 1, "%.0f",
          [&](uint64_t i) { return svc.GetMirrorSkyTextureByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetMirrorSkyTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locMirrorSkyTexture.c_str(), svc.GetMirrorSkyTextureByIndex(nextProf, nextVar), 0, 1, "%.0f",
                   [&](float v) { svc.SetMirrorSkyTextureByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMirrorSkyTexture(); },
          [&](float v) { svc.SetBlendedMirrorSkyTexture(v, 0, 1); },
          progress);
    }

    // 4. Cloud Shadows
    if (ImGui::CollapsingHeader(m_locSectionCloudShadows.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, m_locCloudShadowWeight.c_str(), svc.GetCloudShadowWeightCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetCloudShadowWeightByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetCloudShadowWeightByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locCloudShadowWeight.c_str(), svc.GetCloudShadowWeightByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetCloudShadowWeightByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedCloudShadowWeight(); },
          [&](float v) { svc.SetBlendedCloudShadowWeight(v, 0, 1); },
          progress);
      RenderTextureVar(actCtx, m_locCloudShadowTexture.c_str(), svc.GetCloudShadowTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetCloudShadowTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetCloudShadowTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture(m_locCloudShadowTexture.c_str(), svc.GetCloudShadowTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetCloudShadowTextureByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderVec2Var(actCtx, m_locCloudShadowAreaSize.c_str(), svc.GetCloudShadowAreaSizeCount(activeProf), 2000, "%.1f",
          [&](uint64_t i) { return svc.GetCloudShadowAreaSizeByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vec2f& v) { svc.SetCloudShadowAreaSizeByIndex(activeProf, i, v); },
          [&]() { RenderNextVec2(m_locCloudShadowAreaSize.c_str(), svc.GetCloudShadowAreaSizeByIndex(nextProf, nextVar), 2000, "%.1f",
                   [&](const Utils::Vec2f& v) { svc.SetCloudShadowAreaSizeByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedCloudShadowAreaSize(); },
          [&](const Utils::Vec2f& v) { svc.SetBlendedCloudShadowAreaSize(v, 2000); },
          progress);
      RenderVec2Var(actCtx, m_locCloudShadowSpeed.c_str(), svc.GetCloudShadowSpeedCount(activeProf), 100, "%.1f",
          [&](uint64_t i) { return svc.GetCloudShadowSpeedByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vec2f& v) { svc.SetCloudShadowSpeedByIndex(activeProf, i, v); },
          [&]() { RenderNextVec2(m_locCloudShadowSpeed.c_str(), svc.GetCloudShadowSpeedByIndex(nextProf, nextVar), 100, "%.1f",
                   [&](const Utils::Vec2f& v) { svc.SetCloudShadowSpeedByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedCloudShadowSpeed(); },
          [&](const Utils::Vec2f& v) { svc.SetBlendedCloudShadowSpeed(v, 100); },
          progress);
    }

    // 5. Temperature
    if (ImGui::CollapsingHeader(m_locSectionTemperature.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, m_locTemperature.c_str(), svc.GetTemperatureCount(activeProf), -50, 100, "%.1f°C",
          [&](uint64_t i) { return svc.GetTemperatureByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetTemperatureByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locTemperature.c_str(), svc.GetTemperatureByIndex(nextProf, nextVar), -50, 100, "%.1f°C",
                   [&](float v) { svc.SetTemperatureByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedTemperature(); },
          [&](float v) { svc.SetBlendedTemperature(v, -50, 100); },
          progress);
    }

    // 6. Rain & Lightning
    if (ImGui::CollapsingHeader(m_locSectionRainLightning.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, m_locRainIntensity.c_str(), svc.GetRainIntensityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetRainIntensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetRainIntensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locRainIntensity.c_str(), svc.GetRainIntensityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetRainIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedRainIntensity(); },
          [&](float v) { svc.SetBlendedRainIntensity(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, m_locLightningIntensity.c_str(), svc.GetLightningIntensityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetLightningIntensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetLightningIntensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locLightningIntensity.c_str(), svc.GetLightningIntensityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetLightningIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedLightningIntensity(); },
          [&](float v) { svc.SetBlendedLightningIntensity(v, 0, 1); },
          progress);
      RenderTextureVar(actCtx, m_locLightningMask.c_str(), svc.GetLightningMaskCount(activeProf),
          [&](uint64_t i) { return svc.GetLightningMaskByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetLightningMaskByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture(m_locLightningMask.c_str(), svc.GetLightningMaskByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetLightningMaskByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderFloatVar(actCtx, m_locRainMaxWetness.c_str(), svc.GetRainMaxWetnessCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetRainMaxWetnessByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetRainMaxWetnessByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locRainMaxWetness.c_str(), svc.GetRainMaxWetnessByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetRainMaxWetnessByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedRainMaxWetness(); },
          [&](float v) { svc.SetBlendedRainMaxWetness(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, m_locRainAdditionalAmbient.c_str(), svc.GetRainAdditionalAmbientCount(activeProf), 0, 20, "%.1f",
          [&](uint64_t i) { return svc.GetRainAdditionalAmbientByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetRainAdditionalAmbientByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locRainAdditionalAmbient.c_str(), svc.GetRainAdditionalAmbientByIndex(nextProf, nextVar), 0, 20, "%.1f",
                   [&](float v) { svc.SetRainAdditionalAmbientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedRainAdditionalAmbient(); },
          [&](float v) { svc.SetBlendedRainAdditionalAmbient(v, 0, 20); },
          progress);
    }

    // 7. Snow
    if (ImGui::CollapsingHeader(m_locSectionSnow.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, m_locSnowIntensity.c_str(), svc.GetSnowIntensityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetSnowIntensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowIntensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSnowIntensity.c_str(), svc.GetSnowIntensityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetSnowIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowIntensity(); },
          [&](float v) { svc.SetBlendedSnowIntensity(v, 0, 1); },
          progress);
      RenderVec2Var(actCtx, m_locSnowFlakeSizeRange.c_str(), svc.GetSnowFlakeSizeRangeCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowFlakeSizeRangeByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vec2f& v) { svc.SetSnowFlakeSizeRangeByIndex(activeProf, i, v); },
          [&]() { RenderNextVec2(m_locSnowFlakeSizeRange.c_str(), svc.GetSnowFlakeSizeRangeByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vec2f& v) { svc.SetSnowFlakeSizeRangeByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowFlakeSizeRange(); },
          [&](const Utils::Vec2f& v) { svc.SetBlendedSnowFlakeSizeRange(v, 1); },
          progress);
      RenderFloatVar(actCtx, m_locSnowAdditionalAmbient.c_str(), svc.GetSnowAdditionalAmbientCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowAdditionalAmbientByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowAdditionalAmbientByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSnowAdditionalAmbient.c_str(), svc.GetSnowAdditionalAmbientByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetSnowAdditionalAmbientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowAdditionalAmbient(); },
          [&](float v) { svc.SetBlendedSnowAdditionalAmbient(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, m_locSnowChaosRate.c_str(), svc.GetSnowChaosRateCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowChaosRateByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowChaosRateByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSnowChaosRate.c_str(), svc.GetSnowChaosRateByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetSnowChaosRateByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowChaosRate(); },
          [&](float v) { svc.SetBlendedSnowChaosRate(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, m_locSnowChaosWeight.c_str(), svc.GetSnowChaosWeightCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowChaosWeightByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowChaosWeightByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSnowChaosWeight.c_str(), svc.GetSnowChaosWeightByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetSnowChaosWeightByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowChaosWeight(); },
          [&](float v) { svc.SetBlendedSnowChaosWeight(v, 0, 1); },
          progress);
    }

    // 8. Fog
    if (ImGui::CollapsingHeader(m_locSectionFog.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, m_locFogColor.c_str(), svc.GetFogColorCount(activeProf), 20, "%.3f",
          [&](uint64_t i) { return svc.GetFogColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetFogColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locFogColor.c_str(), svc.GetFogColorByIndex(nextProf, nextVar), 20, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetFogColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedFogColor(v, 20); },
          progress);
      RenderVec3Var(actCtx, m_locFogColor2.c_str(), svc.GetFogColor2Count(activeProf), 20, "%.3f",
          [&](uint64_t i) { return svc.GetFogColor2ByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetFogColor2ByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locFogColor2.c_str(), svc.GetFogColor2ByIndex(nextProf, nextVar), 20, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetFogColor2ByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogColor2(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedFogColor2(v, 20); },
          progress);
      RenderFloatVar(actCtx, m_locFogVgradient.c_str(), svc.GetFogVgradientCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetFogVgradientByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetFogVgradientByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locFogVgradient.c_str(), svc.GetFogVgradientByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetFogVgradientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogVgradient(); },
          [&](float v) { svc.SetBlendedFogVgradient(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, m_locFogOffset.c_str(), svc.GetFogOffsetCount(activeProf), 0, 500, "%.1f",
          [&](uint64_t i) { return svc.GetFogOffsetByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetFogOffsetByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locFogOffset.c_str(), svc.GetFogOffsetByIndex(nextProf, nextVar), 0, 500, "%.1f",
                   [&](float v) { svc.SetFogOffsetByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogOffset(); },
          [&](float v) { svc.SetBlendedFogOffset(v, 0, 500); },
          progress);
      RenderFloatVar(actCtx, m_locFogDensity.c_str(), svc.GetFogDensityCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetFogDensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetFogDensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locFogDensity.c_str(), svc.GetFogDensityByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetFogDensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogDensity(); },
          [&](float v) { svc.SetBlendedFogDensity(v, 0, 1); },
          progress);
    }

    // 9. Ambient & Env
    if (ImGui::CollapsingHeader(m_locSectionAmbientEnv.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, m_locAmbient.c_str(), svc.GetAmbientCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetAmbientByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetAmbientByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locAmbient.c_str(), svc.GetAmbientByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetAmbientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedAmbient(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedAmbient(v, 50); },
          progress);
      RenderVec3Var(actCtx, m_locDiffuse.c_str(), svc.GetDiffuseCount(activeProf), 200, "%.3f",
          [&](uint64_t i) { return svc.GetDiffuseByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetDiffuseByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locDiffuse.c_str(), svc.GetDiffuseByIndex(nextProf, nextVar), 200, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetDiffuseByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedDiffuse(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedDiffuse(v, 200); },
          progress);
      RenderVec3Var(actCtx, m_locSpecular.c_str(), svc.GetSpecularCount(activeProf), 200, "%.3f",
          [&](uint64_t i) { return svc.GetSpecularByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSpecularByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3(m_locSpecular.c_str(), svc.GetSpecularByIndex(nextProf, nextVar), 200, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSpecularByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSpecular(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSpecular(v, 200); },
          progress);
      RenderFloatVar(actCtx, m_locEnv.c_str(), svc.GetEnvCount(activeProf), 0, 2, "%.3f",
          [&](uint64_t i) { return svc.GetEnvByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetEnvByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locEnv.c_str(), svc.GetEnvByIndex(nextProf, nextVar), 0, 2, "%.3f",
                   [&](float v) { svc.SetEnvByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedEnv(); },
          [&](float v) { svc.SetBlendedEnv(v, 0, 2); },
          progress);
      RenderFloatVar(actCtx, m_locEnvStaticMod.c_str(), svc.GetEnvStaticModCount(activeProf), 0, 5, "%.3f",
          [&](uint64_t i) { return svc.GetEnvStaticModByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetEnvStaticModByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locEnvStaticMod.c_str(), svc.GetEnvStaticModByIndex(nextProf, nextVar), 0, 5, "%.3f",
                   [&](float v) { svc.SetEnvStaticModByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedEnvStaticMod(); },
          [&](float v) { svc.SetBlendedEnvStaticMod(v, 0, 5); },
          progress);
    }

    // 10. Post-Process
    if (ImGui::CollapsingHeader(m_locSectionPostProcess.c_str(), ImGuiTreeNodeFlags_None)) {
      // Tone Mapping
      ImGui::PushID("tone_mapping");
      if (ImGui::TreeNodeEx(m_locToneMapping.c_str(), ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, m_locContrast.c_str(), svc.GetContrastCount(activeProf), 0, 2, "%.6f",
            [&](uint64_t i) { return svc.GetContrastByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetContrastByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locContrast.c_str(), svc.GetContrastByIndex(nextProf, nextVar), 0, 2, "%.6f",
                     [&](float v) { svc.SetContrastByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedContrast(); },
            [&](float v) { svc.SetBlendedContrast(v, 0, 2); },
            progress);
        RenderFloatVar(actCtx, m_locShoulderLength.c_str(), svc.GetShoulderLengthCount(activeProf), 0, 5, "%.6f",
            [&](uint64_t i) { return svc.GetShoulderLengthByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetShoulderLengthByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locShoulderLength.c_str(), svc.GetShoulderLengthByIndex(nextProf, nextVar), 0, 5, "%.6f",
                     [&](float v) { svc.SetShoulderLengthByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedShoulderLength(); },
            [&](float v) { svc.SetBlendedShoulderLength(v, 0, 5); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Color Grading
      ImGui::PushID("color_grading");
      if (ImGui::TreeNodeEx(m_locColorGrading.c_str(), ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, m_locColorBalance.c_str(), svc.GetColorBalanceCount(activeProf), -10, 10, "%.6f",
            [&](uint64_t i) { return svc.GetColorBalanceByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetColorBalanceByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locColorBalance.c_str(), svc.GetColorBalanceByIndex(nextProf, nextVar), -10, 10, "%.6f",
                     [&](float v) { svc.SetColorBalanceByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedColorBalance(); },
            [&](float v) { svc.SetBlendedColorBalance(v, -10, 10); },
            progress);
        RenderFloatVar(actCtx, m_locColorSaturation.c_str(), svc.GetColorSaturationCount(activeProf), 0, 2, "%.3f",
            [&](uint64_t i) { return svc.GetColorSaturationByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetColorSaturationByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locColorSaturation.c_str(), svc.GetColorSaturationByIndex(nextProf, nextVar), 0, 2, "%.3f",
                     [&](float v) { svc.SetColorSaturationByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedColorSaturation(); },
            [&](float v) { svc.SetBlendedColorSaturation(v, 0, 2); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Bloom
      ImGui::PushID("bloom");
      if (ImGui::TreeNodeEx(m_locBloom.c_str(), ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, m_locBloomThreshold.c_str(), svc.GetBloomThresholdCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetBloomThresholdByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomThresholdByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locBloomThreshold.c_str(), svc.GetBloomThresholdByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetBloomThresholdByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomThreshold(); },
            [&](float v) { svc.SetBlendedBloomThreshold(v, 0, 1); },
            progress);
        RenderFloatVar(actCtx, m_locBloomLimit.c_str(), svc.GetBloomLimitCount(activeProf), 0, 500, "%.3f",
            [&](uint64_t i) { return svc.GetBloomLimitByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomLimitByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locBloomLimit.c_str(), svc.GetBloomLimitByIndex(nextProf, nextVar), 0, 500, "%.3f",
                     [&](float v) { svc.SetBloomLimitByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomLimit(); },
            [&](float v) { svc.SetBlendedBloomLimit(v, 0, 500); },
            progress);
        RenderFloatVar(actCtx, m_locBloomIntensity.c_str(), svc.GetBloomIntensityCount(activeProf), 0, 2, "%.6f",
            [&](uint64_t i) { return svc.GetBloomIntensityByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomIntensityByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locBloomIntensity.c_str(), svc.GetBloomIntensityByIndex(nextProf, nextVar), 0, 2, "%.6f",
                     [&](float v) { svc.SetBloomIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomIntensity(); },
            [&](float v) { svc.SetBlendedBloomIntensity(v, 0, 2); },
            progress);
        RenderFloatVar(actCtx, m_locBloomStandardDeviation.c_str(), svc.GetBloomStandardDeviationCount(activeProf), 0, 500, "%.3f",
            [&](uint64_t i) { return svc.GetBloomStandardDeviationByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomStandardDeviationByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locBloomStandardDeviation.c_str(), svc.GetBloomStandardDeviationByIndex(nextProf, nextVar), 0, 500, "%.3f",
                     [&](float v) { svc.SetBloomStandardDeviationByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomStandardDeviation(); },
            [&](float v) { svc.SetBlendedBloomStandardDeviation(v, 0, 500); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Depth of Field
      ImGui::PushID("dof");
      if (ImGui::TreeNodeEx(m_locDepthOfField.c_str(), ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, m_locDofStart.c_str(), svc.GetDofStartCount(activeProf), 0, 5000, "%.1f",
            [&](uint64_t i) { return svc.GetDofStartByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDofStartByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locDofStart.c_str(), svc.GetDofStartByIndex(nextProf, nextVar), 0, 5000, "%.1f",
                     [&](float v) { svc.SetDofStartByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDofStart(); },
            [&](float v) { svc.SetBlendedDofStart(v, 0, 5000); },
            progress);
        RenderFloatVar(actCtx, m_locDofTransition.c_str(), svc.GetDofTransitionCount(activeProf), 0, 5000, "%.1f",
            [&](uint64_t i) { return svc.GetDofTransitionByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDofTransitionByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locDofTransition.c_str(), svc.GetDofTransitionByIndex(nextProf, nextVar), 0, 5000, "%.1f",
                     [&](float v) { svc.SetDofTransitionByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDofTransition(); },
            [&](float v) { svc.SetBlendedDofTransition(v, 0, 5000); },
            progress);
        RenderFloatVar(actCtx, m_locDofFilterSize.c_str(), svc.GetDofFilterSizeCount(activeProf), 0, 10, "%.6f",
            [&](uint64_t i) { return svc.GetDofFilterSizeByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDofFilterSizeByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locDofFilterSize.c_str(), svc.GetDofFilterSizeByIndex(nextProf, nextVar), 0, 10, "%.6f",
                     [&](float v) { svc.SetDofFilterSizeByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDofFilterSize(); },
            [&](float v) { svc.SetBlendedDofFilterSize(v, 0, 10); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Eye Adaptation
      ImGui::PushID("eye_adaptation");
      if (ImGui::TreeNodeEx(m_locEyeAdaptation.c_str(), ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, m_locLowIntensityMin.c_str(), svc.GetLowIntensityMinimumCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetLowIntensityMinimumByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetLowIntensityMinimumByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locLowIntensityMin.c_str(), svc.GetLowIntensityMinimumByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetLowIntensityMinimumByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedLowIntensityMinimum(); },
            [&](float v) { svc.SetBlendedLowIntensityMinimum(v, 0, 1); },
            progress);
        RenderFloatVar(actCtx, m_locLowIntensityMax.c_str(), svc.GetLowIntensityMaximumCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetLowIntensityMaximumByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetLowIntensityMaximumByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locLowIntensityMax.c_str(), svc.GetLowIntensityMaximumByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetLowIntensityMaximumByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedLowIntensityMaximum(); },
            [&](float v) { svc.SetBlendedLowIntensityMaximum(v, 0, 1); },
            progress);
        RenderVec3Var(actCtx, m_locLowIntensityColor.c_str(), svc.GetLowIntensityColorCount(activeProf), 2, "%.6f",
            [&](uint64_t i) { return svc.GetLowIntensityColorByIndex(activeProf, i); },
            [&](uint64_t i, const Utils::Vector3& v) { svc.SetLowIntensityColorByIndex(activeProf, i, v); },
            [&]() { RenderNextVec3(m_locLowIntensityColor.c_str(), svc.GetLowIntensityColorByIndex(nextProf, nextVar), 2, "%.6f",
                     [&](const Utils::Vector3& v) { svc.SetLowIntensityColorByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedLowIntensityColor(); },
            [&](const Utils::Vector3& v) { svc.SetBlendedLowIntensityColor(v, 2); },
            progress);
        RenderFloatVar(actCtx, m_locDarkAdaptationSpeed.c_str(), svc.GetDarkAdaptationSpeedCount(activeProf), 0, 5, "%.6f",
            [&](uint64_t i) { return svc.GetDarkAdaptationSpeedByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDarkAdaptationSpeedByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locDarkAdaptationSpeed.c_str(), svc.GetDarkAdaptationSpeedByIndex(nextProf, nextVar), 0, 5, "%.6f",
                     [&](float v) { svc.SetDarkAdaptationSpeedByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDarkAdaptationSpeed(); },
            [&](float v) { svc.SetBlendedDarkAdaptationSpeed(v, 0, 5); },
            progress);
        RenderFloatVar(actCtx, m_locBrightAdaptationSpeed.c_str(), svc.GetBrightAdaptationSpeedCount(activeProf), 0, 5, "%.6f",
            [&](uint64_t i) { return svc.GetBrightAdaptationSpeedByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBrightAdaptationSpeedByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locBrightAdaptationSpeed.c_str(), svc.GetBrightAdaptationSpeedByIndex(nextProf, nextVar), 0, 5, "%.6f",
                     [&](float v) { svc.SetBrightAdaptationSpeedByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBrightAdaptationSpeed(); },
            [&](float v) { svc.SetBlendedBrightAdaptationSpeed(v, 0, 5); },
            progress);
        RenderFloatVar(actCtx, m_locTargetGray.c_str(), svc.GetTargetGrayCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetTargetGrayByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetTargetGrayByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locTargetGray.c_str(), svc.GetTargetGrayByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetTargetGrayByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedTargetGray(); },
            [&](float v) { svc.SetBlendedTargetGray(v, 0, 1); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Exposure Scale
      ImGui::PushID("exposure_scale");
      if (ImGui::TreeNodeEx(m_locExposureScale.c_str(), ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, m_locMinScale.c_str(), svc.GetMinScaleCount(activeProf), 0, 10, "%.6f",
            [&](uint64_t i) { return svc.GetMinScaleByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetMinScaleByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locMinScale.c_str(), svc.GetMinScaleByIndex(nextProf, nextVar), 0, 10, "%.6f",
                     [&](float v) { svc.SetMinScaleByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedMinScale(); },
            [&](float v) { svc.SetBlendedMinScale(v, 0, 10); },
            progress);
        RenderFloatVar(actCtx, m_locMaxScale.c_str(), svc.GetMaxScaleCount(activeProf), 0, 50, "%.6f",
            [&](uint64_t i) { return svc.GetMaxScaleByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetMaxScaleByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locMaxScale.c_str(), svc.GetMaxScaleByIndex(nextProf, nextVar), 0, 50, "%.6f",
                     [&](float v) { svc.SetMaxScaleByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedMaxScale(); },
            [&](float v) { svc.SetBlendedMaxScale(v, 0, 50); },
            progress);
        RenderFloatVar(actCtx, m_locScaleOverride.c_str(), svc.GetScaleOverrideCount(activeProf), 0, 10, "%.6f",
            [&](uint64_t i) { return svc.GetScaleOverrideByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetScaleOverrideByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat(m_locScaleOverride.c_str(), svc.GetScaleOverrideByIndex(nextProf, nextVar), 0, 10, "%.6f",
                     [&](float v) { svc.SetScaleOverrideByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedScaleOverride(); },
            [&](float v) { svc.SetBlendedScaleOverride(v, 0, 10); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();
    }

    // 11. Wind & Blending
    if (ImGui::CollapsingHeader(m_locSectionWindBlending.c_str(), ImGuiTreeNodeFlags_None)) {
      RenderIntVar(actCtx, m_locWindType.c_str(), svc.GetWindTypeCount(activeProf), 0, 3,
          [&](uint64_t i) { return svc.GetWindTypeByIndex(activeProf, i); },
          [&](uint64_t i, int32_t v) { svc.SetWindTypeByIndex(activeProf, i, v); },
          [&]() { RenderNextInt(m_locWindType.c_str(), svc.GetWindTypeByIndex(nextProf, nextVar), 0, 3,
                   [&](int32_t v) { svc.SetWindTypeByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderFloatVar(actCtx, m_locSpeedCoef.c_str(), svc.GetSpeedCoefCount(activeProf), 0, 5000, "%.1f",
          [&](uint64_t i) { return svc.GetSpeedCoefByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSpeedCoefByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locSpeedCoef.c_str(), svc.GetSpeedCoefByIndex(nextProf, nextVar), 0, 5000, "%.1f",
                   [&](float v) { svc.SetSpeedCoefByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSpeedCoef(); },
          [&](float v) { svc.SetBlendedSpeedCoef(v, 0, 5000); },
          progress);
      RenderFloatVar(actCtx, m_locStability.c_str(), svc.GetStabilityCount(activeProf), 0, 10, "%.1f",
          [&](uint64_t i) { return svc.GetStabilityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetStabilityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat(m_locStability.c_str(), svc.GetStabilityByIndex(nextProf, nextVar), 0, 10, "%.1f",
                   [&](float v) { svc.SetStabilityByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderIntVar(actCtx, m_locBlendWeight.c_str(), svc.GetWeightCount(activeProf), 0, 10,
          [&](uint64_t i) { return svc.GetWeightByIndex(activeProf, i); },
          [&](uint64_t i, int32_t v) { svc.SetWeightByIndex(activeProf, i, v); },
          [&]() { RenderNextInt(m_locBlendWeight.c_str(), svc.GetWeightByIndex(nextProf, nextVar), 0, 10,
                   [&](int32_t v) { svc.SetWeightByIndex(nextProf, nextVar, v); }, nextVar); });
    }
  }

  // --- Final Stats ---
  ImGui::Separator();
}

}  // namespace UI
SPF_NS_END
