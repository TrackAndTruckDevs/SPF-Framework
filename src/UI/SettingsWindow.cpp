#include "SPF/UI/SettingsWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/Modules/InputDisplay.hpp"
#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIManager.hpp"
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
      m_onKeybindsModifiedSink(std::make_unique<Utils::Sink<void(const Events::Config::OnKeybindsModified&)>>(eventManager.System.OnKeybindsModified)) {
  m_defaultTitle = "Settings";
  m_titleLocalizationKey = "settings_window.title";
  m_keybindsDrawerHeight = m_keybindsDrawerMinHeight;

  m_onFocusComponentSink->Connect<&SettingsWindow::OnFocusComponent>(this);
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
              UIManager::GetInstance().GetKeyCapturePopup().Open(fullActionName, nlohmann::ordered_json::object());
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
                  UIManager::GetInstance().GetKeyCapturePopup().Open(fullActionName, nlohmann::ordered_json::object());
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

                  std::string buttonText = Modules::GetDisplayNameWithIcon(*input);

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
                    UIManager::GetInstance().GetKeyCapturePopup().Open(fullActionName, bindingJson);
                  }
                  if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", loc.Get("settings_window.keybinds_drawer.table.key_button_tooltip").c_str());
                  }
                  ImGui::PopID();
                  ImGui::SameLine();

                  ImGui::PushID(("details_" + uniqueId).c_str());
                  if (ImGui::SmallButton(ICON_FA_GEAR)) {
                    UIManager::GetInstance().GetBindingDetailsPopup().Open(fullActionName, bindingJson);
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
}

}  // namespace UI

SPF_NS_END
