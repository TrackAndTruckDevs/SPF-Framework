#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
class WindowCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "WindowCameraDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
