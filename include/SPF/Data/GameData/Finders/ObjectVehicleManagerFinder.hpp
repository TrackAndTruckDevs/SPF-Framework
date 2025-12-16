#pragma once

#include "SPF/Data/GameData/IObjectDataFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {

/**
 * @class ObjectManagerFinder
 * @brief Finds the pointer to the global TrafficManager instance.
 */
class ObjectManagerFinder : public IObjectDataFinder {
public:
    virtual const char* GetName() const override { return "ObjectManagerFinder"; }
    virtual bool TryFindOffsets(GameObjectVehicleService& owner) override;
};

} // namespace Data::GameData::Finders
SPF_NS_END
