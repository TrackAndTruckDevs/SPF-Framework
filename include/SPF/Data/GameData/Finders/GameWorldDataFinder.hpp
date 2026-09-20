#pragma once

#include "SPF/Data/GameData/IGameWorldDataFinder.hpp"

namespace SPF::Data::GameData::Finders {

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

}  // namespace SPF::Data::GameData::Finders
