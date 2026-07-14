/**
 * @file GameWorldDataFinder.hpp
 * @brief Dynamic pattern searcher for World and Environment-related data.
 *
 * @details This finder is responsible for locating the global environment state
 *          and the critical UpdateEnvironmentState function. It avoids hardcoded
 *          offsets by scanning the game's executable for specific instruction
 *          patterns found during Ghidra analysis.
 */

#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/IGameWorldDataFinder.hpp"


SPF_NS_BEGIN
namespace Data::GameData::Finders {

/**
 * @class WorldDataFinder
 * @brief Specialized class for resolving World-related addresses at runtime.
 */
class WorldDataFinder : public IGameWorldDataFinder {
 public:
  /**
   * @brief Attempts to find all necessary offsets and addresses for GameWorldAPI.
   * @details Scans for the UpdateEnvironmentState function, the global environment
   *          pointer, and the specific offsets for world time.
   * @param owner Reference to the GameWorldService where the results will be stored.
   * @return true if all critical patterns were successfully resolved.
   */
  bool TryFindOffsets(GameWorldService& owner) override;

  /** @brief Returns the internal name of the finder for logging purposes. */
  const char* GetName() const override { return "WorldDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
