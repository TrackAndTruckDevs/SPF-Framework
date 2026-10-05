/**
 * @file EconomyDataFinder.hpp
 * @brief Dynamic pattern searcher for economy bank transaction functions.
 */

#pragma once

#include "SPF/Data/GameData/IEconomyDataFinder.hpp"

namespace SPF::Data::GameData::Finders {

/**
 * @class EconomyDataFinder
 * @brief Specialized class for resolving bank transaction function addresses at runtime.
 */
class EconomyDataFinder : public IEconomyDataFinder {
 public:
  /**
   * @brief Attempts to find ProcessBankDeposit, ProcessBankWithdrawal and TryProcessTransaction.
   * @param owner Reference to the EconomyService where the results will be stored.
   * @return true if all critical patterns were successfully resolved.
   */
  bool TryFindOffsets(EconomyService& owner) override;

  /** @brief Returns the internal name of the finder for logging purposes. */
  const char* GetName() const override { return "EconomyDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
