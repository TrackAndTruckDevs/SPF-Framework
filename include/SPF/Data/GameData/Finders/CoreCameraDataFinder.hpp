#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

namespace SPF::Data::GameData::Finders {
/**
 * @class CoreCameraDataFinder
 * @brief Finds core camera system data like the standard manager and active camera ID offset.
 */
class CoreCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "CoreCameraDataFinder"; }
};

}  // namespace SPF::Data::GameData::Finders
