#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
class DebugCameraStateDataFinder : public ICameraDataFinder {
 public:
  virtual const char* GetName() const override { return "DebugCameraStateDataFinder"; }
  virtual bool TryFindOffsets(GameDataCameraService& owner) override;
};
}  // namespace SPF::Data::GameData::Finders
