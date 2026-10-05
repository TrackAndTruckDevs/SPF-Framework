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

  // Editable amount for money operations
  int64_t m_amount = 0;
};

}  // namespace SPF::UI
