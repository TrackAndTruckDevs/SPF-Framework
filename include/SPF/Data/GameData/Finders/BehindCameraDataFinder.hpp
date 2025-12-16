#pragma once

#include "SPF/Data/GameData/ICameraDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
/**
 * @class BehindCameraDataFinder
 * @brief Finds all memory offsets related to the behind/chase camera (ID 1).
 */
class BehindCameraDataFinder : public ICameraDataFinder {
 public:
  bool TryFindOffsets(GameDataCameraService& owner) override;
  const char* GetName() const override { return "BehindCameraDataFinder"; }
};

}  // namespace Data::GameData::Finders
SPF_NS_END
