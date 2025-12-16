#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData {

// Forward declare the service classes to avoid circular dependencies
class GameObjectVehicleService;

/**
 * @class IObjectDataFinder
 * @brief An interface for a class that finds a specific set of game object-related memory offsets or pointers.
 *
 * This pattern allows splitting data-finding logic into smaller, manageable pieces.
 * Each class implementing this interface is responsible for finding one logical group of data.
 * This is analogous to ICameraDataFinder but for general game objects.
 */
class IObjectDataFinder {
 public:
  virtual ~IObjectDataFinder() = default;

  virtual const char* GetName() const = 0;

  /**
   * @brief Attempts to find the memory offsets and pointers for a specific game object feature.
   * This method is designed to be called repeatedly until it succeeds.
   * @param owner The GameObjectVehicleService to populate with found data.
   * @return True if all data was found successfully, false otherwise.
   */
  virtual bool TryFindOffsets(GameObjectVehicleService& owner) = 0;
  
  /**
   * @brief Checks if the finder has successfully found all its required data.
   * @return True if ready, false otherwise.
   */
  bool IsReady() const { return m_isReady; }

 protected:
  bool m_isReady = false;
};

}  // namespace Data::GameData
SPF_NS_END
