#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
/**
 * @class PhotoCameraDataFinder
 * @brief Finds memory offsets for the photo mode camera.
 */
class PhotoCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "PhotoCameraDataFinder"; }
};
}  // namespace SPF::Data::GameData::Finders
