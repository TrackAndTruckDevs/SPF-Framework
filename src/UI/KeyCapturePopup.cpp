#include "SPF/UI/KeyCapturePopup.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/Modules/InputDisplay.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/Utils/Signal.hpp"

#include "fmt/core.h"
#include "imgui.h"
#include "nlohmann/json.hpp"

SPF_NS_BEGIN

namespace UI {
using namespace SPF::Localization;
using namespace SPF::Logging;

KeyCapturePopup::KeyCapturePopup(Events::EventManager& eventManager, Config::IConfigService& configService)
    : m_eventManager(eventManager),
      m_configService(configService),
      m_onInputCaptureUpdateSink(std::make_unique<Utils::Sink<void(const Input::InputCaptureUpdate&)>>(eventManager.System.OnInputCaptureUpdate)),
      m_onLanguageChangedSink(std::make_unique<Utils::Sink<void(const std::string&)>>(LocalizationManager::GetInstance().OnFrameworkLanguageChanged)) {
  m_onInputCaptureUpdateSink->Connect<&KeyCapturePopup::OnInputCaptureUpdate>(this);
  m_onLanguageChangedSink->Connect<&KeyCapturePopup::RefreshLocalizationOnLanguageChange>(this);

  RefreshLocalization();
}

void KeyCapturePopup::RefreshLocalizationOnLanguageChange(const std::string& /*componentName*/) { RefreshLocalization(); }

void KeyCapturePopup::RefreshLocalization() {
  auto& loc = LocalizationManager::GetInstance();

  m_popupTitleKey = loc.Get("settings_window.key_capture_popup.title");
  m_pressKeyTextKey = loc.Get("settings_window.key_capture_popup.press_key_text");
  m_deleteButtonKey = loc.Get("settings_window.key_capture_popup.delete_button");
  m_cancelButtonKey = loc.Get("settings_window.key_capture_popup.cancel_button");
  m_conflictTitleKey = loc.Get("settings_window.key_capture_popup.conflict_title");
  m_conflictTextDetailedKey = loc.Get("settings_window.key_capture_popup.conflict_text_detailed");
  m_reassignShortPressButtonKey = loc.Get("settings_window.key_capture_popup.reassign_short_press_button");
  m_reassignLongPressButtonKey = loc.Get("settings_window.key_capture_popup.reassign_long_press_button");
  m_reassignPositiveSideButtonKey = loc.Get("settings_window.key_capture_popup.reassign_positive_side_button");
  m_reassignNegativeSideButtonKey = loc.Get("settings_window.key_capture_popup.reassign_negative_side_button");
  m_addShortPressButtonKey = loc.Get("settings_window.key_capture_popup.add_short_press_button");
  m_addLongPressButtonKey = loc.Get("settings_window.key_capture_popup.add_long_press_button");
  m_addPositiveSideButtonKey = loc.Get("settings_window.key_capture_popup.add_positive_side_button");
  m_addNegativeSideButtonKey = loc.Get("settings_window.key_capture_popup.add_negative_side_button");
  m_reassignEntireAxisButtonKey = loc.Get("settings_window.key_capture_popup.reassign_entire_axis_button");
  m_actionListFormatKey = loc.Get("settings_window.key_capture_popup.action_list_format");

  m_enumPressTypeShortKey = loc.Get("enums.press_type.short");
  m_enumPressTypeLongKey = loc.Get("enums.press_type.long");
  m_enumSideBothKey = loc.Get("enums.side.both");
  m_enumSidePositiveKey = loc.Get("enums.side.positive");
  m_enumSideNegativeKey = loc.Get("enums.side.negative");
}

void KeyCapturePopup::Open(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding) {
  m_actionBeingEdited = actionFullName;
  m_editingBindingObject = originalBinding;
  m_conflictInfo.reset();
  m_bufferedInputInfo.reset();
  m_currentChordInputs.clear();
  m_eventManager.System.OnRequestInputCapture.Call({actionFullName, m_editingBindingObject});
}

void KeyCapturePopup::OnInputCaptured(const Input::InputCaptured& e) {
  // This is called from a non-render thread; just buffer it, actual processing happens in Render().
  m_bufferedInputInfo = e;
}

void KeyCapturePopup::OnInputCaptureCancelled(const Input::InputCaptureCancelled& e) {
  m_actionBeingEdited.reset();
  m_currentChordInputs.clear();
}

void KeyCapturePopup::OnInputCaptureUpdate(const Input::InputCaptureUpdate& e) {
  if (!m_actionBeingEdited.has_value() || m_actionBeingEdited.value() != e.actionFullName) return;
  m_currentChordInputs = e.currentChordInputs;
}

void KeyCapturePopup::OnInputCaptureConflict(const Input::InputCaptureConflict& e) {
  m_conflictInfo = e;
  // m_actionBeingEdited is left untouched; the conflict popup still needs it.
}

void KeyCapturePopup::Render() {
  auto& loc = LocalizationManager::GetInstance();

  if (m_actionBeingEdited.has_value()) {
    ImGui::OpenPopup(m_popupTitleKey.c_str());
  }

  ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Appearing);
  if (!ImGui::BeginPopupModal(m_popupTitleKey.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    return;
  }

  // Check for conflict first
  if (m_conflictInfo.has_value()) {
    auto logger = LoggerFactory::GetInstance().GetLogger("KeyCapturePopup");
    std::string inputDisplayName = m_conflictInfo->capturedInput->GetDisplayName();

    Typography::Text(TextStyle::H3().Separator().Color(UI::Colors::RED), "%s", m_conflictTitleKey.c_str());
    std::string conflictDetails = loc.GetFormatted("framework", "settings_window.key_capture_popup.conflict_text_detailed", inputDisplayName);
    Typography::RenderMarkdownText(conflictDetails, TextStyle::Regular().Wrapped().Padding({0.0f, 10.0f}));

    ImGui::Separator();

    auto analysis = Modules::KeyBindsManager::GetInstance().AnalyzeConflictsForInput(*m_conflictInfo->capturedInput);

    auto renderConflictInfo = [&](const auto& conflict, const std::string& typeKey, bool isSide = false) {
      (void)isSide;
      if (!conflict) return;

      const std::string& fullActionName = conflict->first;

      size_t lastDot = fullActionName.rfind('.');
      if (lastDot == std::string::npos) return;
      std::string group = fullActionName.substr(0, lastDot);
      size_t firstDot = group.find('.');
      std::string ownerName = (firstDot != std::string::npos) ? group.substr(0, firstDot) : group;

      std::string ownerDisplayName = ownerName;
      auto it_owner = m_configService.GetAllComponentInfo().find(ownerName);
      if (it_owner != m_configService.GetAllComponentInfo().end() && it_owner->second.name.has_value()) {
        ownerDisplayName = it_owner->second.name.value();
      }

      std::string actionName = Modules::GetTranslatedActionName(m_configService, fullActionName);
      std::string translatedType = typeKey;

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

    auto addBinding = [&](const std::string& pressType, const std::string& side = "") {
      m_eventManager.System.OnRequestInputCaptureCancel.Call({});
      logger->Info("User chose to add a new binding for action '{}' with press type '{}' and side '{}'.", m_conflictInfo->actionFullName, pressType, side);

      nlohmann::ordered_json newBinding = m_conflictInfo->capturedInput->ToJson();

      if (isAxis && !side.empty()) {
        newBinding["side"] = side;
        newBinding["mode"] = "analog";
      } else {
        newBinding["press_type"] = pressType;
      }

      m_eventManager.System.OnRequestBindingUpdate.Call({m_conflictInfo->actionFullName, m_conflictInfo->originalBinding, newBinding, std::nullopt});

      m_conflictInfo.reset();
      m_actionBeingEdited.reset();
      ImGui::CloseCurrentPopup();
    };

    auto reassignBinding = [&](const std::optional<std::pair<std::string, nlohmann::ordered_json>>& conflict) {
      if (!conflict) return;
      m_eventManager.System.OnRequestInputCaptureCancel.Call({});
      logger->Info("User confirmed reassigning input '{}' from '{}' to '{}'.", inputDisplayName, conflict->first, m_conflictInfo->actionFullName);

      nlohmann::ordered_json newBindingJson = m_conflictInfo->capturedInput->ToJson();
      const auto& conflictingBindingJson = conflict->second;

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

      m_eventManager.System.OnRequestBindingUpdate.Call({m_conflictInfo->actionFullName, m_conflictInfo->originalBinding, newBindingJson, *conflict});

      m_conflictInfo.reset();
      m_actionBeingEdited.reset();
      ImGui::CloseCurrentPopup();
    };

    if (isAxis) {
      if (analysis.isPositiveAvailable || analysis.bothConflict) {
        if (Button(m_addPositiveSideButtonKey.c_str())) {
          if (analysis.bothConflict) {
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({analysis.bothConflict->first, analysis.bothConflict->second, "side", "negative"});
          }
          addBinding("short", "positive");
        }
        ImGui::SameLine();
      }

      if (analysis.isNegativeAvailable || analysis.bothConflict) {
        if (Button(m_addNegativeSideButtonKey.c_str())) {
          if (analysis.bothConflict) {
            m_eventManager.System.OnRequestBindingPropertyUpdate.Call({analysis.bothConflict->first, analysis.bothConflict->second, "side", "positive"});
          }
          addBinding("short", "negative");
        }
        ImGui::SameLine();
      }

      if (analysis.bothConflict) {
        if (Button(m_reassignEntireAxisButtonKey.c_str())) {
          reassignBinding(analysis.bothConflict);
        }
        ImGui::SameLine();
      } else {
        if (analysis.positiveConflict) {
          if (Button(m_reassignPositiveSideButtonKey.c_str())) {
            reassignBinding(analysis.positiveConflict);
          }
          ImGui::SameLine();
        }
        if (analysis.negativeConflict) {
          if (Button(m_reassignNegativeSideButtonKey.c_str())) {
            reassignBinding(analysis.negativeConflict);
          }
          ImGui::SameLine();
        }
      }
    } else {
      if (analysis.isShortPressAvailable) {
        if (Button(m_addShortPressButtonKey.c_str())) {
          addBinding("short");
        }
        ImGui::SameLine();
      }

      if (analysis.isLongPressAvailable) {
        if (Button(m_addLongPressButtonKey.c_str())) {
          addBinding("long");
        }
        ImGui::SameLine();
      }
    }

    if (analysis.shortPressConflict) {
      if (Button(m_reassignShortPressButtonKey.c_str())) {
        reassignBinding(analysis.shortPressConflict);
      }
      ImGui::SameLine();
    }

    if (analysis.longPressConflict) {
      if (Button(m_reassignLongPressButtonKey.c_str())) {
        reassignBinding(analysis.longPressConflict);
      }
      ImGui::SameLine();
    }

    if (Button(m_cancelButtonKey.c_str())) {
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
    if (m_actionBeingEdited.has_value() && m_bufferedInputInfo->actionFullName == m_actionBeingEdited.value()) {
      const auto& actionFullName = m_bufferedInputInfo->actionFullName;
      const auto& originalBinding = m_bufferedInputInfo->originalBinding;
      const auto& newBindingJson = m_bufferedInputInfo->capturedInput->ToJson();

      auto logger = LoggerFactory::GetInstance().GetLogger("KeyCapturePopup");
      logger->Info("Requesting keybinding update for action '{}' to new input '{}'.", actionFullName, newBindingJson.dump());

      m_eventManager.System.OnRequestBindingUpdate.Call({actionFullName, originalBinding, newBindingJson, std::nullopt});
    }

    m_bufferedInputInfo.reset();
    m_actionBeingEdited.reset();
    m_currentChordInputs.clear();
    ImGui::CloseCurrentPopup();
  } else {
    // No key captured yet: show the popup's waiting content.
    Typography::Text(TextStyle::Regular().Wrapped(), m_pressKeyTextKey.c_str());

    ImGui::Spacing();
    if (!m_currentChordInputs.empty()) {
      float totalWidth = 0;
      float spacing = 4.0f;
      for (size_t i = 0; i < m_currentChordInputs.size(); ++i) {
        std::string label = Modules::GetDisplayNameWithIcon(*m_currentChordInputs[i]);
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
        std::string label = Modules::GetDisplayNameWithIcon(*input);

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

    Typography::Text(TextStyle::H3().Color(UI::Colors::YELLOW).Align(TextAlign::Center), Modules::GetTranslatedActionName(m_configService, m_actionBeingEdited.value()).c_str());
    ImGui::Separator();

    if (!m_editingBindingObject.empty()) {
      ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
      if (Button(m_deleteButtonKey.c_str())) {
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

    if (Button(m_cancelButtonKey.c_str(), TextStyle::DefaultButton(), ImVec2(120, 0))) {
      m_eventManager.System.OnRequestInputCaptureCancel.Call({});
      m_actionBeingEdited.reset();
      m_currentChordInputs.clear();
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::EndPopup();
}

}  // namespace UI
SPF_NS_END
