#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class ViewportDataFinder
 * @brief Finds data related to the game's viewport and projection matrices.
 */
class ViewportDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "ViewportDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
