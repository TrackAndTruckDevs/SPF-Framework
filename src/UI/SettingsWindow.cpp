#include "SPF/UI/SettingsWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/EnumMappings.hpp"
#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/ChordInput.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/Utils/Signal.hpp"

#include "fmt/core.h"
#include "fmt/format.h"
#include "imgui.h"
#include "nlohmann/json_fwd.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>  // For roundf
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// IWYU insists on a direct provider for _s functions.
// MinGW: pull in MSVC-compat decl; MSVC gets them from <cstdio> natively.
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <sec_api/string_s.h>
#endif

SPF_NS_BEGIN

namespace UI {
using namespace SPF::Localization;
using namespace SPF::Logging;
using namespace SPF::System;

SettingsWindow::SettingsWindow(const std::string& componentName, const std::string& windowId, Config::IConfigService& configService, const std::vector<std::string>& logLevels, Events::EventManager& eventManager)
    : BaseWindow(componentName, windowId),
      m_configService(configService),
      m_logLevels(logLevels),
      m_eventManager(eventManager),
      m_onFocusComponentSink(std::make_unique<Utils::Sink<void(const Events::UI::FocusComponentInSettingsWindow&)>>(eventManager.System.OnFocusComponentInSettingsWindow)),
      m_onInputCaptureUpdateSink(std::make_unique<Utils::Sink<void(const Input::InputCaptureUpdate&)>>(eventManager.System.OnInputCaptureUpdate)),
      m_onKeybindsModifiedSink(std::make_unique<Utils::Sink<void(const Events::Config::OnKeybindsModified&)>>(eventManager.System.OnKeybindsModified)) {
  m_defaultTitle = "Settings";
  m_titleLocalizationKey = "settings_window.title";
  m_keybindsDrawerHeight = m_keybindsDrawerMinHeight;

  m_conflictPressTypeMessage = "settings_window.conflict.press_type_message";
  m_conflictSwapQuestion = "settings_window.conflict.swap_question";
  m_conflictYesSwapButton = "settings_window.conflict.yes_swap_button";
  m_conflictCancelButton = "settings_window.conflict.cancel_button";

  m_onFocusComponentSink->Connect<&SettingsWindow::OnFocusComponent>(this);
  m_onInputCaptureUpdateSink->Connect<&SettingsWindow::OnInputCaptureUpdate>(this);
  m_onKeybindsModifiedSink->Connect<&SettingsWindow::UpdateHardwareCodeUsageCount>(this);

  RefreshLocalization();

  UpdateHardwareCodeUsageCount({});
}
void SettingsWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_keybindsDrawerTitleKey = loc.Get("settings_window.keybinds_drawer.title");
  m_keybindsActionHeaderKey = loc.Get("settings_window.keybinds_drawer.table.action");
  m_keybindsKeyHeaderKey = loc.Get("settings_window.keybinds_drawer.table.key");

  m_keyCapturePopupTitleKey = loc.Get("settings_window.key_capture_popup.title");
  m_keyCapturePressKeyTextKey = loc.Get("settings_window.key_capture_popup.press_key_text");
  m_keyCaptureDeleteButtonKey = loc.Get("settings_window.key_capture_popup.delete_button");
  m_keyCaptureCancelButtonKey = loc.Get("settings_window.key_capture_popup.cancel_button");
  m_keyCaptureConflictTitleKey = loc.Get("settings_window.key_capture_popup.conflict_title");
  m_keyCaptureConflictTextDetailedKey = loc.Get("settings_window.key_capture_popup.conflict_text_detailed");
  m_keyCaptureReassignShortPressButtonKey = loc.Get("settings_window.key_capture_popup.reassign_short_press_button");
  m_keyCaptureReassignLongPressButtonKey = loc.Get("settings_window.key_capture_popup.reassign_long_press_button");
  m_keyCaptureReassignPositiveSideButtonKey = loc.Get("settings_window.key_capture_popup.reassign_positive_side_button");
  m_keyCaptureReassignNegativeSideButtonKey = loc.Get("settings_window.key_capture_popup.reassign_negative_side_button");
  m_keyCaptureAddShortPressButtonKey = loc.Get("settings_window.key_capture_popup.add_short_press_button");
  m_keyCaptureAddLongPressButtonKey = loc.Get("settings_window.key_capture_popup.add_long_press_button");
  m_keyCaptureAddPositiveSideButtonKey = loc.Get("settings_window.key_capture_popup.add_positive_side_button");
  m_keyCaptureAddNegativeSideButtonKey = loc.Get("settings_window.key_capture_popup.add_negative_side_button");
  m_keyCaptureReassignEntireAxisButtonKey = loc.Get("settings_window.key_capture_popup.reassign_entire_axis_button");
  m_keyCaptureActionListFormatKey = loc.Get("settings_window.key_capture_popup.action_list_format");

  m_bindingDetailsPopupTitleKey = loc.Get("settings_window.binding_details_popup.title");
  m_bindingDetailsPressTypeLabelKey = loc.Get("settings_window.binding_details_popup.press_type_label");
  m_bindingDetailsBehaviorLabelKey = loc.Get("settings_window.binding_details_popup.behavior_label");
  m_bindingDetailsBehaviorToggleKey = loc.Get("settings_window.binding_details_popup.behavior_toggle");
  m_bindingDetailsBehaviorHoldKey = loc.Get("settings_window.binding_details_popup.behavior_hold");
  m_bindingDetailsConsumeLabelKey = loc.Get("settings_window.binding_details_popup.consume_label");
  m_bindingDetailsThresholdLabelKey = loc.Get("settings_window.binding_details_popup.threshold_label");
  m_bindingDetailsCloseButtonKey = loc.Get("settings_window.binding_details_popup.close_button");

  m_bindingDetailsModeLabelKey = loc.Get("settings_window.binding_details_popup.mode_label");
  m_bindingDetailsModeAnalogKey = loc.Get("settings_window.binding_details_popup.mode_analog");
  m_bindingDetailsModeDigitalKey = loc.Get("settings_window.binding_details_popup.mode_digital");
  m_bindingDetailsDeadzoneLabelKey = loc.Get("settings_window.binding_details_popup.deadzone_label");
  m_bindingDetailsSaturationLabelKey = loc.Get("settings_window.binding_details_popup.saturation_label");
  m_bindingDetailsSensitivityLabelKey = loc.Get("settings_window.binding_details_popup.sensitivity_label");
  m_bindingDetailsCurveLabelKey = loc.Get("settings_window.binding_details_popup.curve_label");
  m_bindingDetailsSmoothingLabelKey = loc.Get("settings_window.binding_details_popup.smoothing_label");
  m_bindingDetailsSideLabelKey = loc.Get("settings_window.binding_details_popup.side_label");
  m_bindingDetailsSideBothKey = loc.Get("settings_window.binding_details_popup.side_both");
  m_bindingDetailsSidePositiveKey = loc.Get("settings_window.binding_details_popup.side_positive");
  m_bindingDetailsSideNegativeKey = loc.Get("settings_window.binding_details_popup.side_negative");
  m_bindingDetailsRangeMinLabelKey = loc.Get("settings_window.binding_details_popup.range_min_label");
  m_bindingDetailsRangeMaxLabelKey = loc.Get("settings_window.binding_details_popup.range_max_label");
  m_bindingDetailsAccumulatorModeLabelKey = loc.Get("settings_window.binding_details_popup.accumulator_mode_label");
  m_bindingDetailsInvertLabelKey = loc.Get("settings_window.binding_details_popup.invert_label");

  m_enumPressTypeShortKey = loc.Get("enums.press_type.short");
  m_enumPressTypeLongKey = loc.Get("enums.press_type.long");
  m_enumSideBothKey = loc.Get("enums.side.both");
  m_enumSidePositiveKey = loc.Get("enums.side.positive");
  m_enumSideNegativeKey = loc.Get("enums.side.negative");

  m_keybindsUnassignedTextKey = loc.Get("settings_window.keybinds_drawer.unassigned_text");

  m_noConfigurableComponentsKey = loc.Get("settings_window.main_area.no_configurable_components");
  m_componentInfoErrorKey = loc.Get("settings_window.main_area.component_info_error");
  m_noConfigurableSystemsKey = loc.Get("settings_window.main_area.no_configurable_systems");
  m_keybindsNotAvailableKey = loc.Get("settings_window.keybinds_drawer.table.not_available");
  m_nullValueFormatKey = loc.Get("settings_window.main_area.null_value_format");
  m_settingHeaderKey = loc.Get("settings_window.table.setting");
  m_valueHeaderKey = loc.Get("settings_window.table.value");
}

void SettingsWindow::OnFocusComponent(const Events::UI::FocusComponentInSettingsWindow& e) {
  m_currentComponent = e.componentName;
  SetVisibility(true);
  Focus();
}

void SettingsWindow::PopulateConfigurableComponents() {
  // This function is now obsolete and replaced by dynamic logic in RenderContent.
}

void SettingsWindow::UpdateHardwareCodeUsageCount(const Events::Config::OnKeybindsModified& e) {
  m_hardwareCodeUsageCount.clear();
  auto config = m_configService.GetMergedConfig("keybinds");
  if (!config) return;

  for (const auto& [group, actions] : config->items()) {
    for (const auto& [actionName, actionObject] : actions.items()) {
      if (actionObject.is_object() && actionObject.contains("bindings") && actionObject["bindings"].is_array()) {
        for (const auto& binding : actionObject["bindings"]) {
          auto input = Modules::InputFactory::CreateFromJson(binding);
          if (input && input->IsValid()) {
            m_hardwareCodeUsageCount[input->GetHardwareCode()]++;
          }
        }
      }
    }
  }
}

std::string SettingsWindow::GetTranslatedActionName(const std::string& fullActionName) const {
  // 1. Parse the fullActionName into group and actionName
  size_t lastDot = fullActionName.rfind('.');
  if (lastDot == std::string::npos) {
    return fullActionName;  // Cannot parse, return raw name
  }
  std::string group = fullActionName.substr(0, lastDot);
  std::string actionName = fullActionName.substr(lastDot + 1);

  // 2. Find the actionObject in m_keybindsConfig
  auto& loc = LocalizationManager::GetInstance();
  if (m_configService.GetMergedConfig("keybinds") && m_configService.GetMergedConfig("keybinds")->contains(group)) {
    const auto& groupObject = (*m_configService.GetMergedConfig("keybinds"))[group];
    if (groupObject.contains(actionName)) {
      const auto& actionObject = groupObject[actionName];

      // 3. Check for _meta and titleKey
      if (actionObject.is_object() && actionObject.contains("_meta")) {
        const auto& meta = actionObject["_meta"];
        if (meta.contains("titleKey") && meta["titleKey"].is_string()) {
          const auto& titleKey = meta["titleKey"].get<std::string>();
          if (!titleKey.empty()) {
            // 4. Get translated string
            size_t firstDot = group.find('.');
            std::string owner = (firstDot != std::string::npos) ? group.substr(0, firstDot) : group;
            std::string translated = loc.Get(owner, titleKey);
            // Check if translation succeeded. If not, loc.Get returns the key.
            if (translated != titleKey) {
              return translated;
            }
          }
        }
      }
    }
  }

  // 5. Fallback
  return fullActionName;
}

void SettingsWindow::RenderSettingsNode(const std::string& key, const nlohmann::ordered_json& node, const std::string& systemName, const std::string& currentPath, int depth) {
  if (depth > 0) {
    ImGui::Dummy(ImVec2(depth * 5.0f, 0.0f));
    ImGui::SameLine();
  }

  std::string fullPath = currentPath.empty() ? key : currentPath + "." + key;
  std::string fullSystemPath = systemName + "." + fullPath;
  auto& loc = LocalizationManager::GetInstance();

  // Extract the actual value node and determine display name
  const nlohmann::ordered_json* valueNode = &node;
  std::string displayName = key;  // Default to raw key

  // Get metadata for display name and description
  const nlohmann::ordered_json* metaNode = nullptr;
  if (node.is_object() && node.contains("_value")) {
    valueNode = &node["_value"];
    if (node.contains("_meta") && node["_meta"].is_object()) {
      metaNode = &node["_meta"];
    }
  } else if (node.is_object() && node.contains("_meta")) {  // It's an object that is just metadata, no _value
    metaNode = &node["_meta"];
  }

  if (metaNode && metaNode->value("hide_in_ui", false)) {
    return;
  }

  if (metaNode) {
    if (metaNode->contains("titleKey") && (*metaNode)["titleKey"].is_string()) {
      const auto& titleKey = (*metaNode)["titleKey"].get<std::string>();
      if (!titleKey.empty()) {
        if (systemName == "logging" || systemName == "localization" || systemName == "ui") {
          displayName = loc.GetWithFallback(m_currentComponent, titleKey);
        } else {
          displayName = loc.Get(m_currentComponent, titleKey);
        }
      }
    }
  }

  std::string label = displayName;

  // Helper lambda to display a tooltip for the last drawn item.
  auto ShowTooltip = [&]() {
    if (ImGui::IsItemHovered() && metaNode && metaNode->contains("descriptionKey") && (*metaNode)["descriptionKey"].is_string()) {
      const auto& descKey = (*metaNode)["descriptionKey"].get<std::string>();
      if (!descKey.empty()) {
        if (systemName == "logging" || systemName == "localization" || systemName == "ui") {
          ImGui::SetTooltip("%s", loc.GetWithFallback(m_currentComponent, descKey).c_str());
        } else {
          ImGui::SetTooltip("%s", loc.Get(m_currentComponent, descKey).c_str());
        }
      }
    }
  };

  bool renderedWithCustomWidget = false;
  if (metaNode && metaNode->contains("ui") && (*metaNode)["ui"].is_object()) {
    const auto& ui_meta = (*metaNode)["ui"];
    if (ui_meta.contains("widget") && ui_meta["widget"].is_string()) {
      std::string widget_type = ui_meta["widget"].get<std::string>();
      const auto& params = ui_meta.value("params", nlohmann::ordered_json::object());  // Get params or empty object

      if (widget_type == "slider") {
        if (valueNode->is_number_integer()) {
          const auto& params = ui_meta.value("params", nlohmann::ordered_json::object());
          ImGuiSliderFlags slider_flags = ImGuiSliderFlags_None;
          if (params.value("is_logarithmic", false)) {
            slider_flags |= ImGuiSliderFlags_Logarithmic;
          }
          int value = valueNode->get<int>();
          int min_val = params.value("min", 0);
          int max_val = params.value("max", 100);
          std::string format = params.value("format", "%d");
          if (ImGui::SliderInt(("##" + key).c_str(), &value, min_val, max_val, format.c_str(), slider_flags)) {
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        } else if (valueNode->is_number_float()) {
          const auto& params = ui_meta.value("params", nlohmann::ordered_json::object());
          ImGuiSliderFlags slider_flags = ImGuiSliderFlags_None;
          if (params.value("is_logarithmic", false)) {
            slider_flags |= ImGuiSliderFlags_Logarithmic;
          }
          float value = valueNode->get<float>();
          float min_val = params.value("min", 0.0f);
          float max_val = params.value("max", 100.0f);
          std::string format = params.value("format", "%.3f");
          if (ImGui::SliderFloat(("##" + key).c_str(), &value, min_val, max_val, format.c_str(), slider_flags)) {
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        }
      } else if (widget_type == "drag") {
        if (valueNode->is_number_integer()) {
          int value = valueNode->get<int>();
          float speed = params.value("speed", 1.0f);
          int min_val = params.value("min", 0);
          int max_val = params.value("max", 100);
          std::string format = params.value("format", "%d");
          if (ImGui::DragInt(("##" + key).c_str(), &value, speed, min_val, max_val, format.c_str())) {
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        } else if (valueNode->is_number_float()) {
          float value = valueNode->get<float>();
          float speed = params.value("speed", 0.1f);
          float min_val = params.value("min", 0.0f);
          float max_val = params.value("max", 100.0f);
          std::string format = params.value("format", "%.3f");
          if (ImGui::DragFloat(("##" + key).c_str(), &value, speed, min_val, max_val, format.c_str())) {
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        }
      } else if (widget_type == "combo" || widget_type == "radio") {
        if (params.contains("options") && params["options"].is_array()) {
          std::vector<std::string> option_labels;
          std::vector<nlohmann::ordered_json> option_values;
          int current_selection = -1;  // For combo

          for (const auto& option : params["options"]) {
            if (option.is_object() && option.contains("value") && option.contains("labelKey")) {
              option_values.push_back(option["value"]);
              std::string label_key = option["labelKey"].get<std::string>();
              std::string display_label = loc.GetWithFallback(m_currentComponent, label_key);
              if (display_label == label_key) {  // Fallback to literal if key not found
                display_label = label_key;
              }
              option_labels.push_back(display_label);

              // Check if this option matches current value
              if (valueNode->type() == option["value"].type() && *valueNode == option["value"]) {
                current_selection = option_labels.size() - 1;
              }
            }
          }
          if (!option_labels.empty()) {
            if (widget_type == "combo") {
              std::string preview_value = (current_selection != -1) ? option_labels[current_selection] : "";
              if (ImGui::BeginCombo(("##" + key).c_str(), preview_value.c_str())) {
                for (int i = 0; i < option_labels.size(); ++i) {
                  bool is_selected = (i == current_selection);
                  if (ImGui::Selectable(option_labels[i].c_str(), is_selected)) {
                    if (i != current_selection) {
                      m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, option_values[i]});
                    }
                  }
                  if (is_selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
              }
              ShowTooltip();
              renderedWithCustomWidget = true;
            } else if (widget_type == "radio") {
              ImGui::TextUnformatted(label.c_str());
              ShowTooltip();
              ImGui::Indent();
              for (int i = 0; i < option_labels.size(); ++i) {
                bool is_selected = (i == current_selection);
                if (ImGui::RadioButton(option_labels[i].c_str(), is_selected)) {
                  if (i != current_selection) {
                    m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, option_values[i]});
                  }
                }
              }
              ImGui::Unindent();
              renderedWithCustomWidget = true;
            }
          }
        }
      } else if (widget_type == "vslider") {
        const auto& params = ui_meta.value("params", nlohmann::ordered_json::object());
        ImGuiSliderFlags slider_flags = ImGuiSliderFlags_None;
        if (params.value("is_logarithmic", false)) {
          slider_flags |= ImGuiSliderFlags_Logarithmic;
        }

        if (valueNode->is_number_integer()) {
          int value = valueNode->get<int>();
          int min_val = params.value("min", 0);
          int max_val = params.value("max", 100);
          float width = params.value("width", 18.0f);
          float height = params.value("height", 60.0f);
          std::string format = params.value("format", "%d");
          if (ImGui::VSliderInt(("##" + key).c_str(), ImVec2(width, height), &value, min_val, max_val, format.c_str(), slider_flags)) {
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        } else if (valueNode->is_number_float()) {
          float value = valueNode->get<float>();
          float min_val = params.value("min", 0.0f);
          float max_val = params.value("max", 1.0f);
          float width = params.value("width", 18.0f);
          float height = params.value("height", 60.0f);
          std::string format = params.value("format", "%.3f");
          if (ImGui::VSliderFloat(("##" + key).c_str(), ImVec2(width, height), &value, min_val, max_val, format.c_str(), slider_flags)) {
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        }
      } else if (widget_type == "color3") {
        if (valueNode->is_array() && valueNode->size() == 3) {
          ImVec4 color = ImVec4(valueNode->at(0).get<float>(), valueNode->at(1).get<float>(), valueNode->at(2).get<float>(), 1.0f);
          int flags = params.value("flags", 0);
          if (ImGui::ColorEdit3(("##" + key).c_str(), (float*)&color, flags)) {
            nlohmann::ordered_json newColor = {color.x, color.y, color.z};
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, newColor});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        } else {
          LoggerFactory::GetInstance().GetLogger("SettingsWindow")->Error("Invalid value for 'color3' widget (key: '{}'). Expected array of 3 floats. Falling back to default.", fullSystemPath);
        }
      } else if (widget_type == "color4") {
        if (valueNode->is_array() && valueNode->size() == 4) {
          ImVec4 color = ImVec4(valueNode->at(0).get<float>(), valueNode->at(1).get<float>(), valueNode->at(2).get<float>(), valueNode->at(3).get<float>());
          int flags = params.value("flags", 0);
          if (ImGui::ColorEdit4(("##" + key).c_str(), (float*)&color, flags)) {
            nlohmann::ordered_json newColor = {color.x, color.y, color.z, color.w};
            m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, newColor});
          }
          ShowTooltip();
          renderedWithCustomWidget = true;
        } else {
          LoggerFactory::GetInstance().GetLogger("SettingsWindow")->Error("Invalid value for 'color4' widget (key: '{}'). Expected array of 4 floats. Falling back to default.", fullSystemPath);
        }
      } else if (widget_type == "multiline" && valueNode->is_string()) {
        std::string value = valueNode->get<std::string>();
        char buf[2048];  // Use a larger buffer for multiline text
        strncpy_s(buf, value.c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = 0;
        int height_in_lines = params.value("height_in_lines", 5);
        if (ImGui::InputTextMultiline(("##" + key).c_str(), buf, sizeof(buf), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * height_in_lines))) {
          m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, std::string(buf)});
        }
        ShowTooltip();
        renderedWithCustomWidget = true;
      } else if (widget_type == "input_with_hint" && valueNode->is_string()) {
        std::string value = valueNode->get<std::string>();
        std::string hint = params.value("hint", "");
        char buf[256];
        strncpy_s(buf, value.c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = 0;

        if (ImGui::InputTextWithHint(("##" + key).c_str(), hint.c_str(), buf, sizeof(buf))) {
          m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, std::string(buf)});
        }
        ShowTooltip();
        renderedWithCustomWidget = true;
      } else if (widget_type == "input") {
        // Explicitly specified "input" widget, fall through to default rendering.
        // This branch is just for clarity, as it would otherwise hit the !renderedWithCustomWidget block.
        // No 'renderedWithCustomWidget = true;' here, let the fallback handle it.
      }
    }
  }

  if (!renderedWithCustomWidget) {
    // For default-rendered widgets, we still check if there are any applicable 'params'
    const nlohmann::ordered_json params = (metaNode && metaNode->contains("ui") && (*metaNode)["ui"].is_object() && (*metaNode)["ui"].contains("params")) ? (*metaNode)["ui"]["params"] : nlohmann::ordered_json::object();

    if (valueNode->is_boolean()) {
      bool value = valueNode->get<bool>();
      if (ImGui::Checkbox(label.c_str(), &value)) {
        m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
      }
      ShowTooltip();
    } else if (valueNode->is_string()) {
      // Special handling for language and log level
      if (systemName == "localization" && key == "language") {
        const auto& availableLangs = loc.GetAvailableLanguagesFor(m_currentComponent);
        std::string currentLang = valueNode->get<std::string>();
        std::string currentLangDisplay = loc.Get(m_currentComponent, "language." + currentLang);
        if (currentLangDisplay == "language." + currentLang) {
          currentLangDisplay = currentLang;
        }

        if (ImGui::BeginCombo(("##" + key).c_str(), currentLangDisplay.c_str())) {
          for (const auto& langCode : availableLangs) {
            bool is_selected = (currentLang == langCode);
            std::string langDisplay = loc.Get(m_currentComponent, "language." + langCode);
            if (langDisplay == "language." + langCode) {
              langDisplay = langCode;
            }
            if (ImGui::Selectable(langDisplay.c_str(), is_selected)) {
              if (currentLang != langCode) {
                m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, langCode});
              }
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        ShowTooltip();
      } else if (systemName == "logging" && key == "level") {
        std::string currentLevelStr = valueNode->get<std::string>();
        if (ImGui::BeginCombo(("##" + key).c_str(), currentLevelStr.c_str())) {
          for (const auto& levelName : m_logLevels) {
            bool is_selected = (currentLevelStr == levelName);
            if (ImGui::Selectable(levelName.c_str(), is_selected)) {
              if (currentLevelStr != levelName) {
                m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, levelName});
              }
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
        }
        ShowTooltip();
      } else {
        std::string value = valueNode->get<std::string>();
        char buf[256];
        strncpy_s(buf, value.c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = 0;
        if (ImGui::InputText(("##" + key).c_str(), buf, sizeof(buf))) {
          m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, std::string(buf)});
        }
        ShowTooltip();
      }
    } else if (valueNode->is_number_integer()) {
      int value = valueNode->get<int>();
      int step = params.value("step", 1);
      if (ImGui::InputInt(("##" + key).c_str(), &value, step)) {
        m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
      }
      ShowTooltip();
    } else if (valueNode->is_number_float()) {
      double value = valueNode->get<double>();
      double step = params.value("step", 0.01);
      if (ImGui::InputDouble(("##" + key).c_str(), &value, step)) {
        m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
      }
      ShowTooltip();
    } else if (node.is_object() && !node.contains("_value")) {  // It's a nested settings group
      bool node_open = ImGui::TreeNode(label.c_str());
      ShowTooltip();  // Show tooltip for the TreeNode label itself
      if (node_open) {
        DrawSettingsRows(node, systemName, fullPath);
        ImGui::TreePop();
      }
    } else if (valueNode->is_array()) {
      bool node_open = ImGui::TreeNode(label.c_str());
      ShowTooltip();
      if (node_open) {
        for (size_t i = 0; i < valueNode->size(); ++i) {
          RenderSettingsNode(std::to_string(i), (*valueNode)[i], systemName, fullPath, depth + 1);
        }
        ImGui::TreePop();
      }
    } else if (valueNode->is_null()) {
      std::string markdownText = loc.GetFormatted("framework", "settings_window.main_area.null_value_format", key);
      Typography::RenderMarkdownText(markdownText, TextStyle::Italic().Color(UI::Colors::GRAY));
      ShowTooltip();
    }
  }
}

void SettingsWindow::RenderKeybindsSettings() {
  auto& loc = LocalizationManager::GetInstance();
  if (!m_configService.GetMergedConfig("keybinds")) {
    Typography::Text(TextStyle::H3().Color(UI::Colors::GRAY).Align(TextAlign::Center), m_keybindsNotAvailableKey.c_str());
    return;
  }

  ImGuiTableFlags container_flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoPadInnerX;

  if (ImGui::BeginTable("keybinds_main_container", 1, container_flags)) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    // --- Header Table ---
    ImGuiTableFlags header_flags = ImGuiTableFlags_Borders;
    if (ImGui::BeginTable("keybinds_header_table", 2, header_flags)) {
      ImGui::TableSetupColumn(m_keybindsActionHeaderKey.c_str(), ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn(m_keybindsKeyHeaderKey.c_str(), ImGuiTableColumnFlags_WidthStretch);

      ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
      for (int column = 0; column < 2; column++) {
        ImGui::TableSetColumnIndex(column);
        const char* columnName = ImGui::TableGetColumnName(column);
        Typography::Text(TextStyle::Bold().Align(TextAlign::Center), columnName);
      }
      ImGui::EndTable();
    }

    // --- Data Grouping ---
    std::map<std::string, std::vector<std::tuple<std::string, std::string, const nlohmann::ordered_json*>>> groupedActions;
    for (const auto& [group, actions] : m_configService.GetMergedConfig("keybinds")->items()) {
      std::string ownerName = group.substr(0, group.find('.'));
      for (const auto& [actionName, actionObject] : actions.items()) {
        std::string fullActionName = group + "." + actionName;
        groupedActions[ownerName].emplace_back(fullActionName, actionName, &actionObject);
      }
    }

    // --- Plugin and Action Rows ---
    for (auto const& [ownerName, actionList] : groupedActions) {
      std::string ownerDisplayName = ownerName;
      auto it_owner = m_configService.GetAllComponentInfo().find(ownerName);
      if (it_owner != m_configService.GetAllComponentInfo().end() && it_owner->second.name.has_value()) {
        ownerDisplayName = it_owner->second.name.value();
      }

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);

      ImGui::PushID(ownerName.c_str());

      ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);

      // --- Centered TreeNode ---
      float availableWidth = ImGui::GetContentRegionAvail().x;
      float textWidth = ImGui::CalcTextSize(ownerDisplayName.c_str()).x;
      float widgetWidth = ImGui::GetTreeNodeToLabelSpacing() + textWidth;

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - widgetWidth) * 0.5f);
      bool tree_open = ImGui::TreeNodeEx(ownerDisplayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth);

      if (tree_open) {
        // --- Actions Table (for this plugin) ---
        ImGuiTableFlags actions_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
        if (ImGui::BeginTable(("actions_table_" + ownerName).c_str(), 2, actions_flags)) {
          ImGui::TableSetupColumn(m_keybindsActionHeaderKey.c_str(), ImGuiTableColumnFlags_WidthStretch);
          ImGui::TableSetupColumn(m_keybindsKeyHeaderKey.c_str(), ImGuiTableColumnFlags_WidthStretch);

          bool canBeModified;
          if (ownerName == "framework") {
            auto pluginIt = m_configService.GetAllComponentInfo().find(ownerName);
            canBeModified = (pluginIt != m_configService.GetAllComponentInfo().end()) ? pluginIt->second.allowUserConfig : false;
          } else {
            bool isEnabledLive = Modules::PluginManager::GetInstance().IsPluginLoaded(ownerName);
            auto pluginIt = m_configService.GetAllComponentInfo().find(ownerName);
            canBeModified = isEnabledLive && ((pluginIt != m_configService.GetAllComponentInfo().end()) ? pluginIt->second.allowUserConfig : false);
          }

          ImGui::BeginDisabled(!canBeModified);

          for (const auto& [fullActionName, actionName, actionObject] : actionList) {
            std::string actionDisplayName = actionName;
            if (actionObject->is_object() && actionObject->contains("_meta")) {
              const auto& meta = (*actionObject)["_meta"];
              if (meta.contains("titleKey") && meta["titleKey"].is_string()) {
                const auto& titleKey = meta["titleKey"].get<std::string>();
                if (!titleKey.empty()) {
                  actionDisplayName = loc.Get(ownerName, titleKey);
                }
              }
            }

            ImGui::TableNextRow();
            // --- Column 0: Action Name and Add Button ---
            ImGui::TableSetColumnIndex(0);

            // --- Vertical Centering Logic ---
            float textHeight = ImGui::GetTextLineHeight();
            float rowContentHeight = ImGui::GetFrameHeight();  // Default height for one line
            if (actionObject->is_object() && actionObject->contains("bindings") && (*actionObject)["bindings"].is_array()) {
              const auto& bindings = (*actionObject)["bindings"];
              if (bindings.size() > 1) {
                // Height of N buttons + (N-1) spacing intervals
                rowContentHeight = bindings.size() * ImGui::GetFrameHeight() + (bindings.size() - 1) * ImGui::GetStyle().ItemSpacing.y;
              }
            }

            float yOffset = (rowContentHeight - textHeight) * 0.5f;
            if (yOffset > 0) {
              ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
            }

            Typography::Text(actionDisplayName.c_str());

            // --- Tooltip for the action name ---
            if (ImGui::IsItemHovered()) {
              if (actionObject->is_object() && actionObject->contains("_meta")) {
                const auto& meta = (*actionObject)["_meta"];
                if (meta.contains("descriptionKey") && meta["descriptionKey"].is_string()) {
                  const auto& descriptionKey = meta["descriptionKey"].get<std::string>();
                  if (!descriptionKey.empty()) {
                    ImGui::SetTooltip("%s", loc.Get(ownerName, descriptionKey).c_str());
                  }
                }
              }
            }

            // Align the button to the far right of the column
            float buttonWidth = ImGui::GetFrameHeight();  // SmallButton is roughly square
            ImGui::SameLine(ImGui::GetColumnWidth() - (buttonWidth * 0.5f) - ImGui::GetStyle().CellPadding.x);

            ImGui::PushID((fullActionName + ":add_button").c_str());
            if (ImGui::SmallButton(ICON_FA_PLUS)) {
              m_actionBeingEdited = fullActionName;
              m_editingBindingObject = nlohmann::ordered_json::object();
              m_eventManager.System.OnRequestInputCapture.Call({fullActionName, m_editingBindingObject});
            }
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s", loc.Get("settings_window.keybinds_drawer.table.add_button_tooltip").c_str());
            }
            ImGui::PopID();

            // --- Column 1: Keybinds ---
            ImGui::TableSetColumnIndex(1);

            const nlohmann::ordered_json* bindings = nullptr;
            if (actionObject->is_object() && actionObject->contains("bindings") && (*actionObject)["bindings"].is_array()) {
              bindings = &(*actionObject)["bindings"];
            }

            if (bindings) {
              if (bindings->empty()) {
                ImGui::PushID((fullActionName + ":add_new").c_str());
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                if (Button(m_keybindsUnassignedTextKey.c_str())) {
                  m_actionBeingEdited = fullActionName;
                  m_editingBindingObject = nlohmann::ordered_json::object();
                  m_eventManager.System.OnRequestInputCapture.Call({fullActionName, m_editingBindingObject});
                }
                if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("%s", loc.Get("settings_window.keybinds_drawer.table.key_button_tooltip").c_str());
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
              } else {
                for (const auto& bindingJson : *bindings) {
                  auto input = Modules::InputFactory::CreateFromJson(bindingJson);
                  if (!input || !input->IsValid()) continue;

                  auto getIconForInput = [](Modules::InputType type) -> const char* {
                    switch (type) {
                      case Modules::InputType::Keyboard:
                        return ICON_FA_KEYBOARD;
                      case Modules::InputType::Gamepad:
                        return ICON_FA_GAMEPAD;
                      case Modules::InputType::GamepadAxis:
                        return ICON_FA_GAMEPAD;
                      case Modules::InputType::Mouse:
                        return ICON_FA_COMPUTER_MOUSE;
                      case Modules::InputType::MouseAxis:
                        return ICON_FA_COMPUTER_MOUSE;
                      case Modules::InputType::Joystick:
                        return ICON_FA_GAMEPAD;
                      case Modules::InputType::JoystickAxis:
                        return ICON_FA_GAMEPAD;
                      default:
                        return "";
                    }
                  };

                  std::string buttonText;
                  if (input->GetType() == Modules::InputType::Chord) {
                    auto chord = dynamic_cast<SPF::Modules::ChordInput*>(input.get());
                    if (chord) {
                      const auto& constituents = chord->GetInputs();
                      Modules::InputType lastType = Modules::InputType::Chord;  // Dummy value
                      for (size_t i = 0; i < constituents.size(); ++i) {
                        Modules::InputType currentType = constituents[i]->GetType();

                        if (currentType != lastType) {
                          // Add icon only when type changes
                          buttonText += fmt::format("{} ", getIconForInput(currentType));
                          lastType = currentType;
                        }

                        buttonText += constituents[i]->GetDisplayName();
                        if (i < constituents.size() - 1) buttonText += " + ";
                      }
                    }
                  } else {
                    buttonText = fmt::format("{} {}", getIconForInput(input->GetType()), input->GetDisplayName());
                  }

                  // --- DISAMBIGUATION (CONFLICT LABELS) ---
                  uint32_t code = input->GetHardwareCode();
                  if (m_hardwareCodeUsageCount.count(code) && m_hardwareCodeUsageCount[code] > 1) {
                    std::string typeSuffix;
                    Modules::InputType inputType = input->GetType();
                    bool isAxisType = (inputType == Modules::InputType::GamepadAxis || inputType == Modules::InputType::MouseAxis || inputType == Modules::InputType::JoystickAxis);

                    if (isAxisType) {
                      std::string side = bindingJson.value("side", "both");
                      typeSuffix = loc.Get("enums.side." + side);
                    } else {
                      std::string pressType = bindingJson.value("press_type", "short");
                      typeSuffix = loc.Get("enums.press_type." + pressType);
                    }
                    buttonText += fmt::format(" ({})", typeSuffix);
                  }

                  if (buttonText.empty() || buttonText.find("Unknown") != std::string::npos) continue;

                  std::string uniqueId = fullActionName + ":" + buttonText;

                  ImGui::PushID(uniqueId.c_str());
                  if (Button(buttonText.c_str())) {
                    m_actionBeingEdited = fullActionName;
                    m_editingBindingObject = bindingJson;
                    m_eventManager.System.OnRequestInputCapture.Call({fullActionName, bindingJson});
                  }
                  if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", loc.Get("settings_window.keybinds_drawer.table.key_button_tooltip").c_str());
                  }
                  ImGui::PopID();
                  ImGui::SameLine();

                  ImGui::PushID(("details_" + uniqueId).c_str());
                  if (ImGui::SmallButton(ICON_FA_GEAR)) {
                    m_editingBindingAction = fullActionName;
                    m_editingBindingDetails = bindingJson;
                    m_currentPressThreshold = bindingJson.value("press_threshold_ms", 500);
                    m_originalBindingCopy = bindingJson;
                    m_shouldOpenBindingDetailsPopup = true;
                  }
                  if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", loc.Get("settings_window.keybinds_drawer.table.details_button_tooltip").c_str());
                  }
                  ImGui::PopID();
                  // No ImGui::SameLine() here, so the next binding is on a new line
                }
              }
            }
          }
          ImGui::EndDisabled();
        }
        ImGui::EndTable();
        ImGui::TreePop();
      }
      ImGui::PopStyleVar();
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}

void SettingsWindow::DrawSettingsRows(const nlohmann::ordered_json& settingsNode, const std::string& systemName, const std::string& parentPath) {
  auto& loc = LocalizationManager::GetInstance();

  for (auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
    const std::string& key = it.key();
    const nlohmann::ordered_json& value = it.value();

    if (key == "_meta") continue;

    ImGui::PushID(key.c_str());

    std::string currentPath = parentPath.empty() ? key : parentPath + "." + key;

    // First check: manifest-based hiding (most reliable, only for "settings" system)
    if (systemName == "settings" && m_configService.IsSettingHidden(m_currentComponent, currentPath)) {
      ImGui::PopID();
      continue;
    }

    // Second check: metadata-based hiding within JSON (fallback/dynamic)
    const nlohmann::ordered_json* metaNode = nullptr;
    if (value.is_object() && value.contains("_meta") && value["_meta"].is_object()) {
      metaNode = &value["_meta"];
    }

    if (metaNode && metaNode->value("hide_in_ui", false)) {
      ImGui::PopID();
      continue;
    }

    const nlohmann::ordered_json* actualValueNode = &value;
    if (value.is_object() && value.contains("_value")) {
      actualValueNode = &value["_value"];
    }

    bool hasCustomWidget = (value.is_object() && value.contains("_meta") && value["_meta"].is_object() && value["_meta"].contains("ui") && value["_meta"]["ui"].is_object() && value["_meta"]["ui"].value("widget", "").empty() == false);

    bool isDefaultBoolean = actualValueNode->is_boolean() && !hasCustomWidget;

    size_t depth = parentPath.empty() ? 0 : std::count(parentPath.begin(), parentPath.end(), '.') + 1;

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    std::string settingDisplayName = key;
    if (value.is_object() && value.contains("_meta") && value["_meta"].contains("titleKey") && value["_meta"]["titleKey"].is_string()) {
      const auto& titleKey = value["_meta"]["titleKey"].get<std::string>();
      if (!titleKey.empty()) {
        settingDisplayName = (systemName == "logging" || systemName == "localization" || systemName == "ui") ? loc.GetWithFallback(m_currentComponent, titleKey) : loc.Get(m_currentComponent, titleKey);
      }
    }

    if (!isDefaultBoolean) {  // Only draw label in column 0 if it's NOT a default boolean
      ImGui::TextUnformatted(settingDisplayName.c_str());

      if (value.is_object() && value.contains("_meta") && value["_meta"].is_object() && value["_meta"].contains("descriptionKey") && value["_meta"]["descriptionKey"].is_string()) {
        if (ImGui::IsItemHovered()) {
          const auto& descriptionKey = value["_meta"]["descriptionKey"].get<std::string>();
          if (!descriptionKey.empty()) {
            ImGui::SetTooltip("%s", (systemName == "logging" || systemName == "localization" || systemName == "ui") ? loc.GetWithFallback(m_currentComponent, descriptionKey).c_str() : loc.Get(m_currentComponent, descriptionKey).c_str());
          }
        }
      }
    }

    // --- Column 2: Setting Control ---
    ImGui::TableSetColumnIndex(1);
    RenderSettingsNode(key, value, systemName, parentPath, depth);

    ImGui::PopID();
  }
}

void SettingsWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();
  // --- Component Selector Dropdown (Part of the main window's static layout) ---
  m_configurableComponents.clear();
  for (const auto& [name, info] : m_configService.GetAllComponentInfo()) {
    if (info.hasSettings) {
      bool isEnabled = false;
      if (name == "framework") {
        isEnabled = true;  // Framework is always "enabled"
      } else {
        isEnabled = Modules::PluginManager::GetInstance().IsPluginLoaded(name);
      }

      if (isEnabled) {
        m_configurableComponents.push_back(name);
      }
    }
  }
  std::stable_sort(m_configurableComponents.begin(), m_configurableComponents.end(), [](const std::string& a, const std::string& b) {
    if (a == "framework") return true;
    if (b == "framework") return false;
    return a < b;
  });

  if (!m_configurableComponents.empty()) {
    if (std::find(m_configurableComponents.begin(), m_configurableComponents.end(), m_currentComponent) == m_configurableComponents.end()) {
      m_currentComponent = m_configurableComponents.front();
    }

    // --- Custom Combobox using InvisibleButton for perfect height matching ---

    float availableWidth = ImGui::GetContentRegionAvail().x;
    if (ImGui::InvisibleButton("##component_selector_button", ImVec2(availableWidth, m_keybindsDrawerMinHeight))) {
      ImGui::OpenPopup("component_selector_popup");
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", loc.Get("settings_window.component_selector.tooltip").c_str());
    }

    // --- Manual Drawing ---
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
    float buttonHeight = p_max.y - p_min.y;

    // Determine color based on state
    ImU32 bgColor = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_FrameBgActive : (ImGui::IsItemHovered() ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));

    draw_list->AddRectFilled(p_min, p_max, bgColor, ImGui::GetStyle().FrameRounding);
    draw_list->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding, 0, 1.0f);

    // Get display name for the currently selected component
    std::string currentDisplayName = m_currentComponent;
    auto it_current = m_configService.GetAllComponentInfo().find(m_currentComponent);
    if (it_current != m_configService.GetAllComponentInfo().end() && it_current->second.name.has_value()) {
      currentDisplayName = it_current->second.name.value();
    }

    // Draw icon and label, vertically stacked and centered
    const char* label = currentDisplayName.c_str();
    const char* icon = ICON_FA_CHEVRON_DOWN;
    ImVec2 label_size = ImGui::CalcTextSize(label);
    ImVec2 icon_size = ImGui::CalcTextSize(icon);

    float vertical_spacing = 2.0f;  // A small gap between icon and text
    float total_content_height = icon_size.y + label_size.y + vertical_spacing;
    float start_y = p_min.y + (buttonHeight - total_content_height) / 2.0f;

    draw_list->AddText(ImVec2(p_min.x + (availableWidth - icon_size.x) / 2.0f, start_y), ImGui::GetColorU32(ImGuiCol_Text), icon);
    draw_list->AddText(ImVec2(p_min.x + (availableWidth - label_size.x) / 2.0f, start_y + icon_size.y + vertical_spacing), ImGui::GetColorU32(ImGuiCol_Text), label);

    // --- Popup with Selectable Items (with added padding) ---
    ImGui::SetNextWindowPos(ImVec2(p_min.x, p_max.y));
    ImGui::SetNextWindowSize(ImVec2(availableWidth, 0));
    if (ImGui::BeginPopup("component_selector_popup")) {
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, ImGui::GetStyle().FramePadding.y * 2.0f));  // Double vertical padding
      for (const auto& componentName : m_configurableComponents) {
        // Get display name for the item in the list
        std::string displayName = componentName;
        auto it_item = m_configService.GetAllComponentInfo().find(componentName);
        if (it_item != m_configService.GetAllComponentInfo().end() && it_item->second.name.has_value()) {
          displayName = it_item->second.name.value();
        }

        const bool is_selected = (m_currentComponent == componentName);
        if (ImGui::Selectable(displayName.c_str(), is_selected)) {
          m_currentComponent = componentName;
        }
        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::PopStyleVar();  // Pop FramePadding
      ImGui::EndPopup();
    }
  }

  ImGui::Separator();

  m_keybindsDrawerMaxHeight = ImGui::GetContentRegionAvail().y;

  // --- Main Settings Area (Child Window) ---
  float mainSettingsHeight = ImGui::GetContentRegionAvail().y - m_keybindsDrawerHeight - ImGui::GetStyle().ItemSpacing.y;
  if (mainSettingsHeight > 1.0f)  // Only draw if there is space
  {
    // This child window can have its own scrollbar if the settings content is large.
    ImGui::BeginChild("MainSettingsContent", ImVec2(0, mainSettingsHeight));
    if (m_configurableComponents.empty()) {
      Typography::Text(TextStyle::H3().Color(UI::Colors::GRAY).Align(TextAlign::Center), m_noConfigurableComponentsKey.c_str());
    } else {
      const auto& infoIt = m_configService.GetAllComponentInfo().find(m_currentComponent);
      if (infoIt == m_configService.GetAllComponentInfo().end()) {
        std::string markdownText = loc.GetFormatted("framework", "settings_window.main_area.component_info_error", m_currentComponent);
        Typography::RenderMarkdownText(markdownText, TextStyle::H3().Color(UI::Colors::RED).Align(TextAlign::Center));
      } else {
        const auto& systemsToRender = infoIt->second.configurableSystems;
        const auto& componentSettingsIt = m_configService.GetAggregatedUserSettings().find(m_currentComponent);
        if (componentSettingsIt == m_configService.GetAggregatedUserSettings().end() || systemsToRender.empty()) {
          Typography::Text(TextStyle::H3().Color(UI::Colors::GRAY).Align(TextAlign::Center), m_noConfigurableSystemsKey.c_str());
        } else {
          const auto& componentSettingsData = componentSettingsIt->second;

          for (const auto& systemName : systemsToRender) {
            if (componentSettingsData.contains(systemName)) {
              const auto& systemSettings = componentSettingsData.at(systemName);

              // --- Collapsing Header with Localized Title ---
              std::string titleKey = "settings_window.system_titles." + systemName;
              std::string systemDisplayName = loc.GetWithFallback(m_currentComponent, titleKey);

              // Fallback to raw system name if translation is not found in plugin or framework
              if (systemDisplayName == titleKey) {
                systemDisplayName = systemName;
              }

              ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));  // Thinner header
              bool headerIsOpen = ImGui::CollapsingHeader(systemDisplayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
              ImGui::PopStyleVar();

              if (headerIsOpen) {
                // --- Settings Table ---
                if (ImGui::BeginTable(("table_" + systemName).c_str(), 2, ImGuiTableFlags_BordersInnerV)) {
                  ImGui::TableSetupColumn(m_settingHeaderKey.c_str(), ImGuiTableColumnFlags_WidthStretch);
                  ImGui::TableSetupColumn(m_valueHeaderKey.c_str(), ImGuiTableColumnFlags_WidthStretch);

                  DrawSettingsRows(systemSettings, systemName, "");

                  ImGui::EndTable();
                }
              }
            }
          }
        }
      }
    }
    ImGui::EndChild();
  }

  // --- Keybinds Drawer ---
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

  // Handle interaction must be processed before rendering the child
  ImGui::InvisibleButton("KeybindsDrawerHandle", ImVec2(-1, m_keybindsDrawerMinHeight));
  if (ImGui::IsItemHovered()) {
    const char* tooltipKey = m_keybindsDrawerExpanded ? "settings_window.keybinds_drawer.tooltip_close" : "settings_window.keybinds_drawer.tooltip_open";
    ImGui::SetTooltip("%s", loc.Get(tooltipKey).c_str());
  }
  bool handleActive = ImGui::IsItemActive();

  // Dragging has priority.
  if (handleActive && ImGui::IsMouseDragging(0)) {
    m_keybindsDrawerHeight -= ImGui::GetIO().MouseDelta.y;
  }
  // If not dragging, check for a simple click to toggle.
  else if (ImGui::IsItemClicked()) {
    m_keybindsDrawerExpanded = !m_keybindsDrawerExpanded;
    m_keybindsDrawerHeight = m_keybindsDrawerExpanded ? m_keybindsDrawerMaxHeight : m_keybindsDrawerMinHeight;
  }

  // Clamp height and update expanded state
  if (m_keybindsDrawerMinHeight <= m_keybindsDrawerMaxHeight) {
    m_keybindsDrawerHeight = std::clamp(m_keybindsDrawerHeight, m_keybindsDrawerMinHeight, m_keybindsDrawerMaxHeight);
  }
  m_keybindsDrawerExpanded = (m_keybindsDrawerHeight > m_keybindsDrawerMinHeight + 5.0f);

  // Custom rendering for the handle
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 p_min = ImGui::GetItemRectMin();
  ImVec2 p_max = ImGui::GetItemRectMax();
  draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Button), ImGui::GetStyle().FrameRounding);
  draw_list->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding, 0, 1.0f);

  const char* title = m_keybindsDrawerTitleKey.c_str();
  const char* icon = m_keybindsDrawerExpanded ? ICON_FA_CHEVRON_DOWN : ICON_FA_CHEVRON_UP;

  ImVec2 title_size = ImGui::CalcTextSize(title);
  ImVec2 icon_size = ImGui::CalcTextSize(icon);
  float drawerWidth = p_max.x - p_min.x;

  // Center the title in the top half of the handle
  float title_y_pos = p_min.y + (m_keybindsDrawerMinHeight / 2 - title_size.y) / 2;
  draw_list->AddText(ImVec2(p_min.x + (drawerWidth - title_size.x) / 2, title_y_pos), ImGui::GetColorU32(ImGuiCol_Text), title);

  // Center the icon in the bottom half of the handle
  float icon_y_pos = p_min.y + (m_keybindsDrawerMinHeight / 2) + (m_keybindsDrawerMinHeight / 2 - icon_size.y) / 2;
  draw_list->AddText(ImVec2(p_min.x + (drawerWidth - icon_size.x) / 2, icon_y_pos), ImGui::GetColorU32(ImGuiCol_Text), icon);

  ImGui::PopStyleVar();  // Pop the style for ItemSpacing

  // Render the content of the drawer
  float contentHeight = m_keybindsDrawerHeight - m_keybindsDrawerMinHeight;
  if (contentHeight > 1.0f) {
    ImGui::BeginChild("KeybindsDrawerContent", ImVec2(0, contentHeight), true, ImGuiWindowFlags_NoScrollbar);
    if (m_keybindsDrawerExpanded) {
      RenderKeybindsSettings();
    }
    ImGui::EndChild();
  }

  // --- Key Capture Popup ---
  if (m_actionBeingEdited.has_value()) {
    ImGui::OpenPopup(m_keyCapturePopupTitleKey.c_str());
  }

  ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(m_keyCapturePopupTitleKey.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    // Check for conflict first
    if (m_conflictInfo.has_value()) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SettingsWindow");
      std::string inputDisplayName = m_conflictInfo->capturedInput->GetDisplayName();

      Typography::Text(TextStyle::H3().Separator().Color(UI::Colors::RED), "%s", m_keyCaptureConflictTitleKey.c_str());
      // The localization string for m_keyCaptureConflictTextDetailedKey should contain markdown, e.g., "This key is already bound to **%s**."
      std::string conflictDetails = loc.GetFormatted("framework", "settings_window.key_capture_popup.conflict_text_detailed", inputDisplayName);
      Typography::RenderMarkdownText(conflictDetails, TextStyle::Regular().Wrapped().Padding({0.0f, 10.0f}));

      ImGui::Separator();

      // --- Get conflict analysis from the manager ---
      auto analysis = Modules::KeyBindsManager::GetInstance().AnalyzeConflictsForInput(*m_conflictInfo->capturedInput);

      // --- List all conflicts ---
      // Helper to render conflict details, now including the plugin name
      auto renderConflictInfo = [&](const auto& conflict, const std::string& typeKey, bool isSide = false) {
        if (!conflict) return;

        const std::string& fullActionName = conflict->first;

        // 1. Get Owner/Plugin Name
        size_t lastDot = fullActionName.rfind('.');
        if (lastDot == std::string::npos) return;  // Should not happen
        std::string group = fullActionName.substr(0, lastDot);
        size_t firstDot = group.find('.');
        std::string ownerName = (firstDot != std::string::npos) ? group.substr(0, firstDot) : group;

        std::string ownerDisplayName = ownerName;
        auto it_owner = m_configService.GetAllComponentInfo().find(ownerName);
        if (it_owner != m_configService.GetAllComponentInfo().end() && it_owner->second.name.has_value()) {
          ownerDisplayName = it_owner->second.name.value();
        }

        // 2. Get Translated Action Name
        std::string actionName = this->GetTranslatedActionName(fullActionName);

        // 3. Get Translated Type (Press Type or Side)
        std::string translatedType = typeKey;

        // 4. Format and Render
        // We use the same format key, but it will now say "Side: Positive" instead of "Press type: Short" if we pass the right strings
        std::string markdownText = loc.GetFormatted("framework", "settings_window.key_capture_popup.action_list_format", ownerDisplayName, actionName, translatedType);
        Typography::RenderMarkdownText(markdownText, TextStyle::Regular().Wrapped().Padding({0.0f, 10.0f}));
      };

      bool isAxis =
        (m_conflictInfo->capturedInput->GetType() == Modules::InputType::GamepadAxis || m_conflictInfo->capturedInput->GetType() == Modules::InputType::MouseAxis || m_conflictInfo->capturedInput->GetType() == Modules::InputType::JoystickAxis);

      if (isAxis) {
        renderConflictInfo(analysis.bothConflict, m_enumSideBothKey, true);
        renderConflictInfo(analysis.positiveConflict, m_enumSidePositiveKey, true);
        renderConflictInfo(analysis.negativeConflict, m_enumSideNegativeKey, true);
      } else {
        renderConflictInfo(analysis.shortPressConflict, m_enumPressTypeShortKey);
        renderConflictInfo(analysis.longPressConflict, m_enumPressTypeLongKey);
      }
      ImGui::Separator();

      // --- Helper lambda to add a binding ---
      auto addBinding = [&](const std::string& pressType, const std::string& side = "") {
        m_eventManager.System.OnRequestInputCaptureCancel.Call({});
        logger->Info("User chose to add a new binding for action '{}' with press type '{}' and side '{}'.", m_conflictInfo->actionFullName, pressType, side);

        nlohmann::ordered_json newBinding = m_conflictInfo->capturedInput->ToJson();

        if (isAxis && !side.empty()) {
          newBinding["side"] = side;
          newBinding["mode"] = "analog";  // Default to analog when specifying side
        } else {
          newBinding["press_type"] = pressType;
        }

        m_eventManager.System.OnRequestBindingUpdate.Call({m_conflictInfo->actionFullName, m_conflictInfo->originalBinding, newBinding, std::nullopt});

        m_conflictInfo.reset();
        m_actionBeingEdited.reset();
        ImGui::CloseCurrentPopup();
      };

      // --- Helper lambda to reassign a binding ---
      auto reassignBinding = [&](const std::optional<std::pair<std::string, nlohmann::ordered_json>>& conflict) {
        if (!conflict) return;
        m_eventManager.System.OnRequestInputCaptureCancel.Call({});
        logger->Info("User confirmed reassigning input '{}' from '{}' to '{}'.", inputDisplayName, conflict->first, m_conflictInfo->actionFullName);

        nlohmann::ordered_json newBindingJson = m_conflictInfo->capturedInput->ToJson();
        const auto& conflictingBindingJson = conflict->second;

        // Copy relevant properties from the conflicting binding
        if (conflictingBindingJson.contains("press_type")) {
          newBindingJson["press_type"] = conflictingBindingJson["press_type"];
        }
        if (conflictingBindingJson.contains("side")) {
          newBindingJson["side"] = conflictingBindingJson["side"];
        }
        if (conflictingBindingJson.contains("mode")) {
          newBindingJson["mode"] = conflictingBindingJson["mode"];
        }
        if (conflictingBindingJson.contains("press_threshold_ms")) {
          newBindingJson["press_threshold_ms"] = conflictingBindingJson["press_threshold_ms"];
        }

        m_eventManager.System.OnRequestBindingUpdate.Call({m_conflictInfo->actionFullName, m_conflictInfo->originalBinding, newBindingJson, *conflict});  // Pass the whole pair

        m_conflictInfo.reset();
        m_actionBeingEdited.reset();
        ImGui::CloseCurrentPopup();
      };

      // --- Dynamic Buttons ---
      if (isAxis) {
        // --- SPLIT LOGIC FOR AXES ---
        // Rule: No two bindings can have 'Both'. If adding a side to 'Both', 'Both' becomes 'Opposite'.

        // 1. Add Positive Side Button
        if (analysis.isPositiveAvailable || analysis.bothConflict) {
          if (Button(m_keyCaptureAddPositiveSideButtonKey.c_str())) {
            if (analysis.bothConflict) {
              // SPLIT: Update existing Both -> Negative
              m_eventManager.System.OnRequestBindingPropertyUpdate.Call({analysis.bothConflict->first, analysis.bothConflict->second, "side", "negative"});
            }
            addBinding("short", "positive");
          }
          ImGui::SameLine();
        }

        // 2. Add Negative Side Button
        if (analysis.isNegativeAvailable || analysis.bothConflict) {
          if (Button(m_keyCaptureAddNegativeSideButtonKey.c_str())) {
            if (analysis.bothConflict) {
              // SPLIT: Update existing Both -> Positive
              m_eventManager.System.OnRequestBindingPropertyUpdate.Call({analysis.bothConflict->first, analysis.bothConflict->second, "side", "positive"});
            }
            addBinding("short", "negative");
          }
          ImGui::SameLine();
        }

        // 3. Reassign Axis Buttons
        if (analysis.bothConflict) {
          if (Button(m_keyCaptureReassignEntireAxisButtonKey.c_str())) {
            reassignBinding(analysis.bothConflict);
          }
          ImGui::SameLine();
        } else {
          if (analysis.positiveConflict) {
            if (Button(m_keyCaptureReassignPositiveSideButtonKey.c_str())) {
              reassignBinding(analysis.positiveConflict);
            }
            ImGui::SameLine();
          }
          if (analysis.negativeConflict) {
            if (Button(m_keyCaptureReassignNegativeSideButtonKey.c_str())) {
              reassignBinding(analysis.negativeConflict);
            }
            ImGui::SameLine();
          }
        }
      } else {
        if (analysis.isShortPressAvailable) {
          if (Button(m_keyCaptureAddShortPressButtonKey.c_str())) {
            addBinding("short");
          }
          ImGui::SameLine();
        }

        if (analysis.isLongPressAvailable) {
          if (Button(m_keyCaptureAddLongPressButtonKey.c_str())) {
            addBinding("long");
          }
          ImGui::SameLine();
        }
      }

      if (analysis.shortPressConflict) {
        if (Button(m_keyCaptureReassignShortPressButtonKey.c_str())) {
          reassignBinding(analysis.shortPressConflict);
        }
        ImGui::SameLine();
      }

      if (analysis.longPressConflict) {
        if (Button(m_keyCaptureReassignLongPressButtonKey.c_str())) {
          reassignBinding(analysis.longPressConflict);
        }
        ImGui::SameLine();
      }

      // --- Cancel Button ---
      if (Button(m_keyCaptureCancelButtonKey.c_str())) {
        logger->Info("User cancelled reassigning input '{}'.", inputDisplayName);
        m_conflictInfo.reset();
        ImGui::CloseCurrentPopup();

        if (m_actionBeingEdited.has_value()) {
          m_eventManager.System.OnRequestInputCapture.Call({m_actionBeingEdited.value(), m_editingBindingObject});
        }
      }
    }
    // Then check for successful capture (either direct or after conflict resolution)
    else if (m_bufferedInputInfo.has_value()) {
      // Verify the captured key is for the action we are currently editing
      if (m_actionBeingEdited.has_value() && m_bufferedInputInfo->actionFullName == m_actionBeingEdited.value()) {
        const auto& actionFullName = m_bufferedInputInfo->actionFullName;
        const auto& originalBinding = m_bufferedInputInfo->originalBinding;
        const auto& newBindingJson = m_bufferedInputInfo->capturedInput->ToJson();

        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SettingsWindow");
        logger->Info("Requesting keybinding update for action '{}' to new input '{}'.", actionFullName, newBindingJson.dump());

        m_eventManager.System.OnRequestBindingUpdate.Call({
          actionFullName,
          originalBinding,
          newBindingJson,
          std::nullopt  // No conflict, so nothing to clear
        });
      }

      // The event has been processed. Clear the buffer, reset the state, and close the popup.
      m_bufferedInputInfo.reset();
      m_actionBeingEdited.reset();
      m_currentChordInputs.clear();
      ImGui::CloseCurrentPopup();
    } else {
      // If no key has been captured yet, display the popup's content
      Typography::Text(TextStyle::Regular().Wrapped(), m_keyCapturePressKeyTextKey.c_str());

      ImGui::Spacing();
      // --- Rich Chord Display ---
      if (!m_currentChordInputs.empty()) {
        auto getIconForInput = [](Modules::InputType type) -> const char* {
          switch (type) {
            case Modules::InputType::Keyboard:
              return ICON_FA_KEYBOARD;
            case Modules::InputType::Gamepad:
              return ICON_FA_GAMEPAD;
            case Modules::InputType::GamepadAxis:
              return ICON_FA_GAMEPAD;
            case Modules::InputType::Mouse:
              return ICON_FA_COMPUTER_MOUSE;
            case Modules::InputType::MouseAxis:
              return ICON_FA_COMPUTER_MOUSE;
            case Modules::InputType::Joystick:
              return ICON_FA_GAMEPAD;
            case Modules::InputType::JoystickAxis:
              return ICON_FA_GAMEPAD;
            default:
              return "";
          }
        };

        float totalWidth = 0;
        float spacing = 4.0f;
        for (size_t i = 0; i < m_currentChordInputs.size(); ++i) {
          const char* icon = getIconForInput(m_currentChordInputs[i]->GetType());
          std::string label = fmt::format("{} {}", icon, m_currentChordInputs[i]->GetDisplayName());
          totalWidth += ImGui::CalcTextSize(label.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
          if (i < m_currentChordInputs.size() - 1) {
            totalWidth += spacing + ImGui::CalcTextSize("+").x + spacing;
          }
        }

        float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
        if (startX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));
        for (size_t i = 0; i < m_currentChordInputs.size(); ++i) {
          const auto& input = m_currentChordInputs[i];
          std::string label = fmt::format("{} {}", getIconForInput(input->GetType()), input->GetDisplayName());

          // Render as a button frame to match settings UI, but inactive
          ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
          Button(label.c_str());
          ImGui::PopStyleColor(3);

          if (i < m_currentChordInputs.size() - 1) {
            ImGui::SameLine();
            Typography::Text(TextStyle::Regular().Color(UI::Colors::GRAY), "+");
            ImGui::SameLine();
          }
        }
        ImGui::PopStyleVar();
        ImGui::Spacing();
      }

      Typography::Text(TextStyle::H3().Color(UI::Colors::YELLOW).Align(TextAlign::Center), this->GetTranslatedActionName(m_actionBeingEdited.value()).c_str());
      ImGui::Separator();

      // Show Delete button only when editing an existing, non-empty binding
      if (!m_editingBindingObject.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
        if (Button(m_keyCaptureDeleteButtonKey.c_str())) {
          // Store the name in a local variable because m_actionBeingEdited
          // will be reset by the Cancel call below.
          std::string actionName = m_actionBeingEdited.value();
          nlohmann::ordered_json bindingCopy = m_editingBindingObject;

          m_eventManager.System.OnRequestInputCaptureCancel.Call({});

          m_currentChordInputs.clear();

          m_eventManager.System.OnRequestDeleteBinding.Call({actionName, bindingCopy});
          m_actionBeingEdited.reset();
          ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
      }

      if (Button(m_keyCaptureCancelButtonKey.c_str(), TextStyle::DefaultButton(), ImVec2(120, 0))) {
        m_eventManager.System.OnRequestInputCaptureCancel.Call({});
        // This is a direct and safe UI state change.
        m_actionBeingEdited.reset();
        m_currentChordInputs.clear();
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::EndPopup();
  }

  // --- Binding Details Popup ---
  if (m_shouldOpenBindingDetailsPopup) {
    ImGui::OpenPopup(m_bindingDetailsPopupTitleKey.c_str());
    m_shouldOpenBindingDetailsPopup = false;
  }

  if (ImGui::BeginPopupModal(m_bindingDetailsPopupTitleKey.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (!m_editingBindingDetails.has_value() || !m_editingBindingAction.has_value()) {
      // Should not happen if popup is open, but as a safeguard
      ImGui::CloseCurrentPopup();
    } else {
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
        ImGui::TextUnformatted(m_bindingDetailsModeLabelKey.c_str());
        addTooltip("settings_window.binding_details_popup.mode_label");
        ImGui::SameLine();

        if (isMouse) ImGui::BeginDisabled();

        if (ImGui::RadioButton(m_bindingDetailsModeAnalogKey.c_str(), mode == "analog")) {
          bindingJson["mode"] = "analog";
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "mode", "analog"});
          m_originalBindingCopy = bindingJson;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton(m_bindingDetailsModeDigitalKey.c_str(), mode == "digital")) {
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
          if (ImGui::Checkbox(m_bindingDetailsAccumulatorModeLabelKey.c_str(), &accumulator)) {
            bindingJson["accumulator"] = accumulator;
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "accumulator", accumulator});
            m_originalBindingCopy = bindingJson;
          }
          addTooltip("settings_window.binding_details_popup.accumulator_mode_label");
        }

        if (!isMouse && !accumulator) {
          // Deadzone
          if (ImGui::SliderFloat(m_bindingDetailsDeadzoneLabelKey.c_str(), &deadzone, 0.0f, 0.5f, "%.2f")) {
            bindingJson["deadzone"] = deadzone;
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "deadzone", deadzone});
            m_originalBindingCopy = bindingJson;
          }
          addTooltip("settings_window.binding_details_popup.deadzone_label");

          // Saturation
          if (ImGui::SliderFloat(m_bindingDetailsSaturationLabelKey.c_str(), &saturation, 0.5f, 1.0f, "%.2f")) {
            bindingJson["saturation"] = saturation;
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "saturation", saturation});
            m_originalBindingCopy = bindingJson;
          }
          addTooltip("settings_window.binding_details_popup.saturation_label");
        }

        // Sensitivity
        if (ImGui::SliderFloat(m_bindingDetailsSensitivityLabelKey.c_str(), &sensitivity, 0.1f, 5.0f, "%.1f")) {
          bindingJson["sensitivity"] = sensitivity;
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "sensitivity", sensitivity});
          m_originalBindingCopy = bindingJson;
        }
        addTooltip("settings_window.binding_details_popup.sensitivity_label");

        if (!isMouse && !accumulator) {
          // Curve
          if (ImGui::BeginCombo(m_bindingDetailsCurveLabelKey.c_str(), curve.c_str())) {
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
          if (ImGui::SliderFloat(m_bindingDetailsSmoothingLabelKey.c_str(), &smoothing, 0.0f, 0.95f, "%.2f")) {
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
        if (ImGui::SliderFloat(m_bindingDetailsRangeMinLabelKey.c_str(), &rMin, rMinLimit, rMax, "%.2f")) {
          bindingJson["range_min"] = rMin;
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "range_min", rMin});
          m_originalBindingCopy = bindingJson;
        }
        addTooltip("settings_window.binding_details_popup.range_min_label");

        // Range Max
        rMax = bindingJson.value("range_max", rMaxLimit);
        if (ImGui::SliderFloat(m_bindingDetailsRangeMaxLabelKey.c_str(), &rMax, rMin, rMaxLimit, "%.2f")) {
          bindingJson["range_max"] = rMax;
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "range_max", rMax});
          m_originalBindingCopy = bindingJson;
        }
        addTooltip("settings_window.binding_details_popup.range_max_label");

        // Invert
        bool invert = bindingJson.value("invert", false);
        if (ImGui::Checkbox(m_bindingDetailsInvertLabelKey.c_str(), &invert)) {
          bindingJson["invert"] = invert;
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "invert", invert});
          m_originalBindingCopy = bindingJson;
        }
        addTooltip("settings_window.binding_details_popup.invert_label");

        // Side
        ImGui::TextUnformatted(m_bindingDetailsSideLabelKey.c_str());
        addTooltip("settings_window.binding_details_popup.side_label");
        ImGui::SameLine();

        if (isTrigger) ImGui::BeginDisabled();

        if (ImGui::RadioButton(m_bindingDetailsSideBothKey.c_str(), side == "both")) {
          bindingJson["side"] = "both";
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "side", "both"});
          m_originalBindingCopy = bindingJson;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton(m_bindingDetailsSidePositiveKey.c_str(), side == "positive")) {
          bindingJson["side"] = "positive";
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "side", "positive"});
          m_originalBindingCopy = bindingJson;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton(m_bindingDetailsSideNegativeKey.c_str(), side == "negative")) {
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
          if (ImGui::SliderFloat(m_bindingDetailsThresholdLabelKey.c_str(), &threshold, 0.05f, 0.95f, "%.2f")) {
            bindingJson["threshold"] = threshold;
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "threshold", threshold});
            m_originalBindingCopy = bindingJson;
          }
          addTooltip("settings_window.binding_details_popup.threshold_label");
          ImGui::Separator();
        }

        // --- Press Type Setting with Radio Buttons ---
        ImGui::TextUnformatted(m_bindingDetailsPressTypeLabelKey.c_str());
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

            // Use project's loc.GetFormatted for formatted localized strings
            std::string pressType = loc.Get("enums.press_type." + newPressType);
            std::string markdownText = loc.GetFormatted(m_currentComponent, m_conflictPressTypeMessage, pressType);
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
            std::string actionDisplayName = GetTranslatedActionName(conflictingActionName);
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
        ImGui::TextUnformatted(m_bindingDetailsBehaviorLabelKey.c_str());
        addTooltip("settings_window.binding_details_popup.behavior_label");
        ImGui::SameLine();
        ImGui::PushID("details_behavior");
        std::string currentBehavior = bindingJson.value("behavior", "toggle");
        if (ImGui::RadioButton(m_bindingDetailsBehaviorToggleKey.c_str(), currentBehavior == "toggle")) {
          bindingJson["behavior"] = "toggle";
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "behavior", "toggle"});
          m_originalBindingCopy = bindingJson;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton(m_bindingDetailsBehaviorHoldKey.c_str(), currentBehavior == "hold")) {
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
      ImGui::TextUnformatted(m_bindingDetailsConsumeLabelKey.c_str());
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
        ImGui::TextUnformatted(m_bindingDetailsThresholdLabelKey.c_str());
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

      if (Button(m_bindingDetailsCloseButtonKey.c_str())) {
        m_editingBindingDetails.reset();
        m_editingBindingAction.reset();
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::EndPopup();
  }
}

void SettingsWindow::OnInputCaptured(const Input::InputCaptured& e) {
  // This is called from a non-render thread.
  // We just buffer the result. The processing will happen safely in RenderContent().
  m_bufferedInputInfo = e;
}

void SettingsWindow::OnInputCaptureCancelled(const Input::InputCaptureCancelled& e) {
  m_actionBeingEdited.reset();
  m_currentChordInputs.clear();
}

void SettingsWindow::OnInputCaptureUpdate(const Input::InputCaptureUpdate& e) {
  if (!m_actionBeingEdited.has_value() || m_actionBeingEdited.value() != e.actionFullName) return;
  m_currentChordInputs = e.currentChordInputs;
}

void SettingsWindow::OnInputCaptureConflict(const Input::InputCaptureConflict& e) {
  // When a conflict is detected, store the info and open the conflict resolution popup.
  m_conflictInfo = e;
  // We don't reset m_actionBeingEdited here, as the conflict popup needs it.
  // The conflict popup will handle closing itself or returning to the capture state.
}

}  // namespace UI

SPF_NS_END
