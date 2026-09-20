#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
/**
 * @class FreeCameraDataFinder
 * @brief Finds pointers and offsets related to the developer free camera.
 */
class FreeCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "FreeCameraDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
