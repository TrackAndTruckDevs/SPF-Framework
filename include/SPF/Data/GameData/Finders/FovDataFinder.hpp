#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
class FovDataFinder : public ICameraDataFinder {
 public:
  virtual const char* GetName() const override { return "FovDataFinder"; }
  virtual bool TryFindOffsets(GameDataCameraService& owner) override;
};
}  // namespace SPF::Data::GameData::Finders
