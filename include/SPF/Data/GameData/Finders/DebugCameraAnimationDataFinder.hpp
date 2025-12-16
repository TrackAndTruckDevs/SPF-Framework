#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class DebugCameraAnimationDataFinder
 * @brief Finds pointers and offsets related to the debug camera animation system.
 */
class DebugCameraAnimationDataFinder : public ICameraDataFinder {
 public:
  const char* GetName() const override { return "DebugCameraAnimationDataFinder"; }

 protected:
  bool TryFindOffsets(GameDataCameraService& owner) override;
};
}  // namespace Data::GameData::Finders
SPF_NS_END
