#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
class DebugCameraDataFinder : public ICameraDataFinder {
 public:
  virtual const char* GetName() const override { return "DebugCameraDataFinder"; }
  virtual bool TryFindOffsets(GameDataCameraService& owner) override;
};
}  // namespace Data::GameData::Finders
SPF_NS_END
