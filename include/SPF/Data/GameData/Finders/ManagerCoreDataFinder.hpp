/**
 * @file ManagerCoreDataFinder.hpp
 * @brief Dynamic pattern searcher for core game managers (GameplayManager, ...).
 */

#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/IManagerDataFinder.hpp"


SPF_NS_BEGIN
namespace Data::GameData::Finders {

/**
 * @class ManagerCoreDataFinder
 * @brief Specialized class for resolving core manager addresses at runtime.
 */
class ManagerCoreDataFinder : public IManagerDataFinder {
 public:
  /**
   * @brief Attempts to find all necessary addresses for ManagerCoreService.
   * @param owner Reference to the ManagerCoreService where the results will be stored.
   * @return true if all critical patterns were successfully resolved.
   */
  bool TryFindOffsets(ManagerCoreService& owner) override;

  /** @brief Returns the internal name of the finder for logging purposes. */
  const char* GetName() const override { return "ManagerCoreDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
