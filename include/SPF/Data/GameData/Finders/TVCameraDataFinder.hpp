#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
class TVCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "TVCameraDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
