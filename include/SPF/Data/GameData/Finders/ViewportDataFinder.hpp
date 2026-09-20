#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
/**
 * @class ViewportDataFinder
 * @brief Finds data related to the game's viewport and projection matrices.
 */
class ViewportDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "ViewportDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
