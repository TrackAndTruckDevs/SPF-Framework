#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class PhotoCameraDataFinder
 * @brief Finds memory offsets for the photo mode camera.
 */
class PhotoCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "PhotoCameraDataFinder"; }
};
}  // namespace Data::GameData::Finders
SPF_NS_END
