#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class CoreCameraDataFinder
 * @brief Finds core camera system data like the standard manager and active camera ID offset.
 */
class CoreCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "CoreCameraDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
