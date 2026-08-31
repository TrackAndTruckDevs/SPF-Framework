#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
/**
 * @class TopCameraDataFinder
 * @brief Finds all memory offsets related to the top-down camera.
 */
class TopCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "TopCameraDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
