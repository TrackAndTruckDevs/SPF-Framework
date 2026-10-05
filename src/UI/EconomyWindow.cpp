#include "SPF/UI/EconomyWindow.hpp"

#include "SPF/Data/GameData/EconomyService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"

#include "imgui.h"

#include <cstdint>
#include <string>

namespace SPF::UI {
using namespace Localization;

EconomyWindow::EconomyWindow(const std::string& componentName, const std::string& windowId, Data::GameData::EconomyService& economyService) : BaseWindow(componentName, windowId), m_economyService(economyService) {
  m_titleLocalizationKey = "economy_window.title";
  RefreshLocalization();
}

void EconomyWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locNotReady = loc.Get("economy_window.not_ready");
  m_locTabMoney = loc.Get("economy_window.tab_money");
  m_locBalanceTitle = loc.Get("economy_window.balance.title");
  m_locBalance = loc.Get("economy_window.balance.value");
  m_locDebtState = loc.Get("economy_window.balance.debt_state");
  m_locDebtYes = loc.Get("economy_window.balance.debt_yes");
  m_locDebtNo = loc.Get("economy_window.balance.debt_no");
  m_locAmountLabel = loc.Get("economy_window.money.amount_label");
  m_locAdd = loc.Get("economy_window.money.add");
  m_locSubtract = loc.Get("economy_window.money.subtract");
  m_locSet = loc.Get("economy_window.money.set");
  m_locControlTitle = loc.Get("economy_window.control.title");
  m_locControlLock = loc.Get("economy_window.control.lock_checkbox");
  m_locControlTooltip = loc.Get("economy_window.control.lock_tooltip");
}

void EconomyWindow::RenderContent() {
  if (!m_economyService.IsReady()) {
    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", m_locNotReady.c_str());
    return;
  }

  ImGui::BeginTabBar("economy_tabs");
  if (ImGui::BeginTabItem(m_locTabMoney.c_str())) {
    RenderMoneyTab();
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
}

void EconomyWindow::RenderMoneyTab() {
  ImGui::Spacing();

  // --- Balance (live) ---
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", m_locBalanceTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  int64_t balance = m_economyService.GetBalance();
  Typography::Text(TextStyle::Regular(), m_locBalance.c_str(), static_cast<long long>(balance));

  bool debt = m_economyService.GetDebtFlag();
  Typography::Text(TextStyle::Regular().Color(debt ? Colors::RED : Colors::GREEN), m_locDebtState.c_str(), debt ? m_locDebtYes.c_str() : m_locDebtNo.c_str());

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Money operations ---
  ImGui::SetNextItemWidth(220.0f);
  ImGui::InputScalar(m_locAmountLabel.c_str(), ImGuiDataType_S64, &m_amount, nullptr, nullptr, "%lld", ImGuiInputTextFlags_EnterReturnsTrue);

  if (ImGui::Button(m_locAdd.c_str())) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyWindow");
    bool ok = m_economyService.AddMoney(m_amount);
    logger->Info("AddMoney({}) -> {}", static_cast<long long>(m_amount), ok);
  }
  ImGui::SameLine();
  if (ImGui::Button(m_locSubtract.c_str())) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyWindow");
    bool ok = m_economyService.AddMoney(-m_amount);
    logger->Info("AddMoney(-{}) -> {}", static_cast<long long>(m_amount), ok);
  }
  ImGui::SameLine();
  if (ImGui::Button(m_locSet.c_str())) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyWindow");
    bool ok = m_economyService.SetMoney(m_amount);
    logger->Info("SetMoney({}) -> {}", static_cast<long long>(m_amount), ok);
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Game control lock ---
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s", m_locControlTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  bool locked = m_economyService.IsGameControlLocked();
  if (ImGui::Checkbox(m_locControlLock.c_str(), &locked)) {
    m_economyService.SetGameControl(locked);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", m_locControlTooltip.c_str());
  }

  ImGui::Spacing();
}

}  // namespace SPF::UI
