#pragma once

#include "SPF/Data/GameData/IObjectDataFinder.hpp"

namespace SPF::Data::GameData::Finders {

/**
 * @class ObjectManagerFinder
 * @brief Finds the pointer to the global TrafficManager instance.
 */
class ObjectManagerFinder : public IObjectDataFinder {
 public:
  virtual const char* GetName() const override { return "ObjectManagerFinder"; }
  virtual bool TryFindOffsets(GameObjectVehicleService& owner) override;
};

}  // namespace SPF::Data::GameData::Finders
