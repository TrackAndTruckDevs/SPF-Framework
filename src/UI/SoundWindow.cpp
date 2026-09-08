#include "SPF/UI/SoundWindow.hpp"

#include "SPF/Data/GameData/SoundService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"

#include "imgui.h"

#include <cstdio>
#include <string>

namespace SPF::UI {
using namespace Localization;

SoundWindow::SoundWindow(const std::string& componentName, const std::string& windowId, Data::GameData::SoundService& soundService) : BaseWindow(componentName, windowId), m_soundService(soundService) {
  m_titleLocalizationKey = "sound_window.title";
  RefreshLocalization();
}

void SoundWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locNotReady = loc.Get("sound_window.not_ready");
  m_locBankComboLabel = loc.Get("sound_window.bank_combo_label");
  m_locEventComboLabel = loc.Get("sound_window.event_combo_label");
  m_locNone = loc.Get("sound_window.none");
  m_locBanksFound = loc.Get("sound_window.banks_found");
  m_locEventsFound = loc.Get("sound_window.events_found");
  m_locBankInfoTitle = loc.Get("sound_window.bank_info_title");
  m_locEventInfoTitle = loc.Get("sound_window.event_info_title");
  m_locBankPath = loc.Get("sound_window.bank_path");
  m_locEventCount = loc.Get("sound_window.event_count");
  m_locEventPath = loc.Get("sound_window.event_path");
  m_locGUID = loc.Get("sound_window.guid");
  m_locRefresh = loc.Get("sound_window.refresh");
  m_locNoSelection = loc.Get("sound_window.no_selection");
}

void SoundWindow::RefreshSoundList() {
  m_banks = m_soundService.GetSoundBankGroups();

  m_bankComboItems.clear();
  m_bankComboItems.push_back(m_locNone.c_str());
  for (const auto& bank : m_banks) {
    m_bankComboItems.push_back(bank.bankPath.c_str());
  }

  if (m_selectedBank >= (int)m_banks.size()) m_selectedBank = -1;
  m_selectedEvent = -1;
  m_eventComboItems.clear();
}

void SoundWindow::RenderContent() {
  if (!m_soundService.IsReady()) {
    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", m_locNotReady.c_str());
    return;
  }

  if (!m_listInitialized) {
    RefreshSoundList();
    m_listInitialized = true;
  }

  // --- Refresh button + summary ---
  ImGui::Spacing();
  if (ImGui::Button(m_locRefresh.c_str())) {
    RefreshSoundList();
  }

  int totalEvents = 0;
  for (const auto& b : m_banks) totalEvents += (int)b.events.size();

  ImGui::SameLine();
  ImGui::TextDisabled("  ");
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::GREEN), m_locBanksFound.c_str(), (int)m_banks.size());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::GREEN), m_locEventsFound.c_str(), totalEvents);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Bank combo ---
  int bankIdx = m_selectedBank + 1;
  if (ImGui::Combo(m_locBankComboLabel.c_str(), &bankIdx, m_bankComboItems.data(), (int)m_bankComboItems.size())) {
    m_selectedBank = bankIdx - 1;
    m_selectedEvent = -1;

    // Rebuild event combo for the selected bank
    m_eventComboItems.clear();
    if (m_selectedBank >= 0 && m_selectedBank < (int)m_banks.size()) {
      m_eventComboItems.push_back(m_locNone.c_str());
      for (const auto& ev : m_banks[m_selectedBank].events) {
        m_eventComboItems.push_back(ev.eventPath.c_str());
      }
    }
  }

  if (m_selectedBank < 0 || m_selectedBank >= (int)m_banks.size()) {
    Typography::Text(TextStyle::Regular().Color(Colors::GRAY), "%s", m_locNoSelection.c_str());
    return;
  }

  const auto& bank = m_banks[m_selectedBank];

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Bank info section ---
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s", m_locBankInfoTitle.c_str());
  ImGui::Spacing();

  const float tableWidth = ImGui::GetContentRegionAvail().x;
  if (ImGui::BeginTable("bank_info", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX)) {
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, tableWidth * 0.35f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    auto rowLabel = [&](const char* label) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", label);
      ImGui::TableSetColumnIndex(1);
    };

    rowLabel(m_locBankPath.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", bank.bankPath.c_str());

    rowLabel(m_locEventCount.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", (int)bank.events.size());

    ImGui::EndTable();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Event combo ---
  if (m_eventComboItems.empty()) {
    m_eventComboItems.push_back(m_locNone.c_str());
    if (m_selectedBank >= 0 && m_selectedBank < (int)m_banks.size()) {
      for (const auto& ev : m_banks[m_selectedBank].events) {
        m_eventComboItems.push_back(ev.eventPath.c_str());
      }
    }
  }

  int eventIdx = m_selectedEvent + 1;
  if (ImGui::Combo(m_locEventComboLabel.c_str(), &eventIdx, m_eventComboItems.data(), (int)m_eventComboItems.size())) {
    m_selectedEvent = eventIdx - 1;
  }

  if (m_selectedEvent < 0 || m_selectedEvent >= (int)bank.events.size()) {
    ImGui::Spacing();
    Typography::Text(TextStyle::Regular().Color(Colors::GRAY), "%s", m_locNoSelection.c_str());
    return;
  }

  const auto& ev = bank.events[m_selectedEvent];

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Event detail table ---
  Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s", m_locEventInfoTitle.c_str());
  ImGui::Spacing();

  // Format GUID as hex string
  char guidHex[33];
  for (int i = 0; i < 16; ++i) {
    snprintf(guidHex + i * 2, 3, "%02x", ev.guid[i]);
  }
  guidHex[32] = '\0';

  if (ImGui::BeginTable("event_detail", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX)) {
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, tableWidth * 0.35f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    auto rowLabel = [&](const char* label) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", label);
      ImGui::TableSetColumnIndex(1);
    };

    rowLabel(m_locBankPath.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.bankPath.c_str());

    rowLabel(m_locEventPath.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.eventPath.c_str());

    rowLabel(m_locGUID.c_str());
    Typography::Text(TextStyle::Monospace().Color(Colors::YELLOW), "%s", guidHex);

    ImGui::EndTable();
  }

  ImGui::Spacing();
}

}  // namespace SPF::UI
