/**
 * @file EconomyWindow.hpp
 * @brief UI window for Economy manipulation (balance, money, game control).
 */

#pragma once

#include "SPF/Data/GameData/EconomyService.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include <cstdint>
#include <string>

namespace SPF::UI {

/**
 * @class EconomyWindow
 * @brief Implements the Economy tab for the framework overlay.
 */
class EconomyWindow : public BaseWindow {
 public:
  EconomyWindow(const std::string& componentName, const std::string& windowId, Data::GameData::EconomyService& economyService);
  virtual ~EconomyWindow() = default;

 protected:
  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  void RenderMoneyTab();
  void RenderLoansTab();

  Data::GameData::EconomyService& m_economyService;

  // Localization keys
  std::string m_locNotReady;
  std::string m_locTabMoney;
  std::string m_locBalanceTitle;
  std::string m_locBalance;
  std::string m_locDebtState;
  std::string m_locDebtYes;
  std::string m_locDebtNo;
  std::string m_locAmountLabel;
  std::string m_locAdd;
  std::string m_locSubtract;
  std::string m_locSet;
  std::string m_locControlTitle;
  std::string m_locControlLock;
  std::string m_locControlTooltip;
  std::string m_locTabLoans;
  std::string m_locLoansCreditLimitTitle;
  std::string m_locLoansCreditLimit;
  std::string m_locLoansActiveTitle;
  std::string m_locLoansOffersTitle;
  std::string m_locLoansNoLoans;
  std::string m_locLoansNoOffers;
  std::string m_locLoansColRemaining;
  std::string m_locLoansColAmount;
  std::string m_locLoansColStartDay;
  std::string m_locLoansColRate;
  std::string m_locLoansColWeeks;
  std::string m_locLoansColAction;
  std::string m_locLoansRepay;
  std::string m_locLoansLimitLabel;
  std::string m_locLoansSetLimit;
  std::string m_locLoansEdit;
  std::string m_locLoansSave;
  std::string m_locLoansCancel;
  std::string m_locLoansEditTitle;
  std::string m_locLoansTake;
  std::string m_locLoansNewLoanTitle;

  // Editable amount for money operations
  int64_t m_amount = 0;

  // Loans tab state
  int64_t m_creditLimit = 0;
  bool m_creditLimitSeeded = false;
  int32_t m_editLoanIdx = -1;
  int64_t m_editRemaining = 0;
  int64_t m_editAmount = 0;
  uint32_t m_editStartDay = 0;
  float m_editRate = 0.0f;
  uint32_t m_editWeeks = 0;
  uint64_t m_newLoanAmount = 10000;
  float m_newLoanRate = 5.0f;
  uint32_t m_newLoanWeeks = 4;
};

}  // namespace SPF::UI
