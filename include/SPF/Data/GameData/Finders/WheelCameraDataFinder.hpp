#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
class WheelCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "WheelCameraDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
