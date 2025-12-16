#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class TopCameraDataFinder
 * @brief Finds all memory offsets related to the top-down camera.
 */
class TopCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "TopCameraDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
