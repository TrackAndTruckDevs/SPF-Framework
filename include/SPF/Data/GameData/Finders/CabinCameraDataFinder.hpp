#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
class CabinCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "CabinCameraDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
