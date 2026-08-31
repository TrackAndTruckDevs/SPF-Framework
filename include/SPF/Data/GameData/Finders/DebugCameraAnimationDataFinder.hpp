#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
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
}  // namespace SPF::Data::GameData::Finders
