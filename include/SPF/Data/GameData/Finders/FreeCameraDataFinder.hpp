#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class FreeCameraDataFinder
 * @brief Finds pointers and offsets related to the developer free camera.
 */
class FreeCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "FreeCameraDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
