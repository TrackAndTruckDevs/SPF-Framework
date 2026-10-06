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
  m_locTabLoans = loc.Get("economy_window.tab_loans");
  m_locLoansCreditLimitTitle = loc.Get("economy_window.loans.credit_limit_title");
  m_locLoansCreditLimit = loc.Get("economy_window.loans.credit_limit");
  m_locLoansActiveTitle = loc.Get("economy_window.loans.active_title");
  m_locLoansOffersTitle = loc.Get("economy_window.loans.offers_title");
  m_locLoansNoLoans = loc.Get("economy_window.loans.no_loans");
  m_locLoansNoOffers = loc.Get("economy_window.loans.no_offers");
  m_locLoansColRemaining = loc.Get("economy_window.loans.col_remaining");
  m_locLoansColAmount = loc.Get("economy_window.loans.col_amount");
  m_locLoansColStartDay = loc.Get("economy_window.loans.col_start_day");
  m_locLoansColRate = loc.Get("economy_window.loans.col_rate");
  m_locLoansColWeeks = loc.Get("economy_window.loans.col_weeks");
  m_locLoansColAction = loc.Get("economy_window.loans.col_action");
  m_locLoansRepay = loc.Get("economy_window.loans.repay");
  m_locLoansLimitLabel = loc.Get("economy_window.loans.limit_label");
  m_locLoansSetLimit = loc.Get("economy_window.loans.set_limit");
  m_locLoansEdit = loc.Get("economy_window.loans.edit");
  m_locLoansSave = loc.Get("economy_window.loans.save");
  m_locLoansCancel = loc.Get("economy_window.loans.cancel");
  m_locLoansEditTitle = loc.Get("economy_window.loans.edit_title");
  m_locLoansTake = loc.Get("economy_window.loans.take");
  m_locLoansNewLoanTitle = loc.Get("economy_window.loans.new_loan_title");
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

  if (ImGui::BeginTabItem(m_locTabLoans.c_str())) {
    RenderLoansTab();
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

// ============================================================================
// Loans Tab
// ============================================================================

void EconomyWindow::RenderLoansTab() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("EconomyWindow");

  ImGui::Spacing();

  // --- Credit limit (live) ---
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", m_locLoansCreditLimitTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  int64_t creditLimit = m_economyService.GetCreditLimit();
  Typography::Text(TextStyle::Regular(), m_locLoansCreditLimit.c_str(), static_cast<long long>(creditLimit));

  if (!m_creditLimitSeeded) {
    m_creditLimit = m_economyService.GetLoanLimit();
    m_creditLimitSeeded = true;
  }
  ImGui::SetNextItemWidth(260.0f);
  ImGui::InputScalar(m_locLoansLimitLabel.c_str(), ImGuiDataType_S64, &m_creditLimit, nullptr, nullptr, "%lld");
  ImGui::SameLine();
  if (ImGui::Button(m_locLoansSetLimit.c_str())) {
    bool ok = m_economyService.SetCreditLimit(m_creditLimit);
    logger->Info("SetCreditLimit({}) -> {}", m_creditLimit, ok);
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Active loans ---
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s", m_locLoansActiveTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  int32_t loanCount = m_economyService.GetActiveLoanCount();
  if (loanCount <= 0) {
    Typography::Text(TextStyle::Regular(), "%s", m_locLoansNoLoans.c_str());
  } else if (ImGui::BeginTable("economy_active_loans", 6,
                               ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn(m_locLoansColRemaining.c_str());
    ImGui::TableSetupColumn(m_locLoansColAmount.c_str());
    ImGui::TableSetupColumn(m_locLoansColStartDay.c_str());
    ImGui::TableSetupColumn(m_locLoansColRate.c_str());
    ImGui::TableSetupColumn(m_locLoansColWeeks.c_str());
    ImGui::TableSetupColumn(m_locLoansColAction.c_str());
    ImGui::TableHeadersRow();

    for (int32_t i = 0; i < loanCount; ++i) {
      Data::GameData::EconomyService::ActiveLoanInfo loan{};
      if (!m_economyService.GetActiveLoan(i, loan)) continue;

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%lld", static_cast<long long>(loan.remaining));
      ImGui::TableNextColumn();
      ImGui::Text("%lld", static_cast<long long>(loan.amount));
      ImGui::TableNextColumn();
      ImGui::Text("%u", loan.startDay);
      ImGui::TableNextColumn();
      ImGui::Text("%.2f%%", loan.rate * 100.0f);
      ImGui::TableNextColumn();
      ImGui::Text("%u", loan.weeks);
      ImGui::TableNextColumn();
      ImGui::PushID(i);
      if (ImGui::Button(m_locLoansRepay.c_str())) {
        bool ok = m_economyService.RepayLoan(i);
        logger->Info("RepayLoan({}) -> {}", i, ok);
      }
      ImGui::SameLine();
      if (ImGui::Button(m_locLoansEdit.c_str())) {
        Data::GameData::EconomyService::ActiveLoanInfo loan{};
        if (m_economyService.GetActiveLoan(i, loan)) {
          m_editLoanIdx = i;
          m_editRemaining = loan.remaining;
          m_editAmount = loan.amount;
          m_editStartDay = loan.startDay;
          m_editRate = loan.rate * 100.0f;
          m_editWeeks = loan.weeks;
        }
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }

  // --- Edit selected loan ---
  if (m_editLoanIdx >= 0) {
    if (m_editLoanIdx >= loanCount) {
      m_editLoanIdx = -1;
    } else {
      ImGui::Spacing();
      Typography::Text(TextStyle::H3().Color(Colors::CYAN), m_locLoansEditTitle.c_str(), m_editLoanIdx);
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::SetNextItemWidth(260.0f);
      ImGui::InputScalar(m_locLoansColRemaining.c_str(), ImGuiDataType_S64, &m_editRemaining, nullptr, nullptr, "%lld");
      ImGui::SetNextItemWidth(260.0f);
      ImGui::InputScalar(m_locLoansColAmount.c_str(), ImGuiDataType_S64, &m_editAmount, nullptr, nullptr, "%lld");
      ImGui::SetNextItemWidth(260.0f);
      ImGui::InputScalar(m_locLoansColStartDay.c_str(), ImGuiDataType_U32, &m_editStartDay);
      ImGui::SetNextItemWidth(260.0f);
      ImGui::InputScalar(m_locLoansColRate.c_str(), ImGuiDataType_Float, &m_editRate, nullptr, nullptr, "%.2f%%");
      ImGui::SetNextItemWidth(260.0f);
      ImGui::InputScalar(m_locLoansColWeeks.c_str(), ImGuiDataType_U32, &m_editWeeks);

      if (ImGui::Button(m_locLoansSave.c_str())) {
        bool ok = m_economyService.SetLoanRemaining(m_editLoanIdx, m_editRemaining);
        ok = m_economyService.SetLoanAmount(m_editLoanIdx, m_editAmount) && ok;
        ok = m_economyService.SetLoanStartDay(m_editLoanIdx, m_editStartDay) && ok;
        ok = m_economyService.SetLoanRate(m_editLoanIdx, m_editRate / 100.0f) && ok;
        ok = m_economyService.SetLoanWeeks(m_editLoanIdx, m_editWeeks) && ok;
        logger->Info("SaveLoanEdit({}) -> {}", m_editLoanIdx, ok);
        m_editLoanIdx = -1;
      }
      ImGui::SameLine();
      if (ImGui::Button(m_locLoansCancel.c_str())) {
        m_editLoanIdx = -1;
      }
    }
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Loan offers ---
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", m_locLoansOffersTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  int32_t offerCount = m_economyService.GetOfferCount();
  if (offerCount <= 0) {
    Typography::Text(TextStyle::Regular(), "%s", m_locLoansNoOffers.c_str());
  } else if (ImGui::BeginTable("economy_loan_offers", 4,
                               ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn(m_locLoansColAmount.c_str());
    ImGui::TableSetupColumn(m_locLoansColRate.c_str());
    ImGui::TableSetupColumn(m_locLoansColWeeks.c_str());
    ImGui::TableSetupColumn(m_locLoansColAction.c_str(), ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableHeadersRow();

    for (int32_t i = 0; i < offerCount; ++i) {
      Data::GameData::EconomyService::LoanOfferInfo offer{};
      if (!m_economyService.GetOffer(i, offer)) continue;

      ImGui::TableNextRow();
      ImGui::PushID(i);
      ImGui::TableNextColumn();
      ImGui::Text("%u", offer.amount);
      ImGui::TableNextColumn();
      ImGui::Text("%.2f%%", offer.rate * 100.0f);
      ImGui::TableNextColumn();
      ImGui::Text("%u", offer.weeks);
      ImGui::TableNextColumn();
      if (ImGui::Button(m_locLoansTake.c_str())) {
        bool ok = m_economyService.TakeOffer(i);
        logger->Info("TakeOffer({} amount={} rate={:.2f}% weeks={}) -> {}", i, offer.amount, offer.rate * 100.0f, offer.weeks, ok);
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }

  // --- Custom loan ---
  ImGui::Spacing();
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", m_locLoansNewLoanTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::SetNextItemWidth(180.0f);
  ImGui::InputScalar((m_locLoansColAmount + "##new_amount").c_str(), ImGuiDataType_U64, &m_newLoanAmount);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(110.0f);
  ImGui::InputFloat((m_locLoansColRate + "##new_rate").c_str(), &m_newLoanRate, 0.0f, 0.0f, "%.2f%%");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(90.0f);
  ImGui::InputScalar((m_locLoansColWeeks + "##new_weeks").c_str(), ImGuiDataType_U32, &m_newLoanWeeks);
  ImGui::SameLine();
  if (ImGui::Button((m_locLoansTake + "##new_loan").c_str())) {
    bool ok = m_economyService.TakeLoan(m_newLoanAmount, m_newLoanRate / 100.0f, m_newLoanWeeks);
    logger->Info("TakeLoan(custom amount={} rate={:.2f}% weeks={}) -> {}", m_newLoanAmount, m_newLoanRate, m_newLoanWeeks, ok);
  }

  ImGui::Spacing();
}

}  // namespace SPF::UI
