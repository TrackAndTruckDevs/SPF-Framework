#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData {
// Forward declare the service class to avoid circular dependencies
class GameDataCameraService;

/**
 * @class ICameraDataFinder
 * @brief An interface for a class that finds a specific set of camera-related offsets or pointers.
 *
 * This pattern allows us to split the monolithic GameDataCameraService into smaller,
 * more manageable pieces. Each class implementing this interface is responsible for
 * finding one logical group of data (e.g., all interior camera offsets, all viewport offsets).
 */
class ICameraDataFinder {
 public:
  virtual ~ICameraDataFinder() = default;

  virtual const char* GetName() const = 0;

  /**
   * @brief Attempts to find the memory offsets and pointers for a specific camera feature.
   * This method is designed to be called repeatedly until it succeeds.
   * @param owner The GameDataCameraService to populate with found data.
   * @return True if all data was found successfully, false otherwise.
   */
  virtual bool TryFindOffsets(GameDataCameraService& owner) = 0;

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
