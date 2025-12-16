#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
class DebugCameraStateDataFinder : public ICameraDataFinder {
 public:
  virtual const char* GetName() const override { return "DebugCameraStateDataFinder"; }
  virtual bool TryFindOffsets(GameDataCameraService& owner) override;
};
}  // namespace Data::GameData::Finders
SPF_NS_END
