/**                                                                                               
 * @file ClimateDataFinder.hpp                                                                          
 * @brief Dynamic pattern searcher for Climate-related data.
 */ 

#pragma once

#include "SPF/Data/GameData/IClimateDataFinder.hpp"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {

/**
 * @class ClimateDataFinder
 * @brief Specialized class for resolving Climate-related addresses at runtime.
 */
class ClimateDataFinder : public IClimateDataFinder {
 public:
  /**
   * @brief Attempts to find all necessary offsets and addresses for ClimateService.
   * @param owner Reference to the ClimateService where the results will be stored.
   * @return true if all critical patterns were successfully resolved.
   */
  bool TryFindOffsets(ClimateService& owner) override;

  /** @brief Returns the internal name of the finder for logging purposes. */
  const char* GetName() const override { return "ClimateDataFinder"; }
};

} // namespace Data::GameData::Finders
SPF_NS_END
