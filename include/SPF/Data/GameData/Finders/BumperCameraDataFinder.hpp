#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
class BumperCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "BumperCameraDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
