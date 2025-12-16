#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class InteriorCameraDataFinder
 * @brief Finds all memory offsets related to the interior camera.
 */
class InteriorCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "InteriorCameraDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
