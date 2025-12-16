#include "SPF/UI/SettingsWindow.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"
#include "SPF/Utils/Signal.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Config/EnumMappings.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <fmt/core.h>
#include <algorithm>
#include <cmath>  // For roundf

SPF_NS_BEGIN

namespace UI {
using namespace SPF::Localization;
using namespace SPF::Logging;
using namespace SPF::System;

SettingsWindow::SettingsWindow(Config::IConfigService& configService, const std::vector<std::string>& logLevels, Events::EventManager& eventManager,
                               const std::string& componentName, const std::string& windowId)
    : BaseWindow(componentName, windowId),
      m_configService(configService),
      m_logLevels(logLevels),
      m_eventManager(eventManager),
      m_onFocusComponentSink(std::make_unique<Utils::Sink<void(const Events::UI::FocusComponentInSettingsWindow&)>>(eventManager.System.OnFocusComponentInSettingsWindow)) {
  m_defaultTitle = "Settings";
  m_titleLocalizationKey = "settings_window.title";
  m_keybindsDrawerHeight = m_keybindsDrawerMinHeight;
  m_keybindsDrawerTitleKey = "settings_window.keybinds_drawer.title";
  m_keybindsActionHeaderKey = "settings_window.keybinds_drawer.table.action";
  m_keybindsKeyHeaderKey = "settings_window.keybinds_drawer.table.key";

  m_keyCapturePopupTitleKey = "settings_window.key_capture_popup.title";
  m_keyCapturePressKeyTextKey = "settings_window.key_capture_popup.press_key_text";
  m_keyCaptureDeleteButtonKey = "settings_window.key_capture_popup.delete_button";
  m_keyCaptureCancelButtonKey = "settings_window.key_capture_popup.cancel_button";
  m_keyCaptureConflictTitleKey = "settings_window.key_capture_popup.conflict_title";
  m_keyCaptureConflictTextKey = "settings_window.key_capture_popup.conflict_text";
  m_keyCaptureConflictReassignQuestionKey = "settings_window.key_capture_popup.reassign_question";
  m_keyCaptureConflictYesButtonKey = "settings_window.key_capture_popup.yes_button";
  m_keyCaptureConflictNoButtonKey = "settings_window.key_capture_popup.no_button";

  m_bindingDetailsPopupTitleKey = "settings_window.binding_details_popup.title";
  m_bindingDetailsPressTypeLabelKey = "settings_window.binding_details_popup.press_type_label";
  m_bindingDetailsBehaviorLabelKey = "settings_window.binding_details_popup.behavior_label";
  m_bindingDetailsBehaviorToggleKey = "settings_window.binding_details_popup.behavior_toggle";
  m_bindingDetailsBehaviorHoldKey = "settings_window.binding_details_popup.behavior_hold";
  m_bindingDetailsConsumeLabelKey = "settings_window.binding_details_popup.consume_label";
  m_bindingDetailsThresholdLabelKey = "settings_window.binding_details_popup.threshold_label";
  m_bindingDetailsCloseButtonKey = "settings_window.binding_details_popup.close_button";

  m_keybindsUnassignedTextKey = "settings_window.keybinds_drawer.unassigned_text";

  m_onFocusComponentSink->Connect<&SettingsWindow::OnFocusComponent>(this);
}

const char* SettingsWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_titleLocalizationKey).c_str(); }

void SettingsWindow::OnFocusComponent(const Events::UI::FocusComponentInSettingsWindow& e) {
  m_currentComponent = e.componentName;
  SetVisibility(true);
  Focus();
}

void SettingsWindow::PopulateConfigurableComponents() {
  // This function is now obsolete and replaced by dynamic logic in RenderContent.
}

void SettingsWindow::RenderSettingsNode(const std::string& key, const nlohmann::json& node, const std::string& systemName, const std::string& currentPath) {
  std::string fullPath = currentPath.empty() ? key : currentPath + "." + key;
  std::string fullSystemPath = systemName + "." + fullPath;

  auto& loc = LocalizationManager::GetInstance();

  // Extract the actual value node and determine display name
  const nlohmann::json* valueNode = &node;
  std::string displayName = key; // Default to raw key

  if (node.is_object() && node.contains("_value")) {
      valueNode = &node["_value"];
      if (node.contains("_meta") && node["_meta"].is_object()) {
          const auto& meta = node["_meta"];
          if (meta.contains("titleKey") && meta["titleKey"].is_string()) {
              const auto& titleKey = meta["titleKey"].get<std::string>();
              if (!titleKey.empty()) {
                  if (systemName == "logging" || systemName == "localization" || systemName == "ui") {
                      displayName = loc.GetWithFallback(m_currentComponent, titleKey);
                  } else {
                      displayName = loc.Get(m_currentComponent, titleKey);
                  }
              }
          }
      }
  } else if (node.is_object() && node.contains("_meta")) { // It's an object that is just metadata, no _value
      const auto& meta = node["_meta"];
      if (meta.contains("titleKey") && meta["titleKey"].is_string()) {
          const auto& titleKey = meta["titleKey"].get<std::string>();
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
    if (ImGui::IsItemHovered() && node.is_object() && node.contains("_meta") && node["_meta"].is_object() && node["_meta"].contains("descriptionKey") && node["_meta"]["descriptionKey"].is_string()) {
        const auto& descKey = node["_meta"]["descriptionKey"].get<std::string>();
        if (!descKey.empty()) {
            if (systemName == "logging" || systemName == "localization" || systemName == "ui") {
                ImGui::SetTooltip("%s", loc.GetWithFallback(m_currentComponent, descKey).c_str());
            } else {
                ImGui::SetTooltip("%s", loc.Get(m_currentComponent, descKey).c_str());
            }
        }
    }
  };

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

      if (ImGui::BeginCombo(label.c_str(), currentLangDisplay.c_str())) {
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
      if (ImGui::BeginCombo(label.c_str(), currentLevelStr.c_str())) {
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
      if (ImGui::InputText(label.c_str(), buf, sizeof(buf))) {
        m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, std::string(buf)});
      }
      ShowTooltip();
    }
  } else if (valueNode->is_number_integer()) {
    int value = valueNode->get<int>();
    if (ImGui::InputInt(label.c_str(), &value)) {
      m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
    }
    ShowTooltip();
  } else if (valueNode->is_number_float()) {
    float value = valueNode->get<float>();
    if (ImGui::InputFloat(label.c_str(), &value)) {
      m_eventManager.System.OnRequestSettingChange.Call({m_currentComponent, fullSystemPath, value});
    }
    ShowTooltip();
  } else if (node.is_object() && !node.contains("_value")) { // It's a nested settings group
    bool node_open = ImGui::TreeNode(label.c_str());
    ShowTooltip(); // Show tooltip for the TreeNode label itself
    if (node_open) {
      for (auto it = node.begin(); it != node.end(); ++it) {
        if (it.key() == "_meta") continue;
        RenderSettingsNode(it.key(), it.value(), systemName, fullPath);
      }
      ImGui::TreePop();
    }
  } else if (valueNode->is_array()) {
    ImGui::Text("%s: [Array]", key.c_str());
    ShowTooltip();
  } else if (valueNode->is_null()) {
    ImGui::Text("%s: [Null]", key.c_str());
    ShowTooltip();
  }
}

void SettingsWindow::RenderKeybindsSettings() {
    if (!m_configService.GetMergedConfig("keybinds")) {
        ImGui::Text("Keybindings configuration is not available.");
        return;
    }

    auto& loc = LocalizationManager::GetInstance();
    ImGuiTableFlags container_flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_NoPadInnerX;
    
    if (ImGui::BeginTable("keybinds_main_container", 1, container_flags)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        // --- Header Table ---
        ImGuiTableFlags header_flags = ImGuiTableFlags_Borders;
        if (ImGui::BeginTable("keybinds_header_table", 2, header_flags)) {
            ImGui::TableSetupColumn(loc.Get(m_keybindsActionHeaderKey).c_str(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(loc.Get(m_keybindsKeyHeaderKey).c_str(), ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
            for (int column = 0; column < 2; column++) {
                ImGui::TableSetColumnIndex(column);
                const char* columnName = ImGui::TableGetColumnName(column);
                float textWidth = ImGui::CalcTextSize(columnName).x;
                float columnWidth = ImGui::GetColumnWidth();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - textWidth) * 0.5f);
                ImGui::TextUnformatted(columnName);
            }
            ImGui::EndTable();
        }

        // --- Data Grouping ---
        std::map<std::string, std::vector<std::tuple<std::string, std::string, const nlohmann::json*>>> groupedActions;
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
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);

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
                        float rowContentHeight = ImGui::GetFrameHeight(); // Default height for one line
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

                        ImGui::TextUnformatted(actionDisplayName.c_str());

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
                        float buttonWidth = ImGui::GetFrameHeight(); // SmallButton is roughly square
                        ImGui::SameLine(ImGui::GetColumnWidth() - (buttonWidth * 0.5f) - ImGui::GetStyle().CellPadding.x);

                        ImGui::PushID((fullActionName + ":add_button").c_str());
                        if (ImGui::SmallButton(ICON_FA_PLUS)) {
                            m_actionBeingEdited = fullActionName;
                            m_editingBindingObject = nlohmann::json::object();
                            m_eventManager.System.OnRequestInputCapture.Call({fullActionName, m_editingBindingObject});
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", loc.Get("settings_window.keybinds_drawer.table.add_button_tooltip").c_str());
                        }
                        ImGui::PopID();


                        // --- Column 1: Keybinds ---
                        ImGui::TableSetColumnIndex(1);

                        const nlohmann::json* bindings = nullptr;
                        if (actionObject->is_object() && actionObject->contains("bindings") && (*actionObject)["bindings"].is_array()) {
                            bindings = &(*actionObject)["bindings"];
                        }

                        if (bindings) {
                            if (bindings->empty()) {
                                ImGui::PushID((fullActionName + ":add_new").c_str());
                                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                                if (ImGui::Button(loc.Get(m_keybindsUnassignedTextKey).c_str())) {
                                    m_actionBeingEdited = fullActionName;
                                    m_editingBindingObject = nlohmann::json::object();
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

                                    std::string displayName = input->GetDisplayName();
                                    if (displayName.empty() || displayName == "Unknown") continue;

                                    const char* bindingIcon = "";
                                    switch (input->GetType()) {
                                        case Modules::InputType::Keyboard: bindingIcon = ICON_FA_KEYBOARD; break;
                                        case Modules::InputType::Gamepad: bindingIcon = ICON_FA_GAMEPAD; break;
                                        case Modules::InputType::Mouse: bindingIcon = ICON_FA_MOUSE; break;
                                        case Modules::InputType::Joystick: bindingIcon = ICON_FA_GAMEPAD; break;
                                        default: break;
                                    }

                                    std::string buttonText = fmt::format("{} {}", bindingIcon, displayName);
                                    std::string uniqueId = fullActionName + ":" + displayName;

                                    ImGui::PushID(uniqueId.c_str());
                                    if (ImGui::Button(buttonText.c_str())) {
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
                                    if (ImGui::SmallButton(ICON_FA_COG)) {
                                        m_editingBindingAction = fullActionName;
                                        m_editingBindingDetails = bindingJson;
                                        m_currentPressThreshold = bindingJson.value("press_threshold_ms", 500);
                                        m_originalBindingCopy = bindingJson;
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
    
    float vertical_spacing = 2.0f; // A small gap between icon and text
    float total_content_height = icon_size.y + label_size.y + vertical_spacing;
    float start_y = p_min.y + (buttonHeight - total_content_height) / 2.0f;

    draw_list->AddText(ImVec2(p_min.x + (availableWidth - icon_size.x) / 2.0f, start_y), ImGui::GetColorU32(ImGuiCol_Text), icon);
    draw_list->AddText(ImVec2(p_min.x + (availableWidth - label_size.x) / 2.0f, start_y + icon_size.y + vertical_spacing), ImGui::GetColorU32(ImGuiCol_Text), label);


    // --- Popup with Selectable Items (with added padding) ---
    ImGui::SetNextWindowPos(ImVec2(p_min.x, p_max.y));
    ImGui::SetNextWindowSize(ImVec2(availableWidth, 0));
    if (ImGui::BeginPopup("component_selector_popup")) {
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, ImGui::GetStyle().FramePadding.y * 2.0f)); // Double vertical padding
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
      ImGui::PopStyleVar(); // Pop FramePadding
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
      ImGui::Text("No configurable components available.");
    } else {
      const auto& infoIt = m_configService.GetAllComponentInfo().find(m_currentComponent);
      if (infoIt == m_configService.GetAllComponentInfo().end()) {
        ImGui::Text("Error: Could not find info for component '%s'", m_currentComponent.c_str());
      } else {
        const auto& systemsToRender = infoIt->second.configurableSystems;
        const auto& componentSettingsIt = m_configService.GetAggregatedUserSettings().find(m_currentComponent);
        if (componentSettingsIt == m_configService.GetAggregatedUserSettings().end() || systemsToRender.empty()) {
          ImGui::Text("This component has no configurable systems.");
        } else {
          const auto& componentSettingsData = componentSettingsIt->second;
          auto& loc = LocalizationManager::GetInstance();

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

              ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f)); // Thinner header
              bool headerIsOpen = ImGui::CollapsingHeader(systemDisplayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
              ImGui::PopStyleVar();

              if (headerIsOpen) {
                // --- Settings Table ---
                if (ImGui::BeginTable(("table_" + systemName).c_str(), 2, ImGuiTableFlags_BordersInnerV)) {
                  ImGui::TableSetupColumn("Setting", ImGuiTableColumnFlags_WidthStretch);
                  ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                  for (auto& [key, value] : systemSettings.items()) {
                    ImGui::TableNextRow();

                    // --- Column 1: Localized Setting Name ---
                    ImGui::TableSetColumnIndex(0);
                    std::string settingDisplayName = key; // Default to raw key
                    if (value.is_object() && value.contains("_meta")) {
                        const auto& meta = value["_meta"];
                        if (meta.contains("titleKey") && meta["titleKey"].is_string()) {
                            const auto& titleKey = meta["titleKey"].get<std::string>();
                            if (!titleKey.empty()) {
                                if (systemName == "logging" || systemName == "localization" || systemName == "ui") {
                                    settingDisplayName = loc.GetWithFallback(m_currentComponent, titleKey);
                                } else {
                                    settingDisplayName = loc.Get(m_currentComponent, titleKey);
                                }
                            }
                        }
                    }
                    ImGui::TextUnformatted(settingDisplayName.c_str());
                    // Add tooltip for the setting name itself
                    if (value.is_object() && value.contains("_meta") && value["_meta"].is_object() && value["_meta"].contains("descriptionKey") && value["_meta"]["descriptionKey"].is_string()) {
                        if (ImGui::IsItemHovered()) {
                            const auto& descriptionKey = value["_meta"]["descriptionKey"].get<std::string>();
                            if (!descriptionKey.empty()) {
                                if (systemName == "logging" || systemName == "localization" || systemName == "ui") {
                                    ImGui::SetTooltip("%s", loc.GetWithFallback(m_currentComponent, descriptionKey).c_str());
                                } else {
                                    ImGui::SetTooltip("%s", loc.Get(m_currentComponent, descriptionKey).c_str());
                                }
                            }
                        }
                    }

                    // --- Column 2: Setting Control ---
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(key.c_str());
                    // Pass the whole node to RenderSettingsNode, it will handle the rest
                    RenderSettingsNode(key, value, systemName, "");
                    ImGui::PopID();
                  }
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
  m_keybindsDrawerHeight = ImClamp(m_keybindsDrawerHeight, m_keybindsDrawerMinHeight, m_keybindsDrawerMaxHeight);
  m_keybindsDrawerExpanded = (m_keybindsDrawerHeight > m_keybindsDrawerMinHeight + 5.0f);

  // Custom rendering for the handle
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImVec2 p_min = ImGui::GetItemRectMin();
  ImVec2 p_max = ImGui::GetItemRectMax();
  draw_list->AddRectFilled(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Button), ImGui::GetStyle().FrameRounding);
  draw_list->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding, 0, 1.0f);

  const char* title = loc.Get(m_keybindsDrawerTitleKey).c_str();
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
    ImGui::OpenPopup(loc.Get(m_keyCapturePopupTitleKey).c_str());
  }

  ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Appearing);
  if (ImGui::BeginPopupModal(loc.Get(m_keyCapturePopupTitleKey).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    auto getTranslatedActionName = [&](const std::string& fullActionName) -> std::string {
      // 1. Parse the fullActionName into group and actionName
      size_t lastDot = fullActionName.rfind('.');
      if (lastDot == std::string::npos) {
        return fullActionName;  // Cannot parse, return raw name
      }
      std::string group = fullActionName.substr(0, lastDot);
      std::string actionName = fullActionName.substr(lastDot + 1);

      // 2. Find the actionObject in m_keybindsConfig
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
    };

    // Check for conflict first
    if (m_conflictInfo.has_value()) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SettingsWindow");
      std::string inputDisplayName = m_conflictInfo->capturedInput->GetDisplayName();

      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", loc.Get(m_keyCaptureConflictTitleKey).c_str());
      ImGui::Text(loc.Get(m_keyCaptureConflictTextKey).c_str(), inputDisplayName.c_str());
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", getTranslatedActionName(m_conflictInfo->conflictingAction).c_str());
      ImGui::TextUnformatted(loc.Get(m_keyCaptureConflictReassignQuestionKey).c_str());
      ImGui::Separator();

      if (ImGui::Button(loc.Get(m_keyCaptureConflictYesButtonKey).c_str(), ImVec2(120, 0))) {
        m_eventManager.System.OnRequestInputCaptureCancel.Call({});
        logger->Info("User confirmed reassigning input '{}' from '{}' to '{}'.", inputDisplayName, m_conflictInfo->conflictingAction, m_conflictInfo->actionFullName);

        // Fire the update event with all information, including the action and key to clear.
        m_eventManager.System.OnRequestBindingUpdate.Call({m_conflictInfo->actionFullName,
                                                           m_conflictInfo->originalBinding,
                                                           m_conflictInfo->capturedInput->ToJson(),
                                                           // Create the pair for bindingToClear
                                                           std::make_pair(m_conflictInfo->conflictingAction, m_conflictInfo->capturedInput->ToJson())});

        // Reset state and close the popup
        m_conflictInfo.reset();
        m_actionBeingEdited.reset();
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button(loc.Get(m_keyCaptureConflictNoButtonKey).c_str(), ImVec2(120, 0))) {
        logger->Info("User cancelled reassigning input '{}'.", inputDisplayName);
        m_conflictInfo.reset();      // Clear conflict info
        ImGui::CloseCurrentPopup();  // Close conflict popup

        // Restart key capture for the same action
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
      ImGui::CloseCurrentPopup();
    } else {
      // If no key has been captured yet, display the popup's content
      ImGui::TextUnformatted(loc.Get(m_keyCapturePressKeyTextKey).c_str());
      ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", getTranslatedActionName(m_actionBeingEdited.value()).c_str());
      ImGui::Separator();

      // Show Delete button only when editing an existing, non-empty binding
      if (!m_editingBindingObject.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
        if (ImGui::Button(loc.Get(m_keyCaptureDeleteButtonKey).c_str())) {
          m_eventManager.System.OnRequestInputCaptureCancel.Call({});
          m_eventManager.System.OnRequestDeleteBinding.Call({m_actionBeingEdited.value(), m_editingBindingObject});
          m_actionBeingEdited.reset();
          ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
      }

      if (ImGui::Button(loc.Get(m_keyCaptureCancelButtonKey).c_str(), ImVec2(120, 0))) {
        m_eventManager.System.OnRequestInputCaptureCancel.Call({});
        // This is a direct and safe UI state change.
        m_actionBeingEdited.reset();
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::EndPopup();
  }

  // --- Binding Details Popup ---
  if (m_editingBindingDetails.has_value()) {
    ImGui::OpenPopup(loc.Get(m_bindingDetailsPopupTitleKey).c_str());
  }

  if (ImGui::BeginPopupModal(loc.Get(m_bindingDetailsPopupTitleKey).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    if (!m_editingBindingDetails.has_value() || !m_editingBindingAction.has_value()) {
      // Should not happen if popup is open, but as a safeguard
      ImGui::CloseCurrentPopup();
    } else {
      // Use a reference to simplify access and allow modification
      auto& bindingJson = m_editingBindingDetails.value();
      const auto& actionFullName = m_editingBindingAction.value();

      // --- Press Type Setting ---
      ImGui::TextUnformatted(loc.Get(m_bindingDetailsPressTypeLabelKey).c_str());
      ImGui::SameLine();
      ImGui::PushID("details_press_type");
      std::string currentPressType = bindingJson.value("press_type", "short");
      std::string currentPressTypeDisplay = currentPressType;  // Default to string_id
      for (const auto& pair : Config::PressTypeMap) {
        if (pair.second.string_id == currentPressType) {
          currentPressTypeDisplay = loc.Get(pair.second.loc_key);
          break;
        }
      }
      if (ImGui::BeginCombo("##presstype", currentPressTypeDisplay.c_str())) {
        for (const auto& pair : Config::PressTypeMap) {
          bool is_selected = (currentPressType == pair.second.string_id);
          if (ImGui::Selectable(loc.Get(pair.second.loc_key).c_str(), is_selected)) {
            bindingJson["press_type"] = pair.second.string_id;  // Update local state
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "press_type", pair.second.string_id});
            m_originalBindingCopy = bindingJson;  // Update original copy for chaining
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::PopID();

      // --- Behavior Setting ---
      ImGui::TextUnformatted(loc.Get(m_bindingDetailsBehaviorLabelKey).c_str());
      ImGui::SameLine();
      ImGui::PushID("details_behavior");
      std::string currentBehavior = bindingJson.value("behavior", "toggle");
      if (ImGui::RadioButton(loc.Get(m_bindingDetailsBehaviorToggleKey).c_str(), currentBehavior == "toggle")) {
        bindingJson["behavior"] = "toggle";
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "behavior", "toggle"});
        m_originalBindingCopy = bindingJson;
      }
      ImGui::SameLine();
      if (ImGui::RadioButton(loc.Get(m_bindingDetailsBehaviorHoldKey).c_str(), currentBehavior == "hold")) {
        bindingJson["behavior"] = "hold";
        m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, m_originalBindingCopy, "behavior", "hold"});
        m_originalBindingCopy = bindingJson;
      }
      ImGui::PopID();

      // --- Consume Policy Setting ---
      ImGui::TextUnformatted(loc.Get(m_bindingDetailsConsumeLabelKey).c_str());
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
      ImGui::TextUnformatted(loc.Get(m_bindingDetailsThresholdLabelKey).c_str());
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
          nlohmann::json oldBinding = m_originalBindingCopy;
          m_originalBindingCopy["press_threshold_ms"] = finalThreshold;
          m_eventManager.System.OnRequestBindingPropertyUpdate.Call({actionFullName, oldBinding, "press_threshold_ms", finalThreshold});

          bindingJson["press_threshold_ms"] = finalThreshold;
        }
      }
      ImGui::PopID();

      ImGui::Separator();

      if (ImGui::Button(loc.Get(m_bindingDetailsCloseButtonKey).c_str())) {
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
  // This is now handled directly in the UI loop within RenderContent for safety.
}

void SettingsWindow::OnInputCaptureConflict(const Input::InputCaptureConflict& e) {
  // When a conflict is detected, store the info and open the conflict resolution popup.
  m_conflictInfo = e;
  // We don't reset m_actionBeingEdited here, as the conflict popup needs it.
  // The conflict popup will handle closing itself or returning to the capture state.
}

}  // namespace UI

SPF_NS_END
