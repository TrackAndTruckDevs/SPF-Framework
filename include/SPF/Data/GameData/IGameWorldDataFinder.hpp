/**
 * @file IGameWorldDataFinder.hpp
 * @brief Interface for GameWorld data finders.
 */

#pragma once
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Data::GameData {
// Forward declaration
class GameWorldService;

/**
 * @interface IGameWorldDataFinder
 * @brief Defines the interface for all GameWorld data finders.
 *
 * @details Each finder is responsible for locating a specific set of memory
 *          addresses or offsets related to the game world (time, weather, etc.)
 *          and reporting them to the GameWorldService.
 */
class IGameWorldDataFinder {
 public:
  virtual ~IGameWorldDataFinder() = default;

  /**
   * @brief Attempts to find game data offsets and addresses.
   * @param owner Reference to the GameWorldService to store found data.
   * @return true if all critical data was found.
   */
  virtual bool TryFindOffsets(GameWorldService& owner) = 0;

  /**
   * @brief Returns the name of the finder for logging purposes.
   * @return A C-style string name of the finder.
   */
  virtual const char* GetName() const = 0;

  /**
   * @brief Checks if the finder has successfully found all its required data.
   * @return True if ready, false otherwise.
   */
  bool IsReady() const { return m_isReady; }
  void Reset() { m_isReady = false; }

 protected:
  bool m_isReady = false;
};

}  // namespace Data::GameData
SPF_NS_END
