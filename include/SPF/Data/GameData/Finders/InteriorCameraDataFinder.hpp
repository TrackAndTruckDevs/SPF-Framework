#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
/**
 * @class InteriorCameraDataFinder
 * @brief Finds all memory offsets related to the interior camera.
 */
class InteriorCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "InteriorCameraDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
