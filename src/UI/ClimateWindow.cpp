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

  if (blendedGetter && blendedSetter && blendProgress >= 0.0f) {
    auto _bl = fmt::format("blend @{:.0f}%", blendProgress * 100.0f);
    RenderBlendedFloat(_bl.c_str(), blendedGetter(), minVal, maxVal, fmt, blendedSetter);
    ImGui::Separator();
  }

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    float v = getter(i);
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? " active" : "");

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

  if (blendedGetter && blendedSetter && blendProgress >= 0.0f) {
    auto _bl = fmt::format("blend @{:.0f}%", blendProgress * 100.0f);
    RenderBlendedVec3(_bl.c_str(), blendedGetter(), maxVal, fmt, blendedSetter);
    ImGui::Separator();
  }

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    Utils::Vector3 vec = getter(i);
    float arr[3] = {vec.x, vec.y, vec.z};
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? " active" : "");

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

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    std::string tex = getter(i);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", tex.c_str());
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? " active" : "");

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

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    int32_t v = getter(i);
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? " active" : "");

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

  if (blendedGetter && blendedSetter && blendProgress >= 0.0f) {
    auto _bl = fmt::format("blend @{:.0f}%", blendProgress * 100.0f);
    RenderBlendedVec2(_bl.c_str(), blendedGetter(), maxVal, fmt, blendedSetter);
    ImGui::Separator();
  }

  for (uint64_t i = 0; i < count; ++i) {
    if (i % 3 != 0) ImGui::SameLine();
    if (i % 3 == 0 && i > 0) ImGui::Spacing();

    ImGui::PushID(static_cast<int>(i));
    Utils::Vec2f vec = getter(i);
    float arr[2] = {vec.x, vec.y};
    auto label = fmt::format("[{}]{}", i, i == ctx.activeVar ? " active" : "");

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
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  char l[64];
  snprintf(l, sizeof(l), "[%llu] %s", nv, label);
  ImGui::SetNextItemWidth(180);
  if (ImGui::SliderFloat(l, &val, minVal, maxVal, fmt)) setter(val);
  ImGui::PopStyleColor();
}

static void RenderNextVec3(const char* label, const Utils::Vector3& val, float maxVal, const char* fmt, const std::function<void(const Utils::Vector3&)>& setter, uint64_t nv) {
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  float arr[3] = {val.x, val.y, val.z};
  char l[64];
  snprintf(l, sizeof(l), "[%llu] %s", nv, label);
  ImGui::SetNextItemWidth(350);
  if (ImGui::SliderFloat3(l, arr, 0.0f, maxVal, fmt)) setter({arr[0], arr[1], arr[2]});
  ImGui::SameLine();
  float norm[3] = {arr[0] / maxVal, arr[1] / maxVal, arr[2] / maxVal};
  if (ImGui::ColorEdit3("##c", norm, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) setter({norm[0] * maxVal, norm[1] * maxVal, norm[2] * maxVal});
  ImGui::PopStyleColor();
}

static void RenderNextVec2(const char* label, const Utils::Vec2f& val, float maxVal, const char* fmt, const std::function<void(const Utils::Vec2f&)>& setter, uint64_t nv) {
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  float arr[2] = {val.x, val.y};
  char l[64];
  snprintf(l, sizeof(l), "[%llu] %s", nv, label);
  ImGui::SetNextItemWidth(280);
  if (ImGui::SliderFloat2(l, arr, 0.0f, maxVal, fmt)) setter({arr[0], arr[1]});
  ImGui::PopStyleColor();
}

static void RenderNextInt(const char* label, int32_t val, int32_t minVal, int32_t maxVal, const std::function<void(int32_t)>& setter, uint64_t nv) {
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  char l[64];
  snprintf(l, sizeof(l), "[%llu] %s", nv, label);
  ImGui::SetNextItemWidth(180);
  if (ImGui::SliderInt(l, &val, minVal, maxVal)) setter(val);
  ImGui::PopStyleColor();
}

static void RenderNextTexture(const char* label, const std::string& val, const std::function<void(const std::string&)>& setter, uint64_t nv) {
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", val.c_str());
  char l[64];
  snprintf(l, sizeof(l), "[%llu] %s", nv, label);
  ImGui::SetNextItemWidth(280);
  if (ImGui::InputText(l, buf, sizeof(buf))) setter(std::string(buf));
  ImGui::PopStyleColor();
}

}  // anonymous namespace

ClimateWindow::ClimateWindow(const std::string& componentName, const std::string& windowId, Data::GameData::ClimateService& climateService) : BaseWindow(componentName, windowId), m_climateService(climateService) {
  m_locTitle = "climate_window.title";
  m_locNotReady = "climate_window.not_ready";
}

const char* ClimateWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_locTitle).c_str(); }

void ClimateWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();
  auto& svc = m_climateService;

  if (!svc.IsReady()) {
    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", loc.Get(m_locNotReady).c_str());
    return;
  }

  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "Weather & Climate");
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

    if (ImGui::Combo("Select Climate", &selectedIdx, items.data(), (int)items.size())) {
      if (selectedIdx >= 0 && selectedIdx < (int)climateCache.size()) {
        svc.SetClimate(climateCache[selectedIdx].token, true);
      }
    }
    if (ImGui::Button("Refresh List")) {
      climateCache = svc.GetAvailableClimates();
    }
  }

  // --- Profile Information ---
  ImGui::Text("Active climate: %s", currentClimate.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  // --- Time Auto-Step Controls ---
  static bool stepBack = false;
  static bool stepFwd = false;
  ImGui::Text("Time Auto-Step (for test)");
  ImGui::SameLine();
  if (ImGui::Button(stepBack ? "Stop -1" : "-1 min")) {
    stepBack = !stepBack;
    stepFwd = false;
  }
  ImGui::SameLine();
  if (ImGui::Button(stepFwd ? "Stop +1" : "+1 min")) {
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
  ImGui::Text("Sun Profiles Count: %d", profileCount);
  ImGui::Text("Active Sun Profile: %s (idx: %d)", svc.GetSunProfileName(aIdx, activeProf.isBad).c_str(), aIdx);
  ImGui::Text("Next Sun Profile: %s (idx: %d)", svc.GetSunProfileName(bIdx, activeProf.isBad).c_str(), bIdx);
  ImGui::Separator();
  float transitionProgress = svc.GetTransitionProgress();
  ImGui::Text("Transition: %.1f%%", transitionProgress * 100.0f);
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text("Active Elev: %.4f rad", svc.GetSunProfileElevation(aIdx));
  ImGui::Text("Next Elev:    %.4f rad", svc.GetSunProfileElevation(bIdx));
  float sunAngle = svc.GetSunAngle();
  ImGui::Text("Sun Angle: %.4f rad (%.1f°)", sunAngle, sunAngle * 180.0f / 3.14159265f);
  static float setSunAngle = 0.0f;
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Weather Mode Selection ---
  int32_t currentWeather = svc.GetWeatherMode();
  static bool interpolatedTransition = false;
  const char* weatherNames[] = {"Nice", "Bad"};

  // --- Next Weather Mode ---
  int32_t nextWeather = svc.GetNextWeatherMode();
  static int32_t lastNextWeather = -1;
  ImGui::Text("Next Weather: %s (%d)", weatherNames[nextWeather], nextWeather);
  ImGui::Spacing();

  const bool isNice = (currentWeather == 0);
  const ImVec2 btnSize(150, 0);
  if (isNice) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
  }
  if (ImGui::Button("Nice", btnSize)) {
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
  if (ImGui::Button("Bad", btnSize)) {
    svc.SetWeatherMode(1, !interpolatedTransition);
  }
  if (isBad) {
    ImGui::PopStyleColor();
  }

  ImGui::SameLine();
  if (ImGui::Checkbox("Interpolated Change", &interpolatedTransition)) {
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("A smooth transition takes about 20 game minutes.\nYou can adjust this with the Weather Blending slider.");
  }
  // --- Weather Blend Progress ---
  ImGui::Spacing();
  float blendProgress = svc.GetWeatherBlendProgress();
  if (blendProgress <= 1.0f) {
    ImGui::Text("weather mixing percentage: %.1f%%", blendProgress * 100.0f);
  } else {
    ImGui::Text("The transition does not occur");
  }
  static int durationMinutes = 20;
  if (ImGui::SliderInt("Duration (min)", &durationMinutes, 1, 120)) {
    svc.SetTransitionDuration(durationMinutes);
  }
  ImGui::Separator();
  ImGui::Spacing();

  // --- Bad Weather Factor Panel ---
  float current = svc.GetBadWeatherFactor();
  static float s_desired = current;
  ImGui::Text("g_bad_weather_factor: %.3f", current);

  ImGui::SetNextItemWidth(260);
  ImGui::SliderFloat("Factor", &s_desired, 0.0f, 1.0f, "%.3f");
  ImGui::SameLine();
  if (ImGui::Button("Set")) {
    svc.SetBadWeatherFactor(s_desired);
  }

  uint32_t badMode = svc.GetBadWeatherMode();
  ImGui::Text("Bad Weather Mode: %s (%u)", badMode ? "Active" : "Inactive", badMode);

  float remainSec = svc.GetRemainingBadWeatherTime();
  uint32_t remainMin = (uint32_t)(remainSec / 60.0f);
  ImGui::Text("real time in seconds until the next weather change: %.0f s (%u min)", remainSec, remainMin);

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
  ImGui::Text("Active: %s (varIdx: %llu/%llu) | Next: %s (varIdx: %llu/%llu)",
      aName.c_str(), activeVar, activeVarCount - 1,
      bName.c_str(), nextVar, nextVarCount - 1);

  ImGui::Spacing();

  auto actLabel = fmt::format("Set Active Variation in {}", aName);
  ImGui::SetNextItemWidth(250);
  ImGui::SliderInt("##setActive", &setActiveVarIdx, 0, (int)(activeVarCount - 1));
  ImGui::SameLine();
  if (ImGui::Button(actLabel.c_str())) {
    svc.SetActiveVariationIndex((uint64_t)setActiveVarIdx);
  }

  auto nextLabel = fmt::format("Set Next Variation in {}", bName);
  ImGui::SetNextItemWidth(250);
  ImGui::SliderInt("##setNext", &setNextVarIdx, 0, (int)(nextVarCount - 1));
  ImGui::SameLine();
  if (ImGui::Button(nextLabel.c_str())) {
    svc.SetNextVariationIndex((uint64_t)setNextVarIdx);
  }
  ImGui::Separator();
  ImGui::Spacing();

  // Env Profile
  if (ImGui::CollapsingHeader("Env Profile", ImGuiTreeNodeFlags_None)) {
    {
      float v = svc.GetLampsOnElevation();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat("Lamps On Elevation", &v, 0.0f, 90.0f, "%.1f°")) svc.SetLampsOnElevation(v);
    }
    {
      float v = svc.GetDayInYear();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat("Day In Year", &v, 1.0f, 366.0f, "%.0f")) svc.SetDayInYear(v);
    }
    {
      float v = svc.GetSummerTime();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat("Summer Time", &v, 0.0f, 2.0f, "%.1f")) svc.SetSummerTime(v);
    }
    {
      float v = svc.GetThunderstormProbability();
      ImGui::SetNextItemWidth(200);
      if (ImGui::SliderFloat("Thunderstorm Probability", &v, 0.0f, 1.0f, "%.3f")) svc.SetThunderstormProbability(v);
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
    if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_None)) {
      {
        float v = svc.GetLowElevation(activeProf);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("Low Elevation", &v, -90.0f, 90.0f, "%.1f°")) svc.SetLowElevation(activeProf, v);
      }
      {
        float v = svc.GetHighElevation(activeProf);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat("High Elevation", &v, -90.0f, 90.0f, "%.1f°")) svc.SetHighElevation(activeProf, v);
      }
      {
        int32_t dir = svc.GetSunDirection(activeProf);
        const char* dirNames[] = {"1 (forward)", "0 (zenith)", "-1 (backward)"};
        int combo = 1 - dir;
        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo("Sun Direction", &combo, dirNames, IM_ARRAYSIZE(dirNames))) svc.SetSunDirection(activeProf, 1 - combo);
      }
      RenderVec3Var(actCtx, "Sun Color", svc.GetSunColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSunColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSunColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Sun Color", svc.GetSunColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSunColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSunColor(v, 50); },
          progress);
      RenderFloatVar(actCtx, "Sun Opacity", svc.GetSunOpacityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetSunOpacityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSunOpacityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Sun Opacity", svc.GetSunOpacityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetSunOpacityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunOpacity(); },
          [&](float v) { svc.SetBlendedSunOpacity(v, 0, 1); },
          progress);
      RenderVec3Var(actCtx, "Sun Halo Color", svc.GetSunHaloColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSunHaloColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSunHaloColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Sun Halo Color", svc.GetSunHaloColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSunHaloColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunHaloColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSunHaloColor(v, 50); },
          progress);
      RenderFloatVar(actCtx, "Sun Shadow Strength", svc.GetSunShadowStrengthCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetSunShadowStrengthByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSunShadowStrengthByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Sun Shadow Strength", svc.GetSunShadowStrengthByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetSunShadowStrengthByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunShadowStrength(); },
          [&](float v) { svc.SetBlendedSunShadowStrength(v, 0, 1); },
          progress);
      RenderVec3Var(actCtx, "Sunshaft Color", svc.GetSunshaftColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSunshaftColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSunshaftColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Sunshaft Color", svc.GetSunshaftColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSunshaftColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunshaftColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSunshaftColor(v, 50); },
          progress);
      RenderFloatVar(actCtx, "Sunshaft Size", svc.GetSunshaftSizeCount(activeProf), 0, 100, "%.3f",
          [&](uint64_t i) { return svc.GetSunshaftSizeByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSunshaftSizeByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Sunshaft Size", svc.GetSunshaftSizeByIndex(nextProf, nextVar), 0, 100, "%.3f",
                   [&](float v) { svc.SetSunshaftSizeByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSunshaftSize(); },
          [&](float v) { svc.SetBlendedSunshaftSize(v, 0, 100); },
          progress);
    }

    // 2. Moon & Stars
    if (ImGui::CollapsingHeader("Moon & Stars", ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, "Moon Color", svc.GetMoonColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetMoonColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetMoonColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Moon Color", svc.GetMoonColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetMoonColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMoonColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedMoonColor(v, 1); },
          progress);
      RenderVec3Var(actCtx, "Moon Halo Color", svc.GetMoonHaloColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetMoonHaloColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetMoonHaloColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Moon Halo Color", svc.GetMoonHaloColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetMoonHaloColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMoonHaloColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedMoonHaloColor(v, 1); },
          progress);
      RenderFloatVar(actCtx, "Moon Halo Scale", svc.GetMoonHaloScaleCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetMoonHaloScaleByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetMoonHaloScaleByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Moon Halo Scale", svc.GetMoonHaloScaleByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetMoonHaloScaleByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMoonHaloScale(); },
          [&](float v) { svc.SetBlendedMoonHaloScale(v, 0, 1); },
          progress);
      RenderVec3Var(actCtx, "Starmap Color", svc.GetStarmapColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetStarmapColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetStarmapColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Starmap Color", svc.GetStarmapColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetStarmapColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedStarmapColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedStarmapColor(v, 1); },
          progress);
      RenderVec3Var(actCtx, "Stars Color", svc.GetStarsColorCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetStarsColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetStarsColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Stars Color", svc.GetStarsColorByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vector3& v) { svc.SetStarsColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedStarsColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedStarsColor(v, 1); },
          progress);
      RenderTextureVar(actCtx, "Stars Texture", svc.GetStarsTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetStarsTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetStarsTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture("Stars Texture", svc.GetStarsTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetStarsTextureByIndex(nextProf, nextVar, v); }, nextVar); });
    }

    // 3. Sky & Textures
    if (ImGui::CollapsingHeader("Sky & Textures", ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, "Sky Color", svc.GetSkyColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSkyColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSkyColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Sky Color", svc.GetSkyColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSkyColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSkyColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSkyColor(v, 50); },
          progress);
      RenderVec3Var(actCtx, "Sky Bottom Color", svc.GetSkyBottomColorCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetSkyBottomColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSkyBottomColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Sky Bottom Color", svc.GetSkyBottomColorByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSkyBottomColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSkyBottomColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSkyBottomColor(v, 50); },
          progress);
      RenderTextureVar(actCtx, "Skybox Texture", svc.GetSkyboxTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetSkyboxTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetSkyboxTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture("Skybox Texture", svc.GetSkyboxTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetSkyboxTextureByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderTextureVar(actCtx, "Skycloud Mask Texture", svc.GetSkycloudMaskTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetSkycloudMaskTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetSkycloudMaskTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture("Skycloud Mask Texture", svc.GetSkycloudMaskTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetSkycloudMaskTextureByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderFloatVar(actCtx, "Mirror Sky Texture", svc.GetMirrorSkyTextureCount(activeProf), 0, 1, "%.0f",
          [&](uint64_t i) { return svc.GetMirrorSkyTextureByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetMirrorSkyTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Mirror Sky Texture", svc.GetMirrorSkyTextureByIndex(nextProf, nextVar), 0, 1, "%.0f",
                   [&](float v) { svc.SetMirrorSkyTextureByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedMirrorSkyTexture(); },
          [&](float v) { svc.SetBlendedMirrorSkyTexture(v, 0, 1); },
          progress);
    }

    // 4. Cloud Shadows
    if (ImGui::CollapsingHeader("Cloud Shadows", ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, "Cloud Shadow Weight", svc.GetCloudShadowWeightCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetCloudShadowWeightByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetCloudShadowWeightByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Cloud Shadow Weight", svc.GetCloudShadowWeightByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetCloudShadowWeightByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedCloudShadowWeight(); },
          [&](float v) { svc.SetBlendedCloudShadowWeight(v, 0, 1); },
          progress);
      RenderTextureVar(actCtx, "Cloud Shadow Texture", svc.GetCloudShadowTextureCount(activeProf),
          [&](uint64_t i) { return svc.GetCloudShadowTextureByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetCloudShadowTextureByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture("Cloud Shadow Texture", svc.GetCloudShadowTextureByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetCloudShadowTextureByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderVec2Var(actCtx, "Cloud Shadow Area Size", svc.GetCloudShadowAreaSizeCount(activeProf), 2000, "%.1f",
          [&](uint64_t i) { return svc.GetCloudShadowAreaSizeByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vec2f& v) { svc.SetCloudShadowAreaSizeByIndex(activeProf, i, v); },
          [&]() { RenderNextVec2("Cloud Shadow Area Size", svc.GetCloudShadowAreaSizeByIndex(nextProf, nextVar), 2000, "%.1f",
                   [&](const Utils::Vec2f& v) { svc.SetCloudShadowAreaSizeByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedCloudShadowAreaSize(); },
          [&](const Utils::Vec2f& v) { svc.SetBlendedCloudShadowAreaSize(v, 2000); },
          progress);
      RenderVec2Var(actCtx, "Cloud Shadow Speed", svc.GetCloudShadowSpeedCount(activeProf), 100, "%.1f",
          [&](uint64_t i) { return svc.GetCloudShadowSpeedByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vec2f& v) { svc.SetCloudShadowSpeedByIndex(activeProf, i, v); },
          [&]() { RenderNextVec2("Cloud Shadow Speed", svc.GetCloudShadowSpeedByIndex(nextProf, nextVar), 100, "%.1f",
                   [&](const Utils::Vec2f& v) { svc.SetCloudShadowSpeedByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedCloudShadowSpeed(); },
          [&](const Utils::Vec2f& v) { svc.SetBlendedCloudShadowSpeed(v, 100); },
          progress);
    }

    // 5. Temperature
    if (ImGui::CollapsingHeader("Temperature", ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, "Temperature", svc.GetTemperatureCount(activeProf), -50, 100, "%.1f°C",
          [&](uint64_t i) { return svc.GetTemperatureByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetTemperatureByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Temperature", svc.GetTemperatureByIndex(nextProf, nextVar), -50, 100, "%.1f°C",
                   [&](float v) { svc.SetTemperatureByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedTemperature(); },
          [&](float v) { svc.SetBlendedTemperature(v, -50, 100); },
          progress);
    }

    // 6. Rain & Lightning
    if (ImGui::CollapsingHeader("Rain & Lightning", ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, "Rain Intensity", svc.GetRainIntensityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetRainIntensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetRainIntensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Rain Intensity", svc.GetRainIntensityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetRainIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedRainIntensity(); },
          [&](float v) { svc.SetBlendedRainIntensity(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, "Lightning Intensity", svc.GetLightningIntensityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetLightningIntensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetLightningIntensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Lightning Intensity", svc.GetLightningIntensityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetLightningIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedLightningIntensity(); },
          [&](float v) { svc.SetBlendedLightningIntensity(v, 0, 1); },
          progress);
      RenderTextureVar(actCtx, "Lightning Mask", svc.GetLightningMaskCount(activeProf),
          [&](uint64_t i) { return svc.GetLightningMaskByIndex(activeProf, i); },
          [&](uint64_t i, const std::string& v) { svc.SetLightningMaskByIndex(activeProf, i, v); },
          [&]() { RenderNextTexture("Lightning Mask", svc.GetLightningMaskByIndex(nextProf, nextVar),
                   [&](const std::string& v) { svc.SetLightningMaskByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderFloatVar(actCtx, "Rain Max Wetness", svc.GetRainMaxWetnessCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetRainMaxWetnessByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetRainMaxWetnessByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Rain Max Wetness", svc.GetRainMaxWetnessByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetRainMaxWetnessByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedRainMaxWetness(); },
          [&](float v) { svc.SetBlendedRainMaxWetness(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, "Rain Additional Ambient", svc.GetRainAdditionalAmbientCount(activeProf), 0, 20, "%.1f",
          [&](uint64_t i) { return svc.GetRainAdditionalAmbientByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetRainAdditionalAmbientByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Rain Additional Ambient", svc.GetRainAdditionalAmbientByIndex(nextProf, nextVar), 0, 20, "%.1f",
                   [&](float v) { svc.SetRainAdditionalAmbientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedRainAdditionalAmbient(); },
          [&](float v) { svc.SetBlendedRainAdditionalAmbient(v, 0, 20); },
          progress);
    }

    // 7. Snow
    if (ImGui::CollapsingHeader("Snow", ImGuiTreeNodeFlags_None)) {
      RenderFloatVar(actCtx, "Snow Intensity", svc.GetSnowIntensityCount(activeProf), 0, 1, "%.3f",
          [&](uint64_t i) { return svc.GetSnowIntensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowIntensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Snow Intensity", svc.GetSnowIntensityByIndex(nextProf, nextVar), 0, 1, "%.3f",
                   [&](float v) { svc.SetSnowIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowIntensity(); },
          [&](float v) { svc.SetBlendedSnowIntensity(v, 0, 1); },
          progress);
      RenderVec2Var(actCtx, "Snow Flake Size Range", svc.GetSnowFlakeSizeRangeCount(activeProf), 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowFlakeSizeRangeByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vec2f& v) { svc.SetSnowFlakeSizeRangeByIndex(activeProf, i, v); },
          [&]() { RenderNextVec2("Snow Flake Size Range", svc.GetSnowFlakeSizeRangeByIndex(nextProf, nextVar), 1, "%.6f",
                   [&](const Utils::Vec2f& v) { svc.SetSnowFlakeSizeRangeByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowFlakeSizeRange(); },
          [&](const Utils::Vec2f& v) { svc.SetBlendedSnowFlakeSizeRange(v, 1); },
          progress);
      RenderFloatVar(actCtx, "Snow Additional Ambient", svc.GetSnowAdditionalAmbientCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowAdditionalAmbientByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowAdditionalAmbientByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Snow Additional Ambient", svc.GetSnowAdditionalAmbientByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetSnowAdditionalAmbientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowAdditionalAmbient(); },
          [&](float v) { svc.SetBlendedSnowAdditionalAmbient(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, "Snow Chaos Rate", svc.GetSnowChaosRateCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowChaosRateByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowChaosRateByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Snow Chaos Rate", svc.GetSnowChaosRateByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetSnowChaosRateByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowChaosRate(); },
          [&](float v) { svc.SetBlendedSnowChaosRate(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, "Snow Chaos Weight", svc.GetSnowChaosWeightCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetSnowChaosWeightByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSnowChaosWeightByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Snow Chaos Weight", svc.GetSnowChaosWeightByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetSnowChaosWeightByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSnowChaosWeight(); },
          [&](float v) { svc.SetBlendedSnowChaosWeight(v, 0, 1); },
          progress);
    }

    // 8. Fog
    if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, "Fog Color", svc.GetFogColorCount(activeProf), 20, "%.3f",
          [&](uint64_t i) { return svc.GetFogColorByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetFogColorByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Fog Color", svc.GetFogColorByIndex(nextProf, nextVar), 20, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetFogColorByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogColor(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedFogColor(v, 20); },
          progress);
      RenderVec3Var(actCtx, "Fog Color 2", svc.GetFogColor2Count(activeProf), 20, "%.3f",
          [&](uint64_t i) { return svc.GetFogColor2ByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetFogColor2ByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Fog Color 2", svc.GetFogColor2ByIndex(nextProf, nextVar), 20, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetFogColor2ByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogColor2(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedFogColor2(v, 20); },
          progress);
      RenderFloatVar(actCtx, "Fog Vgradient", svc.GetFogVgradientCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetFogVgradientByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetFogVgradientByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Fog Vgradient", svc.GetFogVgradientByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetFogVgradientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogVgradient(); },
          [&](float v) { svc.SetBlendedFogVgradient(v, 0, 1); },
          progress);
      RenderFloatVar(actCtx, "Fog Offset", svc.GetFogOffsetCount(activeProf), 0, 500, "%.1f",
          [&](uint64_t i) { return svc.GetFogOffsetByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetFogOffsetByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Fog Offset", svc.GetFogOffsetByIndex(nextProf, nextVar), 0, 500, "%.1f",
                   [&](float v) { svc.SetFogOffsetByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogOffset(); },
          [&](float v) { svc.SetBlendedFogOffset(v, 0, 500); },
          progress);
      RenderFloatVar(actCtx, "Fog Density", svc.GetFogDensityCount(activeProf), 0, 1, "%.6f",
          [&](uint64_t i) { return svc.GetFogDensityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetFogDensityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Fog Density", svc.GetFogDensityByIndex(nextProf, nextVar), 0, 1, "%.6f",
                   [&](float v) { svc.SetFogDensityByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedFogDensity(); },
          [&](float v) { svc.SetBlendedFogDensity(v, 0, 1); },
          progress);
    }

    // 9. Ambient & Env
    if (ImGui::CollapsingHeader("Ambient & Env", ImGuiTreeNodeFlags_None)) {
      RenderVec3Var(actCtx, "Ambient", svc.GetAmbientCount(activeProf), 50, "%.3f",
          [&](uint64_t i) { return svc.GetAmbientByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetAmbientByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Ambient", svc.GetAmbientByIndex(nextProf, nextVar), 50, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetAmbientByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedAmbient(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedAmbient(v, 50); },
          progress);
      RenderVec3Var(actCtx, "Diffuse", svc.GetDiffuseCount(activeProf), 200, "%.3f",
          [&](uint64_t i) { return svc.GetDiffuseByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetDiffuseByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Diffuse", svc.GetDiffuseByIndex(nextProf, nextVar), 200, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetDiffuseByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedDiffuse(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedDiffuse(v, 200); },
          progress);
      RenderVec3Var(actCtx, "Specular", svc.GetSpecularCount(activeProf), 200, "%.3f",
          [&](uint64_t i) { return svc.GetSpecularByIndex(activeProf, i); },
          [&](uint64_t i, const Utils::Vector3& v) { svc.SetSpecularByIndex(activeProf, i, v); },
          [&]() { RenderNextVec3("Specular", svc.GetSpecularByIndex(nextProf, nextVar), 200, "%.3f",
                   [&](const Utils::Vector3& v) { svc.SetSpecularByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSpecular(); },
          [&](const Utils::Vector3& v) { svc.SetBlendedSpecular(v, 200); },
          progress);
      RenderFloatVar(actCtx, "Env", svc.GetEnvCount(activeProf), 0, 2, "%.3f",
          [&](uint64_t i) { return svc.GetEnvByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetEnvByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Env", svc.GetEnvByIndex(nextProf, nextVar), 0, 2, "%.3f",
                   [&](float v) { svc.SetEnvByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedEnv(); },
          [&](float v) { svc.SetBlendedEnv(v, 0, 2); },
          progress);
      RenderFloatVar(actCtx, "Env Static Mod", svc.GetEnvStaticModCount(activeProf), 0, 5, "%.3f",
          [&](uint64_t i) { return svc.GetEnvStaticModByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetEnvStaticModByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Env Static Mod", svc.GetEnvStaticModByIndex(nextProf, nextVar), 0, 5, "%.3f",
                   [&](float v) { svc.SetEnvStaticModByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedEnvStaticMod(); },
          [&](float v) { svc.SetBlendedEnvStaticMod(v, 0, 5); },
          progress);
    }

    // 10. Post-Process
    if (ImGui::CollapsingHeader("Post-Process", ImGuiTreeNodeFlags_None)) {
      // Tone Mapping
      ImGui::PushID("tone_mapping");
      if (ImGui::TreeNodeEx("Tone Mapping", ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, "Contrast", svc.GetContrastCount(activeProf), 0, 2, "%.6f",
            [&](uint64_t i) { return svc.GetContrastByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetContrastByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Contrast", svc.GetContrastByIndex(nextProf, nextVar), 0, 2, "%.6f",
                     [&](float v) { svc.SetContrastByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedContrast(); },
            [&](float v) { svc.SetBlendedContrast(v, 0, 2); },
            progress);
        RenderFloatVar(actCtx, "Shoulder Length", svc.GetShoulderLengthCount(activeProf), 0, 5, "%.6f",
            [&](uint64_t i) { return svc.GetShoulderLengthByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetShoulderLengthByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Shoulder Length", svc.GetShoulderLengthByIndex(nextProf, nextVar), 0, 5, "%.6f",
                     [&](float v) { svc.SetShoulderLengthByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedShoulderLength(); },
            [&](float v) { svc.SetBlendedShoulderLength(v, 0, 5); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Color Grading
      ImGui::PushID("color_grading");
      if (ImGui::TreeNodeEx("Color Grading", ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, "Color Balance", svc.GetColorBalanceCount(activeProf), -10, 10, "%.6f",
            [&](uint64_t i) { return svc.GetColorBalanceByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetColorBalanceByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Color Balance", svc.GetColorBalanceByIndex(nextProf, nextVar), -10, 10, "%.6f",
                     [&](float v) { svc.SetColorBalanceByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedColorBalance(); },
            [&](float v) { svc.SetBlendedColorBalance(v, -10, 10); },
            progress);
        RenderFloatVar(actCtx, "Color Saturation", svc.GetColorSaturationCount(activeProf), 0, 2, "%.3f",
            [&](uint64_t i) { return svc.GetColorSaturationByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetColorSaturationByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Color Saturation", svc.GetColorSaturationByIndex(nextProf, nextVar), 0, 2, "%.3f",
                     [&](float v) { svc.SetColorSaturationByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedColorSaturation(); },
            [&](float v) { svc.SetBlendedColorSaturation(v, 0, 2); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Bloom
      ImGui::PushID("bloom");
      if (ImGui::TreeNodeEx("Bloom", ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, "Threshold", svc.GetBloomThresholdCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetBloomThresholdByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomThresholdByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Threshold", svc.GetBloomThresholdByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetBloomThresholdByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomThreshold(); },
            [&](float v) { svc.SetBlendedBloomThreshold(v, 0, 1); },
            progress);
        RenderFloatVar(actCtx, "Limit", svc.GetBloomLimitCount(activeProf), 0, 500, "%.3f",
            [&](uint64_t i) { return svc.GetBloomLimitByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomLimitByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Limit", svc.GetBloomLimitByIndex(nextProf, nextVar), 0, 500, "%.3f",
                     [&](float v) { svc.SetBloomLimitByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomLimit(); },
            [&](float v) { svc.SetBlendedBloomLimit(v, 0, 500); },
            progress);
        RenderFloatVar(actCtx, "Bloom Intensity", svc.GetBloomIntensityCount(activeProf), 0, 2, "%.6f",
            [&](uint64_t i) { return svc.GetBloomIntensityByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomIntensityByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Bloom Intensity", svc.GetBloomIntensityByIndex(nextProf, nextVar), 0, 2, "%.6f",
                     [&](float v) { svc.SetBloomIntensityByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomIntensity(); },
            [&](float v) { svc.SetBlendedBloomIntensity(v, 0, 2); },
            progress);
        RenderFloatVar(actCtx, "Standard Deviation", svc.GetBloomStandardDeviationCount(activeProf), 0, 500, "%.3f",
            [&](uint64_t i) { return svc.GetBloomStandardDeviationByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBloomStandardDeviationByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Standard Deviation", svc.GetBloomStandardDeviationByIndex(nextProf, nextVar), 0, 500, "%.3f",
                     [&](float v) { svc.SetBloomStandardDeviationByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBloomStandardDeviation(); },
            [&](float v) { svc.SetBlendedBloomStandardDeviation(v, 0, 500); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Depth of Field
      ImGui::PushID("dof");
      if (ImGui::TreeNodeEx("Depth of Field", ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, "Start", svc.GetDofStartCount(activeProf), 0, 5000, "%.1f",
            [&](uint64_t i) { return svc.GetDofStartByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDofStartByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Start", svc.GetDofStartByIndex(nextProf, nextVar), 0, 5000, "%.1f",
                     [&](float v) { svc.SetDofStartByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDofStart(); },
            [&](float v) { svc.SetBlendedDofStart(v, 0, 5000); },
            progress);
        RenderFloatVar(actCtx, "Transition", svc.GetDofTransitionCount(activeProf), 0, 5000, "%.1f",
            [&](uint64_t i) { return svc.GetDofTransitionByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDofTransitionByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Transition", svc.GetDofTransitionByIndex(nextProf, nextVar), 0, 5000, "%.1f",
                     [&](float v) { svc.SetDofTransitionByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDofTransition(); },
            [&](float v) { svc.SetBlendedDofTransition(v, 0, 5000); },
            progress);
        RenderFloatVar(actCtx, "Filter Size", svc.GetDofFilterSizeCount(activeProf), 0, 10, "%.6f",
            [&](uint64_t i) { return svc.GetDofFilterSizeByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDofFilterSizeByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Filter Size", svc.GetDofFilterSizeByIndex(nextProf, nextVar), 0, 10, "%.6f",
                     [&](float v) { svc.SetDofFilterSizeByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDofFilterSize(); },
            [&](float v) { svc.SetBlendedDofFilterSize(v, 0, 10); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Eye Adaptation
      ImGui::PushID("eye_adaptation");
      if (ImGui::TreeNodeEx("Eye Adaptation", ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, "Low Intensity Min", svc.GetLowIntensityMinimumCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetLowIntensityMinimumByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetLowIntensityMinimumByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Low Intensity Min", svc.GetLowIntensityMinimumByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetLowIntensityMinimumByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedLowIntensityMinimum(); },
            [&](float v) { svc.SetBlendedLowIntensityMinimum(v, 0, 1); },
            progress);
        RenderFloatVar(actCtx, "Low Intensity Max", svc.GetLowIntensityMaximumCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetLowIntensityMaximumByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetLowIntensityMaximumByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Low Intensity Max", svc.GetLowIntensityMaximumByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetLowIntensityMaximumByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedLowIntensityMaximum(); },
            [&](float v) { svc.SetBlendedLowIntensityMaximum(v, 0, 1); },
            progress);
        RenderVec3Var(actCtx, "Low Intensity Color", svc.GetLowIntensityColorCount(activeProf), 2, "%.6f",
            [&](uint64_t i) { return svc.GetLowIntensityColorByIndex(activeProf, i); },
            [&](uint64_t i, const Utils::Vector3& v) { svc.SetLowIntensityColorByIndex(activeProf, i, v); },
            [&]() { RenderNextVec3("Low Intensity Color", svc.GetLowIntensityColorByIndex(nextProf, nextVar), 2, "%.6f",
                     [&](const Utils::Vector3& v) { svc.SetLowIntensityColorByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedLowIntensityColor(); },
            [&](const Utils::Vector3& v) { svc.SetBlendedLowIntensityColor(v, 2); },
            progress);
        RenderFloatVar(actCtx, "Dark Adaptation Speed", svc.GetDarkAdaptationSpeedCount(activeProf), 0, 5, "%.6f",
            [&](uint64_t i) { return svc.GetDarkAdaptationSpeedByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetDarkAdaptationSpeedByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Dark Adaptation Speed", svc.GetDarkAdaptationSpeedByIndex(nextProf, nextVar), 0, 5, "%.6f",
                     [&](float v) { svc.SetDarkAdaptationSpeedByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedDarkAdaptationSpeed(); },
            [&](float v) { svc.SetBlendedDarkAdaptationSpeed(v, 0, 5); },
            progress);
        RenderFloatVar(actCtx, "Bright Adaptation Speed", svc.GetBrightAdaptationSpeedCount(activeProf), 0, 5, "%.6f",
            [&](uint64_t i) { return svc.GetBrightAdaptationSpeedByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetBrightAdaptationSpeedByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Bright Adaptation Speed", svc.GetBrightAdaptationSpeedByIndex(nextProf, nextVar), 0, 5, "%.6f",
                     [&](float v) { svc.SetBrightAdaptationSpeedByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedBrightAdaptationSpeed(); },
            [&](float v) { svc.SetBlendedBrightAdaptationSpeed(v, 0, 5); },
            progress);
        RenderFloatVar(actCtx, "Target Gray", svc.GetTargetGrayCount(activeProf), 0, 1, "%.6f",
            [&](uint64_t i) { return svc.GetTargetGrayByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetTargetGrayByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Target Gray", svc.GetTargetGrayByIndex(nextProf, nextVar), 0, 1, "%.6f",
                     [&](float v) { svc.SetTargetGrayByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedTargetGray(); },
            [&](float v) { svc.SetBlendedTargetGray(v, 0, 1); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();

      // Exposure Scale
      ImGui::PushID("exposure_scale");
      if (ImGui::TreeNodeEx("Exposure Scale", ImGuiTreeNodeFlags_None)) {
        RenderFloatVar(actCtx, "Min Scale", svc.GetMinScaleCount(activeProf), 0, 10, "%.6f",
            [&](uint64_t i) { return svc.GetMinScaleByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetMinScaleByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Min Scale", svc.GetMinScaleByIndex(nextProf, nextVar), 0, 10, "%.6f",
                     [&](float v) { svc.SetMinScaleByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedMinScale(); },
            [&](float v) { svc.SetBlendedMinScale(v, 0, 10); },
            progress);
        RenderFloatVar(actCtx, "Max Scale", svc.GetMaxScaleCount(activeProf), 0, 50, "%.6f",
            [&](uint64_t i) { return svc.GetMaxScaleByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetMaxScaleByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Max Scale", svc.GetMaxScaleByIndex(nextProf, nextVar), 0, 50, "%.6f",
                     [&](float v) { svc.SetMaxScaleByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedMaxScale(); },
            [&](float v) { svc.SetBlendedMaxScale(v, 0, 50); },
            progress);
        RenderFloatVar(actCtx, "Scale Override", svc.GetScaleOverrideCount(activeProf), 0, 10, "%.6f",
            [&](uint64_t i) { return svc.GetScaleOverrideByIndex(activeProf, i); },
            [&](uint64_t i, float v) { svc.SetScaleOverrideByIndex(activeProf, i, v); },
            [&]() { RenderNextFloat("Scale Override", svc.GetScaleOverrideByIndex(nextProf, nextVar), 0, 10, "%.6f",
                     [&](float v) { svc.SetScaleOverrideByIndex(nextProf, nextVar, v); }, nextVar); },
            [&]() { return svc.GetBlendedScaleOverride(); },
            [&](float v) { svc.SetBlendedScaleOverride(v, 0, 10); },
            progress);
        ImGui::TreePop();
      }
      ImGui::PopID();
    }

    // 11. Wind & Blending
    if (ImGui::CollapsingHeader("Wind & Blending", ImGuiTreeNodeFlags_None)) {
      RenderIntVar(actCtx, "Wind Type", svc.GetWindTypeCount(activeProf), 0, 3,
          [&](uint64_t i) { return svc.GetWindTypeByIndex(activeProf, i); },
          [&](uint64_t i, int32_t v) { svc.SetWindTypeByIndex(activeProf, i, v); },
          [&]() { RenderNextInt("Wind Type", svc.GetWindTypeByIndex(nextProf, nextVar), 0, 3,
                   [&](int32_t v) { svc.SetWindTypeByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderFloatVar(actCtx, "Speed Coef", svc.GetSpeedCoefCount(activeProf), 0, 5000, "%.1f",
          [&](uint64_t i) { return svc.GetSpeedCoefByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetSpeedCoefByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Speed Coef", svc.GetSpeedCoefByIndex(nextProf, nextVar), 0, 5000, "%.1f",
                   [&](float v) { svc.SetSpeedCoefByIndex(nextProf, nextVar, v); }, nextVar); },
          [&]() { return svc.GetBlendedSpeedCoef(); },
          [&](float v) { svc.SetBlendedSpeedCoef(v, 0, 5000); },
          progress);
      RenderFloatVar(actCtx, "Stability", svc.GetStabilityCount(activeProf), 0, 10, "%.1f",
          [&](uint64_t i) { return svc.GetStabilityByIndex(activeProf, i); },
          [&](uint64_t i, float v) { svc.SetStabilityByIndex(activeProf, i, v); },
          [&]() { RenderNextFloat("Stability", svc.GetStabilityByIndex(nextProf, nextVar), 0, 10, "%.1f",
                   [&](float v) { svc.SetStabilityByIndex(nextProf, nextVar, v); }, nextVar); });
      RenderIntVar(actCtx, "Blend Weight", svc.GetWeightCount(activeProf), 0, 10,
          [&](uint64_t i) { return svc.GetWeightByIndex(activeProf, i); },
          [&](uint64_t i, int32_t v) { svc.SetWeightByIndex(activeProf, i, v); },
          [&]() { RenderNextInt("Blend Weight", svc.GetWeightByIndex(nextProf, nextVar), 0, 10,
                   [&](int32_t v) { svc.SetWeightByIndex(nextProf, nextVar, v); }, nextVar); });
    }
  }

  // --- Final Stats ---
  ImGui::Separator();
}

}  // namespace UI
SPF_NS_END
