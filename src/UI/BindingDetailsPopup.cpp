#include "SPF/UI/BindingDetailsPopup.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/EnumMappings.hpp"
#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/Modules/InputDisplay.hpp"
#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/Utils/Signal.hpp"

#include "fmt/core.h"
#include "fmt/format.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

SPF_NS_BEGIN

namespace UI {
using namespace SPF::Localization;

BindingDetailsPopup::BindingDetailsPopup(Events::EventManager& eventManager, Config::IConfigService& configService)
    : m_eventManager(eventManager), m_configService(configService), m_onLanguageChangedSink(std::make_unique<Utils::Sink<void(const std::string&)>>(LocalizationManager::GetInstance().OnFrameworkLanguageChanged)) {
  m_onLanguageChangedSink->Connect<&BindingDetailsPopup::RefreshLocalizationOnLanguageChange>(this);

  m_conflictPressTypeMessage = "settings_window.conflict.press_type_message";
  m_conflictSwapQuestion = "settings_window.conflict.swap_question";
  m_conflictYesSwapButton = "settings_window.conflict.yes_swap_button";
  m_conflictCancelButton = "settings_window.conflict.cancel_button";

  RefreshLocalization();
}

void BindingDetailsPopup::RefreshLocalizationOnLanguageChange(const std::string& /*componentName*/) { RefreshLocalization(); }

void BindingDetailsPopup::RefreshLocalization() {
  auto& loc = LocalizationManager::GetInstance();

  m_popupTitleKey = loc.Get("settings_window.binding_details_popup.title");
  m_pressTypeLabelKey = loc.Get("settings_window.binding_details_popup.press_type_label");
  m_behaviorLabelKey = loc.Get("settings_window.binding_details_popup.behavior_label");
  m_behaviorToggleKey = loc.Get("settings_window.binding_details_popup.behavior_toggle");
  m_behaviorHoldKey = loc.Get("settings_window.binding_details_popup.behavior_hold");
  m_consumeLabelKey = loc.Get("settings_window.binding_details_popup.consume_label");
  m_thresholdLabelKey = loc.Get("settings_window.binding_details_popup.threshold_label");
  m_closeButtonKey = loc.Get("settings_window.binding_details_popup.close_button");

  m_modeLabelKey = loc.Get("settings_window.binding_details_popup.mode_label");
  m_modeAnalogKey = loc.Get("settings_window.binding_details_popup.mode_analog");
  m_modeDigitalKey = loc.Get("settings_window.binding_details_popup.mode_digital");
  m_deadzoneLabelKey = loc.Get("settings_window.binding_details_popup.deadzone_label");
  m_saturationLabelKey = loc.Get("settings_window.binding_details_popup.saturation_label");
  m_sensitivityLabelKey = loc.Get("settings_window.binding_details_popup.sensitivity_label");
  m_curveLabelKey = loc.Get("settings_window.binding_details_popup.curve_label");
  m_smoothingLabelKey = loc.Get("settings_window.binding_details_popup.smoothing_label");
  m_sideLabelKey = loc.Get("settings_window.binding_details_popup.side_label");
  m_sideBothKey = loc.Get("settings_window.binding_details_popup.side_both");
  m_sidePositiveKey = loc.Get("settings_window.binding_details_popup.side_positive");
  m_sideNegativeKey = loc.Get("settings_window.binding_details_popup.side_negative");
  m_rangeMinLabelKey = loc.Get("settings_window.binding_details_popup.range_min_label");
  m_rangeMaxLabelKey = loc.Get("settings_window.binding_details_popup.range_max_label");
  m_accumulatorModeLabelKey = loc.Get("settings_window.binding_details_popup.accumulator_mode_label");
  m_invertLabelKey = loc.Get("settings_window.binding_details_popup.invert_label");
}

void BindingDetailsPopup::Open(const std::string& actionFullName, const nlohmann::ordered_json& bindingJson) {
  m_editingBindingAction = actionFullName;
  m_editingBindingDetails = bindingJson;
  m_currentPressThreshold = bindingJson.value("press_threshold_ms", 500);
  m_originalBindingCopy = bindingJson;
  m_pressTypeSwapConflict.reset();
  m_pressTypeSwapNewValue.reset();
  m_shouldOpen = true;
}

void BindingDetailsPopup::Render() {
  auto& loc = LocalizationManager::GetInstance();

  if (m_shouldOpen) {
    ImGui::OpenPopup(m_popupTitleKey.c_str());
    m_shouldOpen = false;
  }

  if (!ImGui::BeginPopupModal(m_popupTitleKey.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  if (!m_editingBindingDetails.has_value() || !m_editingBindingAction.has_value()) {
    // Should not happen if popup is open, but as a safeguard
    ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
    return;
  }

  // Use a reference to simplify access and allow modification
  auto& bindingJson = m_editingBindingDetails.value();
  const auto& actionFullName = m_editingBindingAction.value();

  auto addTooltip = [&](const std::string& key) {
    std::string descKey = key + "_description";
    std::string description = loc.Get(descKey);
    if (description != descKey) {
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(description.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
      }
    }
  };

  std::string type = bindingJson.value("type", "");
  bool isAxis = (type == "gamepad_axis" || type == "mouse_axis" || type == "joystick_axis");
  bool isMouse = (type == "mouse_axis");
  std::string mode = bindingJson.value("mode", isAxis ? "analog" : "digital");

  if (isAxis) {
    ImGui::TextUnformatted(m_modeLabelKey.c_str());
    addTooltip("settings_window.binding_details_popup.mode_label");
    ImGui::SameLine();

    if (isMouse) ImGui::BeginDisabled();

    if (ImGui::RadioButton(m_modeAnalogKey.c_str(), mode == "analog")) {
      bindingJson["mode"] = "analog";
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "mode", "analog"});
      m_originalBindingCopy = bindingJson;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(m_modeDigitalKey.c_str(), mode == "digital")) {
      bindingJson["mode"] = "digital";
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "mode", "digital"});
      m_originalBindingCopy = bindingJson;
    }

    if (isMouse) ImGui::EndDisabled();

    ImGui::Separator();
  }

  if (mode == "analog") {
    // --- Analog Axis Specific Settings ---
    std::string keyName = bindingJson.value("key", "");
    bool isTrigger = (keyName == "LEFT_TRIGGER_AXIS" || keyName == "RIGHT_TRIGGER_AXIS");
    float rMin = bindingJson.value("range_min", isTrigger ? 0.0f : -1.0f);
    float rMax = bindingJson.value("range_max", 1.0f);
    bool isCentered = (rMin < -0.1f);

    // We need a stable input object to maintain smoothing state and get hardware codes
    static nlohmann::ordered_json cachedJson;
    static std::shared_ptr<Modules::IBindableInput> liveInput;
    if (cachedJson != bindingJson) {
      liveInput = Modules::InputFactory::CreateFromJson(bindingJson);
      cachedJson = bindingJson;
    }

    // Pre-fetch values for graph logic
    float deadzone = isMouse ? 0.0f : bindingJson.value("deadzone", 0.0f);
    float saturation = isMouse ? 1.0f : bindingJson.value("saturation", 1.0f);
    float sensitivity = bindingJson.value("sensitivity", 1.0f);
    std::string curve = isMouse ? "linear" : bindingJson.value("curve", "linear");
    float smoothing = isMouse ? 0.0f : bindingJson.value("smoothing", 0.0f);
    bool accumulator = bindingJson.value("accumulator", isMouse);  // Mouse is always accumulator

    if (isCentered && !isMouse) {
      if (ImGui::Checkbox(m_accumulatorModeLabelKey.c_str(), &accumulator)) {
        bindingJson["accumulator"] = accumulator;
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "accumulator", accumulator});
        m_originalBindingCopy = bindingJson;
      }
      addTooltip("settings_window.binding_details_popup.accumulator_mode_label");
    }

    if (!isMouse && !accumulator) {
      // Deadzone
      if (ImGui::SliderFloat(m_deadzoneLabelKey.c_str(), &deadzone, 0.0f, 0.5f, "%.2f")) {
        bindingJson["deadzone"] = deadzone;
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "deadzone", deadzone});
        m_originalBindingCopy = bindingJson;
      }
      addTooltip("settings_window.binding_details_popup.deadzone_label");

      // Saturation
      if (ImGui::SliderFloat(m_saturationLabelKey.c_str(), &saturation, 0.5f, 1.0f, "%.2f")) {
        bindingJson["saturation"] = saturation;
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "saturation", saturation});
        m_originalBindingCopy = bindingJson;
      }
      addTooltip("settings_window.binding_details_popup.saturation_label");
    }

    // Sensitivity
    if (ImGui::SliderFloat(m_sensitivityLabelKey.c_str(), &sensitivity, 0.1f, 5.0f, "%.1f")) {
      bindingJson["sensitivity"] = sensitivity;
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "sensitivity", sensitivity});
      m_originalBindingCopy = bindingJson;
    }
    addTooltip("settings_window.binding_details_popup.sensitivity_label");

    if (!isMouse && !accumulator) {
      // Curve
      if (ImGui::BeginCombo(m_curveLabelKey.c_str(), curve.c_str())) {
        const char* curves[] = {"linear", "exponential", "logarithmic", "s-curve"};
        for (auto c : curves) {
          if (ImGui::Selectable(c, curve == c)) {
            curve = c;
            bindingJson["curve"] = c;
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "curve", c});
            m_originalBindingCopy = bindingJson;
          }
        }
        ImGui::EndCombo();
      }
      addTooltip("settings_window.binding_details_popup.curve_label");

      // Smoothing
      if (ImGui::SliderFloat(m_smoothingLabelKey.c_str(), &smoothing, 0.0f, 0.95f, "%.2f")) {
        bindingJson["smoothing"] = smoothing;
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "smoothing", smoothing});
        m_originalBindingCopy = bindingJson;
      }
      addTooltip("settings_window.binding_details_popup.smoothing_label");
    }

    // Range settings based on Side
    std::string side = bindingJson.value("side", "both");
    float rMinLimit = isMouse ? -100.0f : (isTrigger ? 0.0f : -1.0f);
    float rMaxLimit = isMouse ? 100.0f : 1.0f;

    if (side == "positive") {
      if (rMinLimit < 0.0f) rMinLimit = 0.0f;
    } else if (side == "negative") {
      if (rMaxLimit > 0.0f) rMaxLimit = 0.0f;
    }

    // Range Min
    rMin = bindingJson.value("range_min", rMinLimit);
    if (ImGui::SliderFloat(m_rangeMinLabelKey.c_str(), &rMin, rMinLimit, rMax, "%.2f")) {
      bindingJson["range_min"] = rMin;
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "range_min", rMin});
      m_originalBindingCopy = bindingJson;
    }
    addTooltip("settings_window.binding_details_popup.range_min_label");

    // Range Max
    rMax = bindingJson.value("range_max", rMaxLimit);
    if (ImGui::SliderFloat(m_rangeMaxLabelKey.c_str(), &rMax, rMin, rMaxLimit, "%.2f")) {
      bindingJson["range_max"] = rMax;
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "range_max", rMax});
      m_originalBindingCopy = bindingJson;
    }
    addTooltip("settings_window.binding_details_popup.range_max_label");

    // Invert
    bool invert = bindingJson.value("invert", false);
    if (ImGui::Checkbox(m_invertLabelKey.c_str(), &invert)) {
      bindingJson["invert"] = invert;
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "invert", invert});
      m_originalBindingCopy = bindingJson;
    }
    addTooltip("settings_window.binding_details_popup.invert_label");

    // Side
    ImGui::TextUnformatted(m_sideLabelKey.c_str());
    addTooltip("settings_window.binding_details_popup.side_label");
    ImGui::SameLine();

    if (isTrigger) ImGui::BeginDisabled();

    if (ImGui::RadioButton(m_sideBothKey.c_str(), side == "both")) {
      bindingJson["side"] = "both";
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "side", "both"});
      m_originalBindingCopy = bindingJson;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(m_sidePositiveKey.c_str(), side == "positive")) {
      bindingJson["side"] = "positive";
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "side", "positive"});
      m_originalBindingCopy = bindingJson;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(m_sideNegativeKey.c_str(), side == "negative")) {
      bindingJson["side"] = "negative";
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "side", "negative"});
      m_originalBindingCopy = bindingJson;
    }

    if (isTrigger) ImGui::EndDisabled();

    ImGui::Separator();

    // --- DIAGNOSTICS & GRAPH ---
    auto& inputMgr = Input::InputManager::GetInstance();
    if (liveInput) {
      // Create physical space for the floating labels above the canvas
      ImGui::Dummy(ImVec2(0, 40.0f));
    }

    ImVec2 canvas_p = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImVec2(ImGui::GetContentRegionAvail().x, 150.0f);
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), ImColor(20, 20, 20, 255));
    draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), ImColor(80, 80, 80, 255));

    // Helper to map normalized function value to screen coordinates
    auto to_canvas = [&](float fx, float fy) -> ImVec2 {
      float x_pct, y_pct;
      if (isCentered) {
        x_pct = (fx + 1.0f) * 0.5f;
        y_pct = 1.0f - (fy + 1.0f) * 0.5f;
      } else {
        x_pct = fx;
        y_pct = 1.0f - fy;
      }
      return ImVec2(canvas_p.x + x_pct * canvas_sz.x, canvas_p.y + y_pct * canvas_sz.y);
    };

    // Draw Grid Lines
    if (isCentered) {
      draw_list->AddLine(to_canvas(0, -1), to_canvas(0, 1), ImColor(60, 60, 60, 255));  // Center X
      draw_list->AddLine(to_canvas(-1, 0), to_canvas(1, 0), ImColor(60, 60, 60, 255));  // Center Y
    }

    // Function to calculate curve
    auto get_mapped_magnitude = [&](float magnitude) -> float {
      if (isMouse || accumulator) return std::clamp(magnitude, 0.0f, 1.0f);

      float processed = 0.0f;
      float safeSaturation = (saturation > deadzone + 0.01f) ? saturation : deadzone + 0.01f;
      if (magnitude > deadzone) {
        processed = (magnitude - deadzone) / (safeSaturation - deadzone);
        processed = (processed > 1.0f) ? 1.0f : processed;
      }
      processed *= sensitivity;
      if (curve == "exponential")
        processed = processed * processed;
      else if (curve == "logarithmic")
        processed = std::sqrt(processed);
      else if (curve == "s-curve")
        processed = processed * processed * (3.0f - 2.0f * processed);
      return std::clamp(processed, 0.0f, 1.0f);
    };

    // Draw the Curve
    const int segments = 100;
    float step = isCentered ? 2.0f / segments : 1.0f / segments;
    float startX = isCentered ? -1.0f : 0.0f;

    for (int i = 0; i < segments; i++) {
      float x1 = startX + i * step;
      float x2 = startX + (i + 1) * step;
      float fy1, fy2;

      if (isCentered) {
        auto get_side_val = [&](float x) {
          if (side == "positive" && x < 0) return 0.0f;
          if (side == "negative" && x > 0) return 0.0f;
          float val = get_mapped_magnitude(std::abs(x));
          return (x < 0) ? -val : val;
        };
        fy1 = get_side_val(x1);
        fy2 = get_side_val(x2);
      } else {
        fy1 = get_mapped_magnitude(x1);
        fy2 = get_mapped_magnitude(x2);
      }

      if (invert) {
        fy1 = isCentered ? -fy1 : 1.0f - fy1;
        fy2 = isCentered ? -fy2 : 1.0f - fy2;
      }
      draw_list->AddLine(to_canvas(x1, fy1), to_canvas(x2, fy2), ImColor(255, 255, 0, 255), 2.0f);
    }

    // Labels for the graph axis (taking inversion into account)
    std::string lMin, lMax, lMid;
    if (isCentered) {
      lMin = fmt::format("{:.1f}", invert ? 1.0f : -1.0f);
      lMax = fmt::format("{:.1f}", invert ? -1.0f : 1.0f);
      lMid = "0.0";
    } else {
      lMin = fmt::format("{:.1f}", invert ? 1.0f : 0.0f);
      lMax = fmt::format("{:.1f}", invert ? 0.0f : 1.0f);
    }

    draw_list->AddText(ImVec2(canvas_p.x + 5, canvas_p.y + canvas_sz.y - 20), ImColor(180, 180, 180, 255), lMin.c_str());
    draw_list->AddText(ImVec2(canvas_p.x + canvas_sz.x - 35, canvas_p.y + canvas_sz.y - 20), ImColor(180, 180, 180, 255), lMax.c_str());
    if (isCentered) {
      draw_list->AddText(ImVec2(canvas_p.x + canvas_sz.x * 0.5f - 10, canvas_p.y + canvas_sz.y - 20), ImColor(180, 180, 180, 255), lMid.c_str());
    }

    // Live Indicator
    static float uiSmoothedInput = 0.0f;
    static uint32_t currentHwCode = 0;

    if (liveInput && liveInput->GetHardwareCode() != currentHwCode) {
      uiSmoothedInput = 0.0f;
      currentHwCode = liveInput->GetHardwareCode();
    }

    if (liveInput) {
      auto const& activeAxes = inputMgr.GetCurrentlyActiveAxisValues();
      uint32_t hwCode = liveInput->GetHardwareCode();
      float rawInput = 0.0f;
      if (activeAxes.count(hwCode)) rawInput = activeAxes.at(hwCode);

      // 1. Normalize physical input to [-1, 1] or [0, 1]
      float normRaw = 0.0f;
      if (std::abs(rMax - rMin) > 0.001f) normRaw = (rawInput - rMin) / (rMax - rMin);
      normRaw = std::clamp(normRaw, 0.0f, 1.0f);
      float physicalPos = isCentered ? (normRaw * 2.0f - 1.0f) : normRaw;

      // 2. Draw Vertical Raw Line (Blue)
      ImVec2 rawTop = to_canvas(physicalPos, 1.0f);
      ImVec2 rawBottom = to_canvas(physicalPos, isCentered ? -1.0f : 0.0f);
      draw_list->AddLine(rawTop, rawBottom, ImColor(0, 120, 255, 200), 1.5f);

      // 3. Draw Raw Value Label (Top)
      std::string rawValStr = fmt::format("In: {:.4f}", rawInput);
      ImVec2 rawTxtSz = ImGui::CalcTextSize(rawValStr.c_str());

      // Edge-aware X positioning
      float labelX = rawTop.x - rawTxtSz.x * 0.5f;
      if (labelX < canvas_p.x) {
        labelX = rawTop.x + 4;  // Flip to right of line
      } else if (labelX + rawTxtSz.x > canvas_p.x + canvas_sz.x) {
        labelX = rawTop.x - rawTxtSz.x - 4;  // Flip to left of line
      }

      ImVec2 rawLabelPos = ImVec2(labelX, canvas_p.y - 35);

      draw_list->AddRectFilled(ImVec2(rawLabelPos.x - 4, rawLabelPos.y - 2), ImVec2(rawLabelPos.x + rawTxtSz.x + 4, rawLabelPos.y + rawTxtSz.y + 2), ImColor(255, 255, 255, 230), 3.0f);
      draw_list->AddText(rawLabelPos, ImColor(0, 0, 0, 255), rawValStr.c_str());

      // 4. Apply visual smoothing to the indicator position
      float alpha = 1.0f - std::clamp(smoothing, 0.0f, 0.99f);
      uiSmoothedInput = uiSmoothedInput + alpha * (physicalPos - uiSmoothedInput);

      float dotX = uiSmoothedInput;
      float dotY = 0.0f;
      float finalOutVal = liveInput->GetValue(inputMgr.GetCurrentlyPressedHardwareCodes(), activeAxes);

      if (accumulator && !isMouse) {
        dotX = physicalPos;
        dotY = finalOutVal;
      } else {
        bool isSideDisabled = false;
        if (isCentered) {
          if (side == "positive" && dotX < 0)
            isSideDisabled = true;
          else if (side == "negative" && dotX > 0)
            isSideDisabled = true;
        }
        if (!isSideDisabled) {
          float magY = get_mapped_magnitude(std::abs(dotX));
          dotY = (isCentered && dotX < 0) ? -magY : magY;
        }
        if (invert) dotY = isCentered ? -dotY : 1.0f - dotY;
      }

      ImVec2 indicatorPos = to_canvas(dotX, dotY);

      // 5. Numerical Value Display (Out)
      std::string valStr = fmt::format("Out: {:.4f}", finalOutVal);
      ImVec2 txtSz = ImGui::CalcTextSize(valStr.c_str());
      float offX = (indicatorPos.x + 12 + txtSz.x > canvas_p.x + canvas_sz.x) ? (-12 - txtSz.x) : 12;
      ImVec2 bgPos = ImVec2(indicatorPos.x + offX, indicatorPos.y - 10);

      ImGui::Dummy(canvas_sz);  // Reserve space for graph

      // Draw indicator
      draw_list->AddCircleFilled(indicatorPos, 6.0f, ImColor(255, 0, 0, 255));
      draw_list->AddCircle(indicatorPos, 7.5f, ImColor(255, 255, 255, 200), 12, 1.5f);
      draw_list->AddRectFilled(ImVec2(bgPos.x - 4, bgPos.y - 2), ImVec2(bgPos.x + txtSz.x + 4, bgPos.y + txtSz.y + 2), ImColor(255, 255, 255, 230), 3.0f);
      draw_list->AddText(bgPos, ImColor(0, 0, 0, 255), valStr.c_str());
    } else {
      ImGui::Dummy(canvas_sz);
    }

    ImGui::Separator();
  } else {
    // --- Digital Mode Settings (Buttons or Axis-as-Button) ---

    if (isAxis) {
      float threshold = bindingJson.value("threshold", 0.5f);
      if (ImGui::SliderFloat(m_thresholdLabelKey.c_str(), &threshold, 0.05f, 0.95f, "%.2f")) {
        bindingJson["threshold"] = threshold;
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "threshold", threshold});
        m_originalBindingCopy = bindingJson;
      }
      addTooltip("settings_window.binding_details_popup.threshold_label");
      ImGui::Separator();
    }

    // --- Press Type Setting with Radio Buttons ---
    ImGui::TextUnformatted(m_pressTypeLabelKey.c_str());
    addTooltip("settings_window.binding_details_popup.press_type_label");
    ImGui::SameLine();
    ImGui::PushID("details_press_type_radios");

    std::string currentPressTypeStr = bindingJson.value("press_type", "short");

    // Create a temporary copy for the radio buttons to modify.
    // This allows us to detect a change and run logic before applying it.
    std::string selectedPressTypeStr = currentPressTypeStr;

    for (const auto& pair : Config::PressTypeMap) {
      if (ImGui::RadioButton(loc.Get(pair.second.loc_key).c_str(), selectedPressTypeStr == pair.second.string_id)) {
        selectedPressTypeStr = pair.second.string_id;
      }
      ImGui::SameLine();
    }
    // Remove the last SameLine
    ImGui::NewLine();

    // If the user selected a new press type, check for conflicts.
    if (selectedPressTypeStr != currentPressTypeStr) {
      auto input = Modules::InputFactory::CreateFromJson(bindingJson);
      if (input && input->IsValid()) {
        auto& kbm = Modules::KeyBindsManager::GetInstance();
        Input::PressType newPressType = (selectedPressTypeStr == "long") ? Input::PressType::Long : Input::PressType::Short;  // Simplified for now

        auto conflict = kbm.FindConflictForBinding(*input, newPressType, actionFullName);

        if (conflict) {
          // Conflict found! Store details to show the inline confirmation UI.
          m_pressTypeSwapConflict = conflict;
          m_pressTypeSwapNewValue = selectedPressTypeStr;
        } else {
          // No conflict, apply the change directly.
          bindingJson["press_type"] = selectedPressTypeStr;
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "press_type", selectedPressTypeStr});
          m_originalBindingCopy = bindingJson;  // Update original copy for chaining
        }
      }
    }
    ImGui::PopID();

    // --- Inline Conflict Resolution UI ---
    ImGui::Spacing();
    if (m_pressTypeSwapConflict.has_value()) {
      // Safeguard check, just in case
      if (!m_pressTypeSwapNewValue.has_value() || !m_editingBindingAction.has_value()) {
        // Should not happen, reset state
        m_pressTypeSwapConflict.reset();
        m_pressTypeSwapNewValue.reset();
      } else {
        ImGui::Separator();
        const auto& conflictingActionName = m_pressTypeSwapConflict->first;
        const auto& newPressType = m_pressTypeSwapNewValue.value();
        const auto& originalPressType = m_originalBindingCopy.value("press_type", "short");

        // "settings_window.*" keys live in the framework's own localization files,
        // so look them up under "framework" regardless of which plugin's action is
        // being edited (matching KeyCapturePopup's equivalent conflict text calls).
        std::string pressType = loc.Get("enums.press_type." + newPressType);
        std::string markdownText = loc.GetFormatted("framework", m_conflictPressTypeMessage, pressType);
        Typography::RenderMarkdownText(markdownText, TextStyle::Bold().Color(UI::Colors::YELLOW).Align(TextAlign::Center));

        // Display Conflicting Plugin and Action Name
        size_t lastDot = conflictingActionName.rfind('.');
        std::string group = (lastDot != std::string::npos) ? conflictingActionName.substr(0, lastDot) : "";
        size_t firstDot = group.find('.');
        std::string ownerName = (firstDot != std::string::npos) ? group.substr(0, firstDot) : group;
        std::string ownerDisplayName = ownerName;
        auto it_owner = m_configService.GetAllComponentInfo().find(ownerName);
        if (it_owner != m_configService.GetAllComponentInfo().end() && it_owner->second.name.has_value()) {
          ownerDisplayName = it_owner->second.name.value();
        }
        std::string actionDisplayName = Modules::GetTranslatedActionName(m_configService, conflictingActionName);
        std::string displayText = fmt::format("{} - {}", ownerDisplayName, actionDisplayName);
        Typography::Text(TextStyle::Bold().Color(UI::Colors::GRAY).Align(TextAlign::Center), displayText.c_str());

        Typography::Text(TextStyle::Bold().Wrapped().Align(TextAlign::Center), loc.Get(m_conflictSwapQuestion).c_str());

        // Centered Buttons
        const char* yesText = loc.Get(m_conflictYesSwapButton).c_str();
        const char* cancelText = loc.Get(m_conflictCancelButton).c_str();
        float buttonWidth1 = ImGui::CalcTextSize(yesText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float buttonWidth2 = ImGui::CalcTextSize(cancelText).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float totalButtonsWidth = buttonWidth1 + buttonWidth2 + ImGui::GetStyle().ItemSpacing.x;
        float availableWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - totalButtonsWidth) * 0.5f);

        if (Button(yesText)) {
          const auto& conflictingBindingJson = m_pressTypeSwapConflict->second;
          const auto& actionBeingEdited = m_editingBindingAction.value();

          // 1. Update the conflicting action to use the old press type
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({conflictingActionName, conflictingBindingJson, "press_type", originalPressType});

          // 2. Update the action being edited to use the new press type
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionBeingEdited, m_originalBindingCopy, "press_type", newPressType});

          // 3. Update local state to reflect the change and allow chaining
          bindingJson["press_type"] = newPressType;
          m_originalBindingCopy["press_type"] = newPressType;

          // 4. Reset state to hide this UI
          m_pressTypeSwapConflict.reset();
          m_pressTypeSwapNewValue.reset();
        }
        ImGui::SameLine();
        if (Button(cancelText)) {
          // Just reset state to hide this UI. The radio button will revert visually
          // because `selectedPressTypeStr` is a local temporary variable and bindingJson is not updated.
          m_pressTypeSwapConflict.reset();
          m_pressTypeSwapNewValue.reset();
        }
        ImGui::Separator();
      }
    }
  }

  // --- Behavior Setting ---
  if (mode == "digital") {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextUnformatted(m_behaviorLabelKey.c_str());
    addTooltip("settings_window.binding_details_popup.behavior_label");
    ImGui::SameLine();
    ImGui::PushID("details_behavior");
    std::string currentBehavior = bindingJson.value("behavior", "toggle");
    if (ImGui::RadioButton(m_behaviorToggleKey.c_str(), currentBehavior == "toggle")) {
      bindingJson["behavior"] = "toggle";
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "behavior", "toggle"});
      m_originalBindingCopy = bindingJson;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(m_behaviorHoldKey.c_str(), currentBehavior == "hold")) {
      bindingJson["behavior"] = "hold";
      m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "behavior", "hold"});
      m_originalBindingCopy = bindingJson;
    }
    ImGui::PopID();
  }

  // --- Consume Policy Setting ---
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::TextUnformatted(m_consumeLabelKey.c_str());
  addTooltip("settings_window.binding_details_popup.consume_label");
  ImGui::SameLine();
  ImGui::PushID("details_consume");
  std::string currentConsume = bindingJson.value("consume", "never");
  std::string currentConsumeDisplay = currentConsume;  // Default to string_id
  for (const auto& pair : Config::ConsumptionPolicyMap) {
    if (pair.second.string_id == currentConsume) {
      currentConsumeDisplay = loc.Get(pair.second.loc_key);
      break;
    }
  }
  if (ImGui::BeginCombo("##consume", currentConsumeDisplay.c_str())) {
    for (const auto& pair : Config::ConsumptionPolicyMap) {
      bool is_selected = (currentConsume == pair.second.string_id);
      if (ImGui::Selectable(loc.Get(pair.second.loc_key).c_str(), is_selected)) {
        bindingJson["consume"] = pair.second.string_id;  // Update local state
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "consume", pair.second.string_id});
        m_originalBindingCopy = bindingJson;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopID();

  // --- Press Threshold Setting ---
  if (mode == "digital") {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextUnformatted(m_thresholdLabelKey.c_str());
    ImGui::SameLine();
    ImGui::PushID("details_press_threshold");

    // The slider and buttons will modify m_currentPressThreshold, which persists across frames.
    bool valueChanged = false;

    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
      m_currentPressThreshold -= 5;
      valueChanged = true;
    }
    ImGui::PopButtonRepeat();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    // Use the return value of SliderInt to detect changes made by dragging.
    if (ImGui::SliderInt("##pressthreshold", &m_currentPressThreshold, 50, 5000, "%d ms")) {
      valueChanged = true;
    }
    // IsItemDeactivatedAfterEdit captures the moment the user releases the mouse.
    bool isSliderDeactivated = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::SameLine();

    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
      m_currentPressThreshold += 5;
      valueChanged = true;
    }
    ImGui::PopButtonRepeat();

    // Clamp the value on every frame to provide immediate feedback.
    m_currentPressThreshold = std::clamp(m_currentPressThreshold, 50, 5000);

    // Update happens when a button is clicked, or when the user stops dragging the slider.
    if (valueChanged || isSliderDeactivated) {
      // Round the value to the nearest 5 before saving.
      int finalThreshold = static_cast<int>(roundf(m_currentPressThreshold / 5.0f)) * 5;
      finalThreshold = std::clamp(finalThreshold, 50, 5000);

      // Update the UI state immediately to the rounded value.
      m_currentPressThreshold = finalThreshold;

      if (m_originalBindingCopy.value("press_threshold_ms", 500) != finalThreshold) {
        nlohmann::ordered_json oldBinding = m_originalBindingCopy;
        m_originalBindingCopy["press_threshold_ms"] = finalThreshold;
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, oldBinding, "press_threshold_ms", finalThreshold});

        bindingJson["press_threshold_ms"] = finalThreshold;
      }
    }
    ImGui::PopID();
  }

  ImGui::Separator();

  if (Button(m_closeButtonKey.c_str())) {
    m_editingBindingDetails.reset();
    m_editingBindingAction.reset();
    ImGui::CloseCurrentPopup();
  }

  ImGui::EndPopup();
}

}  // namespace UI
SPF_NS_END
